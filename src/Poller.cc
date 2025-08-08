#include"Poller.h"
#include"Channel.h"


Poller::Poller(Eventloop* loop):ownerloop_(loop){}

bool Poller::hasChannel(Channel* channel)const{
    if(channels_.find(channel->fd())!=channels_.end())return true;
    else return false;
}