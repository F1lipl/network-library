#pragma once 

#include<functional>
#include<mutex>
#include<condition_variable>
#include<string>

#include"noncopyable.h"
#include"Thread.h"

class Eventloop;


class EventLoopThread:noncopyable{
public:
    using ThreadInitCallback=std::function<void(Eventloop*loop_)>;
    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),const std::string& name = std::string());
    ~EventLoopThread();
    Eventloop* startLoop();

private:
    void ThreadFunc();
    
    Eventloop*loop_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback Callback_;
    bool exciting_;
};