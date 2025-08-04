#include<arpa/inet.h>
#include<netinet/in.h>
#include<string>

class inetaddress
{
private:
    sockaddr_in sock_address;
public:
    explicit inetaddress(uint16_t port=0,std::string ip="127.0.0.1");
    explicit inetaddress(sockaddr_in &m_address):sock_address(m_address){};
    std::string toIp()const{};
    std::string toIport()const{};
    uint16_t toport()const{};
    const sockaddr_in* getSockAddr()const{return &sock_address;};
    void setSockAddr(sockaddr_in &addr){sock_address=addr;};
    ~inetaddress();
};

