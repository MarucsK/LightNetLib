#pragma once 

#include "noncopyable.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Acceptor.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"

#include <functional>
#include <memory>
#include <string>
#include <atomic>
#include <unordered_map>

// 对外的服务器编程使用的类
class TcpServer : noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    enum Option
    {
        kNoReusePort,
        kReusePort,
    };
    TcpServer(EventLoop *loop, const InetAddress &listenAddr, 
        const std::string &nameArg, Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

    // 设置底层subloop个数
    void setThreadNum(int numThreads);

    // 开始服务器监听
    void start();

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    EventLoop *loop_; // mainloop,运行Acceptor
    std::unique_ptr<Acceptor> acceptor_; // 运行在mainloop,任务就是监听新连接

    const std::string ipPort_;
    const std::string name_;

    std::shared_ptr<EventLoopThreadPoll> threadPool_; // subloop threadpool

    ConnectionCallback connectionCallback_; // 连接状态变化的回调(建立或断开)
    MessageCallback messageCallback_; // 收到客户端数据时触发
    WriteCompleteCallback writeCompleteCallback_; // 消数据全部发送完成后的回调

    ThreadInitCallback threadInitCallback_; // subloop线程初始化的回调
    std::atomic_int started_;

    int nextConnId_;
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;
    ConnectionMap connections_; // 保存所有活跃的连接
};
