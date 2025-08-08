#pragma once
#include<vector>
#include<unordered_map>
#include"noncopyable.h"
#include"Timestamp.h"

class Channel;
class Evetloop;

class Poller{
public:
    using ChannelList_=std::vector<Channel*>;
    Poller(Eventloop*);
    virtual ~Poller()=default;


    //给所有的io复用保留统一的接口
    virtual Timestamp poll(int timeoutMS,ChannelList_* activechannel)=0;
    virtual void updateChannel(Channel* channel)=0;
    virtual void removeChannel(Channel* channel)=0;

    //判断channel是否在这个poller里
    bool hasChannel(Channel* channel)const;

    //eventloop可以通过该接口获取默认的io复用的具体实现
    static Poller *newDefaultPoller(Eventloop* loop);


protected:
    //map的key：sockfd
    using ChannelMap=std::unordered_map<int,Channel*>;
    ChannelMap channels_;



private:
    Eventloop *ownerloop_;

};