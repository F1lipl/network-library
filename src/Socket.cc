#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<netinet/tcp.h>
#include "Socket.h"
#include "inetaddress.h"
#include "noncopyable.h"

Socket::~Socket(){
    close(sockfd);
}
void Socket::bindaddress(const inetaddress& addr){
    const sockaddr_in *m_sock=addr.getSockAddr();
    if(bind(sockfd,(sockaddr*)m_sock,sizeof(*m_sock))!=0){
        
    }
}
void Socket::listen(){
    if(::listen(sockfd,1024)!=0){

    }
}

int Socket::accept(inetaddress *addr){
    sockaddr_in clnt_sock;
    ::memset(&clnt_sock,0,sizeof clnt_sock);
    socklen_t clnt_size;
    clnt_size=sizeof(clnt_sock);
    int clntfd=accept4(sockfd,(sockaddr*)&clnt_sock,&clnt_size,SOCK_NONBLOCK|SOCK_CLOEXEC);
    if(clntfd>=0){
        addr->setSockAddr(clnt_sock);
    }
    return clntfd;
}

void Socket::shutdownwrite(){
    ::shutdown(sockfd,SHUT_WR);
}

void Socket::setTcpNoDelay(bool on){
    
    int option=on?1:0;
    ::setsockopt(sockfd,IPPROTO_TCP,TCP_NODELAY,&option,sizeof(option));
}

void Socket::setKeepAlive(bool on){
    int option=on ?1:0;
    ::setsockopt(sockfd,SOL_SOCKET,SO_KEEPALIVE,&option,sizeof option);
}

void Socket::setReuseAddr(bool on){
    int option =on?1:0;
    ::setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&option,sizeof option);
}
void Socket::setReusePort(bool on){
    int option=on ?1:0;
    ::setsockopt(sockfd,SOL_SOCKET,SO_REUSEPORT,&option,sizeof option);
}