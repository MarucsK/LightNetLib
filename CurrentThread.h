#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread{
    // thread_local:声明线程局部存储的变量
    // 每个线程在访问 thread_local 变量时，都会得到该变量的一个独立副本。
    // 修改一个线程中的变量不会影响其他线程中的副本。
    extern thread_local int t_cachedTid;

    void cacheTid();

    // 调用此方法,获取当前线程id
    inline int tid(){
        // 即 if(t_cachedTid == 0)
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}