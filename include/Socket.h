#pragma once
#include "noncopyable.h"
#include "inetaddress.h"

class inetaddress;

class socket:noncopyable
{
private:
    const int sockfd;
public:
    socket(int sock):sockfd(sock){};
    ~socket();
    int get_fd() const {
        return sockfd;
    }
    void bindaddress(const inetaddress& addr);
    void listen();
    int accept(inetaddress *addr);
    void shutdownwrite();
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);







};

