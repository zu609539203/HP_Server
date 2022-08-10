#ifndef _Cell_hpp_
#define _Cell_hpp_

// SOCKET 相关头文件
#ifdef _WIN32
#define FD_SETSIZE	5024	// 突破系统select只能限制64个客户端连接服务器的限制
#define WIN32_LEAN_AND_MEAN
#include<WS2tcpip.h>
#include<windows.h>
#include<winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>
#include<signal.h>

#define SOCKET int;
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR		   (-1)
#endif // _WIN32

// 自定义头文件
#include"MessageHeader.hpp"
#include"CELLTimestamp.hpp"
#include"CellTask.hpp"
#include"CELLLog.hpp"

// 缓冲区大小单元 10Kb
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240 * 10
#define SEND_BUFF_SIZE 10240 * 10
#endif // !RECV_BUFF_SIZE

#endif // !_Cell_hpp_

