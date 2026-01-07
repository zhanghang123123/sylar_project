#include "sylar/sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run_in_fiber() {
    MYLOG_INFO(g_logger) << "run_in_fiber begin";
    sylar::Fiber::YieldToHold();
    MYLOG_INFO(g_logger) << "run_in_fiber end";
    sylar::Fiber::YieldToHold();      // 任务协程运行完了，就当前fiber 析构了(调用析构函数)
}

void test_fiber() {
    MYLOG_INFO(g_logger) << "main begin -1";
    {
        sylar::Fiber::GetThis();
        MYLOG_INFO(g_logger) << "main begin";
        sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber));
        fiber->swapIn();
        MYLOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        MYLOG_INFO(g_logger) << "main after end";
        fiber->swapIn();
    }
    MYLOG_INFO(g_logger) << "main after end2";
}

void test_one_fiber() {
    MYLOG_INFO(g_logger) << "main begin -1";
    {
        sylar::Fiber::GetThis();    // 对于使用fiber 的线程，就在使用它前调用GetThis() 来把线程包装成协程。这样别的线程可以保留自己线程的运行方式
        MYLOG_INFO(g_logger) << "main begin";
        sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber));
        fiber->swapIn();            // 从主协程 切换到 当前任务fiber
        MYLOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        MYLOG_INFO(g_logger) << "main after end";
        fiber->swapIn();
    }
    MYLOG_INFO(g_logger) << "main after end2";
}

void test_many_threads_fiber()
{
    std::vector<sylar::Thread::ptr> threads;
    for(int i = 0; i < 3; ++i)
    {
        threads.push_back(sylar::Thread::ptr(new sylar::Thread(&test_fiber, "name_" + std::to_string(i)))); // 这里执行的是线程函数。具体协程是线程函数中再去new 的
    }
    for(auto i : threads)
    {
        i->join();
    }
}


int main(int argc, char** argv)
{
    sylar::Thread::SetName("main"); // 修改主线程的 名称 （之前为UNKNOWN）

    test_many_threads_fiber();

//    test_one_fiber();

    return 0;
}
