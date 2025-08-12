#include<memory>
#include"EventLoopTheadPool.h"
#include"EventLoopThread.h"
#include"Logger.h"


EventLoopThreadPool::EventLoopThreadPool(Eventloop* baseloop,const std::string &name):loop_(baseloop),
name_(name),
numThreads_(0),
started_(false),
next_(0)
{}
EventLoopThreadPool::~EventLoopThreadPool(){}

void EventLoopThreadPool::start(const ThreadInitCallback &cb=ThreadInitCallback()){
    started_=true;
    for(int i=0;i<numThreads_;++i){
        char buf[name_.size()+32];
        snprintf(buf,sizeof buf,"%s%d",name_.c_str(),i);
        EventLoopThread* t=new EventLoopThread(cb,buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop());//底层创建线程，绑定一个新的eventloop，并返回该地址
    }
    if(numThreads_==0&&cb){//整个服务端只有一个线程运行baseloop
        cb(loop_);
    }
}

//如果工作在多线程中，baseloop会默认已轮询的方式把channel分配给subloop
Eventloop* EventLoopThreadPool::getNextLoop(){
     // 如果只设置一个线程 也就是只有一个mainReactor 无subReactor 
    // 那么轮询只有一个线程 getNextLoop()每次都返回当前的baseLoop_
    Eventloop *loop = loop_;    

    // 通过轮询获取下一个处理事件的loop
    // 如果没设置多线程数量，则不会进去，相当于直接返回baseLoop
    if(!loops_.empty())             
    {
        loop = loops_[next_];
        ++next_;
        // 轮询
        if(next_ >= loops_.size())
        {
            next_ = 0;
        }
    }

    return loop;
}


std::vector<Eventloop *> EventLoopThreadPool::getAllLoops()
{
    if (loops_.empty())
    {
        return std::vector<Eventloop *>(1,loop_);
    }
    else
    {
        return loops_;
    }
}


