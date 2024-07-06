#include "util.h"
#include "log.h"
#include <iostream>
#include <execinfo.h>
#include "fiber.h"

namespace sylar {

pid_t getThreadId()
{
    return syscall(SYS_gettid);
}

uint32_t getFiberId()
{
//    return 0; // TODO
//    return sylar::getFiberId();
    return sylar::Fiber::GetFiberId();
}

void Backtrace(std::vector<std::string>& bt, int size, int skip) {
    void** array = (void**)malloc((sizeof(void*) * size));
    size_t s = ::backtrace(array, size);

    char** strings = backtrace_symbols(array, s);
    if(strings == NULL) {
        MYLOG_ERROR(SYLAR_LOG_ROOT()) << "backtrace_synbols error";
        return;
    }

    for(size_t i = skip; i < s; ++i) {
        bt.push_back(strings[i]);
    }

    free(strings);
    free(array);
}

std::string BacktraceToString(int size, int skip, const std::string& prefix) {
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::stringstream ss;
    ss << std::endl;
    for(size_t i = 0; i < bt.size(); ++i) {
        ss << prefix << bt[i] << std::endl;
    }
    return ss.str();
}


}
