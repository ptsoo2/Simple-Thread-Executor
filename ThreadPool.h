#pragma once

#include "MPSCQueue.h"

#include <vector>
#include <functional>

#include <thread>
#include <mutex>

namespace TSUtil
{
	struct stThreadWaitState_t
	{
		stThreadWaitState_t(time_t deadlineMilliSec);

		// 일부 lost wakeup 발생할 수 있음
		void						notify();
		void						wait();

	protected:
		time_t						deadlineMilliSec_ = 0;

		std::atomic_bool			isWait_ = false;
		std::mutex					lock_;
		std::condition_variable		cond_;
	};
}

namespace TSUtil
{
	class CThreadPool
	{
		using lstThread_t = std::vector<std::thread>;
		using fnRun_t = std::function<void(size_t tid)>;

	public:
		CThreadPool() = default;
		~CThreadPool() = default;

	public:
		void				init(size_t size, fnRun_t&& fnRun);
		void				stop();

		bool				isRun() const;

	protected:
		std::stop_source	stopSource_;
		lstThread_t			lstThread_;
	};
}

namespace TSUtil
{
	class CThreadExecutor
	{
	public:
		using fnTask_t = std::function<void()>;

	protected:
		struct CRunnable
		{
			using queTask_t = TSUtil::CMPSCQueue<fnTask_t>;

		public:
			CRunnable(time_t deadlineMilliSec);

		public:
			void					post(fnTask_t&& task);
			void					flush();
			void					run();

		public:
			queTask_t				queTask_;
			stThreadWaitState_t		waitState_;
		};
		using lstRunnable_t = std::deque<CRunnable>;
		using roundrobin_t = std::atomic_size_t;

	public:
		CThreadExecutor() = default;
		~CThreadExecutor();

	public:
		void				init(size_t threadCount, time_t deadlineMilliSec = 100);
		void				stop();
		void				flush();

		void				postAt(size_t tid, fnTask_t&& task);
		void				post(fnTask_t&& task);

	protected:
		size_t				_gentid();

	protected:
		CThreadPool			threadPool_;

		lstRunnable_t		lstRunnable_;
		roundrobin_t		roundrobin_ = 0;
	};
}
