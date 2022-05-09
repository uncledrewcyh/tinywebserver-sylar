#pragma once 
#include <string>
#include <stdint.h>
#include <memory>
#include <sstream>
#include <fstream>
#include <map>
#include <list>
#include <stdarg.h>
#include <vector>

#include "singleton.h"
#include "mutex.h"
#include "thread.h"
#include "util.h"
/**
 * @brief 使用流式方式将日志级别level的日志写入到logger
 */
#define PUDGE_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        pudge::LogEventWrap(pudge::LogEvent::ptr(new pudge::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, pudge::GetThreadId(),\
        pudge::GetFiberId(), time(0), pudge::Thread::GetName()))).getSS()

/**
 * @brief 使用流式方式将日志级别debug的日志写入到logger
 */
#define PUDGE_LOG_DEBUG(logger) PUDGE_LOG_LEVEL(logger, pudge::LogLevel::DEBUG)

/**
 * @brief 使用流式方式将日志级别info的日志写入到logger
 */
#define PUDGE_LOG_INFO(logger) PUDGE_LOG_LEVEL(logger, pudge::LogLevel::INFO)

/**
 * @brief 使用流式方式将日志级别warn的日志写入到logger
 */
#define PUDGE_LOG_WARN(logger) PUDGE_LOG_LEVEL(logger, pudge::LogLevel::WARN)

/**
 * @brief 使用流式方式将日志级别error的日志写入到logger
 */
#define PUDGE_LOG_ERROR(logger) PUDGE_LOG_LEVEL(logger, pudge::LogLevel::ERROR)

/**
 * @brief 使用流式方式将日志级别fatal的日志写入到logger
 */
#define PUDGE_LOG_FATAL(logger) PUDGE_LOG_LEVEL(logger, pudge::LogLevel::FATAL)

/**
 * @brief 使用格式化方式将日志级别level的日志写入到logger
 */
#define PUDGE_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        pudge::LogEventWrap(pudge::LogEvent::ptr(new pudge::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, pudge::GetThreadId(),\
                pudge::GetFiberId(), time(0), pudge::Thread::GetName()))).getEvent()->format(fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别debug的日志写入到logger
 */
#define PUDGE_LOG_FMT_DEBUG(logger, fmt, ...) PUDGE_LOG_FMT_LEVEL(logger, pudge::LogLevel::DEBUG, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别info的日志写入到logger
 */
#define PUDGE_LOG_FMT_INFO(logger, fmt, ...)  PUDGE_LOG_FMT_LEVEL(logger, pudge::LogLevel::INFO, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别warn的日志写入到logger
 */
#define PUDGE_LOG_FMT_WARN(logger, fmt, ...)  PUDGE_LOG_FMT_LEVEL(logger, pudge::LogLevel::WARN, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别error的日志写入到logger
 */
#define PUDGE_LOG_FMT_ERROR(logger, fmt, ...) PUDGE_LOG_FMT_LEVEL(logger, pudge::LogLevel::ERROR, fmt, __VA_ARGS__)

/**
 * @brief 使用格式化方式将日志级别fatal的日志写入到logger
 */
#define PUDGE_LOG_FMT_FATAL(logger, fmt, ...) PUDGE_LOG_FMT_LEVEL(logger, pudge::LogLevel::FATAL, fmt, __VA_ARGS__)

/**
 * @brief 获取主日志器
 */
#define PUDGE_LOG_ROOT() pudge::LoggerMgr::GetInstance()->getRoot()

/**
 * @brief 获取name的日志器
 */
#define PUDGE_LOG_NAME(name) pudge::LoggerMgr::GetInstance()->getLogger(name)


namespace pudge {

class Logger;
class LoggerManager;

class LogLevel {
public:
    /**
     * @brief 日志级别枚举
     */
    enum Level {
        /// 未知级别
        UNKNOW = 0,
        /// DEBUG 级别
        DEBUG = 1,
        /// INFO 级别
        INFO = 2,
        /// WARN 级别
        WARN = 3,
        /// ERROR 级别
        ERROR = 4,
        /// FATAL 级别
        FATAL = 5
    };
    /**
     * @brief 将日志级别转成文本输出
     * @param[in] level 日志级别
     */
    static const char* ToString(LogLevel::Level level);
    /**
     * @brief 将文本转换成日志级别
     * @param[in] str 日志级别文本
     */
    static LogLevel::Level FromString(const std::string& str);

};
/**
*
 * @brief: 日志事件：包含了具体的日志信息
 * @param[in] {*}
 */
class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
            ,const char* file, int32_t line, uint32_t elapse
            ,uint32_t thread_id, uint32_t fiber_id, uint64_t time
            ,const std::string& thread_name);   

    const char* getFile() const { return m_file;}

    int32_t getLine() const { return m_line;}

    uint32_t getElapse() const { return m_elapse;}

    uint32_t getThreadId() const { return m_threadId;}

    uint32_t getFiberId() const { return m_fiberId;}

    uint64_t getTime() const { return m_time;}

    const std::string& getThreadName() const { return m_threadName;}
    
    std::string getContent() const { return m_ss.str();}

    std::shared_ptr<Logger> getLogger() const { return m_logger;}

    LogLevel::Level getLevel() const { return m_level;}

    std::stringstream& getSS() { return m_ss;}

    void format(const char* fmt, ...);
    
    void format(const char* fmt, va_list al);
    
private:
    const char* m_file = nullptr;
    int32_t m_line = 0;
    uint32_t m_elapse = 0;
    int32_t m_threadId = 0;
    uint32_t m_fiberId = 0;
    uint64_t m_time;
    std::string m_threadName;
    std::stringstream m_ss;
    std::shared_ptr<Logger> m_logger;
    LogLevel::Level m_level;
};

