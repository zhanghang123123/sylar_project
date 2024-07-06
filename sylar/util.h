#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include<pthread.h>
#include<sys/types.h>
#include<sys/syscall.h>
#include<unistd.h>
#include<stdint.h>

#include <vector>
#include <string>

namespace sylar{

// 获取线程ID
pid_t getThreadId();

// 或者协程ID
uint32_t getFiberId();

void Backtrace(std::vector<std::string>& bt, int size, int skip);

std::string BacktraceToString(int size, int skip, const std::string& prefix);
}

#endif
