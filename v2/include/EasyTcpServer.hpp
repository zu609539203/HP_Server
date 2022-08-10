#ifndef _EasyTcpServer_hpp
#define _EasyTcpServer_hpp

#include"CELL.hpp"
#include"CELLClient.hpp"
#include"INetEvent.hpp"
#include"CELLServer.hpp"
#include"CELLNetWork.hpp"

#include<iostream>
#include<map>
#include<vector>
#include<thread>
#include<mutex>
#include<atomic>

// 
class EasyTcpServer : public INetEvent
{
public:
	EasyTcpServer() {
		_lfd = INVALID_SOCKET;
		_recvCount = 0;
		_msgCount = 0;
		_clientCount = 0;
	}

	virtual ~EasyTcpServer() {
		Close();
	}

	// 初始化Socket
	SOCKET InitSocket() {
		// 为什么要剥离初始化网络环境？因为会多个线程启动服务端，多次初始化会遇到问题
		CELLNetWork::Init();

		// 创建socket
		if (_lfd != INVALID_SOCKET) {
			CELLLog::Info("<socket = %d>关闭先前的连接...\n", _lfd);
			Close();
		}
		_lfd = socket(AF_INET, SOCK_STREAM, 0);
		if (_lfd == INVALID_SOCKET) {
			CELLLog::Info("建立socket失败...\n");
		}
		else {
			CELLLog::Info("建立socket成功...\n");
		}
		return _lfd;
	}

	// 绑定IP 和 端口号
	int Bind(const char * ip, unsigned short port) {
		if (_lfd == INVALID_SOCKET) {
			InitSocket();
		}
		sockaddr_in saddr;
#ifdef _WIN32
		if (ip) {
			inet_pton(AF_INET, ip, &saddr.sin_addr.S_un.S_addr);
		}
		else {
			inet_pton(AF_INET, "127.0.0.1", &saddr.sin_addr.S_un.S_addr);
		}
#else
		if (ip) {
			inet_pton(AF_INET, ip, &saddr.sin_addr.s_addr);
		}
		else {
			inet_pton(AF_INET, "127.0.0.1", &saddr.sin_addr.s_addr);
		}
#endif // _WIN32
		saddr.sin_port = htons(port);
		saddr.sin_family = AF_INET;
		int ret = bind(_lfd, (sockaddr *)&saddr, sizeof(saddr));
		if (SOCKET_ERROR == ret) {
			CELLLog::Info("绑定失败<Port:%d>...\n", port);
		}
		else {
			CELLLog::Info("绑定成功<Port:%d>...\n", port);
		}
		return ret;
	}

	// 监听端口号
	int Listen(int backlog = 5) {
		int ret = listen(_lfd, backlog);
		if (SOCKET_ERROR == ret) {
			CELLLog::Info("<Socket:%d>监听失败...\n", _lfd);
		}
		else {
			CELLLog::Info("<Socket:%d>监听成功...\n", _lfd);
		}
		return ret;
	}

	// 接受客户端连接
	SOCKET Accept() {
		sockaddr_in caddr;
		int c_len = sizeof(caddr);
		SOCKET cfd = INVALID_SOCKET;
#ifdef _WIN32
		cfd = accept(_lfd, (sockaddr *)&caddr, &c_len);
#else
		cfd = accept(_lfd, (sockaddr *)&caddr, &c_len);
#endif
		if (cfd == INVALID_SOCKET) {
			CELLLog::Info("<Socket:%d>错误，连接到无效客户端Socket...\n", _lfd);
		}
		else {
			// 获取IP地址： 
			// char cli_ip[15];
			// inet_ntop(AF_INET, &caddr.sin_addr, cli_ip, sizeof(cli_ip));

			// 把新客户端分配给客户数量最少的 cellServer
			addClient2CellServer(new CellClient(cfd));
		}
		return cfd;
	}

	void addClient2CellServer(CellClient* pClient) {
		// 查找客户数量最少的CellServer消息处理对象
		auto pMinServer = _cellServers[0];
		for (auto pCellServer : _cellServers) {
			if (pMinServer->getClientCount() > pCellServer->getClientCount()) {
				pMinServer = pCellServer;
			}
		}
		pMinServer->addClients(pClient);
		OnNetJoin(pClient);
	}

