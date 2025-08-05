#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<netinet/tcp.h>
#include "socket.h"
#include "inetaddress.h"
#include "noncopyable.h"

socket::~socket(){
    close(sockfd);
}
void socket::bindaddress(const inetaddress& addr){
    const sockaddr_in *m_sock=addr.getSockAddr();
    if(bind(sockfd,(sockaddr*)m_sock,sizeof(*m_sock))!=0){
        
    }
}
void socket::listen(){
    if(::listen(sockfd,1024)!=0){

    }
}

int socket::accept(inetaddress *addr){
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

void socket::shutdownwrite(){
    ::shutdown(sockfd,SHUT_WR);
}

void socket::setTcpNoDelay(bool on){
    
    int option=on?1:0;
    ::setsockopt(sockfd,IPPROTO_TCP,TCP_NODELAY,&option,sizeof(option));
}

void socket::setKeepAlive(bool on){
    int option=on ?1:0;
    ::setsockopt(sockfd,SOL_SOCKET,SO_KEEPALIVE,&option,sizeof option);
}

void socket::setReuseAddr(bool on){
    int option =on?1:0;
    ::setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&option,sizeof option);
}
void socket::setReusePort(bool on){
    int option=on ?1:0;
    ::setsockopt(sockfd,SOL_SOCKET,SO_REUSEPORT,&option,sizeof option);
}