class LogEventWrap {
public:
    LogEventWrap(LogEvent::ptr e);

    ~LogEventWrap();
    /**
    *
     * @brief:  用于格式化格式输出 
     * @param[in] {*}
     */
    LogEvent::ptr getEvent() const { return m_event;} 
    /**
    *
     * @brief: 用于流式格式输出
     * @param[in] {*}
     */    
    std::stringstream& getSS();
private:
    LogEvent::ptr m_event;
};


class LogFormatter {
public:
    typedef std::shared_ptr<LogFormatter> ptr;
    /**
     * @brief 构造函数
     * @param[in] pattern 格式模板
     * @details 
     *  %m 消息
     *  %p 日志级别
     *  %r 累计毫秒数
     *  %c 日志名称
     *  %t 线程id
     *  %n 换行
     *  %d 时间
     *  %f 文件名
     *  %l 行号
     *  %T 制表符
     *  %F 协程id
     *  %N 线程名称
     *
     *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
     */
    LogFormatter(const std::string& pattern);

    std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
    std::ostream& format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
    
    /**
     * @brief 日志内容项格式化
     */
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        /**
         * @brief 析构函数
         */
        virtual ~FormatItem() {}
        /**
         * @brief 格式化日志到流
         * @param[in, out] os 日志输出流
         * @param[in] logger 日志器
         * @param[in] level 日志等级
         * @param[in] event 日志事件
         */
        virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    };

    /**
     * @brief 初始化,解析日志模板
     */
    void init();

    /**
     * @brief 是否有错误
     */
    bool isError() const { return m_error;}

    /**
     * @brief 返回日志模板
     */
    const std::string getPattern() const { return m_pattern;}
private:
    /// 日志格式模板
    std::string m_pattern;
    /// 日志格式解析后格式
    std::vector<FormatItem::ptr> m_items;
    /// 是否有错误
    bool m_error = false;
};

/**
 * @brief: 日志输出目标抽象基类
 */
class LogAppender {
friend class Logger; 
public: 
    typedef std::shared_ptr<LogAppender> ptr;

    typedef Spinlock MutexType;

