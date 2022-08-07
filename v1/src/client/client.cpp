#include "EasyTcpClient.hpp"
#include "MessageHeader.hpp"
#include <thread>

#define clientCount 1000 // 客户端连接数量，总共1000个客户端连接
#define threadCount 4    // 线程数量，1000个连接分为4个线程，每个线程承担250个连接

bool g_bRun = true;
void cmdThread()
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

// 客户端数组，放在外面是作为共享资源，被多线程任务访问
EasyTcpClient *client[clientCount];

void sendThread(int id)
{
    // 总共4个线程 ID ： 1 ~ 4
    int cUnit = clientCount / threadCount;
    int begin = (id - 1) * cUnit;
    int end = id * cUnit;

    for (int i = begin; i < end; i++)
    {
        if (g_bRun)
        {
            client[i] = new EasyTcpClient();
        }
    }

    for (int i = begin; i < end; i++)
    {
        if (g_bRun)
        {
            client[i]->InitSocket();
            client[i]->Connect("127.0.0.1", 9999);
        }
    }

    printf("thread<%d>,Connect<begin=%d, end=%d>\n", id, begin, end);

    std::chrono::milliseconds t(3000);
    std::this_thread::sleep_for(t);

    Login login[10];
    for (int i = 0; i < 10; i++)
    {
        strcpy(login[i].userName, "lyd");
        strcpy(login[i].passWord, "lydmm");
    }
    const int nLen = sizeof(login);

    while (g_bRun)
    {
        for (int i = begin; i < end; i++)
        {
            client[i]->SendData(login, nLen);
            client[i]->OnRun();
        }
        // printf("空闲时间处理其他业务...\n");
        // Sleep(1000);
    }

    for (int i = begin; i < end; i++)
    {
        client[i]->Close();
        delete client[i];
    }

    printf("thread<%d> 退出...\n", id);
}

int main()
{
    // 创建 exit 线程，用于退出程序
    std::thread t1(cmdThread);
    t1.detach(); // 线程分离

    // 创建发送线程
    for (int i = 0; i < threadCount; i++)
    {
        std::thread t1(sendThread, i + 1);
        t1.detach(); // 线程分离
    }

    while (g_bRun)
    {
        Sleep(100);
    }

    printf("已退出。\n");
    return 0;
}