#include "zookeeperutil.h"
#include "mprpcapplication.h"
#include "logger.h"

#include <iostream>

void global_watch(zhandle_t *zh, int type, 
        int state, const char *path,void *watcherCtx)
{
    if(type == ZOO_SESSION_EVENT) // 回调消息类型
    {
        if(state == ZOO_CONNECTED_STATE)
        {
            sem_t *sem = (sem_t*)zoo_get_context(zh);
            sem_post(sem);
        }
    }
}

ZkClient::ZkClient()
    : m_zhandle(nullptr)
{}

ZkClient::~ZkClient()
{
    if(m_zhandle != nullptr)
    {
        zookeeper_close(m_zhandle);
    }
}

void ZkClient::Start()
{
    std::string host = MprpcApplication::GetInstance().getConfig().Load("zookeeperip");
    std::string port = MprpcApplication::GetInstance().getConfig().Load("zookeeperport");
    std::string connstr = host + ":" + port;

    m_zhandle = zookeeper_init(connstr.c_str(), global_watch, 30000, nullptr, nullptr, 0);
    if(nullptr == m_zhandle)
    {
        LOG_ERROR("zookeeper_init error!");
        exit(EXIT_FAILURE);
    }

    sem_t sem;
    sem_init(&sem, 0, 0);
    zoo_set_context(m_zhandle, &sem);

    sem_wait(&sem);
    LOG_INFO("zookeeper_init success!");
}

void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int flag;
    // 先判断path的结点是否存在
    flag = zoo_exists(m_zhandle, path, 0, nullptr);
    if(ZNONODE == flag)
    {
        flag = zoo_create(m_zhandle, path, data, datalen, 
            &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
        if(flag == ZOK)
        {
            LOG_INFO("create znode success... path: %s", path);
        }
        else
        {
            LOG_ERROR("flag: %d\ncreate znode error... path: %s", flag, path);
            exit(EXIT_FAILURE);
        }
    }
}

std::string ZkClient::GetData(const char *path)
{
    char buffer[64];
    int bufflen = sizeof(buffer);
    int flag = zoo_get(m_zhandle, path, 0, buffer, &bufflen, nullptr);
    if(flag != ZOK)
    {
        LOG_ERROR("get znode error... path: %s", path);
        return "";
    }
    return buffer;
}