    virtual ~LogAppender() {}

    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

    void setFormatter(LogFormatter::ptr val);

    LogFormatter::ptr getFormatter();

    LogLevel::Level getLevel() const { return m_level;}

    void setLevel(LogLevel::Level level) { m_level = level;}

    /**
     * @brief 将日志输出目标的配置转成YAML String
     */
    virtual std::string toYamlString() = 0;

protected:
    LogLevel::Level m_level = LogLevel::DEBUG;

    bool m_hasFormatter = false;

    /// 自旋锁
    MutexType m_mutex;

    LogFormatter::ptr m_formatter;
};

class Logger: public std::enable_shared_from_this<Logger> {
friend class LoggerManager;
public:
    typedef std::shared_ptr<Logger> ptr;
    typedef Spinlock MutexType;

    Logger(const std::string& name = "root");
    
    void log(LogLevel::Level level, LogEvent::ptr event);

    void debug(LogEvent::ptr event);

    void info(LogEvent::ptr event);

    void warn(LogEvent::ptr event);

    void error(LogEvent::ptr event);

    void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);

    void delAppender(LogAppender::ptr appender);

    void clearAppenders();

    LogLevel::Level getLevel() const { return m_level;}

    /**
     * @brief 设置日志级别
     */
    void setLevel(LogLevel::Level val) { m_level = val;}

    const std::string& getName() const { return m_name;}

    /**
     * @brief: 设置日志器格式by ptr 
     */
    void setFormatter(LogFormatter::ptr val);

    /**
     * @brief: 设置日志器格式by name
     */
    void setFormatter(const std::string& val);

    /**
     * @brief 获取日志格式器
     */
    LogFormatter::ptr getFormatter();

    /**
     * @brief 将日志器的配置转成YAML String
     */
    std::string toYamlString();
private:
    std::string m_name;

    LogLevel::Level m_level;

    MutexType m_mutex;

    std::list<LogAppender::ptr> m_appenders;

    LogFormatter::ptr m_formatter;

    Logger::ptr m_root;
};

/**
 * @brief 输出到控制台的Appender
 */
class StdoutLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;

    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

    std::string toYamlString() override;
};

/**
 * @brief 输出到文件的Appender
 */
class FileLogAppender : public LogAppender {
public:
    typedef std::shared_ptr<FileLogAppender> ptr;

    FileLogAppender(const std::string& filename);

    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
    
    std::string toYamlString() override;

    /**
     * @brief 重新打开日志文件
     * @return 成功返回true
     */
    bool reopen();
private:
    /// 文件路径
    std::string m_filename;
    /// 文件流
    std::ofstream m_filestream;
    /// 上次重新打开时间
    uint64_t m_lastTime = 0;
};


/**
*
 * @brief: 日志管理器类 
 * @param[in] {*}
 */
class LoggerManager {
public:
    typedef Spinlock MutexType;

    LoggerManager();

    Logger::ptr getLogger(const std::string& name);

    Logger::ptr getRoot() const { return m_root;}

    std::string toYamlString();

private:

    std::map<std::string, Logger::ptr> m_loggers;
    
    Logger::ptr m_root;

    MutexType m_mutex;
};
/// 日志器管理类单例模式
typedef pudge::Singleton<LoggerManager> LoggerMgr;

/**
 * @brief: 日志输出的定义格式
 */
struct LogAppenderDefine {
    int type = 0; //1 File, 2 Stdout
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine& oth) const {
        return type == oth.type
            && level == oth.level
            && formatter == oth.formatter
            && file == oth.file;
    }
};

/**
 * @brief: 日志器的定义格式
 */
struct LogDefine {
    std::string name;
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const {
        return name == oth.name
            && level == oth.level
            && formatter == oth.formatter
            && appenders == appenders;
    }

    bool operator<(const LogDefine& oth) const {
        return name < oth.name;
    }

    bool isValid() const {
        return !name.empty();
    }
};

}