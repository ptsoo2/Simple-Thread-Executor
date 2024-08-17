# 📦 DEPRECATED
- [new repository link](https://github.com/ptsoo2/TSUtil)

# Simple Thread Executor

### Directory
- include: source file
- example.cpp: simple async thread process

### Content
- ThreadPool 을 베이스로 삼음
- 각 Thread 마다 Task Queue 를 가지며 긴 시간이 소요되는 비동기 로직을 스레드에서 처리
- MPSCQueue (https://github.com/ptsoo2/MPSCQueue) 를 사용해서 Task 를 큐잉해서 처리
- stThreadWaitState_t
  - Thread 가 처리할 일이 없어서 놀고있는 경우의 대기 용도
- CThreadExecutor::postAt
  - 지정된 Thread 로 Task 를 던짐. 줄을 세워서 처리하기 위함.
- CThreadExecutor::post
  - Round Robin 으로 결정된 Thread 로 Task 를 던짐.
- CThreadExecutor::flush
  - stop 이후에 잔존한 Task 가 존재할 수 있음. 잔존한 Task 를 모두 실행해야하는 경우 optional 하게 사용

### TODO
- 

## EXAMPLE
```c
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
```
## OUTPUT
```c
[18224] long term job(post)
[43880] long term job(post)
[34432] long term job(post)
[14316] long term job(post)
[14316] long term job(postAt)
[34116] long term job(post)
[39032] long term job(post)
[20784] long term job(post)
[14316] long term job(postAt)
[14316] long term job(postAt)
[49528] long term job(post)
[32936] long term job(post)
[14316] long term job(postAt)
[14316] long term job(postAt)
[14316] long term job(postAt)
[14316] long term job(postAt)
[14316] long term job(postAt)
[28244] long term job(post)
[14316] long term job(postAt)
[14316] long term job(postAt)
