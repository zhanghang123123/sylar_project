#include "log.h"
#include <iostream>
#include <map>
#include <functional>
#include <time.h>
#include <string.h>
#include <ctype.h>

namespace sylar {

const char* LogLevel::ToString(LogLevel::Level level) 
{
    switch(level) { // [宏函数的使用]
#define XX(name) \
        case LogLevel::name: \
            return #name; \
            break;
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    default:
        return "UNKONW";
    }
    return "UNKONW";
}

LogLevel::Level LogLevel::FromString(const std::string &str)
{
#define XX(level, v) \
    if(str == #v) { \
        return LogLevel::level; \
    }
    XX(DEBUG, debug);
    XX(INFO, info);
    XX(WARN, warn);
    XX(ERROR, error);
    XX(FATAL, fatal);

    XX(DEBUG, DEBUG);
    XX(INFO, INFO);
    XX(WARN, WARN);
    XX(ERROR, ERROR);
    XX(FATAL, FATAL);
    return LogLevel::UNKNOW;
#undef XX
}

class MessageFormatItem : public LogFormatter::FormatItem { // [继承类中类]
public:
    MessageFormatItem(const std::string& fmt = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override {
        os << event->getContent();  // 直接返回event中的 信息对应的字符串 到 os 中: std::ostream& os; os << "Hello, ";  os << "World!"; 最终 os 中的内容是 "Hello, World!"。
    }
};

class LevelFormatItem : public LogFormatter::FormatItem { 
public:
    LevelFormatItem(const std::string& fmt = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override {
        os << LogLevel::ToString(level);
    }
};

class LineFormatItem : public LogFormatter::FormatItem { 
public:
    LineFormatItem(const std::string& fmt = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << std::endl;    // 换行
    }
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
    FilenameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class StringFormatItem : public LogFormatter::FormatItem { 
public:
    StringFormatItem(const std::string& str) 
        :m_string(str) {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override {
        os << m_string;
    }
private:
    std::string m_string;
};

class EplaseFormatItem : public LogFormatter::FormatItem { 
public:
    EplaseFormatItem(const std::string& fmt = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override {
        os << event->getEplase();
    }
};

class LoggerNameFormatItem : public LogFormatter::FormatItem { 
public:
    LoggerNameFormatItem(const std::string& fmt = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override {
        os << logger->getName();    // logger 参数就是为了获取 logger日志起名称
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem { 
public:
    ThreadIdFormatItem(const std::string& fmt = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(const std::string& fmt = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override {
        os << event->getThreadName();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem { 
public:
    FiberIdFormatItem(const std::string& fmt = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem { 
public:
    DateTimeFormatItem(const std::string& format = "%Y:%m:%d %H:%M:%S") 
        : m_format(format)
    {
        if(m_format.empty())
        {
            m_format = "%Y:%m:%d %H:%M:%S";
        }
    }
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override 
    {
        // os << event->getTime();
        struct tm tm;
        time_t time = event->getTime();     // 当前时间戳 引入time.h
        localtime_r(&time, &tm);            // 将时间戳转换为本地时间，并将结果存放在tm中
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
    }
private:
    std::string m_format;
};

class TabFormatItem : public LogFormatter::FormatItem 
{ // 输出tab
public:
    TabFormatItem(const std::string& fmt = "") {}
    void format(std::ostream& os, std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) override {
        os << "\t";
    }
};

LogEvent::LogEvent(std::shared_ptr<Logger> logger,
                    LogLevel::Level level,
                    const char *file, 
                    int32_t line, 
                    uint32_t elapse, 
                    uint32_t thread_id, 
                    uint32_t fiber_id, 
                    uint64_t time, 
                    const std::string &thread_name)
    :m_file(file),
    m_line(line),
    m_elapse(elapse),
    m_threadId(thread_id),
    m_fiberId(fiber_id),
    m_time(time),
    m_threadName(thread_name),
    m_logger(logger),
    m_level(level)
{
}


Logger::Logger(const std::string& name) 
    :m_name(name),
     m_level(LogLevel::DEBUG)
{
    // const char[] formatter = "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n";  false
    // const char formatter[] = "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"; 
//    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));

    // mode1 ^-------v mode2

    // 定义日志格式
    std::vector<std::pair<LogFormatter::LogPattern, std::string>> patterns = {
//        {static_cast<LogFormatter::LogPattern>(100), "MyCustomFormat"}, // 使用动态注册的格式
        {LogFormatter::LogPattern::DateTimeFormat, ""},
        {LogFormatter::LogPattern::TabFormat, ""},
        {LogFormatter::LogPattern::LevelFormat, ""},                   // 使用默认格式
        {LogFormatter::LogPattern::TabFormat, ""},
        {LogFormatter::LogPattern::ThreadIdFormat, ""},
        {LogFormatter::LogPattern::TabFormat, ""},
        {LogFormatter::LogPattern::ThreadNameFormat, ""},
        {LogFormatter::LogPattern::TabFormat, ""},
        {LogFormatter::LogPattern::FiberIdFormat, ""},
        {LogFormatter::LogPattern::TabFormat, ""},
        {LogFormatter::LogPattern::MessageFormat, ""},
        {LogFormatter::LogPattern::NewLineFormat, ""}
    };

    // 初始化日志格式
    m_formatter.reset(new LogFormatter(patterns));
}

void Logger::addAppender(LogAppender::ptr appender) {
    if(!appender->getFormatter()) {
        appender->setFormatter(m_formatter); // 保证每一个日志都有默认格式
    }
    m_appender.push_back(appender);
}           
void Logger::delAppender(LogAppender::ptr appender) {
    for(auto it = m_appender.begin(); it != m_appender.end(); ++ it) {
        if(*it == appender) {
            m_appender.erase(it);
            break;
        }
    }
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event)  {
    if(level >= m_level) {
        auto self = shared_from_this(); // [?] 只有继承std::enable_shared_from_this<Logger>，才能在自己的成员函数中获得自己的智能指针，这样以后才能把它的智能指针传出去(智能指针哦)
        for(auto &i : m_appender) {
            i->log(self,level, event);
        } 
    }
}

void Logger::debug(LogEvent::ptr event) { log(LogLevel::DEBUG, event); }
void Logger::info(LogEvent::ptr event)  { log(LogLevel::INFO, event); }
void Logger::warn(LogEvent::ptr event)  { log(LogLevel::WARN, event); }
void Logger::fatal(LogEvent::ptr event) { log(LogLevel::ERROR, event); }
void Logger::error(LogEvent::ptr event) { log(LogLevel::FATAL, event); }


FileLogAppender::FileLogAppender(const std::string& filename) 
    : m_filename(filename) 
{
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
{
    if(level >= m_level) {
        m_filestream << m_formatter->format(logger,level,event);
    }
}

bool FileLogAppender::reopen() {
    if(m_filestream) {
        m_filestream.close();
    }

    m_filestream.open(m_filename);
    return !!m_filestream; // [?]   双感叹号!!作用就是非0值转成1，而0值还是0. example: 非0值是true,-400就是true, !(-400)=false, !!(-400) = true
}

void StdoutAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
{
    if(level >= m_level) {
        std::cout << m_formatter->format(logger,level,event); // << std::endl; // namespace "std" has no member "cout" 加入iostream
    }
}

// 枚举值到格式化项的映射
std::map<LogFormatter::LogPattern, std::function<LogFormatter::FormatItem::ptr(const std::string&)> > LogFormatter::s_c_format_items = {
    {LogPattern::MessageFormat,    [](const std::string& fmt) { return std::make_shared<MessageFormatItem>(fmt); }},
    {LogPattern::LevelFormat,      [](const std::string& fmt) { return std::make_shared<LevelFormatItem>(fmt); }},
    {LogPattern::EplaseFormat,     [](const std::string& fmt) { return std::make_shared<EplaseFormatItem>(fmt); }},
    {LogPattern::LoggerNameFormat, [](const std::string& fmt) { return std::make_shared<LoggerNameFormatItem>(fmt); }},
    {LogPattern::ThreadIdFormat,   [](const std::string& fmt) { return std::make_shared<ThreadIdFormatItem>(fmt); }},
    {LogPattern::NewLineFormat,    [](const std::string& fmt) { return std::make_shared<NewLineFormatItem>(fmt); }},
    {LogPattern::DateTimeFormat,   [](const std::string& fmt) { return std::make_shared<DateTimeFormatItem>(fmt); }},
    {LogPattern::FilenameFormat,   [](const std::string& fmt) { return std::make_shared<FilenameFormatItem>(fmt); }},
    {LogPattern::LineNumFormat,    [](const std::string& fmt) { return std::make_shared<LineFormatItem>(fmt); }},
    {LogPattern::FiberIdFormat,    [](const std::string& fmt) { return std::make_shared<FiberIdFormatItem>(fmt); }},
    {LogPattern::TabFormat,        [](const std::string& fmt) { return std::make_shared<TabFormatItem>(fmt); }},
    {LogPattern::ThreadNameFormat, [](const std::string& fmt) { return std::make_shared<ThreadNameFormatItem>(fmt); }},
};

LogFormatter::LogFormatter(const std::string& pattern) 
    : m_pattern(pattern)
{
    init();
}

LogFormatter::LogFormatter(const std::vector<std::pair<LogFormatter::LogPattern, std::string>>& patterns)
    : m_logpatterns(patterns)
{
    init2();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event) {
    std::stringstream ss;
    for(auto &i : m_items) {
        i->format(ss,logger,level,event);
      
    }
    return ss.str();
}

// 日志格式定义
void LogFormatter::init() 
{
    // "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"  解析该字符串 (这里实现初始化日志格式，可由用户指定字符串 lai 指定日志格式)
    std::vector<std::tuple<std::string, std::string, int> > vec;  // [tuple]  str, format, type
    std::string nstr; //当前str
    for(size_t i = 0; i < m_pattern.size(); ++i)    // size_t(无符号) 适配不同的机器的一种类型，常在STL标准库中使用，无符号整型，可以任意表示string和数组的大小
    {
        /// 状态机的三种状态（以%来进行状态转换）： 1  != '%'
        if(m_pattern[i] != '%') 
        {   
            nstr.append(1, m_pattern[i]);
            // std::cout << " for nstr : " << nstr << std::endl;  // 输出 是 [, ] ： 和 空 等
            continue;
        }
        /// 状态机的三种状态（以%来进行状态转换）： 2  == "%%" （这个模式忽略）
        if((i + 1) < m_pattern.size() &&  m_pattern[i + 1] == '%')
        {   /// == "%%" 即：如果连续两个%，代表不是一个模式，而是一个字符(不过这种情况基本不存在)
            {
                nstr.append(1, '%');    /// ==  nstr.append(m_pattern[i]);
                continue;
            }
        }

        /// 状态机的三种状态（以%来进行状态转换）： 3  == "%~" 解析%后下一个字符不是%的字符串。 从当前%后的第一个字符开始遍历后面的字符. 截取 {} 中的内容
        size_t n = i + 1;
        int fmt_status = 0;     // fmt_status 表示包含{}的状态， =0，表示{}完整或者都没有，状态正确。而=1 就是这个括号{}不完整，残缺
                                // 换个变量名字  bool braces_out = true; （true表示在 {} 内部）
        size_t fmt_begin = 0;   // 记录 '{' 开始的位置 size_t braces_begin = 0;
        std::string str;
        std::string fmt;        // 这个字符串 专门记录 {} 中的内容， std::string braces_content; 

        while(n < m_pattern.size())  
        {   // 解析一个标准的 formatter 格式，你可以把formatter 看成这样：%formatter1%formatter2%formatter3%formatter4..., 
            // 此while 循环就解析这其中的某一个 formatter， 比如 %T 或者 %d{%Y-%m-%d %H:%M:%S}
            // std::cout << " in while n = " << n << "; m_pattern[n] = " <<  m_pattern[n] << std::endl;
            // std::cout << " in  = " << !isalpha(m_pattern[n]) << std::endl;
            if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}'))  // braces_out
            {  
                // 判断当前字符是否不是字母 '{','}']中的一个（实际中该字符只对普通字母和{} 有意义， 其他 如数字字符'2', 等字符';'是没有意义的）
                // 实际上这里主要目的是判断空格，对于这样的字符（"%d [%p] <%f:%l> %m %n"），判断空格来分割为不同的子pattern
                str = m_pattern.substr(i + 1, n - i - 1);
                // std::cout << " while str : " << str << std::endl;   // 输出是基本的formatter ，比如 T, t, N 等
                break;                                                                  
            }
            // 一般的格式，比如 %T %p 等，上面那一个if() 语句就可以了
            // 下面这个if else 语句专门解析出 时间格式 即 {} 中内容；顺带校验输入正确与否
            if(fmt_status == 0)                                 // if(braces_out) 第一次进入 '{'前的状态
            {
                if(m_pattern[n] == '{')                         // 第一次进入 '{'
                {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    fmt_status = 1;                             // braces_out = false; 进入 '{'， 状态切换
                    fmt_begin = n;                              // 记录第一次进入 '{' 的位置
                    ++n;
                    continue;
                }
            }
            else if(fmt_status == 1)                                         // if(!braces_out) // 正在 {} 之中循环（的状态）                    
            {                                                                // 注意， 这里必须用 else if; 因为 在这个while循环中，前一刻fmt_status状态变了,不能变两次
                if(m_pattern[n] == '}')                                      // 这个字符表示 解析 {} 的 '}' 字符到了，该退出了，退出前，把信息更新
                {
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);// 退出函数中把 {} 中的信息记录下来 braces_content = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    fmt_status = 0;                                          // braces_out = false 标志更新（退出了 {} ）
                    // std::cout << " while fmt : " << fmt << std::endl;        // 输出结果 %Y-%m-%d %H:%M:%S
                    ++n;
                    break;                                                   // 存在的 {} 解析完了， 退出 while 循环
                }
            }

            ++n;
            if(n == m_pattern.size()) 
            {
                if(str.empty())     
                {
                    str = m_pattern.substr(i + 1);
                }
            }
        }

        if(fmt_status == 0)      // if(braces_out)  避免只有 '{' 情况， 只有 '{' 就报错
        {
            if(!nstr.empty()) 
            {
                vec.push_back(std::make_tuple(nstr, std::string(), 0));     // 普通格式 ： (T) - () - (1)
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));                    // 时间格式 ： (d) - (%Y-%m-%d %H:%M:%S) - (1)
            i = n - 1;
        } 
        else if(fmt_status == 1)
        {
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            vec.push_back(std::make_tuple("<parse_error>", fmt, 0));
        }  
    }

    if(!nstr.empty()) 
    {
        vec.push_back(std::make_tuple(nstr, std::string(), 0));
    }
    // [function] 引入function
    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)>> s_format_items = {
        // {"m",[](const std::string& fmt) { return FormatItem::ptr(new MessageFormatItem(fmt)); } }
#define XX(str,C) \
        {#str, [] (const std::string& fmt) { return FormatItem::ptr(new C(fmt)); }}

        XX(m, MessageFormatItem),           //m:消息
        XX(p, LevelFormatItem),             //p:日志级别
        XX(r, EplaseFormatItem),            //r:累计毫秒数
        XX(c, LoggerNameFormatItem),        //c:日志名称
        XX(t, ThreadIdFormatItem),          //t:线程id
        XX(n, NewLineFormatItem),           //n:换行
        XX(d, DateTimeFormatItem),          //d:时间
        XX(f, FilenameFormatItem),          //f:文件名
        XX(l, LineFormatItem),              //l:行号
        XX(F, FiberIdFormatItem),           //F:协程id
        XX(T, TabFormatItem),               //T:TAB
        XX(N, ThreadNameFormatItem),        //N:线程名称
#undef XX
    };
    /* 直接把所有的类型都实例化到静态map里(知识点18: static静局部态变量初始化与函数执行关系 https://chat.deepseek.com/a/chat/s/457fb921-90f9-4a27-8b39-2636ce2e2315)
        在C++中，static局部变量的初始化只会在函数第一次被调用时执行一次。之后的每次函数调用都会跳过这个初始化过程，直接使用已经初始化好的静态变量。
        具体到你的代码：
            int A() {
                xxx;
                static std::map<...> s = {...};
                yyy;
            }
        第一次调用函数 A 时：
            1. 执行 xxx;
            2. 初始化静态变量 s，即 static std::map<...> s = {...}; 会被执行。
            3. 执行 yyy;。
        后续调用函数 A 时：
            1. 执行 xxx;。
            2. 跳过 static std::map<...> s = {...}; 的初始化，直接使用已经初始化好的 s。
            3. 执行 yyy;。
        因此，静态变量 s 的初始化只会在第一次调用函数 A 时发生，后续调用时不会再重新初始化，而是直接使用已经存在的 s。
        这样可以避免重复初始化，提高效率。
        需要注意的是，静态变量的生命周期是整个程序运行期间，所以它的值会在多次函数调用之间保持持久性。
     */

    for(auto& i : vec)
    {
        if(std::get<2>(i) == 0)
        {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        }
        else
        {
            auto it = s_format_items.find(std::get<0>(i));
            if(it == s_format_items.end())
            {
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
            } else
            {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
        std::cout << '(' << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ')' << std::endl;
    }
}

void LogFormatter::init2()
{
    m_items.clear();
//(放到外面)    // 枚举值到格式化项的映射
//    static const std::map<LogPattern, std::function<FormatItem::ptr(const std::string&)> > s_c_format_items = {
//        {LogPattern::MessageFormat,    [](const std::string& fmt) { return std::make_shared<MessageFormatItem>(fmt); }},
//        {LogPattern::LevelFormat,      [](const std::string& fmt) { return std::make_shared<LevelFormatItem>(fmt); }},
//        {LogPattern::EplaseFormat,     [](const std::string& fmt) { return std::make_shared<EplaseFormatItem>(fmt); }},
//        {LogPattern::LoggerNameFormat, [](const std::string& fmt) { return std::make_shared<LoggerNameFormatItem>(fmt); }},
//        {LogPattern::ThreadIdFormat,   [](const std::string& fmt) { return std::make_shared<ThreadIdFormatItem>(fmt); }},
//        {LogPattern::NewLineFormat,    [](const std::string& fmt) { return std::make_shared<NewLineFormatItem>(fmt); }},
//        {LogPattern::DateTimeFormat,   [](const std::string& fmt) { return std::make_shared<DateTimeFormatItem>(fmt); }},
//        {LogPattern::FilenameFormat,   [](const std::string& fmt) { return std::make_shared<FilenameFormatItem>(fmt); }},
//        {LogPattern::LineNumFormat,    [](const std::string& fmt) { return std::make_shared<LineFormatItem>(fmt); }},
//        {LogPattern::FiberIdFormat,    [](const std::string& fmt) { return std::make_shared<FiberIdFormatItem>(fmt); }},
//        {LogPattern::TabFormat,        [](const std::string& fmt) { return std::make_shared<TabFormatItem>(fmt); }},
//        {LogPattern::ThreadNameFormat, [](const std::string& fmt) { return std::make_shared<ThreadNameFormatItem>(fmt); }},
//    };

    // 遍历枚举值，生成格式化项
    for (const auto& pattern_pair : m_logpatterns)
    {
        LogPattern pattern = pattern_pair.first;       // 获取枚举值
        const std::string& fmt = pattern_pair.second;  // 获取格式字符串

        auto it = s_c_format_items.find(pattern);
        if (it != s_c_format_items.end())
            m_items.push_back(it->second(fmt)); // 使用用户指定的格式
        else
            m_items.push_back(std::make_shared<StringFormatItem>("<error_format>"));    // 处理未知枚举值
    }

//        // 遍历枚举值，生成格式化项
//        for (const auto& [pattern, fmt] : patterns)
//        {
//            auto it = s_c_format_items.find(pattern);
//            if (it != s_c_format_items.end()) {
//                m_items.push_back(it->second(fmt)); // 使用用户指定的格式
//            } else {
//                // 处理未知枚举值
//                m_items.push_back(std::make_shared<StringFormatItem>("<error_format>"));
//            }
//        }

//        // 遍历枚举值，生成格式化项
//        for (const auto& pattern : m_logpatterns)
//        {
//            auto it = s_c_format_items.find(pattern);
//            if (it != s_c_format_items.end())
//                m_items.push_back(it->second("")); // 使用默认格式
//            else
//                // 未知枚举值，生成错误提示
//                m_items.push_back(std::make_shared<StringFormatItem>("<error_format>"));
//        }

        /* bad design
        for(int i = 0; i < m_logpatterns.cout(); ++i)
        {
            LogPattern pattern = m_logpatterns.at(i);
            switch (pattern) {
            case MessageFormat:
                m_items.push_back(new MessageFormatItem(""));
                break;
            case LevelFormat:
                m_items.push_back(new LevelFormatItem(""));
                break;
            case EplaseFormat:
                m_items.push_back(new EplaseFormatItem(""));
                break;
            case LoggerNameFormat:
                m_items.push_back(new LoggerNameFormatItem(""));
                break;
            case ThreadIdFormat:
                m_items.push_back(new ThreadIdFormatItem(""));
                break;
            case NewLineFormat:
                m_items.push_back(new NewLineFormatItem(""));
                break;
            case DateTimeFormat:
                m_items.push_back(new DateTimeFormatItem(""));
                break;
            case FilenameFormat:
                m_items.push_back(new FilenameFormatItem(""));
                break;
            case LineNumFormat:
                m_items.push_back(new LineFormatItem(""));
                break;
            case FiberIdFormat:
                m_items.push_back(new FiberIdFormatItem(""));
                break;
            case TabFormat:
                m_items.push_back(new TabFormatItem(""));
                break;
            case ThreadNameFormat:
                m_items.push_back(new ThreadNameFormatItem(""));
                break;
            default:
                break;
            }
        }
        */
}

void LogFormatter::registerFormat(LogFormatter::LogPattern pattern, std::function<LogFormatter::FormatItem::ptr (const std::string &)> creator)
{
    s_c_format_items[pattern] = creator;
}

LogEventWrap::LogEventWrap(LogEvent::ptr event)
    :m_event(event)
{
}

LogEventWrap::~LogEventWrap()
{
    m_event->getLogger()->log(m_event->getLevel(), m_event);    // 把自己写入日志 ?  深入分析一下，怎么输出到控制台
}
std::stringstream &LogEventWrap::getSS()
{
    return m_event->getSS();
}

LoggerManager::LoggerManager()
{
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutAppender));

    m_loggers[m_root->getName()] = m_root;
}

Logger::ptr LoggerManager::getLogger(const std::string &name)
{
    auto it = m_loggers.find(name);
    return it == m_loggers.end() ? m_root : it->second;
    // return Logger::ptr();
}

}
