/*
	v1.0
*/
#ifndef _CELL_TASK_H_
#define _CELL_TASK_H_

#include<thread>
#include<mutex>
#include<list>
#include<functional>

#include"CELLThread.hpp"
//#include"CELLLog.hpp"

/*
// 任务类型 - 基类
class CellTask
{
public:
	CellTask()
	{

	}

	virtual ~CellTask()
	{

	}

	// 执行任务
	virtual void doTask()
	{

	}

private:

};
*/

// 执行任务的服务类型
class CellTaskServer
{
public:
	// 所属server id
	int serverID = -1;

private:
	typedef std::function<void()> CellTask;

public:
	// 添加任务
	void addTask(CellTask task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_taskBuf.push_back(task);

	}

	// 启动服务
	void Start()
	{
		_thread.Start(
			nullptr, 
			[this](CELLThread* pThread){ OnRun(pThread);} );
	}

	// 关闭服务
	void Close()
	{
		//CELLLog::Info("CellTaskServer<serverID = %d> Close begin\n", serverID);
		_thread.Close();
		//CELLLog::Info("CellTaskServer<serverID = %d> Close end\n", serverID);
	}

protected:
	// 工作函数
	void OnRun(CELLThread* pThread)
	{
		while (pThread->isRun())
		{
			// 从缓冲区取数据
			if (!_taskBuf.empty())
			{
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pTask : _taskBuf)
				{
					_tasks.push_back(pTask);
				}
				_taskBuf.clear();
			}
			// 若没有任务
			if(_tasks.empty())
			{
				// 休眠一毫秒
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			// 处理任务
			for (auto pTask : _tasks)
			{
				pTask();
			}
			// 清空任务
			_tasks.clear();
		}
		// 处理缓冲队列中的任务
		for (auto pTask : _taskBuf)
		{
			pTask();
		}
		//CELLLog::Info("CellTaskServer OnRun <serverID = %d> Close\n", serverID);
	}

private:
	// 任务数据
	std::list<CellTask> _tasks;
	// 任务数据缓冲区
	std::list<CellTask> _taskBuf;
	// 改变数据缓冲区时需要加锁
	std::mutex _mutex;
	// 
	CELLThread _thread;
	//
	bool _isRun = false;
};

#endif // !_CELL_TASK_H_
