#include<string.h>
#include<strings.h>

#include "inetaddress.h"

inetaddress:: inetaddress(uint16_t port,std::string ip){
    sock_address.sin_family=AF_INET;
    sock_address.sin_port=htons(port);
    sock_address.sin_addr.s_addr=inet_addr(ip.c_str());//c_str()把string转换成char
}

std::string inetaddress::toIp()const{
    char buf[64]={0};
    inet_ntop(AF_INET,&sock_address.sin_addr,buf,sizeof buf);//inet_pton反过来了，把ip地址转换成字符串；
    return buf;
}
std::string inetaddress:: toIport()const{
    char buf[64]={0};
    inet_ntop(AF_INET,&sock_address.sin_addr,buf,sizeof buf);
    size_t len=sizeof(buf);
    uint16_t port=ntohs(sock_address.sin_port);
    sprintf(buf+len,":%u",port);
    return buf;
}
uint16_t inetaddress::toport()const{
    return ntohs(sock_address.sin_port);
    }