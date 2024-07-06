#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__     // 这种比program once 更好

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <vector>
#include <sstream>
#include <fstream>
#include <map>

#include "util.h"
#include "singleton.h"
#include "thread.h"


/// ******************** 使用流式方式将日志级别level的日志写入到logger ********************
#define MYLOG(logger, level) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent( \
            logger, level, __FILE__, __LINE__, 0, sylar::getThreadId(), \
            sylar::getFiberId(), time(0), sylar::Thread::GetName()))).getSS()

#define MYLOG_DEBUG(logger) MYLOG(logger, sylar::LogLevel::DEBUG)
#define MYLOG_INFO(logger) MYLOG(logger, sylar::LogLevel::INFO)
#define MYLOG_WARN(logger) MYLOG(logger, sylar::LogLevel::WARN)
#define MYLOG_ERROR(logger) MYLOG(logger, sylar::LogLevel::ERROR)
#define MYLOG_FATAL(logger) MYLOG(logger, sylar::LogLevel::FATAL)

#define SYLAR_LOG_ROOT() sylar::LoggerMgr::getInstance()->getRoot()         /// 获取主日志器



namespace sylar {

/// ******************** 自定义日志级别 ********************
class LogLevel {
public:
    enum Level{
        UNKNOW = 0,         //  未知 级别
        DEBUG = 1,          //  DEBUG 级别 : 记录有关通过系统的流量的详细信息，应用程序记录的大多数为 DEBUG；
        INFO = 2,           //  INFO 级别 : 运行时事件（启动/关闭）；
        WARN = 3,           //  WARN 级别 : 使用已弃用的 API、API 使用不当、“几乎”错误、其他不合需要或意外但不一定“错误”的运行时情况；
        ERROR = 4,          //  ERROR 级别 : 其他运行时错误或意外情况
        FATAL = 5           //  FATAL 级别 : 导致提前终止的严重错误
    };

    // LogLevel类中还提供了两个静态函数，便于日志级别和字符串之间的相互转换；
    static const char* ToString(LogLevel::Level level);         // Level转字符串
    static LogLevel::Level FromString(const std::string& str);  // 字符串转Level
};

class Logger;               // <把Logger放到这里的目的?> 在定义Logger之前的一些类会用到Logger，不加会报未定义错误


/// ******************** 日志的一些配置 ********************
class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;      // [智能指针]

    LogEvent(std::shared_ptr<Logger> logger,    // 日志器
            LogLevel::Level level,              // 日志级别
            const char* file,                   // 文件名
            int32_t line,                       // 文件行号
            uint32_t elapse,                    // 程序启动依赖的耗时(毫秒)
            uint32_t thread_id,                 // 线程id
            uint32_t fiber_id,                  // 协程id
            uint64_t time,                      // 日志事件(秒)
            const std::string& thread_name);    // 线程名称             

    const char* getFile() const                 { return m_file; }
    int32_t getLine() const                     { return m_line; }
    uint32_t getEplase() const                  { return m_elapse; }
    uint32_t getThreadId() const                { return m_threadId; }
    uint32_t getFiberId() const                 { return m_fiberId; }
    uint64_t getTime() const                    { return m_time; }
    const std::string& getThreadName() const    { return m_threadName; }
    const std::string getContent() const        { return m_ss.str(); }
    std::stringstream& getSS()                  { return m_ss; }        // getStringStream();
    std::shared_ptr<Logger> getLogger() const   { return m_logger; }
    LogLevel::Level getLevel() const            { return m_level; }

private:
    const char* m_file = nullptr;               // 文件名   (c++11 支持类里面初始化const char *)
    int32_t m_line = 0;                         // 行号，引入头文件stdint.h (因为int在不同的系统中占得大小可能是16也可能是32，但是int_32肯定是32，避免一些不必要的问题)
    uint32_t m_elapse = 0;                      // 程序启动到现在的毫秒 （程序启动依赖的耗时(毫秒)）
    uint32_t m_threadId = 0;                    // 线程ID
    uint32_t m_fiberId = 0;                     // 协程ID
    uint64_t m_time = 0;                        // 时间戳
    std::string m_threadName;                   // 线程名
    std::stringstream m_ss;                     // 日志内容流
    std::shared_ptr<Logger> m_logger;           // 日志器
    LogLevel::Level m_level;                    // 日志等级
};

