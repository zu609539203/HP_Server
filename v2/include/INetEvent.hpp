#ifndef _I_NET_EVENT_HPP
#define _I_NET_EVENT_HPP

#include"CELL.hpp"
#include"CELLClient.hpp"

class CellServer;

// 网络事件接口
class INetEvent {
public:
	// 纯虚函数
	// 检测客户端加入
	virtual void OnNetJoin(CellClient* pClient) = 0;
	// 检测客户端退出
	virtual void OnNetLeave(CellClient* pClient) = 0;
	// 客户端消息事件
	virtual void OnNetMsg(CellClient* pClient, NetMsg_DataHeader* header, CellServer* pCellServer) = 0;
	// recv事件
	virtual void OnNetRecv(CellClient* pClient) = 0;
};

#endif // !_I_NET_EVENT_HPP
