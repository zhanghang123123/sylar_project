#include "sylar/sylar.h"


sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int main(int argc, char** argv)
{
    sylar::Scheduler scheduler;

    scheduler.start();
    scheduler.stop();
    return 0;
}
