#ifndef _CELL_SERVER_HPP_
#define _CELL_SERVER_HPP_

#include"CELL.hpp"
#include"INetEvent.hpp"
#include"CELLClient.hpp"
#include"CELLSemaphore.hpp"
#include"CELLThread.hpp"

#include<map>
#include<vector>

// 消费者线程 : 网络消息接受服务类
class CellServer
{
public:
	CellServer(int id) {
		_id = id;
		_pNetEvent = nullptr;
		_taskServer.serverID = id;
	}

	~CellServer() {
		Close();
	}

	void Start()
	{
		_thread.Start(
			nullptr,
			[this](CELLThread* pthread) {   OnRun(pthread); },
			[this](CELLThread* pthread) { 	ClearClients(); } );
		_taskServer.Start();
	}

	// 关闭Socket
	void Close() {
		CELLLog::Info("CellServer<%d> Close begin\n", _id);
		//
		_taskServer.Close();
		_thread.Close();
		//
		CELLLog::Info("CellServer<%d> Close end\n", _id);
	}

	void setNetEventObj(INetEvent* event) {
		_pNetEvent = event;
	}

	// 处理网络消息
	void OnRun(CELLThread* pthread) {
		while (pthread->isRun()) {
			if (_clientsBuff.size() > 0) {
				// 从缓冲队列中取出客户数据
				std::lock_guard<std::mutex> lock(_mutex);	// 自解锁
				for (auto pClient : _clientsBuff) 
				{
					pClient->serverID = _id;
					_clients[pClient->sockfd()] = pClient;
				}
				_clientsBuff.clear();
				_clients_change = true;
			}

			// 如果没有需要处理的客户端，跳过
			if (_clients.empty()) {
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				// 旧的时间戳
				_oldTime = CELLTime::getNowTimeInMilliSec();
				continue;
			}

			fd_set fdRead;
			fd_set fdWrite;
			//fd_set fdEsc;

			FD_ZERO(&fdRead);

			if (_clients_change) {
				_clients_change = false;
				// 将客户端（socket）加入集合

				_maxSock = _clients.begin()->second->sockfd();

				// 将客户端的socket复制到内核

				for (auto iter : _clients) {
					FD_SET(iter.second->sockfd(), &fdRead);
					if (_maxSock < iter.second->sockfd()) {
						_maxSock = iter.second->sockfd();
					}
				}

				memcpy(&_fdRead_back, &fdRead, sizeof(fdRead));
			}
			else {
				// 若没有新消息到来，则直接复制上次的 fd_set
				memcpy(&fdRead, &_fdRead_back, sizeof(_fdRead_back));
			}

			memcpy(&fdWrite, &_fdRead_back, sizeof(fdWrite));
			//memcpy(&fdEsc, &_fdRead_back, sizeof(fdEsc));

			// 具体参数含义参见 IO多路复用 , nfds的设置在Windows中可以随意设置，但实际含义表示最大文件描述符的数目加一
			timeval t = { 0, 1 };
			int ret = select(_maxSock + 1, &fdRead, &fdWrite, nullptr, &t);	// select设置为阻塞

			if (ret < 0) 
			{
				CELLLog::Info("CELLServer.OnRun select error!\n");
				pthread->Exit();
				break;
			}

			ReadData(fdRead);
			WriteData(fdWrite);
			// WriteData(fdEsc);
			CheckTime();
			//std::cout << "空闲时间处理其他业务..." << std::endl;
		}
		CELLLog::Info("CellServer OnRun<%d> Close\n", _id);
	}

	// 旧的时间戳
	time_t _oldTime = CELLTime::getNowTimeInMilliSec();
	void CheckTime()
	{
		// 当前时间戳 
		auto nowTime = CELLTime::getNowTimeInMilliSec();
		auto dt = nowTime - _oldTime;
		_oldTime = nowTime;

		for (auto iter = _clients.begin(); iter != _clients.end(); iter++)
		{
			// 心跳检测
			if (iter->second->checkHeart(dt))
			{
				if (_pNetEvent)
					_pNetEvent->OnNetLeave(iter->second);
				_clients_change = true;
				delete iter->second;
				_clients.erase(iter);
			}

			//// 定时发送检测
			//iter->second->checkSend(dt);
		}
	}

