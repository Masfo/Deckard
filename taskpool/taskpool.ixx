export module deckard.taskpool;

import std;
import deckard.types;
import deckard.as;
import deckard.function_ref;

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


	export class taskpool
	{
	public:
		using work = function_ref<void(size_t id)>;

	private:
		std::vector<std::thread> threads;

		std::queue<work>        work_queue;
		std::mutex              work_lock;
		std::condition_variable queue_condition;


	public:
		taskpool()
			: taskpool(std::thread::hardware_concurrency())
		{
		}

		taskpool(size_t threadcount)
		{
			threadcount = std::max(threadcount, as<size_t>(std::thread::hardware_concurrency()));
			threads.reserve(threadcount);

			for (size_t i = 0; i < threadcount; i++)
			{
				threads.emplace_back(
				  [this, i]
				  {
					  while (true)
					  {
						  work task = get();
						  if (not task)
						  {
							  break;
						  }
						  else
						  {
							  task(i);
						  }
					  }
				  });
			}
		}

		void join()
		{

			for (auto& task : threads)
				task.join();
			threads.clear();
		}

		~taskpool() { join(); }

		void add(work task)
		{
			{
				std::lock_guard<std::mutex> lock(work_lock);
				work_queue.push(std::move(task));
			}
			queue_condition.notify_one();
		}

		work get()
		{
			std::unique_lock<std::mutex> lock(work_lock);
			queue_condition.wait(lock, [this]() { return not work_queue.empty(); });

			work task = std::move(work_queue.front());
			work_queue.pop();
			return task;
		}

		size_t size() const { return threads.size(); }
	};

} // namespace deckard::taskpool
