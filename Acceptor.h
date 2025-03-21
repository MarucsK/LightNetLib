#pragma once

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class EventLoop;
class InetAddress;

class Acceptor : noncopyable{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;

    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback &cb) { newConnectionCallback_ = cb; }

    bool listenning() const { return listenning_; }
    void listen();

private:
    void handleRead();

    EventLoop *loop_; // Acceptor用的就是用户定义的baseloop,也即mainloop
    Socket acceptSocket_; // 封装listenfd,负责监听
    Channel acceptChannel_; // 将acceptSocket_的文件描述符封装为channel, 注册到loop_(mainloop)的事件循环中，监听读事件（新连接）。
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};