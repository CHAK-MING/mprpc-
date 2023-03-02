#include <iostream>
#include "mprpcapplication.h"
#include "user.pb.h"

int main(int argc, char *argv[])
{
    // 初始化一次
    MprpcApplication::Init(argc, argv);

    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    fixbug::LoginRequest login_request;
    login_request.set_name("zhang san");
    login_request.set_pwd("123456");

    fixbug::LoginResponse login_response;

    MprpcController controller;
    
    // 调用远程发布的rpc方法Login
    stub.Login(&controller, &login_request, &login_response, nullptr); // RpcChannel->callMethod(); 来做序列化和网络发送
    
    if(controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl;
        exit(EXIT_FAILURE);
    }
    // 一次rpc调用完成
    if(login_response.result().errcode() == 0)
    {
        std::cout << "rpc login response: " << login_response.success() << std::endl;
    }
    else
    {
        std::cout << "rpc login response error: " << login_response.result().errmsg() << std::endl;
    }

    // 演示注册方法
    fixbug::RegisterRequest reg_request;
    reg_request.set_id(1);
    reg_request.set_name("zhang san");
    reg_request.set_pwd("123456");

    fixbug::RegisterResponse reg_response;
    
    // 调用远程发布的rpc方法Login
    stub.Register(&controller, &reg_request, &reg_response, nullptr); // RpcChannel->callMethod(); 来做序列化和网络发送
    
    if(controller.Failed())
    {
        std::cout << controller.ErrorText() << std::endl;
        exit(EXIT_FAILURE);
    }

    // 一次rpc调用完成
    if(reg_response.result().errcode() == 0)
    {
        std::cout << "rpc register response: " << reg_response.success() << std::endl;
    }
    else
    {
        std::cout << "rpc register response error: " << reg_response.result().errmsg() << std::endl;
    }

    return 0;
}