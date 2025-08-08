#include<sys/epoll.h>

#include"Channel.h"
#include"Eventloop.h"
#include"Logger.h"

const int Channel::kNoneEvent=0;//空事件
const int Channel::kReadEvent=EPOLLIN|EPOLLPRI;//读事件
const int Channel::kWriteEvent=EPOLLOUT;//写事件


Channel:: Channel(Eventloop *loop,int fd):loop_(loop),
fd_(fd),
events_(0),
revents_(0),
index_(-1),
tied_(false)
{}

Channel::~Channel(){}
// channel的tie方法什么时候调用过?  TcpConnection => channel
/**
 * TcpConnection中注册了Channel对应的回调函数，传入的回调函数均为TcpConnection
 * 对象的成员方法，因此可以说明一点就是：Channel的结束一定晚于TcpConnection对象！
 * 此处用tie去解决TcpConnection和Channel的生命周期时长问题，从而保证了Channel对象能够在
 * TcpConnection销毁前销毁。
 **/
void Channel::tie(const std::shared_ptr<void>&obj){
    tie_=obj;
    tied_=true;
}

//update和remove=>EpollPoller 更新channe在poller中的状态
//当改变channel所表示的fd的events事件后，update负责poller里改变fd相应的事件epoll——ctl；
void Channel::update(){
    loop_->updateChannel(this);
}

void Channel::remove(){
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime){
    if(tied_){
        std::shared_ptr<void>guard=tie_.lock();
        if(guard){
            handleEventWithGuard(receiveTime);
        }
        else {
            handleEventWithGuard(receiveTime);
        }

    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime){
    LOG_INFO("channel handleEvent revents:%d\n",revents_);
    if((revents_&EPOLLHUP)&&!(revents_&EPOLLIN)){//
        if(closeCallBack){
            closeCallBack();
        }
    }
    if(revents_&EPOLLERR){
        if(errnoCallBack)errnoCallBack();
    }
    if(revents_&(EPOLLIN|EPOLLPRI)){
        if(readCallBack)readCallBack(receiveTime);
    }
    if(revents_&EPOLLOUT){
        if(writeCallBack)writeCallBack();
    }
}    