#pragma once
#include "logger.h"

#include <string>
#include <unordered_map>
#include <functional>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>


// 框架提供专门服务发布rpc的服务的网络对象类
class RpcProvider
{
public:
    // 这是框架提供给外部使用的，与业务无关
    void NotifyService(google::protobuf::Service* service);

    // 启动rpc服务的结点，开始提供rpc远程网络调用服务
    void Run();

private:
    // 新的socket连接回调
    void OnConnection(const muduo::net::TcpConnectionPtr&);
    // 已建立连接用户的读写事件的回调
    void OnMessage(const muduo::net::TcpConnectionPtr&, muduo::net::Buffer*, muduo::Timestamp);
    // Closure的回调操作，用于序列化rpc的响应和网络发送
    void SendRpcResponse(const muduo::net::TcpConnectionPtr&, google::protobuf::Message*);
private:
    muduo::net::EventLoop m_eventLoop;

    // 服务类型信息
    struct ServiceInfo
    {
        google::protobuf::Service* m_service; // 服务对象
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor*> m_methodMap; // 保存服务方法
    };
    std::unordered_map<std::string, ServiceInfo> m_serviceInfoMap; // 存储注册成功的服务对象和期服务方法的信息
};