#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

/*
内存布局 : [prependable(前缀预留)][writable(可写入区域)]
                               ^
                               |
                    初始:  readerIndex_
                           writerIndex_

          [prependable(前缀预留)][readable(待处理数据)][writable(可写入区域)]
                               ^                    ^
                               |                    |
            写入了一些数据： readerIndex_         writerIndex_

          [prependable(前缀预留)][已读数据][readable(剩余待处理数据)][writable(可写入区域)]
                                        ^                       ^
                                        |                       |
            读取了一些数据:           readerIndex_            writerIndex_
*/

class Buffer{
public:
    static const size_t kCheapPrepend = 8; // 预留给协议头等前缀的空间
    static const size_t kInitialSize = 1024; // 初始缓冲区大小

    explicit Buffer(size_t initialSize = kInitialSize) 
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
    {}
    
    // 返回可读的数据长度
    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    // 返回剩余可写的空间
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }
    // 返回前置空闲空间
    size_t prependableBytes() const { return readerIndex_; }
    
    // 返回可读数据的起始地址(起始指针)
    const char *peek() const { return begin() + readerIndex_; }


    void retrieve(size_t len){
        if(len < readableBytes()){
            readerIndex_ += len; // 只读了可读区的部分(len)
        }
        else{ // len == readableBytes()
            retrieveAll();
        }
    }
    void retrieveAll(){
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }
    // 把onMessage函数上报的Buffer数据,转成string类型的数据返回
    std::string retrieveAllAsString() { 
        return retrieveAsString(readableBytes()); 
    }
    std::string retrieveAsString(size_t len){
        std::string result(peek(), len);
        retrieve(len); // 上面一句已经把缓冲区内可读的数据读了出来,这里肯定要对缓冲区进行复位
        return result;
    }

    void ensureWriteableBytes(size_t len){
        if(writableBytes() < len){
            makeSpace(len);
        }
    }

    // 把[data,data+len]内存上的数据,添加到writable缓冲区中
    void append(const char *data, size_t len){
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    char *beginWrite() { return begin() + writerIndex_; }
    const char *beginWrite() const { return begin() + writerIndex_; }

    // 从fd上读数据
    ssize_t readFd(int fd, int *saveErrno);
    // 通过fd发送数据
    ssize_t writeFd(int fd, int *saveErrno);

private:
    void makeSpace(size_t len){
        if(writableBytes() + prependableBytes() < len + kCheapPrepend){
            buffer_.resize(writerIndex_ + len);
        }
        else{
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                      begin() + writerIndex_,
                      begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    // 返回 buffer_首元素的地址
    char *begin() { return &*buffer_.begin(); }
    const char *begin() const { return &*buffer_.begin(); }

    std::vector<char> buffer_;
    size_t readerIndex_; // 可读数据起始位置
    size_t writerIndex_; // 可写区域起始位置
};
