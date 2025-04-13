#include "Acceptor.h"
#include "logger.h"
#include "InetAddress.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

static int createNonblocking(){
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if(sockfd < 0){
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop)
    , acceptSocket_(createNonblocking()) // 创建非阻塞socket
    , acceptChannel_(loop, acceptSocket_.fd())
    , listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr); // 将socket与ip+port绑定
    // TcpServer::start() Acceptor.listen 有新用户连接,要执行一个回调(connfd -> channel -> subloop)
    // baseLoop监听到acceptChannel_(listenfd)的读事件(即有新连接) => 执行handleRead回调
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor(){
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

// mainloop开始监听新连接
void Acceptor::listen(){
    listenning_ = true;
    acceptSocket_.listen();         // 开始监听
    acceptChannel_.enableReading(); // acceptChannel => poller
}

// listenfd有读事件发生,即有新用户连接
void Acceptor::handleRead(){
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >= 0){
        if(newConnectionCallback_){
            newConnectionCallback_(connfd, peerAddr); // 轮询找到subloop,唤醒并分发当前新channel
        }
        else{
            ::close(connfd);
        }
    }
    else{
        LOG_ERROR("%s:%s:%d accept err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
        if(errno == EMFILE){
            LOG_ERROR("%s:%s:%d sockfd reached limit! \n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}
