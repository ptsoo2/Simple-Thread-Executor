#include "ThreadPool.h"

#include <latch>
#include <thread>

namespace TSUtil
{
	using threadId_t = uint32_t;
	inline static threadId_t get_tid()
	{
#ifdef __linux__
		return (threadId_t)syscall(SYS_gettid);
#else // __linux__
		// 로컬에서 해도될듯?
		std::thread::id id = std::this_thread::get_id();
		return *(threadId_t*)&id;
#endif // __linux__
	}
}

int main()
{
	TSUtil::CThreadExecutor threadExecutor;
	threadExecutor.init(10);

	static std::latch latch(20);

	for (size_t i = 0; i < 10; ++i)
	{
		threadExecutor.post(
			[]()
			{
				printf("[%u] long term job(post) \n", TSUtil::get_tid());
				latch.count_down();
			}
		);
	}

	for (size_t i = 0; i < 10; ++i)
	{
		threadExecutor.postAt(0,
			[]()
			{
				printf("[%u] long term job(postAt) \n", TSUtil::get_tid());
				latch.count_down();
			}
		);
	}

	latch.wait();
	threadExecutor.stop();

	return 0;
}
