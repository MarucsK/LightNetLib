#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class Channel;
/*
epoll的使用：
    epoll_create:构造函数，创建的epoll实例文件描述符存储在成员变量epollfd_中
    epoll_ctl：updateChannel、removeChannel
    epoll_wait: Timestamp poll
*/
class EPollPoller : public Poller {
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    static const int kInitEventListSize = 16;
    // 将发生事件的channel 全部加入到activeChannles中
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    // 更新channel
    void update(int operation, Channel *channel);

    // struct epoll_event数组,为了方便扩容用vector
    // 作为epoll操作函数参数使用
    using EventList = std::vector<epoll_event>;
    int epollfd_;
    EventList events_;
};