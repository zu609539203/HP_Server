#ifndef _CELL_BUFFER_HPP_
#define _CELL_BUFFER_HPP_

#include"CELL.hpp"

class CELLBuffer
{
public:
	CELLBuffer(int nSize = 1024 * 8)
	{
		_nSize = nSize;
		_pBuff = new char[_nSize];
	}

	~CELLBuffer()
	{
		if (_pBuff)
		{
			delete[] _pBuff;
			_pBuff = nullptr;
		}
	}

	// 向缓冲区内填充数据
	bool push(const char* pData, int nLen)
	{
		/*
		if (_nLastPos + nLen > _nSize)
		{	// 写入大量数据不一定存在内存中，也可以存在数据库或是磁盘等存储器中
			// 需要写入的数据大于可用空间
			int extra = (_nLastPos + nLen) - _nSize;
			if (extra < 8192)
				extra = 8192;
			// 拓展 Buff 空间
			char* buff = new char[_nSize + extra];
			memcpy(buff, _pBuff, _nLastPos);
			delete[] buff;
			_pBuff = buff;
			_nSize += extra;
		}
		*/

		// 异步发送：发送缓冲区未满时，继续向缓冲区添加数据
		if (_nLastPos + nLen <= _nSize)
		{
			// 将要发送的数据 拷贝到发送缓冲区的尾部
			memcpy(_pBuff + _nLastPos, pData, nLen);
			// 发送缓冲区的尾部偏移
			_nLastPos += nLen;
			// 发送缓冲区满
			if (_nLastPos == _nSize)
			{
				_BuffFullCount++;
			}

			return true;
		}
		// 发送缓冲区满
		else
		{
			_BuffFullCount++;
		}
		return false;
	}

	// 发送缓冲区内的数据
	int write2socket(SOCKET socketfd)
	{
		int ret = 0;
		// 缓冲区存在数据
		if (_nLastPos > 0)
		{
			ret = send(socketfd, _pBuff, _nLastPos, 0);
			// 初始化偏移量
			_nLastPos = 0;
			// 重置发送缓冲区已满计数
			_BuffFullCount = 0;
		}
		return ret;
	}
	// 发送 
///---------------------------------------------------------///
	// 接收
	int read4socket(SOCKET socketfd)
	{
		// 缓冲区未满
		if (_nSize - _nLastPos)
		{
			// 接受客户端数据
			char* szRecv = _pBuff + _nLastPos;
			int len = recv(socketfd, szRecv, _nSize - _nLastPos, 0);
			// 客户端退出
			if (len <= 0) 
			{
				return len;
			}
			// 缓冲区数据尾部后移
			_nLastPos += len;
			return len;
		}
		return 0;
	}

	// 缓冲区是否有数据
	bool hasMsg()
	{
		// 判断第二缓冲区的数据长度是否大于消息头NetMsg_DataHeader长度
		if (_nLastPos >= sizeof(NetMsg_DataHeader)) {
			// 知道消息体（header + body）长度
			NetMsg_DataHeader* header = (NetMsg_DataHeader*)_pBuff;
			// 判断当前消息长度是否大于整个消息长度
			return _nLastPos >= header->dataLength;
		}
		return false;
	}

	void pop(int nLen)
	{
		// 防御判断：传入值须小于当前缓冲区尾指针的指向
		if (_nLastPos - nLen > 0)
		{
			memcpy(_pBuff, _pBuff + nLen, _nLastPos - nLen);
		}
		// 
		_nLastPos = _nLastPos - nLen;
		// 缓冲区有空间了，可以继续填充数据了
		if (_BuffFullCount > 0)
		{
			--_BuffFullCount;
		}
	}

	void* data()
	{
		return _pBuff;
	}

private:
	// 发送缓冲区
	char* _pBuff = nullptr;
	// 发送缓冲区尾部指针
	int _nLastPos = 0;
	// 缓冲区总的空间大小，字节长度
	int _nSize = 0;
	// 缓冲区写满计数
	int _BuffFullCount = 0;
};

#endif // !_CELL_BUFFER_HPP_
