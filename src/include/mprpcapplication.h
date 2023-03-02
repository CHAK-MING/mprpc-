#pragma once
#include "mprpcconfig.h"
#include "mprpcchannel.h"
#include "mprpccontroller.h"


// mprpc框架的基础类
class MprpcApplication
{
public:
    static void Init(int argc, char *argv[]);
    static MprpcApplication& GetInstance();
    static MprpcConfig& getConfig() { return m_config; }
private:
    MprpcApplication() {}
    MprpcApplication(const MprpcApplication&) = delete;
    MprpcApplication(MprpcApplication&&) = delete;
    MprpcApplication& operator=(const MprpcApplication&) = delete;
    MprpcApplication& operator=(MprpcApplication&&) = delete;
private:
    static MprpcConfig m_config;
};