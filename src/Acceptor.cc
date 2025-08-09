#include<sys/types.h>
#include<sys/socket.h>
#include<errno.h>
#include<unistd.h>

#include"Acceptor.h"
#include"Logger.h"
#include"inetaddress.h"

static int createNonblocking(){
    int sockfd=::socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK,IPPROTO_TCP);
    if(sockfd<0){
        LOG_FATAL("%s:%s:%d listen socket create errno:%d\n",__FILE__,__FUNCTION__,__LINE__,errno);
    }
    return sockfd;
}

Acceptor::Acceptor(Eventloop*loop,const inetaddress&cb,bool requestport):loop_(loop),
acceptSocket_(createNonblocking()),
acceptChannel_(loop,acceptSocket_.get_fd()),
listening_(requestport)
{
    acceptSocket_.setKeepAlive(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindaddress(cb);
    // TcpServer::start() => Acceptor.listen() 如果有新用户连接 要执行一个回调(accept => connfd => 打包成Channel => 唤醒subloop)
    // baseloop监听到有事件发生 => acceptChannel_(listenfd) => 执行该回调函数
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
}


Acceptor::~Acceptor(){
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen(){
    listening_=true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}


//触发这个函数就说明有新的连接了
void Acceptor::handleRead(){
    inetaddress addr;
    int clnt_sock=acceptSocket_.accept(&addr);
    if(clnt_sock>=0){
        if(NewConnectionCallback_){
            NewConnectionCallback_(clnt_sock,addr);
        }
        else ::close(clnt_sock);
    }
    else{
        LOG_ERROR("%s:%s:%d accept err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit\n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}   

