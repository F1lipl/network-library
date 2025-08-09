#pragma once 
#include<functional>
#include"noncopyable.h"
#include"Socket.h"
#include"Channel.h"



class Acceptor:noncopyable{
public:
    using NewConnectionCallback=std::function<void(int sockfd,const inetaddress&)>;
    Acceptor(Eventloop* loop,const inetaddress& listenAddr,bool requestport);
    ~Acceptor();
    //设置新的回调函数
    void setConnectionCallback(const NewConnectionCallback& cb){NewConnectionCallback_=cb;}
    //判断是否在监听
    bool listening()const{return listening_;}
    //监听本地端口
    void listen();

private:
    void handleRead();
    Eventloop *loop_;
    bool listening_;
    Channel acceptChannel_;
    Socket acceptSocket_;
    NewConnectionCallback NewConnectionCallback_;


};
