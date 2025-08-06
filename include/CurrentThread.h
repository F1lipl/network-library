#pragma once

#include<unistd.h>
#include<sys/syscall.h>

namespace CurrentThread{
    extern __thread int t_cachedTid;//保存tid缓存，因为系统调用非常费时所以拿到tid后保存
    //使用__thread关键字声明为线程局部存储（每个线程独立副本）
    //初始值为0，首次获取时通过系统调用初始化

    void cacheTid();
    inline int tid(){
        if(__builtin_expect(t_cachedTid=0,0)){
            cacheTid();
        }//__builtin_expect是一种底层优化，意思是如果没有获得tid，通过系统调用cacheTid（）获取tid
        return t_cachedTid;
    }
    
}