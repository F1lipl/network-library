#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

#include "Eventloop.h"
#include "Logger.h"
#include "Channel.h"
#include "Poller.h"

//防止一个线程创建多个eventloop
__thread Eventloop* t_loopInThisThread=nullptr;

//定义默认poller的io复用接口的超时时间
const int kPollTime=10000;

/* 创建线程之后主线程和子线程谁先运行是不确定的。
 * 通过一个eventfd在线程之间传递数据的好处是多个线程无需上锁就可以实现同步。
 * eventfd支持的最低内核版本为Linux 2.6.27,在2.6.26及之前的版本也可以使用eventfd，但是flags必须设置为0。
 * 函数原型：
 *     #include <sys/eventfd.h>
 *     int eventfd(unsigned int initval, int flags);
 * 参数说明：
 *      initval,初始化计数器的值。
 *      flags, EFD_NONBLOCK,设置socket为非阻塞。
 *             EFD_CLOEXEC，执行fork的时候，在父进程中的描述符会自动关闭，子进程中的描述符保留。
 * 场景：
 *     eventfd可以用于同一个进程之中的线程之间的通信。
 *     eventfd还可以用于同亲缘关系的进程之间的通信。
 *     eventfd用于不同亲缘关系的进程之间通信的话需要把eventfd放在几个进程共享的共享内存中（没有测试过）。
 */
// 创建wakeupfd 用来notify唤醒subReactor处理新来的channel
int createEventfd(){
    int eventfd=::eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);
    if(eventfd<0){
        LOG_FATAL("eventfd error:%d\n",errno);
    }
    return eventfd;
}
Eventloop::Eventloop():looping_(false),
quit_(false),
threadId_(CurrentThread::tid()),
callingPendingFunctors_(false),
poller_(Poller::newDefaultPoller(this)),
wakeupFd_(createEventfd()),
wakeupChannel_(new Channel(this,wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d\n", this, threadId_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d\n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }
    
    wakeupChannel_->setReadCallback(std::bind(&Eventloop::handleRead, this)); // 设置wakeupfd的事件类型以及发生事件后的回调操作
    wakeupChannel_->enableReading(); // 每一个EventLoop都将监听wakeupChannel_的EPOLL读事件了
}


Eventloop::~Eventloop(){
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread=nullptr;
}

void Eventloop::loop(){
    looping_=true;
    quit_=false;
    LOG_INFO("Eventloop %p start looping\n",this);

    while(!quit_){
        activeChannels_.clear();
        pollerReturnTime_=poller_->poll(kPollTime,&activeChannels_);
        for(Channel*Channel:activeChannels_){
            Channel->handleEvent(pollerReturnTime_);
            doPendingFunctors();
        }
    }
    /**
         * 执行当前EventLoop事件循环需要处理的回调操作 对于线程数 >=2 的情况 IO线程 mainloop(mainReactor) 主要工作：
         * accept接收连接 => 将accept返回的connfd打包为Channel => TcpServer::newConnection通过轮询将TcpConnection对象分配给subloop处理
         *
         * mainloop调用queueInLoop将回调加入subloop（该回调需要subloop执行 但subloop还在poller_->poll处阻塞） queueInLoop通过wakeup将subloop唤醒
         **/
    LOG_INFO("EventLoop %p stop looping.\n", this);
    looping_ = false;
}

void Eventloop::quit(){
    quit_=true;
    if(!isInLoopThread()){
        wakeup();
    }
}

void Eventloop::runInloop(Functor cb){
    if(isInLoopThread()){//如果loop在自己所在的线程里
        cb();
    }
    else{
        queueInloop(cb);
    }
}

void Eventloop::queueInloop(Functor cb){
    std::unique_lock<std::mutex>lock(mutex_);
    pendingFunctors_.emplace_back(cb);
}

void Eventloop::handleRead(){
    uint64_t one=1;
    ssize_t n=read(wakeupFd_,&one,sizeof(one));
    if(n!=sizeof(one)){
        LOG_ERROR("Eventloop::handleRead() reads%lu bytes instead of 8\n",n);
    }
}


//用来唤醒loop所在线程 向wakeupFd_写一个数据 wakeupchannel就发生读事件 当前loop线程就会被唤醒
void Eventloop::wakeup(){
    uint64_t one=1;
    ssize_t n=write(wakeupFd_,&one,sizeof(one));
    if(n!=sizeof(one)){
        LOG_ERROR("Eventloop::wakeup() wtites%lu bytes instead of 8\n");
    }
}


// EventLoop的方法 => Poller的方法
void Eventloop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void Eventloop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

bool Eventloop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

void Eventloop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_); // 交换的方式减少了锁的临界区范围 提升效率 同时避免了死锁 如果执行functor()在临界区内 且functor()中调用queueInLoop()就会产生死锁
    }

    for (const Functor &functor : functors)
    {
        functor(); // 执行当前loop需要执行的回调操作
    }

    callingPendingFunctors_ = false;
}