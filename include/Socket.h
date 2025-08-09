#pragma once
#include "noncopyable.h"
#include "inetaddress.h"

class inetaddress;

class Socket:noncopyable
{
private:
    const int sockfd;
public:
    Socket(int sock):sockfd(sock){};
    ~Socket();
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

