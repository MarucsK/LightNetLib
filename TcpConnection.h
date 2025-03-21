#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Timestamp.h"
#include "Callbacks.h"
#include "Buffer.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

/**
 * TcpServer => Acceptor => 有一个新用户连接,通过accept函数拿到connfd
 * => 打包TcpConnection 设置回调 => Channel => Poller => Channel的回调
 */
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>{
public:
    TcpConnection(EventLoop *loop,
                  const std::string &nameArg,
                  int sockfd,
                  const InetAddress &localAddr,
                  const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop *getloop() const { return loop_; }
    const std::string &name() const { return name_; }
    const InetAddress &localAddress() const { return localAddr_; }
    const InetAddress &peerAddress() const { return peerAddr_; }

    bool connected() const { return state_ == kConnected; }

    // 发送数据
    void send(const std::string &buf);
    // 关闭连接
    void shutdown();
    void shutdownInLoop();

    void setConnectionCallback(const ConnectionCallback &cb) 
    { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) 
    { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) 
    { writeCompleteCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
    { highWaterMarkCallback_=cb; highWaterMark_=highWaterMark; }
    void setCloseCallback(const CloseCallback &cb)
    { closeCallback_=cb; }

    // 连接建立
    void ConnectEstablished();
    // 连接销毁
    void connectDestroyed();

private:
    // 给channel的四个回调函数
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void *message, size_t len);

    EventLoop *loop_; // 指向管理此连接的subloop
    const std::string name_; // TcpConnection_1、TcpConnection_2
    enum StateE{ kDisconnected, kConnecting, kConnected, kDisconnecting };
    void setState(StateE state) { state_ = state; }
    std::atomic_int state_;
    
    bool reading_; // 是否正在监听读事件

    std::unique_ptr<Socket> socket_; // 封装 服务器的与客户端通信的fd
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;       // 连接状态变化的回调(建立或断开)
    MessageCallback messageCallback_;             // 收到新数据并存入inputBuffer_后触发此回调
    WriteCompleteCallback writeCompleteCallback_; // 数据全部发送完成后的回调
    HighWaterMarkCallback highWaterMarkCallback_; // 待发送数据超过阈值时触发
    CloseCallback closeCallback_; // 连接关闭时触发
    size_t highWaterMark_;

    Buffer inputBuffer_;  // 存储从socket读取的数据,供messageCallback_消费
    Buffer outputBuffer_; // 暂存待发送数据，应对TCP发送窗口满的情况。
};

/**理解：
 * 务必注意：socket_封装的是：服务器端的 与客户端通信的文件描述符！！！
 * (1)客户端发送数据 -> EventLoop监听到socket可读事件,调用TcpConnection::handleRead() -> handleRead()从socket读取数据到inputBuffer
 *    -> 数据存入inputBuffer后，触发messageCallback_，从inputBuffer读数据，供服务器处理数据 (messageCallback_中就可以执行服务器发送数据,把数据原封不动发回去)
 * (2)服务器发送数据,TcpConnection::send() -> 先写入socket_的写缓冲区,没写完的数据暂存在outputBuffer_
 *    -> 给socket_注册epollout, 只要可写就触发handleWrite, 把outputBuffer_中的数据写入到socket_的写缓冲区(内核会负责把它传到网络另一端)
 */