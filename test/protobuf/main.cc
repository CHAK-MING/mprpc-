#include "test.pb.h"
#include <iostream>
#include <string>

using namespace fixbug;

int main()
{
    // LoginResponse rsp;
    // ResultCode* rc = rsp.mutable_result();
    // rc->set_errcode(0);
    // rc->set_errmsg("登录处理失败了");

    GetFriendListsResponse rsp;
    ResultCode* rs = rsp.mutable_result();
    rs->set_errcode(0);
    User* user1 = rsp.add_friend_list();
    user1->set_name("zhangsan");
    user1->set_sex(User::MAN);
    
    std::cout << rsp.friend_list_size() << std::endl;

    return 0;
}

// int main()
// {
//     // 封装了login请求对象的数据
//     LoginRequest req;
//     req.set_name("zhang san");
//     req.set_pwd("123456");
    
//     // 对象数据序列化 => char*
//     std::string send_str;
//     if(req.SerializeToString(&send_str))
//     {
//         std::cout << send_str << std::endl;
//     }

//     // 反序列化
//     LoginRequest reqB;
//     if(reqB.ParseFromString(send_str))
//     {
//         std::cout << reqB.name() << std::endl;
//         std::cout << reqB.pwd() << std::endl;
//     }

//     return 0;
// }