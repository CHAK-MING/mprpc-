#include "rpcprovider.h"
#include "mprpcapplication.h"
#include "rpcheader.pb.h"
#include "zookeeperutil.h"

using namespace std::placeholders;

void RpcProvider::NotifyService(google::protobuf::Service *service)
{
    ServiceInfo service_info;

    // 获取服务对象的描述信息
    auto pserviceDesc = service->GetDescriptor();
    // 获取服务方法
    std::string service_name = pserviceDesc->name();
    // 获取服务对象service的方法的数量
    int methodCnt = pserviceDesc->method_count();

    LOG_INFO("service_name: %s", service_name);

    for(int i = 0; i < methodCnt; ++i)
    {
        // 获取服务对象指定的服务方法描述
        auto pmethodDesc = pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();
        service_info.m_methodMap.insert({method_name, pmethodDesc});

        LOG_INFO("method_name: %s", method_name);

    }
    service_info.m_service = service;
    m_serviceInfoMap.insert({service_name, service_info});
}

void RpcProvider::Run()
{
    std::string ip = MprpcApplication::GetInstance().getConfig().Load("rpcserverip");
    uint16_t port = std::stoul(MprpcApplication::GetInstance().getConfig().Load("rpcserverport"));
    muduo::net::InetAddress address(ip, port);

    // 创建TcpServer
    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");
    // 绑定连接回调和消息读写回调
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, _1));
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this, _1, _2, _3));

    // 设置muduo库的线程数量
    server.setThreadNum(4);

    // 把当前rpc节点上要发布的服务全部注册到zk上面
    ZkClient zkCli;
    zkCli.Start();
    // service_name为永久性结点，method_name为临时性结点
    for(auto& sp : m_serviceInfoMap)
    {
        std::string service_path = "/" + sp.first;
        zkCli.Create(service_path.c_str(), nullptr, 0);
        for(auto& mp : sp.second.m_methodMap)
        {
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            // ZOO_EPHEMERAL是临时性结点
            zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }

    LOG_INFO("RpcProvider start service at ip: %s port: %d",ip.c_str(), port);
    // 启动网络服务
    server.start();
    m_eventLoop.loop();
}

void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        // 和rpc client的连接断开了
        conn->shutdown();
    }
}

/*
 * 在框架内部 RpcProvider和RpcConsumer协商好之间通信的protobuf数据类型
 * service_name method_name args 定义proto的message类型，进行数据头的序列化和反序列化
 *                               service_name method_name args_size
 * header_size + header_str + args_str
*/

void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr & conn,
                            muduo::net::Buffer* buffer, muduo::Timestamp time)
{
    // 网络上接受的远程rpc调用的字符流
    std::string recv_buf = buffer->retrieveAllAsString();

    // 从字符流中读取前4个字节的内容
    uint32_t header_size = 0;
    recv_buf.copy((char*)&header_size , 4, 0);

    // 根据header_size读取数据头的原始字节流
    std::string rpc_header_str = recv_buf.substr(4, header_size);
    mprpc::RpcHeader rpcHeader;
    
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    if(rpcHeader.ParseFromString(rpc_header_str))
    {
        // 数据头反序列化成功
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size();
    }
    else
    {
        LOG_ERROR("rpc_header_str: %s parse error!",rpc_header_str.c_str());
        return;
    }

    // 获取rpc方法参数的字符流数据
    std::string args_str = recv_buf.substr(4 + header_size, args_size);

    // 打印调试信息
    LOG_INFO("=========================================");
    LOG_INFO("%s header_size: %d", __FUNCTION__, header_size);
    LOG_INFO("%s rpc_header_str: %s",__FUNCTION__, rpc_header_str.c_str());
    LOG_INFO("%s service_name: %s",__FUNCTION__, service_name.c_str());
    LOG_INFO("%s method_name: %s",__FUNCTION__, method_name.c_str());
    LOG_INFO("%s args_str: %s",__FUNCTION__, args_str.c_str());
    LOG_INFO("=========================================");

    // 获取service对象和method对象
    auto it = m_serviceInfoMap.find(service_name);
    if(it == m_serviceInfoMap.end())
    {
        LOG_ERROR("%s is not exist!", service_name.c_str());
        return;
    }

    auto mit = it->second.m_methodMap.find(method_name);
    if(mit == it->second.m_methodMap.end())
    {
        LOG_ERROR("%s:%s is not exist!", service_name.c_str(), method_name.c_str());
        return;
    }

    // 获取服务对象 
    google::protobuf::Service* service = it->second.m_service;
    // 获取方法描述
    const google::protobuf::MethodDescriptor* method = mit->second;

    // 生成rpc方法调用的请求request和响应response参数
    google::protobuf::Message* request = service->GetRequestPrototype(method).New();
    if(!request->ParseFromString(args_str))
    {
        LOG_ERROR("request parse error, content: %s", args_str.c_str());
        return;
    }
    google::protobuf::Message* response = service->GetResponsePrototype(method).New();

    // 给下面method方法调用，绑定一个Closure的回调
    auto done = 
        google::protobuf::NewCallback <RpcProvider,
                            const muduo::net::TcpConnectionPtr&,
                            google::protobuf::Message*>
        (this, &RpcProvider::SendRpcResponse, conn, response);

    // 调用rpc节点的方法
    // 相当于调用UserService().Login(controller,request, response, done);
    service->CallMethod(method, nullptr, request, response, done);
}

void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &conn, google::protobuf::Message *response)
{
    std::string response_str;
    if(response->SerializeToString(&response_str))
    {
        // 序列化成功后，通过网络的方法发送给rpc的调用方
        conn->send(response_str);
    }
    else
    {
        LOG_ERROR("serialize response_str error!");
    }
    conn->shutdown(); // 模拟短连接，由rpcprovider主动断开
}
