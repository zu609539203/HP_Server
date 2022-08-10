#include"EasyTcpServer.hpp"
#include"MessageHeader.hpp"
#include<thread>

bool g_bRun = true;

/* 线程用于接收退出命令
void cmdTread() {
	while (1) {
		char cmdBuf[256] = { };
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit")) {
			g_bRun = false;
			CELLLog::Info("退出cmdTread线程...\n");
			break;
		}
		else {
			CELLLog::Info("不被支持的命令...\n");
		}
	}
}
*/

class MyServer : public EasyTcpServer 
{
public:
	//只会被一个线程触发 安全
	virtual void OnNetJoin(CellClient* pClient) {
		EasyTcpServer::OnNetJoin(pClient);
	}

	//cellServer 4 多个线程触发 不安全
	//如果只开启1个cellServer就是安全的
	virtual void OnNetLeave(CellClient* pClient) {
		EasyTcpServer::OnNetLeave(pClient);
	}

	//cellServer 4 多个线程触发 不安全
	//如果只开启1个cellServer就是安全的
	virtual void OnNetMsg(CellClient* pClient, NetMsg_DataHeader* header, CellServer* pCellServer) {
		EasyTcpServer::OnNetMsg(pClient, header, pCellServer);

		// 处理请求
		switch (header->cmd) {
		case CMD_NetMsg_Login: {
			pClient->resetDTHeart();
			NetMsg_Login* NetMsgLogin = (NetMsg_Login*)header;
			//CELLLog::Info("收到客户端<%d>命令：CMD_NetMsgLogin，数据长度：%d，用户名：%s\n", cfd, NetMsgLogin->dataLength, NetMsgLogin->userName);
			// 忽略判断用户名密码是否正确
			NetMsg_LoginR ret;
			if (SOCKET_ERROR == pClient->SendData(&ret))
			{
				// 发送缓冲区已满，没能发送成功
				CELLLog::Info("<SOCKET = %d>Send Full\n", pClient->sockfd());
			}
			//NetMsg_LoginR* ret = new NetMsg_LoginR();
			//pCellServer->addSendTask(pClient, ret);
		}
		break;
		case CMD_NetMsg_Logout: {
			NetMsg_Logout* NetMsgLogout = (NetMsg_Logout*)header;
			//CELLLog::Info("收到客户端<%d>命令：CMD_NetMsg_Logout，数据长度：%d，用户名：%s\n", cfd, NetMsg_Logout->dataLength, NetMsg_Logout->userName);
			// 忽略判断用户名密码是否正确
			//NetMsg_LogoutR ret;
			//SendData(&ret, cfd);
		}
		break;
		case CMD_HEART_C2S: {
			pClient->resetDTHeart();
			NetMsg_Heart_S2C ret;
			pClient->SendData(&ret);
		}

		default:
			CELLLog::Info("收到未定义消息, 数据长度为：%d\n", header->dataLength);
			break;
		}
	}

};

int main() {
	CELLLog::Instance().setLogPath("serverLog.txt", "w");
	MyServer server;
	server.InitSocket();
	server.Bind("127.0.0.1", 9999);
	server.Listen(10);
	server.Start(4);	// 选择 N 线程启动

	/*	
	//线程 thread
	std::thread t1(cmdTread);
	t1.detach();	// 线程分离
	*/

	while (true) {
		char cmdBuf[256] = { };
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit")) 
		{
			//server.Close();
			CELLLog::Info("退出cmdTread线程...\n");
			break;
		}
		else {
			CELLLog::Info("不被支持的命令...\n");
		}
	}

	server.Close();
	CELLLog::Info("任务完成...\n");

	system("pause");
	return 0;
}