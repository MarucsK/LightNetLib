#include "InetAddress.h"

#include <string.h>
#include <iostream>

InetAddress::InetAddress(uint16_t port, std::string ip){
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
}
// 取 addr_中的ip(要转成主机字节序)
std::string InetAddress::toIp() const{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return buf;
}
// 取 addr_中的ip+port(要转成主机字节序)
std::string InetAddress::toIpPort() const{
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf + end, ":%u", port);
    return buf;
}
// 取 addr_中的port(要转成主机字节序)
uint16_t InetAddress::toPort() const{
    return ntohs(addr_.sin_port);
}

// int main(){
//     InetAddress addr(8080);
//     std::cout << addr.toIpPort() << std::endl;
//     std::cout << addr.toIp() << std::endl;
//     std::cout << addr.toPort() << std::endl;
// }
