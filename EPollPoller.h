#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class Channel;

class EPollPoller : public Poller {
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    static const int kInitEventListSize = 16;
    // 将发生事件的channel 全部加入到activeChannles中
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

    void update(int operation, Channel *channel);

    // struct epoll_event, 为了方便扩容用vector
    using EventList = std::vector<epoll_event>;
    int epollfd_;
    EventList events_;
};
