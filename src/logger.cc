#include "logger.h"

#include <ctime>
#include <fstream>
#include <iostream>

Logger &Logger::GetInstance()
{
    static Logger logger;
    return logger;
}

Logger::Logger()
{
    // 启动专门的写日志线程
    std::thread writeLogTask([&](){
        while(true)
        {
            // 获取当天的日期，取日志信息，写入相应的日志文件当中
            time_t now = time(nullptr);
            tm* nowtm = localtime(&now);
            
            char file_name[128];
            sprintf(file_name, "%d-%d-%d-log.txt", 
                    nowtm->tm_year + 1900, nowtm->tm_mon + 1, nowtm->tm_mday);
            
            std::ofstream out(file_name, std::ios_base::app);
            if(!out.is_open())
            {
                std::cout << "logger file: " << file_name << " open error!" << std::endl;
                exit(EXIT_FAILURE);
            }
            std::string msg = std::move(m_lckQue.Pop());

            char time_buf[128] = {0};
            sprintf(time_buf, "%d:%d:%d => ", nowtm->tm_hour, nowtm->tm_min, nowtm->tm_sec);
            out << time_buf << msg << std::endl;
            out.close();
        }
    });
    writeLogTask.detach();
}

void Logger::SetLogLevel(LogLevel level)
{
    m_loglevel = level;
}

// 吧日志信息写入到队列缓冲区中
void Logger::Log(std::string msg)
{
    m_lckQue.Push(msg);
}
