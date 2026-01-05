#include "sylar/sylar.h"
#include <assert.h>

void test_assert()
{
//    assert(0);
//    MYLOG_INFO(SYLAR_LOG_ROOT()) << sylar::BacktraceToString(10, 0, "    ");
    SYLAR_ASSERT(false);
}



int main(int argc, char *argv[])
{
    sylar::InstallCrashHandler(); // 安装全局崩溃捕获
    test_assert();
    return 0;
}

