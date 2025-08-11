#include <functional>
#include <string>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include <sys/sendfile.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close

#include"TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "Eventloop.h"

static Eventloop *CheckLoopNotNull(Eventloop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}



TcpConnection::TcpConnection(Eventloop *loop,
                             const std::string &nameArg,
                             int sockfd,
                             const inetaddress &localAddr,
                             const inetaddress &peerAddr)
    : loop_(CheckLoopNotNull(loop))
    , name_(nameArg)
    , state_(kConnecting)
    , reading_(true)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop, sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , highWaterMark_(64 * 1024 * 1024) // 64M
{
    // 下面给channel设置相应的回调函数 poller给channel通知感兴趣的事件发生了 channel会回调相应的回调函数
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this));
    channel_->setErrnoCallback(bind(&TcpConnection::handleError,this));

    auto bound_func = std::bind(&TcpConnection::handleError, this);
    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}


TcpConnection::~TcpConnection(){
    LOG_INFO("TcpConnection:dtor[%s] at fd=%d state=%d\n",name_.c_str(),channel_->fd(),(int)state_);
}



void TcpConnection::send(const std::string&buf){
    if(state_==kConnected){
        if(loop_->isInLoopThread())sendInLoop(buf.c_str(),buf.size());
        else loop_->runInloop(std::bind(&TcpConnection::sendInLoop,this,buf.c_str(),buf.size()));
    }

}
/** 
 * 发送数据 应用写的快 而内核发送数据慢 需要把待发送数据写入缓冲区，而且设置了水位回调
 **/
void TcpConnection::sendInLoop(const void *data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    if (state_ == kDisconnected) // 之前调用过该connection的shutdown 不能再进行发送了
    {
        LOG_ERROR("disconnected, give up writing");
    }

    // 表示channel_第一次开始写数据或者缓冲区没有待发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                // 既然在这里数据全部发送完成，就不用再给channel设置epollout事件了
                loop_->queueInloop(
                    std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK) // EWOULDBLOCK表示非阻塞情况下没有数据后的正常返回 等同于EAGAIN
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET) // SIGPIPE RESET
                {
                    faultError = true;
                }
            }
        }
    }

    /**
     * 说明当前这一次write并没有把数据全部发送出去 剩余的数据需要保存到缓冲区当中
     * 然后给channel注册EPOLLOUT事件，Poller发现tcp的发送缓冲区有空间后会通知
     * 相应的sock->channel，调用channel对应注册的writeCallback_回调方法，
     * channel的writeCallback_实际上就是TcpConnection设置的handleWrite回调，
     * 把发送缓冲区outputBuffer_的内容全部发送完成
     **/
    if (!faultError && remaining > 0)
    {
        // 目前发送缓冲区剩余的待发送的数据的长度
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInloop(
                std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append((char *)data + nwrote, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting(); // 这里一定要注册channel的写事件 否则poller不会给channel通知epollout
        }
    }
}


void TcpConnection::shutdown(){
   if(state_==kConnected){
    setState(kDisconnecting);
    loop_->runInloop(std::bind(&TcpConnection::shutdownInLoop,this));
   }
}


void TcpConnection::shutdownInLoop(){
     if(!channel_->isWriting())//说明当前outputbuffer_的数据已经全部向外发送
      socket_->shutdownwrite();
}


void TcpConnection::connectEstablished(){
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();//向poller里注册事件
}


void TcpConnection::connectDestroyed(){
    if(state_==kConnected){
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::handleRead(Timestamp receivetime){
    int saveErrno=0;
    ssize_t n=inputBuffer_.readFd(channel_->fd(),&saveErrno);
    if(n>0){//有数据到达
        //已建立连接的用户有可读事件发生了，调用用户传入的回调操作onmessage share_from_this就是获取了TcpConnection的智能指针
        messageCallback_(shared_from_this(),&inputBuffer_,receivetime);
    }
    else if(n==0)handleClose();
    else{//出错了
        errno=saveErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite(){
    if(channel_->isWriting()){
        int saveErrno=0;
        ssize_t n=outputBuffer_.writeFd(channel_->fd(),&saveErrno);
        if(n>0){
            outputBuffer_.retrieve(n);//把传输的数据的空间回收
            if(outputBuffer_.readableBytes()==0){
                channel_->disableWriting();
                if(writeCompleteCallback_){
                    loop_->queueInloop(std::bind(writeCompleteCallback_,shared_from_this()));
                }
                if(state_==kDisconnected){
                    shutdownInLoop();
                }
            }

        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing", channel_->fd());
    }
}


void TcpConnection::handleClose(){
    LOG_INFO("TcpConnection:handleColse fd=%d state=%d\n",channel_->fd(),int(state_));
    setState(kDisconnected);
    channel_->disableAll();
    TcpConnectionPtr connptr(shared_from_this());
    connectionCallback_(connptr);
    closeCallback_(connptr);
}

void TcpConnection::handleError(){
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d\n", name_.c_str(), err);
}


// 零拷贝发送函数
void TcpConnection::sendFile(int fileDescriptor,off_t offset,size_t count){
    if(connected()){
        if(loop_->isInLoopThread()){
            sendFileInLoop(fileDescriptor,offset,count);
        }
        else{
            loop_->runInloop(std::bind(TcpConnection::sendFileInLoop,shared_from_this(),fileDescriptor,offset,count));
        }
    }
    else{
        LOG_ERROR("TcpConnection::sendFile - not connected");
    }
}


void TcpConnection::sendFileInLoop(int fileDescriptor,off_t offset,size_t count){
    ssize_t bytesSent = 0; // 发送了多少字节数
    size_t remaining = count; // 还要多少数据要发送
    bool faultError = false; // 错误的标志位

    if (state_ == kDisconnecting) { // 表示此时连接已经断开就不需要发送数据了
        LOG_ERROR("disconnected, give up writing");
        return;
    }

    // 表示Channel第一次开始写数据或者outputBuffer缓冲区中没有数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        bytesSent = sendfile(socket_->get_fd(), fileDescriptor, &offset, remaining);
        if (bytesSent >= 0) {
            remaining -= bytesSent;
            if (remaining == 0 && writeCompleteCallback_) {
                // remaining为0意味着数据正好全部发送完，就不需要给其设置写事件的监听。
                loop_->queueInloop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        } else { // bytesSent < 0
            if (errno != EWOULDBLOCK) { // 如果是非阻塞没有数据返回错误这个是正常显现等同于EAGAIN，否则就异常情况
                LOG_ERROR("TcpConnection::sendFileInLoop");
            }
            if (errno == EPIPE || errno == ECONNRESET) {
                faultError = true;
            }
        }
    }
    // 处理剩余数据
    if (!faultError && remaining > 0) {
        // 继续发送剩余数据
        loop_->queueInloop(
            std::bind(&TcpConnection::sendFileInLoop, shared_from_this(), fileDescriptor, offset, remaining));
    }
}

