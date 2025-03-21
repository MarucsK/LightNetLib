#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

/// Reactor

// 事件循环类 主要包含两个大模块 Channel Poller(epoll的抽象)
class EventLoop : noncopyable {
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();
    
    // 开始事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    // 在当前loop中执行cb
    void runInLoop(Functor cb);
    // 把cb放入队列中，唤醒loop所在线程，执行cb
    void queueInLoop(Functor cb);

    // 唤醒loop所在的线程
    void wakeup();

    // EventLoop =》Poller
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断EventLoop对象是否在自己的线程里
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

private:
    void handleRead(); 
    void doPendingFunctors();

    using ChannelList = std::vector<Channel *>;

    std::atomic_bool looping_; // 标志 事件循环运行
    std::atomic_bool quit_; // 标志 事件循环终止
    
    const pid_t threadId_; // 记录当前loop所在线程的id
    Timestamp pollReturnTime_; // poller返回发生事件的channels 的时间
    std::unique_ptr<Poller> poller_; // 当前loop管理的poller

    int wakeupFd_; // 作用: 当mainloop获取一个新的channel,通过轮询算法选择一个subloop,通过该成员唤醒subloop处理channel
    std::unique_ptr<Channel> wakeupChannel_; // 封装wakeupFd_为Channel

    ChannelList activeChannels_; // 临时存储一次事件循环中检测到的所有活跃Channel(作为poller->poll函数的传出参数)

    std::atomic_bool callingPendingFunctors_; // 标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_; // 存储loop需要执行的所有回调操作
    std::mutex mutex_; // 保护pendingFunctors_线程安全
};