class LogEventWrap
{
public:
    LogEventWrap(LogEvent::ptr event);
    ~LogEventWrap();
    std::stringstream& getSS();         // getStringStream();
    LogEvent::ptr getEvent() const      { return m_event;   }
private:
    LogEvent::ptr m_event;
};


/// ******************** 日志格式化（日志定向输出器）********************
class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    LogFormatter(const std::string& pattern);
    /** @brief 返回格式化日志文本
     * @param[in] logger 日志器, @param[in] level 日志级别, @param[in] event 日志事件
     */
    std::string format(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event);
public:
    class FormatItem {   // [类中类]
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem() {}
        virtual void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) = 0;
    };
    void init(); 
private:
    std::string m_pattern;                                                              // 解析格式
    std::vector<FormatItem::ptr> m_items;                                               // 解析内容
};

/// ******************** 日志输出地 ********************
class LogAppender {
public:
    typedef std::shared_ptr<LogAppender> ptr;
virtual ~LogAppender() {}                                                               // 为了便于该类的派生类调用，定义为[虚类]，
    virtual void log(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) = 0;// [纯虚函数]，子类必须重写； 写入日志; 参数： 日志器，日志级别， 日志时间

    void setLevel(LogLevel::Level level)            { m_level = level; } 
    LogLevel::Level getLevel()                      { return m_level; }
    void setFormatter(LogFormatter::ptr val)        { m_formatter = val; }              // 更改日志格式器
    LogFormatter::ptr getFormatter() const          { return m_formatter; }             // 获取日志格式器
protected:
    LogLevel::Level m_level = LogLevel::DEBUG;                                          // 级别,为了便于子类访问该变量，设置在保护视图下
    LogFormatter::ptr m_formatter;                                                      // 定义输出格式
};

/// ******************** 日志器 ********************
class Logger : public std::enable_shared_from_this<Logger>{ // [?]
public:
    typedef std::shared_ptr<Logger> ptr;
    
    Logger(const std::string& name = "root");
    void log(LogLevel::Level level, LogEvent::ptr event);

    // 不同级别的日志输出函数
    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);
    void error(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);                                        // 添加一个appender
    void delAppender(LogAppender::ptr appender);                                        // 删除一个appender
    LogLevel::Level getLevel() const { return m_level; }                                // [const放在函数后]
    void setLevel(LogLevel::Level val) { m_level = val; }                               // 设置级别
    const std::string& getName() const { return m_name; }
private:
    std::string m_name;                                                                 // 日志名称
    LogLevel::Level m_level;                                                            // 级别
    std::list<LogAppender::ptr> m_appender;                                             // Appender集合,引入list
    LogFormatter::ptr m_formatter;
};

/// ******************** 日志输出地（输出方法分类：输出到控制台） ********************
class StdoutAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutAppender> ptr; 
    void log(Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override;
private:
};

/// ******************** 日志输出地（输出方法分类：输出到文件） ********************
class FileLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);                                       // 输出的文件名
    void log(Logger::ptr logger,LogLevel::Level level, LogEvent::ptr event) override;   // [override]
    bool reopen();                                                                      // 重新打开文件，成功返回true
private:
    std::string m_filename;
    std::ofstream m_filestream;                                                         // stringstream要报错，引入sstream
};

/// ******************** 日志管理器类 ********************
class LoggerManager 
{
public:
    LoggerManager();
    Logger::ptr getLogger(const std::string& name);
    Logger::ptr getRoot()       { return m_root; }                                      // 返回主日志器
    // std::string toYamlString();                                                         // 将所有的日志器配置转成YAML String

private:
    std::map<std::string, Logger::ptr> m_loggers;                                       // 日志容器
    Logger::ptr m_root;                                                                 // 主日志器
};

typedef sylar::Singleton<LoggerManager> LoggerMgr;                                      /// 日志器管理类单例模式

}

#endif
