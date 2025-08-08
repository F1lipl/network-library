#pragma once
#include<functional>
#include<memory>

#include "noncopyable.h"
#include"Timestamp.h"
class Eventloop;

class Channel:noncopyable
{
private:
    using EventCallback=std::function<void()>;// 事件回调函数
    using ReadEventCallback=std::function<void(Timestamp)>;
    void update();
    void handleEventWithGuard(Timestamp recevieTime);

    static const int kNoneEvent;//用来设置epoll_event.events
    static const int kReadEvent;
    static const int kWriteEvent;

    Eventloop* loop_;//事件循环
    const int fd_;//fd，poller监听对象
    int events_;//注册fd感兴趣的事件
    int revents_;//poller返回的具体发生的事件
    int index_;

    std::weak_ptr<void>tie_;
    bool tied_;
    //channel 会知道fd里具体的发生的事件是什么，所以它应该负责回调；
    ReadEventCallback readCallBack;
    EventCallback writeCallBack;
    EventCallback closeCallBack;
    EventCallback errnoCallBack;
    
public:
    Channel(Eventloop*loop,int fd);
    ~Channel();
    void handleEvent(Timestamp receiveTime);

    //设置回调函数
    void setReadCallback(ReadEventCallback cb){
        readCallBack=std::move(cb);
    }
    void setWriteCallback(EventCallback write){
        writeCallBack=std::move(write);
    }
    void setCloseCallback(EventCallback close){
        closeCallBack=std::move(close);
    }
    void setErrnoCallback(EventCallback errno){
        errnoCallBack=std::move(errno);
    } 

    void tie(const std::shared_ptr<void>&);
    int fd()const{
        return fd_;
    }
    int events()const{
        return events_;
    }
    void set_revents(int revt){
        revents_=revt;
    }
    

    //设置fd相应的事件状态
    void enableReading(){
        events_|=kReadEvent;
        update();
    }
    void disableReading(){
        events_&=~kReadEvent;
        update();
    }
    void enableWriting(){
        events_|=kWriteEvent;
        update();
    }
    void disableWriting(){
        events_&=~kWriteEvent;
        update();
    }
    void disableAll(){
        events_=kNoneEvent;
        update();
    }
    // 返回fd当前的事件状态
    bool isNoneEvent()const {return events_==kNoneEvent;}
    bool isWriting()const {return events_&kWriteEvent;}
    bool isreading()const {return events_&kReadEvent;}

    int index(){return index_;}
    void set_index(int idx){index_=idx;}
    
    // one loop per thread

    Eventloop* ownerLoop(){return  loop_;}
    void remove();

};

