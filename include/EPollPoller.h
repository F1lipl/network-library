#pragma once
#include<vector>
#include<sys/epoll.h>
#include"Poller.h"
#include"Eventloop.h"
#include<Timestamp.h>

class EPollPoller:public Poller{

public:
    EPollPoller(Eventloop*loop);
    ~EPollPoller()override;

    Timestamp poll(int timeoutMS,ChannelList_*activeChannel)override;
    void updateChannel(Channel*Channel)override;
    void removeChannel(Channel*Channel)override;

private:
    static const int KInitEventListSize=16;
    void fillActiveChannels(int numEvetns,ChannelList_*activeChannel)const;//向channellist里写入活跃的channel
    
    
    //更新channel的状态
    void update(Channel*Channel,int option);

    using EventList=std::vector<epoll_event>;

    int epollfd_;//poller类就相当于epoll_event结构体的表用来监听每个epoll-event事件
    EventList events;




};

