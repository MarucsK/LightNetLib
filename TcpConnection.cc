#include "TcpConnection.h"
#include "logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string>

static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnection Loop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                           const std::string &nameArg,
                           int sockfd,
                           const InetAddress &localAddr,
                           const InetAddress &peerAddr)
    : loop_(CheckLoopNotNull(loop))
    , name_(nameArg)
    , state_(kConnecting)
    , reading_(true)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop,sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , highWaterMark_(64*1024*1024) // 64M
{
    // 给channel_设置相应的回调函数,poller给channel通知它感兴趣的事件发生,channel会回调相应的操作函数
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d \n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection(){
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n", name_.c_str(), channel_->fd(), (int)state_);
}

// 发送数据到客户端
void TcpConnection::send(const std::string &buf){
    if(state_ == kConnected){
        if(loop_->isInLoopThread()){
            sendInLoop(buf.c_str(), buf.size());
        }
        else{
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

void TcpConnection::sendInLoop(const void *data, size_t len){
    ssize_t nwrote = 0;
    ssize_t remaining = len;
    bool faultError = false;
    // 之前调用过该connection的shutdown,不能再发送
    if(state_ == kDisconnected){
        LOG_ERROR("disconnected, give up writing!");
        return;
    }
    // 当前Channel未注册可写事件监听,说明此时内核发送缓冲区可能未满; outputBuffer_没有待发送数据
    // 这说明fd的内核写缓冲区可能未满,可以尝试直接往里发送数据
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0){
        nwrote = ::write(channel_->fd(), data, len);
        if(nwrote >= 0){
            // 剩余未发送的数据长度
            remaining = len - nwrote;
            if(remaining == 0 && writeCompleteCallback_){
                // 既然在这里数据全部发送完成,就不用再给channel设置epollout事件了,不会再执行handleWrite
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                // 如果在这里没发完,说明fd写缓冲区满了,那就注册epollout事件,等待可写
            }
        }
        else{ // nwrote < 0
            if(errno != EWOULDBLOCK){
                LOG_ERROR("TcpConnection::sendInLoop");
                if(errno == EPIPE || errno == ECONNRESET){
                    faultError = true;
                }
            }
        }
    }
    // 没出错,还有剩余未发送数据,剩余的数据需要保存到outputBuffer_缓冲区中,然后给channel注册epollout事件
    // poller会监听,当发现fd的写缓冲区有空间后会通知相应的channel,调用writeCallback_回调方法
    // 也就是调用TcpConnection::handleWrite方法,把outputBuffer_继续写入fd的写缓冲区
    if(!faultError && remaining > 0){
        size_t oldLen = outputBuffer_.readableBytes();
        // oldLen : outputBuffer_中目前剩余的待发送的数据长度
        // remaining : 参数data还没有发完的数据长度, 需要把这段数据保存到outputBuffer_中
        if(oldLen + remaining >= highWaterMark_ 
            && oldLen < highWaterMark_
            && highWaterMarkCallback_)
        {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append((char *)data + nwrote, remaining);
        if(!channel_->isWriting()){
            channel_->enableWriting();
        }
    }
}

// 连接建立
void TcpConnection::ConnectEstablished(){
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading(); // 向poller注册channel的读事件

    // 新连接建立,执行回调
    connectionCallback_(shared_from_this());
}
// 连接销毁
void TcpConnection::connectDestroyed(){
    if(state_ == kConnected){
        setState(kDisconnected);
        channel_->disableAll(); // 把channel所有感兴趣的事件从poller中del
        connectionCallback_(shared_from_this());
    }
    channel_->remove(); // 把channel从poller中del
}

void TcpConnection::shutdown(){
    if(state_ == kConnected){
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop(){
    if(!channel_->isWriting()){ // 说明outputBuffer_中的数据已经全部发送完成
        // 关闭写端,触发channel的EPOLLHUP,则channel调用closeCallback_回调,即TcpConnection::handleClose
        socket_->shutdownWrite(); 
    }
}

// 接收客户端的数据
// 监听channel->fd的读事件,当fd里有数据来了,则可读,调用此handleRead回调,把fd里的数据读到inputBuffer_
// 然后触发messageCallback_
// 也就是说,客户端发来的数据,channel的读回调仅负责把它读到inputBuffer_,对于发来的数据真正的处理是在messageCallback_
void TcpConnection::handleRead(Timestamp receiveTime){
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &saveErrno);
    if(n > 0){
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if(n == 0){
        handleClose();
    }
    else{
        errno = saveErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

// 监听channel->fd的写事件,当fd可写(即内核发送缓冲区有空间),把outputBuffer_里的数据写入到fd
void TcpConnection::handleWrite(){
    if(channel_->isWriting()){
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno); // 将outputBuffer_中的数据写入内核发送缓冲区。
        if(n > 0){
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes() == 0){
                // 如果写完之后outputBuffer_没有数据了,就不要再监听fd的的写事件了,否则一直监听它可写就要一直调用handleWrite,而又没东西可写
                channel_->disableWriting(); 
                if(writeCompleteCallback_){
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
            }
            if(state_ == kDisconnecting){
                shutdownInLoop();
            }
        }
        else{
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else{
        LOG_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());
    }
}
void TcpConnection::handleClose(){
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();
    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr); // 执行连接关闭的回调
    closeCallback_(connPtr); // 关闭连接的回调 执行的是TcpServer::removeConnection回调方法
}
void TcpConnection::handleError(){
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0){
        err = errno;
    }
    else{
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
}