#pragma once
#include<functional>
#include<vector>
#include<atomic>
#include<memory>
#include<mutex>

#include"noncopyable.h"
#include"Timestamp.h"
#include"CurrentThread.h"

class Poller;
class Channel;

class Eventloop:noncopyable
{
private:
    void handleRead();//
    void doPendingFunctors();//执行上层回调

    using ChannelList=std::vector<Channel*>;

    std::atomic_bool looping_;//原子操作,通过底层cas实现
    std::atomic_bool quit_;//标识退出loop循环

    const pid_t threadId_;//记录当前eventloop是被哪个线程id创建的,即标记当前eventloop属于哪个id的线程;
    
    Timestamp pollerReturnTime_;//poller 返回发生事件的channel的时间戳;
    std::unique_ptr<Poller>poller_;

    int wakeupFd_;//保存创建的eventfd
    std::unique_ptr<Channel>wakeupChannel_;//作用：当mainLoop获取一个新用户的Channel 需通过轮询算法选择一个subLoop 通过该成员唤醒subLoop处理Channel

    ChannelList activeChannels_;//返回poller检测到当期所有有事件发生的Channel列表;

    std::atomic_bool callingPendingFunctors_;//标识当前loop是否有需要执行的回调操作
    using Functor=std::function<void()>;
    std::vector<Functor>pendingFunctors_;//存储loop需要执行的所有回调操作
    std::mutex mutex_;//互斥锁

public:
    Eventloop();
    ~Eventloop();
    void loop();
    void quit();
    Timestamp pollerReturnTime()const{
        return pollerReturnTime_;
    }
    void runInloop(Functor cb);//把上层注册的回调函数cb放入队列里,唤醒loop所在的线程执行cb
    void queueInloop(Functor cb);

    void wakeup();//通过eventfd唤醒loop所在线程

    //eventloop的方法=>Poller的方法
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    //判断eventloop对象是否在自己的线程里
    bool isInLoopThread()const{return threadId_==CurrentThread::tid();}

};