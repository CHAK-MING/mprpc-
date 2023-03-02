#include "mprpcchannel.h"
#include "rpcheader.pb.h"
#include "mprpcapplication.h"
#include "mprpccontroller.h"
#include "zookeeperutil.h"
#include "logger.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

/**
 * header_size + service_name method_name args_size + args
 * 
*/

void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                              google::protobuf::RpcController *controller,
                              const google::protobuf::Message *request,
                              google::protobuf::Message *response,
                              google::protobuf::Closure *done)
{
    const google::protobuf::ServiceDescriptor* sd = method->service();
    std::string service_name = sd->name();
    std::string method_name = method->name();

    // 获取参数序列化字符串长度args_size
    uint32_t args_size = 0;
    std::string args_str;
    if(request->SerializeToString(&args_str))
    {
        args_size = args_str.size();
    }
    else
    {
        controller->SetFailed("serialize request error!");
        return;
    }

    // 定义rpc请求header
    uint32_t header_size = 0;
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    // 序列化
    std::string rpc_header_str;
    if(rpcHeader.SerializeToString(&rpc_header_str))
    {
        header_size = rpc_header_str.size();
    }
    else
    {
        controller->SetFailed("serialize rpc header error!");
        return;
    }

    // 组织待发送的rpc请求的字符串
    std::string send_rpc_str;
    send_rpc_str.insert(0, std::string((char*)&header_size, 4));
    send_rpc_str += rpc_header_str;
    send_rpc_str += args_str;

     // 打印调试信息
    LOG_INFO("=========================================");
    LOG_INFO("%s header_size: %d", __FUNCTION__, header_size);
    LOG_INFO("%s rpc_header_str: %s",__FUNCTION__, rpc_header_str.c_str());
    LOG_INFO("%s service_name: %s",__FUNCTION__, service_name.c_str());
    LOG_INFO("%s method_name: %s",__FUNCTION__, method_name.c_str());
    LOG_INFO("%s args_str: %s",__FUNCTION__, args_str.c_str());
    LOG_INFO("=========================================");

    // 使用tcp编程，完成rpc方法的远程调用
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == clientfd)
    {
        controller->SetFailed("create socket error, errno: " + std::to_string(errno));
        return;
    }

    //std::string ip = MprpcApplication::GetInstance().getConfig().Load("rpcserverip");
    //uint16_t port = std::stoul(MprpcApplication::GetInstance().getConfig().Load("rpcserverport"));

    ZkClient zkCli;
    zkCli.Start();
    std::string method_path = "/" + service_name + "/" + method_name;
    std::string host_data = zkCli.GetData(method_path.c_str());
    if(host_data == "")
    {
        controller->SetFailed(method_path + " is not exist!");
        return;
    }
    int idx = host_data.find(":");
    if(idx == -1)
    {
        controller->SetFailed(method_path + " address is invalid!");
        return;
    }

    std::string ip = host_data.substr(0, idx);
    uint16_t port = std::stoi(host_data.substr(idx + 1, host_data.size() - idx));

    sockaddr_in server_addr;
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if(- 1 == connect(clientfd, (sockaddr*)&server_addr, sizeof(sockaddr)))
    {
        controller->SetFailed("connect error, errno: " + std::to_string(errno));
        close(clientfd);
        return;
    }

    // 发送rpc请求
    if(-1 == send(clientfd, send_rpc_str.c_str(), send_rpc_str.size() , 0))
    {
        controller->SetFailed("send error, errno: " + std::to_string(errno));
        close(clientfd);
        return;
    }

    // 接受rpc请求的响应值
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if(-1 == (recv_size = recv(clientfd, recv_buf, 1024, 0)))
    {
        controller->SetFailed("recv error, errno: " + std::to_string(errno));
        close(clientfd);
        return;
    }
    recv_buf[recv_size] = '\0';
    if(!response->ParseFromArray(recv_buf, recv_size))
    {
        controller->SetFailed("parse error! response_str: " + std::string(recv_buf));
        close(clientfd);
        return;
    }

    close(clientfd);
}
