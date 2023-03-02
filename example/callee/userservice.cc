#include <iostream>
#include <string>

#include "user.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include "logger.h"


/**
 * UserService原本是一个本地服务器，提供了两个进程内的本地方法，Login和GetFriendLists
 *
 */
class UserService : public fixbug::UserServiceRpc
{
public:
    bool Login(std::string name, std::string pwd)
    {
        std::cout << "doing local service: Login" << std::endl;
        std::cout << "name: " << name << " pwd: " << pwd << std::endl;
        return true;
    }

    bool Register(uint32_t id, std::string name, std::string pwd)
    {
        std::cout << "doing local service: Register" << std::endl;
        std::cout << "id: " << id << "name: " << name << " pwd: " << pwd << std::endl;
        return true;
    }

    /**
     * 重写UserServicePPC的虚函数 下面这些方法都是框架调用
     * 1. caller => Login(LoginRequest) => muduo => callee
     * 2. callee => Login(LoginRequest) => 下面重写的这个方法
    */

    void Login(::google::protobuf::RpcController *controller,
               const ::fixbug::LoginRequest *request,
               ::fixbug::LoginResponse *response,
               ::google::protobuf::Closure *done)
    {
        // 框架给业务上报了请求参数LoginRequest，应用获取相应数据做本地业务
        std::string name = request->name();
        std::string pwd = request->pwd();

        bool login_result = Login(name,pwd); // 本地业务

        // 响应写入response
        fixbug::ResultCode* code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(login_result);

        // 执行回调操作 执行相应对象数据的序列化和网络发送（框架来完成）
        done->Run();
    }

    void Register(::google::protobuf::RpcController *controller,
               const ::fixbug::RegisterRequest *request,
               ::fixbug::RegisterResponse *response,
               ::google::protobuf::Closure *done)
    {
        // 框架给业务上报了请求参数LoginRequest，应用获取相应数据做本地业务
        uint32_t id = request->id();
        std::string name = request->name();
        std::string pwd = request->pwd();

        bool register_result = Register(id, name, pwd); // 本地业务

        // 响应写入response
        fixbug::ResultCode* code = response->mutable_result();
        code->set_errcode(0);
        code->set_errmsg("");
        response->set_success(register_result);

        // 执行回调操作 执行相应对象数据的序列化和网络发送（框架来完成）
        done->Run();
    }
};

int main(int argc, char* argv[])
{
    // 框架的初始化
    MprpcApplication::Init(argc, argv);

    // 把UserService对象发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new UserService());

    // 启动一个rpc服务发布节点
    provider.Run();

}