	void ReadData(fd_set& fdRead)
	{
#ifdef _WIN32
		// 检查哪些 socket 的监听值发生了变化
		for (int n = 0; n < fdRead.fd_count; n++) {
			auto iter = _clients.find(fdRead.fd_array[n]);
			if (iter != _clients.end())
			{
				// 有客户端退出
				if (-1 == RecvData(iter->second)) {
					OnClientLeave(iter->second);
					_clients.erase(iter);
				}
			}
			else
			{
				CELLLog::Info("error. iter != _clients.end() \n");
			}
		}
#else
		std::vector<CellClient*>	temp;
		for (auto iter : _clients)
		{
			if (FD_ISSET(iter.second->sockfd(), &fdRead))
			{
				if (-1 == RecvData(iter.second))
				{
					OnClientLeave(iter.second);
					close(iter->first);
					temp.push_back(iter.second);
				}
			}
		}
		for (auto pClient : temp)
		{
			_clients.erase(pClient->sockfd());
			delete pClient;
		}
#endif
	}

	void WriteData(fd_set& fdWrite)
	{
#ifdef _WIN32
		// 检查哪些 socket 的监听值发生了变化
		for (int n = 0; n < fdWrite.fd_count; n++) {
			auto iter = _clients.find(fdWrite.fd_array[n]);
			if (iter != _clients.end())
			{
				// 有客户端退出
				if (-1 == iter->second->sendDataReal()) 
				{
					OnClientLeave(iter->second);
					_clients.erase(iter);
				}
			}
		}
#else
		std::vector<CellClient*>	temp;
		for (auto iter : _clients)
		{
			if (FD_ISSET(iter.second->sockfd(), &fdWrite))
			{
				if (-1 == iter->second->sendDataReal())
				{
					OnClientLeave(iter.second);
					close(iter->first);
					temp.push_back(iter.second);
				}
			}
		}
		for (auto pClient : temp)
		{
			_clients.erase(pClient->sockfd());
			delete pClient;
		}
#endif
	}

	// 接收缓冲区(第一缓冲区)
	char _szRecv[RECV_BUFF_SIZE] = { };

	// 接收数据 处理粘包
	int RecvData(CellClient* pClient) 
	{
		// 接受客户端数据
		int len = pClient->RecvData();
		if (len <= 0) {
			//CELLLog::Info("len = %d...客户端<%d>已退出...\n", len, pClient->sockfd());
			return -1;
		}
		// 触发网络事件：接收消息计数
		_pNetEvent->OnNetRecv(pClient);

		// 循环 判断是否有消息需要处理
		while (pClient->hasMsg()) 
		{
			// 处理网络消息
			OnNetMsg(pClient, pClient->front_msg(), this);
			// “消息缓冲区”尾部数据指针前移
			pClient->pop_front_msg();
		}
		return 0;
	}

	//
	void OnClientLeave(CellClient* pClient)
	{
		if (_pNetEvent)
			_pNetEvent->OnNetLeave(pClient);
		_clients_change = true;
		delete pClient;
	}

	// 响应网络消息
	virtual void OnNetMsg(CellClient* pClient, NetMsg_DataHeader* header, CellServer* pCellServer) {
		_pNetEvent->OnNetMsg(pClient, header, this);
	}

	// 把消息队列的客户端连接加入进来
	void addClients(CellClient* pClient) {
		_mutex.lock();
		_clientsBuff.push_back(pClient);
		_mutex.unlock();
	}

	// 获取客户端连接数量
	int getClientCount() {
		return _clients.size() + _clientsBuff.size();
	}

	/*
		void addSendTask(CellClient* pClient, NetMsg_DataHeader* header)
		{
			//CellSendMsg2ClientTask* task = new CellSendMsg2ClientTask(pClient, header);
			_taskServer.addTask([pClient, header](){
			pClient->SendData(header);
			delete header;
			});
		}
	*/

private:
	// 清理接收客户端的缓冲区
	void ClearClients()
	{
		for (auto iter : _clients)
		{
			delete iter.second;
		}
		_clients.clear();

		for (auto iter : _clientsBuff)
		{
			delete iter;
		}
		_clientsBuff.clear();
	}

private:
	// 正式客户队列
	std::map<SOCKET, CellClient*> _clients;
	// 缓冲客户队列
	std::vector<CellClient*> _clientsBuff;
	// 缓冲队列的锁
	std::mutex _mutex;
	// 网络事件对象
	INetEvent* _pNetEvent;
	// 
	CellTaskServer _taskServer;
	// 线程
	CELLThread _thread;
	// 备份客户socket fd_set
	fd_set _fdRead_back;
	// fd_set 中最大文件描述符的数
	SOCKET _maxSock;
	// 线程编号
	int _id = -1;
	// 客户列表是否有变化
	bool _clients_change = true;

};

#endif // !_CELL_SERVER_HPP_
