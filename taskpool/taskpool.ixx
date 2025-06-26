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


	export class taskpool
	{
	public:
		taskpool(size_t num_threads = std::thread::hardware_concurrency())
		{

			for (size_t i = 0; i < num_threads; ++i)
			{
				threads_.emplace_back(
				  [this]
				  {
					  while (true)
					  {
						  auto                  start_time = std::chrono::steady_clock::now();
						  std::function<void()> task;
						  {
							  std::unique_lock<std::mutex> lock(queue_mutex_);

							  cv_.wait(lock, [this] { return !tasks_.empty() || stop_; });

							  if (stop_ && tasks_.empty())
								  return;

							  task = move(tasks_.front());
							  tasks_.pop();
						  }

						  task();
						  auto end_time = std::chrono::steady_clock::now();
						  #ifdef _DEBUG
						  dbg::println("task done in {}", pretty_time(end_time-start_time));
						  #endif
					  }
				  });
			}
		}

		~taskpool() { stop(); }

		void stop()
		{
			{
				std::unique_lock<std::mutex> lock(queue_mutex_);
				stop_ = true;
			}

			cv_.notify_all();

			for (auto& thread : threads_)
				thread.join();
		}

		void push(std::function<void()> task)
		{
			{
				std::unique_lock<std::mutex> lock(queue_mutex_);
				tasks_.emplace(move(task));
			}
			cv_.notify_one();
		}

	private:
		std::vector<std::thread> threads_;

		std::queue<std::function<void()>> tasks_;

		std::mutex queue_mutex_;

		std::condition_variable cv_;

		bool stop_ = false;
	};
} // namespace deckard::taskpool
