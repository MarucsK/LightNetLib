#include "EPollPoller.h"
#include "logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <string.h>

// 当前channel未添加到poller中
const int kNew = -1; // channel::index_ = -1
// 当前channel已添加到poller中
const int kAdded = 1;
// 当前channel已从poller中删除
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize) // vector<epoll_event>
{
    if(epollfd_ < 0){
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EPollPoller::~EPollPoller(){
    ::close(epollfd_);
}

// channel:update remove => EventLoop:updateChannel removeChannel => Poller:updateChannel removeChannel
/*
               EventLoop
    若干Channel            Poller
                        ChannelMap<fd,channel*> epollfd

    这些Channel要注册到Poller中，来让Poller监听(也有当前处于未注册状态的,但此Channel仍在EventLoop中)
*/
void EPollPoller::updateChannel(Channel *channel) {
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);

    if(index == kNew || index == kDeleted){
        if(index==kNew){
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else { // channel已经在Poller上注册过
        int fd = channel->fd();
        if(channel->isNoneEvent()){
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else{
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 从Poller中删除当前channel
void EPollPoller::removeChannel(Channel *channel) {
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);

    int index = channel->index();
    if(index == kAdded){
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

// 实现epoll_ctl: add del mod 的具体操作
void EPollPoller::update(int operation, Channel *channel) {
    epoll_event event;
    bzero(&event, sizeof(event));
    int fd = channel->fd();
    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;
    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0){
        if(operation == EPOLL_CTL_DEL){
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else{
            LOG_ERROR("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}

// epoll_wait
// activeChannels是传出参数,存储所有发生事件的channel集合
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // 用LOG_DEBUG更合理
    LOG_INFO("func=%s => fd total count:%lu \n", __FUNCTION__, channels_.size());

    // events_.begin()返回首元素迭代器,先解引用得首元素，再取地址。即得到首元素的地址
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        LOG_INFO("%d events happended \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else
    {
        if (saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
    return now;
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        // events_作为epoll_wait的传出参数,里面存储了发生事件的struct epoll_event集合
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); // EventLoop拿到了它的poller返回的所有发生事件的channel列表
    }
}
