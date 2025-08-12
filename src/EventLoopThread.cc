#include"EventLoopThread.h"
#include"Eventloop.h"

EventLoopThread::EventLoopThread(const  ThreadInitCallback &cb,std::string&name):loop_(nullptr),
mutex_(),
cond_(),
exciting_(false),
Callback_(cb),
thread_(std::bind(&EventLoopThread::ThreadFunc,this),name)
{}

 
EventLoopThread::~EventLoopThread(){
    exciting_=true;
    if(!loop_){
        loop_->quit();
        thread_.join();
    }
}

void EventLoopThread::ThreadFunc() {
    Eventloop loop;                       // 在新线程中创建事件循环对象
    if(Callback_) {
        Callback_(&loop);                // 初始化回调（如设置事件处理器）
    }
    
    { // 临界区开始
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;                   // 将事件循环指针暴露给外部
        cond_.notify_one();              // 通知主线程初始化完成
    } // 临界区结束（锁自动释放）

    loop.loop();                         // 启动事件循环（阻塞直到退出）
    
    { // 临界区开始
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = nullptr;                 // 事件循环结束，置空指针
    }
}



Eventloop* EventLoopThread::startLoop() {
    thread_.start();                  // 启动工作线程
    Eventloop *loop = nullptr;        // 准备接收事件循环指针
    
    { // 临界区开始
        std::unique_lock<std::mutex> lock(mutex_);
        // 等待新线程完成初始化（loop_不为空）
        cond_.wait(lock, [this]{ return loop_ != nullptr; }); 
        loop = loop_;                 // 获取事件循环指针
    } // 临界区结束（锁自动释放）
    
    return loop;                      // 返回事件循环对象指针
}
