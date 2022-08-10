#include"EasyTcpClient.hpp"
#include"MessageHeader.hpp"
#include"CELLTimestamp.hpp"
#include<thread>
#include<atomic>

bool g_bRun = true;

void cmdThread() {
	while (1) {
		char cmdBuf[256] = { };
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit")) {
			g_bRun = false;
			printf("退出...\n");
			break;
		}
		else {
			printf("不被支持的命令...\n");
		}
	}
}

#define clientCount 10000	// 客户端连接数量，总共1000个客户端连接
#define threadCount 4	// 线程数量，1000个连接分为4个线程，每个线程承担250个连接

EasyTcpClient* client[clientCount];	// 客户端数组，放在外面是作为共享资源，被多线程任务访问

std::atomic_int sendCount = 0;	   // 客户端发送消息次数
std::atomic_int readyCount = 0;	   // 已准备好的客户端线程计数



void recvThread(int begin, int end)
{
	CELLTimestamp t;

	// 接收数据
	while (g_bRun) 
	{
		for (int i = begin; i < end; i++)
		{
			if (t.getElapsedSecond() > 3.0 && i == begin)
				continue;
			client[i]->OnRun();
		}
	}
}

void sendThread(int id) {
	printf("thread<%d>,start\n", id);
	// 总共4个线程 ID ： 1 ~ 4  
	int cUnit = clientCount / threadCount;
	int begin = (id - 1) * cUnit;
	int end = id * cUnit;

	for (int i = begin; i < end; i++) {
		if (g_bRun) {
			client[i] = new EasyTcpClient();
		}
	}
	
	for (int i = begin; i < end; i++) {
		if (g_bRun) {
			client[i]->InitSocket();
			// 阿里云服务器IP ： 121.40.202.115
			client[i]->Connect("127.0.0.1", 9999);
		}
	}	

	// 心跳检测 死亡计时

	printf("thread<%d>,Connect<begin=%d, end=%d>\n", id, begin, end);
	
	readyCount++;
	while (readyCount < threadCount)
	{
		// 等待其他线程准备就绪再进行发送数据
		std::chrono::milliseconds t(100);
		std::this_thread::sleep_for(t);
	}

	// 创建接受线程
	std::thread t1(recvThread, begin, end);
	t1.detach();	// 线程分离

	NetMsg_Login login[10];
	for (int i = 0; i < 10; i++) {
		strcpy(login[i].userName, "lyd");
		strcpy(login[i].passWord, "lydmm");
	}

	const int nLen = sizeof(login);

	// 发送数据
	while (g_bRun) {
		for (int i = begin; i < end; i++)
		{
			if (SOCKET_ERROR != client[i]->SendData(login, nLen))
			{
				sendCount++;
			}
		}
		// 发送时增加延迟
		std::chrono::milliseconds t(10);
		std::this_thread::sleep_for(t);
	}

	for (int i = begin; i < end; i++) {
		client[i]->Close();
		delete client[i];
	}

	printf("thread<%d> 退出...\n", id);
}


int main() {
	// 创建 exit 线程，用于退出程序
	std::thread t1(cmdThread);
	t1.detach();	// 线程分离

	// 创建发送线程
	for (int i = 0; i < threadCount; i++) {
		std::thread t1(sendThread, i + 1);
		t1.detach();	// 线程分离
	}

	CELLTimestamp tTime;

	while (g_bRun) {
		auto t = tTime.getElapsedSecond();
		if (t >= 1.0)
		{
			printf("time<%lf>, thread<%d>, clients<%d>, send<%d>\n", t, threadCount, clientCount, (int)(sendCount / t));
			//std::cout << " send = " << (int)(sendCount / t) << std::endl;	// atomic对象不能使用printf输出，会报错
			sendCount = 0;
			tTime.update();
		}
		Sleep(1);
	}

	printf("已退出。\n");
	return 0;
}