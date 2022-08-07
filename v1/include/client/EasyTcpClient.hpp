#ifndef _EasyTcpClient_hpp
#define _EasyTcpClient_hpp

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WS2tcpip.h>
#include <windows.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#define SOCKET int;
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#endif // _WIN32
#include <iostream>
#include "MessageHeader.hpp"

class EasyTcpClient
{
private:
    SOCKET cfd;

public:
    EasyTcpClient()
    {
        cfd = INVALID_SOCKET;
    }

    virtual ~EasyTcpClient()
    {
        closesocket(cfd);
    }

    // 初始化socket
    void InitSocket()
    {
#ifdef _WIN32
        // 启动Win Socket 2.x环境
        WORD ver = MAKEWORD(2, 2);
        WSADATA dat;
        WSAStartup(ver, &dat);
#endif
        // 创建socket
        if (cfd != INVALID_SOCKET)
        {
            printf("<socket = %d>关闭先前的连接...\n", cfd);
            Close();
        }
        cfd = socket(AF_INET, SOCK_STREAM, 0);
        if (cfd == INVALID_SOCKET)
        {
            printf("建立socket失败...\n");
        }
        else
        {
            // printf("建立socket成功...\n");
        }
    }

    // 客户端连接服务器
    int Connect(const char *ip, unsigned short port)
    {
        if (cfd == INVALID_SOCKET)
        {
            InitSocket();
        }
        sockaddr_in saddr;
#ifdef _WIN32
        inet_pton(AF_INET, ip, &saddr.sin_addr.S_un.S_addr);
#else
        inet_pton(AF_INET, ip, &saddr.sin_addr.s_addr);
#endif // _WIN32
        saddr.sin_port = htons(port);
        saddr.sin_family = AF_INET;
        int ret = connect(cfd, (sockaddr *)&saddr, sizeof(saddr));
        if (ret == SOCKET_ERROR)
        {
            // printf("连接失败...\n");
        }
        else
        {
            // printf("连接成功...\n");
        }
        return ret;
    }

    // 关闭客户端socket
    void Close()
    {
        if (cfd != INVALID_SOCKET)
        {
#ifdef _WIN32
            closesocket(cfd);
            // 清除Windows socket环境
            WSACleanup();
#else
            close(cfd);
#endif // _WIN32
            cfd = INVALID_SOCKET;
        }
    }

    // 处理网络消息
    int _count = 0;
    bool OnRun()
    {
        if (isRun())
        {
            // select IO多路复用
            fd_set fdRead;

            FD_ZERO(&fdRead);
            FD_SET(cfd, &fdRead);
            timeval t = {1, 0};
            int ret = select(100, &fdRead, NULL, NULL, &t);
            // printf("ret = %d ... count = %d\n", ret, _count++);

            if (ret < 0)
            {
                printf("<socket = %d> select error...\n", cfd);
                return false;
            }
            if (FD_ISSET(cfd, &fdRead))
            {
                FD_CLR(cfd, &fdRead);
                // 接收、处理数据
                if (-1 == RecvData(cfd))
                {
                    printf("<socket = %d>服务端退出，select任务结束...\n", cfd);
                    return false;
                }
            }
            return true;
        }
        else
        {
            return false;
        }
    }

    // 判断客户端是否工作中
    bool isRun()
    {
        return cfd != INVALID_SOCKET;
    };

        // 接收缓冲区大小单元
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif // !RECV_BUFF_SIZE

    // 接收缓冲区
    char _szRecv[RECV_BUFF_SIZE] = {};
    // 第二缓冲区 消息缓冲区
    char _szMsgBuf[RECV_BUFF_SIZE * 10] = {};
    // 第二缓冲区尾部指针
    int _lastPos = 0;

    // 接收数据 处理粘包
    int RecvData(SOCKET cfd)
    {
        // 接受服务器数据
        int len = recv(cfd, _szRecv, RECV_BUFF_SIZE, 0);
        // printf("len = %d\n", len);
        if (len <= 0)
        {
            printf("len = %d...与服务器断开连接...\n", len);
            return -1;
        }
        // 将“接收缓冲区”数据拷贝到“第二缓冲区”
        memcpy(_szMsgBuf + _lastPos, _szRecv, len);
        // 第二缓冲区数据尾部后移
        _lastPos += len;
        // 判断第二缓冲区的数据长度是否大于消息头DataHeader长度
        while (_lastPos >= sizeof(DataHeader))
        {
            // 知道消息体（header + body）长度
            DataHeader *header = (DataHeader *)_szMsgBuf;
            if (_lastPos >= header->dataLength) // 处理 粘包 情况
            {
                // 剩余未处理消息缓冲区的长度
                int nSize = _lastPos - header->dataLength;
                // 处理网络消息
                OnNetMsg(header);
                // 将“第二缓冲区”未处理数据前移
                memcpy(_szMsgBuf, _szMsgBuf + header->dataLength, nSize);
                // “第二缓冲区”尾部数据指针前移
                _lastPos = nSize;
            }
            else // 处理 少包 情况
            {
                // “第二缓冲区”剩余数据不足够一条完整消息
                break;
            }
        }

        return 0;
    }

    // 响应网络消息
    virtual void OnNetMsg(DataHeader *header)
    {
        switch (header->cmd)
        {
        case CMD_LOGIN_RESULT:
        {
            LoginResult *loginresult = (LoginResult *)header;
            // printf("收到服务器消息：CMD_LOGIN_RESULT, result：%d, 数据长度为：%d\n", loginresult->result, loginresult->dataLength);
        }
        break;
        case CMD_LOGOUT_RESULT:
        {
            LogoutResult *logoutresult = (LogoutResult *)header;
            // printf("收到服务器消息：CMD_LOGOUT_RESULT...result：%d, 数据长度为：%d\n", logoutresult->result, logoutresult->dataLength);
        }
        break;
        case CMD_NEW_USER_IN:
        {
            NewUserIn *new_uer_in = (NewUserIn *)header;
            // printf("新的客户端加入，socket为%d\n", new_uer_in->sock);
        }
        break;
        case CMD_ERROR:
        {
            printf("收到服务器消息：CMD_ERROR, 数据长度为：%d\n", header->dataLength);
        }
        break;
        default:
            printf("收到未定义消息, 数据长度为：%d\n", header->dataLength);
            break;
        }
    }

    // 发送数据
    int SendData(DataHeader *header, int nLen)
    {
        int ret = SOCKET_ERROR;
        if (isRun() && header)
        {
            ret = send(cfd, (const char *)header, nLen, 0);
            if (ret == SOCKET_ERROR)
            {
                Close();
            }
        }
        return ret;
    }
};

#endif // !_EasyTcpClient_hpp
