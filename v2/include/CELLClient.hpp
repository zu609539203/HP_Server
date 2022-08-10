#ifndef _CellClient_hpp_
#define _CellClient_hpp_

#include"CELL.hpp"
#include"CELLBuffer.hpp"

// 客户端死亡倒计时 60,000 ms
#define CLIENT_HEART_DEAD_TIME 60000
// 计时一段时间后，将缓冲区中剩余未填满的待发送数据全部发出去
#define CLIENT_SEND_BUFF_TIME 200

// 客户端的数据类型
class CellClient
{
public:
	// 编号 测试用
	int id = -1;
	int serverID = -1;
public:
	CellClient(SOCKET sockfd = INVALID_SOCKET) :
		_sendBuff(SEND_BUFF_SIZE),	// 构造中 类的初始化
		_recvBuff(RECV_BUFF_SIZE)
	{
		static int n = 1;
		id = n++;

		_cfd = sockfd;

		resetDTHeart();
		resetDTSend();
	}

	~CellClient()
	{
		CELLLog::Info("<s = %d> CellClient <ID = %d> Close\n", serverID, id);

		if (_cfd != INVALID_SOCKET)
		{

#ifdef _WIN32
			closesocket(_cfd);
#else
			close(_cfd);
#endif
			_cfd = INVALID_SOCKET;
		}

	}

	SOCKET sockfd() {
		return _cfd;
	}

	// 触发发送计时，立即"清空"发送缓冲区
	int sendDataReal() 
	{
		// 重置发送缓冲区计时器
		resetDTSend();
		return _sendBuff.write2socket(_cfd);
	}

	// 缓冲区的控制根据业务需求的差异而调整
	// 定量发送数据
	int SendData(NetMsg_DataHeader* header) 
	{
		// 要发送数据的长度
		int nSendLen = header->dataLength;
		// 要发送的数据
		const char* pSendData = (const char*)header;

		if (_sendBuff.push(pSendData, nSendLen))
		{
			return nSendLen;
		}
		
		return SOCKET_ERROR;
	}

	int RecvData()
	{
		return _recvBuff.read4socket(_cfd);
	}

	bool hasMsg()
	{
		return _recvBuff.hasMsg();
	}

	NetMsg_DataHeader* front_msg()
	{
		return (NetMsg_DataHeader*)_recvBuff.data();
	}

	void pop_front_msg()
	{
		if( hasMsg() )
			_recvBuff.pop(front_msg()->dataLength);
	}

	void resetDTHeart()
	{
		_dtHeart = 0;
	}

	void resetDTSend()
	{
		_dtSend = 0;
	}

	// 心跳检测
	bool checkHeart(time_t dt)
	{	
		_dtHeart += dt;
		if (_dtHeart >= CLIENT_HEART_DEAD_TIME)
		{
			CELLLog::Info("checkHeart dead : s<%d>, _dtHeart<%d>\n", _cfd, _dtHeart);
			return true;
		}
		return false;
	}

	// 定时发送消息检测
	bool checkSend(time_t dt)
	{
		_dtSend += dt;
		if (_dtSend >= CLIENT_SEND_BUFF_TIME)
		{
			// 时间到! 立即将发送缓冲区中的数据发送出去
			int ret = sendDataReal();
			CELLLog::Info("checkSend : s<%d>, _dtSend<%d>\n", _cfd, _dtSend);
			// 重置发送计时
			resetDTSend();
			return true;
		}
		return false;
	}

private:
	// socket fd_set file desc set
	SOCKET _cfd;
	// 接收消息缓冲区
	CELLBuffer _recvBuff;
	// 发送消息缓冲区
	CELLBuffer _sendBuff;
	// 心跳死亡计时
	time_t _dtHeart;
	// 上次发送消息的时间
	time_t _dtSend;
	// 发送缓冲区遇到写满计数
	int _sendBuffFullCount = 0;
};

#endif // !_CellClient_hpp_