	void Start(int nCellServer) {
		for (int i = 0; i < nCellServer; i++) {
			auto ser = new CellServer(i + 1);
			_cellServers.push_back(ser);
			// 注册网络事件接受对象
			ser->setNetEventObj(this);
			// 启动消息处理线程
			ser->Start();
		}
		_thread.Start(
			nullptr,
			[this](CELLThread* pThread) {OnRun(pThread); }
		);
	}

	// 关闭Socket
	void Close() {
		CELLLog::Info("EasyTcpServer Close begin\n");
		_thread.Close();

		if (_lfd != INVALID_SOCKET) 
		{
			for (auto s : _cellServers)
			{
				delete s;
			}
			_cellServers.clear();
#ifdef _WIN32
			// 关闭socket _lfd
			closesocket(_lfd);
#else
			// 关闭socket _lfd
			close(_lfd);
#endif // _WIN32
			_lfd = INVALID_SOCKET;
		}
		CELLLog::Info("EasyTcpServer Close end\n");
	}


	//只会被一个线程触发 安全
	virtual void OnNetJoin(CellClient* pClient) {
		_clientCount++;
		//CELLLog::Info("client<%d> join\n", pClient->sockfd());
	}

	//cellServer 4 多个线程触发 不安全
	//如果只开启1个cellServer就是安全的
	virtual void OnNetLeave(CellClient* pClient) {
		_clientCount--;
		//CELLLog::Info("client<%d> leave\n", pClient->sockfd());
	}

	//cellServer 4 多个线程触发 不安全
	//如果只开启1个cellServer就是安全的
	virtual void OnNetMsg(CellClient* pClient, NetMsg_DataHeader* header, CellServer* pCellServer) {
		_msgCount++;
	}

	virtual void OnNetRecv(CellClient* pClient) {
		_recvCount++;
		//CELLLog::Info("client<%d> leave\n", pClient->sockfd());
	}

private:
	// 处理网络消息
	void OnRun(CELLThread* pThread) {
		while (pThread->isRun()) 
		{
			time4Msg();

			fd_set fdRead;

			FD_ZERO(&fdRead);

			FD_SET(_lfd, &fdRead);

			// 具体参数含义参见 IO多路复用 , nfds的设置在Windows中可以随意设置，但实际含义表示最大文件描述符的数目加一
			timeval t = { 0, 1 };
			int ret = select(_lfd + 1, &fdRead, NULL, NULL, &t);
			//CELLLog::Info("ret = %d ... count = %d\n", ret, _count++);

			if (ret < 0) {
				CELLLog::Info("EasyTcpServer OnRun Select Error!\n");
				pThread->Exit();
				break;
			}

			//判断描述符（socket）是否在集合中
			if (FD_ISSET(_lfd, &fdRead)) {
				// 若是有客户端接入，则添加到 fdRead，然后将其置为 0
				FD_CLR(_lfd, &fdRead);
				// accept 新的客户端加入
				Accept();
			}

			//std::cout << "空闲时间处理其他业务..." << std::endl;
		}
	}

	// 响应网络消息
	void time4Msg() {
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0)
		{
			CELLLog::Info("thread<%d>, time<%lf>, socket<%d>, client_nums<%d>, recvCount<%d>, msg(server_send)Count<%d>\n",
				_cellServers.size(), t1, _lfd, (int)_clientCount, (int)(_recvCount / t1), (int)(_msgCount / t1));

			_recvCount = 0;
			_msgCount = 0;
			_tTime.update();
		}
	}

protected:
	// 收到消息计数
	std::atomic<int> _msgCount;
	// 客户端计数
	std::atomic<int> _clientCount;
	// recv调用计数
	std::atomic<int> _recvCount;

private:
	// 线程
	CELLThread _thread;
	//
	SOCKET _lfd;
	//消息处理对象，内部会创建线程
	std::vector<CellServer*>   _cellServers;
	// 高精度计时器
	CELLTimestamp _tTime;	
};

#endif // !_EasyTcpServer_hpp
