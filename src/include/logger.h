#pragma once

#include "lockqueue.hpp"

#include <string>
#include <iostream>

enum LogLevel
{
    INFO, 
    ERROR,
};

// Mprpc框架提供的日志系统
class Logger
{
public:
    // 获取日志的单例
    static Logger& GetInstance();
    // 设置日志级别
    void SetLogLevel(LogLevel level);
    // 写日志
    void Log(std::string msg);
private:
    int m_loglevel; // 日志级别
    LockQueue<std::string> m_lckQue; // 日志缓冲队列

    Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(const Logger&&) = delete;
    Logger& operator=(const Logger&&) = delete;
};

// 日志宏
#define LOG_INFO(logmsgformat, ...)\
    do\
    {\
        Logger::GetInstance().SetLogLevel(INFO);\
        char c[1024] = {0};\
        snprintf(c, 1024, logmsgformat, ##__VA_ARGS__);\
        Logger::GetInstance().Log(c);\
    }while(0)

#define LOG_ERROR(logmsgformat, ...)\
    do\
    {\
        Logger::GetInstance().SetLogLevel(ERROR);\
        char c[1024] = {0};\
        snprintf(c, 1024, logmsgformat, ##__VA_ARGS__);\
        Logger::GetInstance().Log(c);\
    }while(0)
