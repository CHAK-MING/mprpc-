#pragma once

#include <semaphore.h>
#include <zookeeper/zookeeper.h>
#include <string>


// 封装的zk客户端类
class ZkClient
{
public:
    ZkClient();
    ~ZkClient();
    // 连接zk服务器
    void Start();
    // 创建znode结点
    void Create(const char* path, const char* data, int datalen, int state = 0);
    // 获取结点的数据
    std::string GetData(const char* path);
private:
    // zk客户端的句柄
    zhandle_t* m_zhandle;
};