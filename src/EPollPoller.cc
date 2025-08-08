#include<errno.h>
#include<unistd.h>
#include<string.h>  

#include"EPollPoller.h"
#include"Logger.h"
#include"Channel.h"

const int kNew=-1;// 某个Channel还没添加到poller里
const int kAdded=1;//channel已经在poller里
const int kDeleted=2;//channel已经从poller里删除

EPollPoller::EPollPoller(Eventloop*loop):epollfd_(epoll_create1(EPOLL_CLOEXEC)),
Poller(loop),
events(KInitEventListSize)
{
    if(epollfd_<0)LOG_FATAL("epoll_create error:%d\n",errno);
}

EPollPoller::~EPollPoller(){
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMS,ChannelList_ *activeChannel){
    //由于频繁调用poll 实际上用log_debug输出日志更为合理 当遇到并发场景 关闭debug日志提高并发效率
    LOG_INFO("func=%s=>fd total count:%lu\n",__FUNCTION__,channels_.size());
    int nums=::epoll_wait(epollfd_,&*events.begin(),events.size(),timeoutMS);
    int saveErrno =errno;
    Timestamp now(Timestamp::now());
    if(nums>0){
        LOG_INFO("%d events happend\n",nums);
        fillActiveChannels(nums,activeChannel);
        if(nums==events.size())events.resize(2*events.size());
    }
    else if(nums==0)
    {
        LOG_DEBUG("%s timeout!\n",__FUNCTION__);
    }
    else {
        if(saveErrno!=EINTR){
            errno=saveErrno;
            LOG_ERROR("EPollPoller::poll() error!");
        }
    }
    return now;
}


void EPollPoller::updateChannel(Channel*channel){//更新channel在poller里的状态
    int index=channel->index();
    LOG_INFO("func=%s=>fd=%d events=%d index=%d\n",__FUNCTION__,channel->fd(),channel->events(),index);
    if(index==kNew||index==kDeleted){
        if(index==kNew){
            int fd=channel->fd();
            channels_[fd]=channel;
        }
        if(index==kDeleted){
            //channels_.erase(channel->fd());
        }
        channel->set_index(kAdded);
        update(channel,EPOLL_CTL_ADD);
    }
    else {//kAdded
        int fd=channel->fd();
        if(channel->isNoneEvent()){
            update(channel,EPOLL_CTL_DEL);
            channel->set_index(kDeleted);
        }
        else{
            update(channel,EPOLL_CTL_MOD);
        }

    }
}

// 从poller里删除channel

void EPollPoller::removeChannel(Channel* channel){
    int fd=channel->fd();
    channels_.erase(fd);
    LOG_INFO("func=%s=>fd=%d\n",__FUNCTION__,fd);
    int index=channel->index();
    if(index==kAdded)update(channel,EPOLL_CTL_DEL);
    channel->set_index(kNew);
}

//填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents,ChannelList_*activechannel)const{
    for(int i=0;i<numEvents;++i){
        Channel*channel=static_cast<Channel*>(events[i].data.ptr);
        //每个channel注册时会默认会调用update（），update函数的本质是创建一个epoll_event其中的ptr会指向channel*；
        channel->set_revents(events[i].events);
        activechannel->push_back(channel);//eventloop拿到有事件发生的channel
    }
}


void EPollPoller::update(Channel*channel,int option){
    epoll_event event;
    ::memset(&event,0,sizeof event);
    event.data.fd=channel->fd();
    event.events=channel->events();
    event.data.ptr=channel;
    if(::epoll_ctl(epollfd_,option,channel->fd(),&event)<0){
        if(option==EPOLL_CTL_DEL)LOG_ERROR("epoll_ctl del errno:%d\n",errno);
        else LOG_ERROR("epoll_ctl add/mod errno:%d\n",errno);
    }
}

