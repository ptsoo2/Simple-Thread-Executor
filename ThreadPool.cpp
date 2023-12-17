#include "ThreadPool.h"

namespace TSUtil
{
	stThreadWaitState_t::stThreadWaitState_t(time_t deadlineMilliSec)
		: deadlineMilliSec_(deadlineMilliSec)
	{
	}

	void stThreadWaitState_t::notify()
	{
		// isWait 이 켜져있는 상태라면 끄고, notify
		bool expected = true;
		if (isWait_.compare_exchange_strong(expected, false) == true)
		{
			cond_.notify_one();
		}
	}

	void stThreadWaitState_t::wait()
	{
		isWait_.store(true);

		{
			std::unique_lock grab(lock_);
			cond_.wait_for(grab, std::chrono::milliseconds(deadlineMilliSec_),
				[this]()
				{
					// isWait 이 꺼져있는 상태라면 탈출
					bool expected = false;
					return isWait_.compare_exchange_strong(expected, false) == true;
				}
			);
		}
	}
}

namespace TSUtil
{
	void CThreadPool::init(size_t size, fnRun_t&& fnRun)
	{
		if (isRun() == true)
			throw std::runtime_error("Already initialized");

		lstThread_.reserve(size);
		for (size_t tid = 0; tid < size; ++tid)
		{
			lstThread_.emplace_back(
				[tid, stopToken = stopSource_.get_token(), fnRun]()
				{
					while (stopToken.stop_requested() == false)
						fnRun(tid);
				}
			);
		}
	}

	void CThreadPool::stop()
	{
		if (isRun() == false)
			throw std::runtime_error("Not running");

		stopSource_.request_stop();
		for (std::thread& thread : lstThread_)
		{
			if (thread.joinable() == true)
				thread.join();
		}
	}

	bool CThreadPool::isRun() const
	{
		if (lstThread_.empty() == true)
			return false;

		return stopSource_.stop_requested() == false;
	}
}

namespace TSUtil
{
	CThreadExecutor::CRunnable::CRunnable(time_t deadlineMilliSec)
		: waitState_(deadlineMilliSec)
	{
	}

	void CThreadExecutor::CRunnable::post(fnTask_t&& task)
	{
		queTask_.emplace(std::forward<fnTask_t>(task));
		waitState_.notify();
	}

	void CThreadExecutor::CRunnable::flush()
	{
		queTask_.flush(
			[](auto& element)
			{
				element();
			}
		);
	}

	void CThreadExecutor::CRunnable::run()
	{
		// swap 해서 처리할 필요가 없다면 wait
		if (queTask_.swap() == false)
		{
			waitState_.wait();
			return;
		}

		auto readQueue = queTask_.getReadQueue();
		while (readQueue->empty() == false)
		{
			readQueue->front()();
			readQueue->pop();
		}
	}

	CThreadExecutor::~CThreadExecutor()
	{
#pragma warning(push)
#pragma warning(disable: 4297)
		if (threadPool_.isRun() == true)
			throw std::runtime_error("Not stopped");
#pragma warning(pop)
	}

	void CThreadExecutor::init(size_t threadCount, time_t deadlineMilliSec)
	{
		if (threadPool_.isRun() == true)
			throw std::runtime_error("Already initialized");

		// runnable 생성
		for (size_t i = 0; i < threadCount; ++i)
			lstRunnable_.emplace_back(deadlineMilliSec);

		// thread pool 생성
		threadPool_.init(threadCount,
			[this](size_t tid)
			{
				CRunnable& executor = lstRunnable_.at(tid);
				executor.run();
			}
		);
	}

	void CThreadExecutor::stop()
	{
		threadPool_.stop();
	}

	void CThreadExecutor::flush()
	{
		for (CRunnable& runnable : lstRunnable_)
			runnable.flush();
	}

	void CThreadExecutor::postAt(size_t tid, fnTask_t&& task)
	{
		if (threadPool_.isRun() == false)
			throw std::runtime_error("Not running");

		tid %= lstRunnable_.size();
		lstRunnable_.at(tid).post(std::forward<fnTask_t>(task));
	}

	void CThreadExecutor::post(fnTask_t&& task)
	{
		if (threadPool_.isRun() == false)
			throw std::runtime_error("Not running");

		// round robin 으로 runnable 을 선택해서 post
		postAt(_gentid(), std::forward<fnTask_t>(task));
	}

	size_t CThreadExecutor::_gentid()
	{
		// round robin
		const size_t tid = roundrobin_++ % lstRunnable_.size();
		return tid;
	}
}
