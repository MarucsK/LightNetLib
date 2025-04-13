#include "Channel.h"
#include "EventLoop.h"
#include "logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop) // 把当前Channel添加到loop中(成员变量loop_代表此Channel属于哪个EventLoop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1)
    , tied_(false)
{
}

Channel::~Channel(){}

// 新创建一个TcpConnection时调用
// Channel执行的回调都是TcpConnection里的,如果TcpConnection被remove掉,Channel执行该回调结果未知
void Channel::tie(const std::shared_ptr<void> &obj){
    tie_ = obj;
    tied_ = true;
}

// 当改变Channel所表示fd的events事件后，update负责在poller里更改fd相应事件(epoll_ctl)
// EventLoop => 1个Poller + 若干Channel
void Channel::update(){
    // 通过Channel所属的EventLoop，调用poller的相应方法，注册fd的events事件
    loop_->updateChannel(this);
}

// 在Channel所属的EventLoop中，把当前的Channel删除掉
void Channel::remove(){
    loop_->removeChannel(this);
}

// fd得到poller通知以后，处理事件(调用相应的回调方法)
void Channel::handleEvent(Timestamp receiveTime){
    if(tied_){
        std::shared_ptr<void> guard = tie_.lock();
        if(guard){
            handleEventWithGuard(receiveTime);
        }
    }
    else{
        handleEventWithGuard(receiveTime);
    }
}

// 根据poller通知的channel发生的具体事件，由channel负责调用具体的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime){
    LOG_INFO("channel handleEvent revents:%d\n", revents_);

    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if(closeCallback_){
            closeCallback_();
        }
    }
    if(revents_ & EPOLLERR){
        if(errorCallback_){
            errorCallback_();
        }
    }
    if(revents_ & (EPOLLIN | EPOLLPRI)){
        if(readCallback_){
            readCallback_(receiveTime);
        }
    }
    if(revents_ & EPOLLOUT){
        if(writeCallback_){
            writeCallback_();
        }
    }
}
