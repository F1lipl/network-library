#pragma once


#include<functional>
#include<thread>
#include<memory>
#include<unistd.h>
#include<string>
#include<atomic>

class Thread
{
private:
    void setDefalutName();

    bool started_;//是否start
    bool join_;//是否join
    std::shared_ptr<std::thread>thread_;
    pid_t tid_;
    using ThreadFunc=std::function<void()>;
    ThreadFunc func_;
    std:: string name_;
    static std::atomic_int numCreated_;
public:
    explicit Thread(ThreadFunc,const std::string &name=std::string());
    ~Thread();
    void start();
    void join();

    bool started(){return started_;}
    pid_t tid()const{return tid_;}
    const std::string &name()const{
        return name_;
    }
    static int numCreated(){
        return numCreated_;
    }

};

