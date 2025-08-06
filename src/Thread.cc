#include"Thread.h"
#include"CurrentThread.h"
#include<semaphore.h>

std::atomic_int Thread::numCreated_(0);
Thread::Thread(ThreadFunc func,const std::string &name):started_(false),join_(false),tid_(0),func_(std::move(func)),name_(name){
    setDefalutName();
}
//move 函数把左值转换成右值，告诉编译器这个地址的内容我已经用另外一个指针或变量保存了，原来的指针和变量可以被舍弃了
Thread::~Thread(){
    if(started_&&!join_){
        thread_->detach();
    }
}
void Thread::start(){
    started_=true;
    sem_t m_sem;
    sem_init(&m_sem,0,0);
    thread_=std::shared_ptr<std::thread>(new std::thread([&]{
        tid_=CurrentThread::tid();
        sem_post(&m_sem);
        func_();
    }));
    sem_wait(&m_sem);
}
void Thread::join(){
    join_=true;
    thread_->join();
}
void Thread:: setDefalutName(){
    int num=++numCreated_;
    if(name_.empty()){
        char buf[32]={0};
        snprintf(buf,sizeof buf,"Thread%d",num);
        name_=buf;
    }

}