#include "EasyTcpServer.hpp"
#include "MessageHeader.hpp"
#include <thread>

bool g_bRun = true;
void cmdTread()
{
    while (1)
    {
        char cmdBuf[256] = {};
        scanf_s("%s", cmdBuf, 256);
        if (0 == strcmp(cmdBuf, "exit"))
        {
            g_bRun = false;
            printf("退出...\n");
            break;
        }
        else
        {
            printf("不被支持的命令...\n");
        }
    }
}

class MyServer : public EasyTcpServer
{
public:
    //只会被一个线程触发 安全
    virtual void OnNetJoin(ClientSocket *pClient)
    {
        _clientCount++;
        // printf("client<%d> join\n", pClient->sockfd());
    }

    // cellServer 4 多个线程触发 不安全
    //如果只开启1个cellServer就是安全的
    virtual void OnNetLeave(ClientSocket *pClient)
    {
        _clientCount--;
        // printf("client<%d> leave\n", pClient->sockfd());
    }

    // cellServer 4 多个线程触发 不安全
    //如果只开启1个cellServer就是安全的
    virtual void OnNetMsg(DataHeader *header, ClientSocket *pClient)
    {
        _recvCount++;

        // 处理请求
        switch (header->cmd)
        {
        case CMD_LOGIN:
        {
            Login *login = (Login *)header;
            // printf("收到客户端<%d>命令：CMD_LOGIN，数据长度：%d，用户名：%s\n", cfd, login->dataLength, login->userName);
            //  忽略判断用户名密码是否正确
            LoginResult ret;
            pClient->SendData(&ret);
        }
        break;
        case CMD_LOGOUT:
        {
            Logout *logout = (Logout *)header;
            // printf("收到客户端<%d>命令：CMD_LOGOUT，数据长度：%d，用户名：%s\n", cfd, logout->dataLength, logout->userName);
            //  忽略判断用户名密码是否正确
            // LogoutResult ret;
            // SendData(&ret, cfd);
        }
        break;
        default:
            printf("收到未定义消息, 数据长度为：%d\n", header->dataLength);
            break;
        }
    }
};

int main()
{
    MyServer server;
    server.InitSocket();
    server.Bind(NULL, 9999);
    server.Listen(10);
    server.Start(4);

    //线程 thread
    std::thread t1(cmdTread);
    t1.detach(); // 线程分离

    while (g_bRun)
    {
        server.OnRun();
        // std::cout << "空闲时间处理其他业务..." << std::endl;
    }

    server.Close();
    printf("任务完成...\n");
    getchar();
    return 0;
}