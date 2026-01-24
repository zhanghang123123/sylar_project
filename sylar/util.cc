#include "util.h"
#include "log.h"
#include <iostream>
#include <execinfo.h>
#include "fiber.h"

namespace sylar {

pid_t GetThreadId()
{
    return syscall(SYS_gettid); // linux 获取thread id的方式，收集下. 搞懂了pthread_self和syscall是不一样的 syscall返回的是linux下的线程ID  pthread_self获取的是pthread库的线程id 不同进程间这个线程id可能会相同?
}

uint32_t GetFiberId()
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
        free(strings);  // don't lose
        free(array);    // don't lose
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

// 信号处理函数
void CrashHandler(int signal) {
    // 获取并打印信号描述
    const char* signal_name = "Unknown";
    switch(signal)
    {
        case SIGSEGV: signal_name = "SIGSEGV (Segmentation Fault)"; break;
        case SIGILL:  signal_name = "SIGILL (Illegal Instruction)"; break;
        case SIGFPE:  signal_name = "SIGFPE (Floating-point Exception)"; break;
        case SIGABRT: signal_name = "SIGABRT (Abort)"; break;
    }

    std::cerr << "\n!!! Program received fatal signal: " << signal_name << " (" << signal << ") !!!" << std::endl;
    std::cerr << "=== Stack trace ===" << std::endl;

    // 调用你已有的函数打印堆栈
    std::string stack_trace = BacktraceToString(100, 0, "    ");    // skip=2 跳过本函数和信号处理框架
    std::cerr << stack_trace << std::endl;

    // 立即刷新输出缓冲区，确保信息被打印出来
    std::cerr.flush();

    // 可选：执行默认的信号处理动作（例如产生core dump）
    // 如果希望生成core文件进行更深入的分析，可以注释掉exit()，并移除信号注册时的SA_RESETHAND
    std::_Exit(signal); // 使用 _Exit 避免再次调用atexit等处理程序
}

// 初始化函数，应在main函数开始处调用
void InstallCrashHandler() {
    struct sigaction sa;
    sa.sa_handler = CrashHandler;               // 设置信号处理函数
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_RESETHAND;    // SA_RESETHAND 表示在处理一次后恢复为默认动作

    // 注册需要捕获的信号
    sigaction(SIGSEGV, &sa, nullptr);           // 非法内存访问
    sigaction(SIGILL,  &sa, nullptr);           // 非法指令
    sigaction(SIGFPE,  &sa, nullptr);           // 算术运算异常
    sigaction(SIGABRT, &sa, nullptr);           // 中止信号（如assert失败）
    // 可以根据需要添加更多信号，如SIGBUS
}


}
