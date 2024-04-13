#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include<pthread.h>
#include<sys/types.h>
#include<sys/syscall.h>
#include<unistd.h>
#include<stdint.h>

namespace sylar{

// 获取线程ID
pid_t getThreadId();

// 或者协程ID
uint32_t getFiberId();
}

#endif