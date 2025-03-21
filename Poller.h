#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

// 事件分发器的核心IO复用模块 --- 监听Channel中的sockfd
class Poller : noncopyable {
public:
    using ChannelList = std::vector<Channel *>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    // 给所有IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    // 判断当前Poller中是否有这个channedl
    bool hasChannel(Channel *channel) const;

    // EventLoop可以通过此接口获取默认的IO复用的具体实现 --- 本项目仅实现epoll
    static Poller *newDefaultPoller(EventLoop *loop);

protected:
    // map的key表示：sockfd   value：sockfd所属的Channel 
    // (1个sockfd和1个Channel一一对应，一个Poller监听多个Channel)
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels_;

private:
    EventLoop *ownerLoop_; // 定义Poller所属的事件循环EventLoop
};