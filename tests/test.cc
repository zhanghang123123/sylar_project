#include <iostream>
#include "../sylar/log.h"
#include "../sylar/util.h"

int main(int argc, char** argv) {
    sylar::Logger::ptr logger(new sylar::Logger);
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutAppender)); 

    // sylar::LogEvent::ptr event(new sylar::LogEvent(__FILE__, __LINE__, 0, 1, 2, time(0)));
    // // event->getSS() << "hello sylar log";
    // logger->log(sylar::LogLevel::DEBUG, event);
    // // std::cout << "hello sylar log" << std::endl;

    sylar::FileLogAppender::ptr file_appender(new sylar::FileLogAppender("./log.txt"));
    // sylar::LogFormatter::ptr fmt(new sylar::LogFormatter("%d%T%m%n"));

    // file_appender->setFormatter(fmt);
    file_appender->setLevel(sylar::LogLevel::FATAL);
    logger->addAppender(file_appender);

    MYLOG_DEBUG(logger) << "my log";

    auto l = sylar::LoggerMgr::getInstance()->getLogger("xx");
    MYLOG_INFO(l) << "XX";


    return 0;
}

