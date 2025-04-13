#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
        : loop_(nullptr)
        , exiting_(false)
        , thread_(std::bind(&EventLoopThread::threadFunc, this), name)
        , cond_()
        , mutex_()
        , callback_(cb)
{}

EventLoopThread::~EventLoopThread(){
    exiting_ = true;
    if(loop_!=nullptr){
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::startLoop(){
    thread_.start(); // 启动底层的新线程,执行的就是threadFunc(下面这个方法)
    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_ == nullptr){
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

void EventLoopThread::threadFunc(){
    // 创建一个独立的Eventloop,与上面的线程是一一对应的, one loop per thread
    EventLoop loop;
    if(callback_){ // ThreadInitCallback
        callback_(&loop);
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    loop.loop();

    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}
