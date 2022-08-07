#ifndef _EasyTcpServer_hpp
#define _EasyTcpServer_hpp

#ifdef _WIN32
#define FD_SETSIZE	1024	// 突破系统select只能限制64个客户端连接服务器的限制
#define WIN32_LEAN_AND_MEAN
#include<WS2tcpip.h>
#include<windows.h>
#include<winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>

#define SOCKET int;
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR		   (-1)
#endif // _WIN32

#include<iostream>
#include<vector>
#include<thread>
#include<mutex>
#include<atomic>

#include"MessageHeader.hpp"
#include"CELLTimestamp.hpp"

// 缓冲区大小单元 10Kb
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif // !RECV_BUFF_SIZE

// 客户端的数据类型
class ClientSocket
{
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET) {
		_cfd = sockfd;
		memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
		_lastPos = 0;
	}

	SOCKET sockfd() {
		return _cfd;
	}

	char* msgBuf() {
		return _szMsgBuf;
	}

	int getLastPos() {
		return _lastPos;
	}

	void setLastPos(int pos) {
		_lastPos = pos;
	}

	// 发送数据
	int SendData(DataHeader* header) {
		if (header) {
			return send(_cfd, (const char *)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}

private:
	// socket fd_set file desc set
	SOCKET _cfd;
	// 第二缓冲区 消息缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE * 10] = { };
	// 第二缓冲区尾部指针
	int _lastPos = 0;
};

// 网络事件接口
class INetEvent {
public:
	// 纯虚函数
	// 检测客户端加入
	virtual void OnNetJoin(ClientSocket* pClient) = 0;
	// 检测客户端退出
	virtual void OnNetLeave(ClientSocket* pClient) = 0;
	// 客户端消息事件
	virtual void OnNetMsg(DataHeader* header, ClientSocket* pClient) = 0;
};


// 消费者线程
class CellServer
{
public:
	CellServer(SOCKET sock = INVALID_SOCKET) {
		lfd = sock;
		_pThread = nullptr;
		_pNetEvent = nullptr;
	}

	~CellServer() {
		Close();
		lfd = INVALID_SOCKET;
	}

	void setNetEventObj(INetEvent* event) {
		_pNetEvent = event;
	}

	// 处理网络消息
	bool OnRun() {
		while (isRun()) {
			if (_clientsBuff.size() > 0) {
				// 从缓冲队列中取出客户数据
				std::lock_guard<std::mutex> lock(_mutex);	// 自解锁
				for (auto pClient : _clientsBuff) {
					_clients.push_back(pClient);
				}
				_clientsBuff.clear();
			}

			// 如果没有需要处理的客户端，跳过
			if (_clients.empty()) {
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}

			fd_set fdRead;

			FD_ZERO(&fdRead);

			// 将客户端（socket）加入集合
			SOCKET maxSock = _clients[0]->sockfd();

			// 将客户端的socket复制到内核
			for (int n = 0; n < _clients.size(); n++) {
				FD_SET(_clients[n]->sockfd(), &fdRead);
				if (maxSock < _clients[n]->sockfd()) {
					maxSock = _clients[n]->sockfd();
				}
			}

			// 具体参数含义参见 IO多路复用 , nfds的设置在Windows中可以随意设置，但实际含义表示最大文件描述符的数目加一
			int ret = select(maxSock + 1, &fdRead, 0, 0, 0);	// select设置为阻塞

			if (ret < 0) {
				printf("select error!\n");
				Close();
				return false;
			}

			for (int n = 0; n < _clients.size(); n++) {
				if (FD_ISSET(_clients[n]->sockfd(), &fdRead)) {
					if (-1 == RecvData(_clients[n])) {
						auto iter = _clients.begin() + n;
						if (iter != _clients.end()) {
							if (_pNetEvent)
								_pNetEvent->OnNetLeave(_clients[n]);
							delete _clients[n];
							_clients.erase(iter);
						}
					}
				}
			}
			//std::cout << "空闲时间处理其他业务..." << std::endl;
		}
	}

	void Start() {
		_pThread = new std::thread(std::mem_fn(&CellServer::OnRun), this);
	}

	// 判断服务端是否工作中
	bool isRun() {
		return lfd != INVALID_SOCKET;
	}

	// 关闭Socket
	void Close() {
		if (lfd != INVALID_SOCKET) {
#ifdef _WIN32
			for (auto n = 0; n < _clients.size(); n++) {
				closesocket(_clients[n]->sockfd());
				delete _clients[n];
			}
			closesocket(lfd);
			//WSACleanup();
#else
			for (auto n = 0; n < _clients.size(); n++) {
				close(_clients[n]->sockfd());
				delete _clients[n];
			}
			close(lfd);
#endif // _WIN32
			_clients.clear();
		}
	}

	// 接收缓冲区
	char szRecv[RECV_BUFF_SIZE] = { };

	// 接收数据 处理粘包
	int RecvData(ClientSocket* pClient) {

		// 接受客户端数据
		int len = recv(pClient->sockfd(), szRecv, RECV_BUFF_SIZE, 0);
		//printf("len = %d\n", len);
		if (len <= 0) {
			//printf("len = %d...客户端<%d>已退出...\n", len, pClient->sockfd());
			return -1;
		}

		// 将“接收缓冲区”数据拷贝到“第二缓冲区”
		memcpy(pClient->msgBuf() + pClient->getLastPos(), szRecv, len);
		// “第二缓冲区”数据尾部后移
		pClient->setLastPos(pClient->getLastPos() + len);
		// 判断第二缓冲区的数据长度是否大于消息头DataHeader长度
		while (pClient->getLastPos() >= sizeof(DataHeader)) {
			// 知道消息体（header + body）长度
			DataHeader* header = (DataHeader*)pClient->msgBuf();
			// 判断当前消息长度是否大于整个消息长度
			if (pClient->getLastPos() >= header->dataLength)	// 处理 粘包 情况
			{
				// 剩余未处理消息缓冲区的长度
				int nSize = pClient->getLastPos() - header->dataLength;
				// 处理网络消息
				OnNetMsg(header, pClient);
				// 将“第二缓冲区”未处理数据前移
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
				// “第二缓冲区”尾部数据指针前移
				pClient->setLastPos(nSize);
			}
			else	// 处理 少包 情况
			{
				// “第二缓冲区”剩余数据不足够一条完整消息
				break;
			}
		}
		return 0;
	}

	// 响应网络消息
	virtual void OnNetMsg(DataHeader* header, ClientSocket* pClient) {
		_pNetEvent->OnNetMsg(header, pClient);
	}

	// 把消息队列的客户端连接加入进来
	void addClients(ClientSocket* pClient) {
		_mutex.lock();
		_clientsBuff.push_back(pClient);
		_mutex.unlock();
	}

	// 获取客户端连接数量
	int getClientCount() {
		return _clients.size() + _clientsBuff.size();
	}


private:
	// 正式客户队列
	std::vector<ClientSocket*> _clients;
	// 缓冲客户队列
	std::vector<ClientSocket*> _clientsBuff;
	// 缓冲队列的锁
	std::mutex _mutex;
	std::thread* _pThread;
	// 网络事件对象
	INetEvent* _pNetEvent;
	SOCKET lfd;
};

// 
class EasyTcpServer : public INetEvent
{
public:
	EasyTcpServer() {
		lfd = INVALID_SOCKET;
		_recvCount = 0;
		_clientCount = 0;
	}

	virtual ~EasyTcpServer() {
		Close();
	}

	// 初始化Socket
	SOCKET InitSocket() {
#ifdef _WIN32
		// 启动Win Socket 2.x环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		WSAStartup(ver, &dat);
#endif
		// 创建socket
		if (lfd != INVALID_SOCKET) {
			printf("<socket = %d>关闭先前的连接...\n", lfd);
			Close();
		}
		lfd = socket(AF_INET, SOCK_STREAM, 0);
		if (lfd == INVALID_SOCKET) {
			printf("建立socket失败...\n");
		}
		else {
			printf("建立socket成功...\n");
		}
		return lfd;
	}

	// 绑定IP 和 端口号
	int Bind(const char * ip, unsigned short port) {
		if (lfd == INVALID_SOCKET) {
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
		int ret = bind(lfd, (sockaddr *)&saddr, sizeof(saddr));
		if (SOCKET_ERROR == ret) {
			printf("绑定失败<Port:%d>...\n", port);
		}
		else {
			printf("绑定成功<Port:%d>...\n", port);
		}
		return ret;
	}

	// 监听端口号
	int Listen(int backlog = 5) {
		int ret = listen(lfd, backlog);
		if (SOCKET_ERROR == ret) {
			printf("<Socket:%d>监听失败...\n", lfd);
		}
		else {
			printf("<Socket:%d>监听成功...\n", lfd);
		}
		return ret;
	}

	// 接受客户端连接
	SOCKET Accept() {
		sockaddr_in caddr;
		int c_len = sizeof(caddr);
		SOCKET cfd = INVALID_SOCKET;
#ifdef _WIN32
		cfd = accept(lfd, (sockaddr *)&caddr, &c_len);
#else
		cfd = accept(lfd, (sockaddr *)&caddr, &c_len);
#endif
		if (cfd == INVALID_SOCKET) {
			printf("<Socket:%d>错误，连接到无效客户端Socket...\n", lfd);
		}
		else {
			// 获取IP地址： 
			// char cli_ip[15];
			// inet_ntop(AF_INET, &caddr.sin_addr, cli_ip, sizeof(cli_ip));

			// 把新客户端分配给客户数量最少的 cellServer
			addClient2CellServer(new ClientSocket(cfd));
		}
		return cfd;
	}

	void addClient2CellServer(ClientSocket* pClient) {
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
			auto ser = new CellServer(lfd);
			_cellServers.push_back(ser);
			// 注册网络事件接受对象
			ser->setNetEventObj(this);
			// 启动消息处理线程
			ser->Start();
		}
	}

	// 关闭Socket
	void Close() {
		if (lfd != INVALID_SOCKET) {
#ifdef _WIN32
			// 关闭socket lfd
			closesocket(lfd);
			// 清除Windows socket环境
			WSACleanup();
#else
			// 关闭socket lfd
			close(lfd);
#endif // _WIN32
		}
	}

	// 处理网络消息
	bool OnRun() {
		if (isRun()) {
			time4Msg();

			fd_set fdRead;

			FD_ZERO(&fdRead);

			FD_SET(lfd, &fdRead);

			// 具体参数含义参见 IO多路复用 , nfds的设置在Windows中可以随意设置，但实际含义表示最大文件描述符的数目加一
			timeval t = { 0, 10 };
			int ret = select(lfd + 1, &fdRead, NULL, NULL, &t);
			//printf("ret = %d ... count = %d\n", ret, _count++);

			if (ret < 0) {
				printf("Accpet Select error!\n");
				Close();
				return false;
			}

			//判断描述符（socket）是否在集合中
			if (FD_ISSET(lfd, &fdRead)) {
				// 若是有客户端接入，则添加到 fdRead，然后将其置为 0
				FD_CLR(lfd, &fdRead);
				// accept 新的客户端加入
				Accept();
				return true;
			}
			return true;
			//std::cout << "空闲时间处理其他业务..." << std::endl;
		}
		return false;
	}

	// 判断服务端是否工作中
	bool isRun() {
		return lfd != INVALID_SOCKET;
	}

	// 接收缓冲区
	char szRecv[RECV_BUFF_SIZE] = { };


	// 响应网络消息
	void time4Msg() {
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0)
		{
			printf("thread<%d>, time<%lf>, socket<%d>, client_nums<%d>, recvCount<%d>\n", _cellServers.size(), t1, lfd, (int)_clientCount, (int)(_recvCount / t1));
			_recvCount = 0;
			_tTime.update();
		}
	}

	//只会被一个线程触发 安全
	virtual void OnNetJoin(ClientSocket* pClient) {
		_clientCount++;
	}

	//cellServer 4 多个线程触发 不安全
	//如果只开启1个cellServer就是安全的
	virtual void OnNetLeave(ClientSocket* pClient) {
		_clientCount--;
	}

	//cellServer 4 多个线程触发 不安全
	//如果只开启1个cellServer就是安全的
	virtual void OnNetMsg(DataHeader* header, ClientSocket* pClient) {
		_recvCount++;
	}

protected:
	// 收到消息计数
	std::atomic<int> _recvCount;
	// 客户端计数
	std::atomic<int> _clientCount;

private:
	std::vector<ClientSocket*> _clients;
	//消息处理对象，内部会创建线程
	std::vector<CellServer*>   _cellServers;
	SOCKET lfd;
	CELLTimestamp _tTime;	// 高精度计时器
};

#endif // !_EasyTcpServer_hpp
