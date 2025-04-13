#include "TcpServer.h"
#include "logger.h"
#include "TcpConnection.h"

#include <functional>
#include <strings.h>

static EventLoop *CheckLoopNotNull(EventLoop *loop){
    if(loop == nullptr){
        LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop, 
                     const InetAddress &listenAddr, 
                     const std::string &nameArg, 
                     Option option)
                     : loop_(CheckLoopNotNull(loop))   // mainloop  
                     , ipPort_(listenAddr.toIpPort())
                     , name_(nameArg)
                     , acceptor_(new Acceptor(loop,listenAddr,option == kReusePort))
                     , threadPool_(new EventLoopThreadPoll(loop,name_))
                     , connectionCallback_()
                     , messageCallback_()
                     , nextConnId_(1)
                     , started_(0)
{
    // 当有新用户连接时,会执行TcpServer::newConnection回调
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
                                                  std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer(){
    for(auto& item : connections_){
        TcpConnectionPtr conn(item.second); // 这个局部的shared_pte出作用域可以自动释放new出来的TcpConnection对象资源
        item.second.reset();
        // 销毁连接
        conn->getloop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadNum(int numThreads){
    threadPool_->setThreadNum(numThreads);
}

// 开始服务器监听
void TcpServer::start(){
    if(started_++ == 0){ // 防止一个TcpServer对象被重复启动多次
        threadPool_->start(threadInitCallback_); // 启动底层的线程池
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

// 有一个新的客户端连接,acceptor会执行此回调操作
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr){
    // 轮询算法,选择一个subloop
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n", 
        name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());
    
    // 通过sockfd获取其绑定的本机的ip+port
    sockaddr_in local;
    bzero(&local, sizeof(local));
    socklen_t addrlen = sizeof(local);
    if(::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0){
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);

    //根据连接成功的sockfd,创建TcpConnection连接对象
    TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));

    connections_[connName] = conn;
    
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));//设置了如何关闭连接的回调

    ioLoop->runInLoop(std::bind(&TcpConnection::ConnectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn){
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn){
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n", name_.c_str(), conn->name().c_str());

    size_t n = connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getloop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
