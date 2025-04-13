#pragma once

#include <unistd.h>
#include <sys/syscall.h>

namespace CurrentThread{
    // thread_local:声明线程局部存储的变量
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
