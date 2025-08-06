#pragma once
#include<functional>
#include<vector>
#include<atomic>
#include<memory>
#include<mutex>

#include"noncopyable.h"
#include"Timestamp.h"

class Poller;
class Channel;

class Eventloop
{
private:
    
public:
    Eventloop();
    ~Eventloop();;

};