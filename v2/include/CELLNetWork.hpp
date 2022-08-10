#ifndef _CELL_NET_WORK_HPP_
#define _CELL_NET_WORK_HPP_

#include"CELL.hpp"

class CELLNetWork
{
private:
	CELLNetWork()
	{
#ifdef _WIN32
		// 启动Win Socket 2.x环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif

#ifndef _WIN32
		//if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		//	return (1);
		// 忽略异常信号，默认情况下会导致进程终止
		signal(SIGPIPE, SIG_IGN);
#endif // !_WIN32
	}

	~CELLNetWork()
	{
#ifdef _WIN32
		// 清除Windows socket环境
		WSACleanup();
#endif // _WIN32
	}

public:
	// 单例模式，无论调用多少次，也只会创建这一个对象，因此不会多次初始化网络环境
	static void Init()
	{
		static CELLNetWork obj;
	}

private:

};

#endif // !_CELL_NET_WORK_HPP_
