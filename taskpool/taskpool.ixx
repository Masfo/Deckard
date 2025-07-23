export module deckard.taskpool;

import std;
import deckard.types;
import deckard.as;
import deckard.debug;
import deckard.helpers;

namespace deckard::taskpool
{


	/*
	 *
	 *
	 *
	 *		_0__1__2__3__4__5__6__7_
	 *		[ ][ ][ ][ ][ ][ ][ ][ ]		Thread 0
	 *		[ ][ ][ ][ ][ ][ ][ ][ ]		Thread 1
	 *		[ ][ ][ ][ ][ ][ ][ ][ ]		Thread 2
	 *		[ ][ ][ ][ ][ ][ ][ ][ ]		Thread 3
	 *
	 *		[ ][ ][ ][ ][ ][ ][ ][ ]		Thread n-1
	 *
	 *
	 *
	 *   pool.add([]( )
			{


			});

			class hash_task: taskpool::work
			{
				void operator(size_t id)
				{
					// do work
				}
			};
	 *
	 *
	 *
	 *
	 */

	class taskpool
	{
	private:
		std::vector<std::thread> threads;
		std::atomic<bool>        stop{false};
		std::mutex               task_mutex;
		std::condition_variable  task_cv;
		std::queue<std::function<void()>> tasks;

		void worker_thread()
		{
			while (!stop)
			{
				std::function<void()> task;
				{
					std::unique_lock lock(task_mutex);
					task_cv.wait(lock, [this]() { return !tasks.empty() || stop; });
					if (stop && tasks.empty())
						return;
					task = std::move(tasks.front());
					tasks.pop();
				}
				task();
			}
		}
	public:
		taskpool() = default;
		taskpool(const taskpool&)            = delete;
		taskpool& operator=(const taskpool&) = delete;

		taskpool(u64 threadcount = std::thread::hardware_concurrency())
		{
			if (threadcount == 0)
				threadcount = 1;

			threads.reserve(threadcount);

			for (u64 i = 0; i < threadcount; i++)
			{
				threads.emplace_back([&, id=i]() { worker_thread(); });
			}
		}
	};


} // namespace deckard::taskpool
