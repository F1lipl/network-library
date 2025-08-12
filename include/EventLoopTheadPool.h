#pragma once

#include<string>
#include<functional>
#include<vector>
#include<memory>

#include"noncopyable.h"
#include"Eventloop.h"

class Eventloop;
class EventLoopThread;

class EventLoopThreadPool{
public:
    using ThreadInitCallback=std::function<void(Eventloop* )>;
    EventLoopThreadPool(Eventloop* baseLoop,const std::string &name);
    ~EventLoopThreadPool();
    void setThreadNum(int numThreads){numThreads_=numThreads;}
    void start(const ThreadInitCallback &cb=ThreadInitCallback());
    //如果工作在多线程中。baseloop会默认以轮询方式分配channel给subloop
    Eventloop* getNextLoop();
    std::vector<Eventloop*> getAllLoops();//获取所有的loops
    
    bool started(){return started_;}//是否已经启动
    const std::string name(){return name_;}//获取名字




private:
    Eventloop* loop_;//用户使用muduo创建的loop 如果线程为1，那直接使用用户创建的loop，否则创建多eventloop
    std::string name_;//线程池名称，多由用户直接指定，线程池中的eventloopthread名称依赖于所属线程池的名称
    bool started_;//判断线程池是否开启
    int numThreads_;//记录线程池里有多少个线程
    int next_;//新连接到来，所选择eventloop的索引
    std::vector<std::unique_ptr<EventLoopThread>>threads_;//IO线程的列表
    std::vector<Eventloop*>loops_;//线程池中的evenloop的列表，指向的是eventloopthread线程所创建的eventloop对象

};