#ifndef _CELL_THREAD_HPP_
#define _CELL_THREAD_HPP_

#include"CELLSemaphore.hpp"

class CELLThread
{
private:
	typedef std::function<void(CELLThread*)> EventCall;

public:
	// 启动线程
	void Start(EventCall OnCreate = NULL, EventCall OnRun = NULL, EventCall  OnDestory = NULL)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (!_isRun)
		{
			_isRun = true;

			// 传事件
			if (OnCreate)
				_OnCreate = OnCreate;
			if (OnRun)
				_OnRun = OnRun;
			if (OnDestory)
				_OnDestory = OnDestory;

			// 线程
			std::thread t(std::mem_fn(&CELLThread::OnWork), this);
			t.detach();
		}
	}

	// 关闭线程
	void Close()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_isRun)
		{
			_isRun = false;
			_sem.waitForCall();
		}
	}

	// 在工作函数中退出，不需要使用信号量，如果使用则会阻塞
	void Exit()
	{
		std::lock_guard<std::mutex> lock(_mutex);
		if (_isRun)
		{
			_isRun = false;
		}
	}

	// 线程是否处于运行状态
	bool isRun()
	{
		return _isRun;
	}

protected:
	// 线程运行时的工作函数
	void OnWork()
	{
		if (_OnCreate)
			_OnCreate(this);		
		if (_OnRun)
			_OnRun(this);
		if (_OnDestory)
			_OnDestory(this);

		_sem.wakeUp();
	}

private:
	EventCall _OnCreate;
	EventCall _OnRun;
	EventCall _OnDestory;
	// 不同线程改变数据时需要加锁
	std::mutex _mutex;
	// 信号量控制线程的退出
	CELLSemaphore _sem;
	// 线程是否处于运行状态
	bool _isRun = false;
};


#endif // !_CELL_THREAD_HPP_
