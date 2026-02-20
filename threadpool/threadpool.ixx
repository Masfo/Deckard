export module deckard.taskpool;

import std;
import deckard.function_ref;
import deckard.threadutil;
import deckard.debug;

namespace deckard::taskpool
{

	// Taskpool job counter for simple coordination
	// 
	// auto job_id = pool.createjob("frequency_map", 50); // this task has 50 parts
	// Divide buffers into threads, each thread
	//
	// each run decrements task, then another task just checks if "frequency_map" job == 0, and merges results
	// if we dont know how many tasks there going to be in a job but we know the end task
	// we can just set "job" = 1 when done, and some merge task can just check if "job" == 1
	//
	// std::unordered_map<std::string, std::atomic_int> job_counters;

	export class threadpool
	{
	private:
		using function_t = std::move_only_function<void()>;
	private:
		std::vector<std::thread>          workers;
		std::queue<function_t> tasks;
		std::mutex                        mutex;
		std::condition_variable           cv;
		bool                              stop = false;

	public:
		explicit threadpool(size_t n = std::thread::hardware_concurrency())
		{

			n = std::max(1ull, n);

			workers.reserve(n);

			for (size_t i = 0; i < n; ++i)
			{
				workers.emplace_back(
				  [i,this]
				  {
					thread::set_thread_name(std::format("deckard-pool-{}", i));

					  while (true)
					  {
						  function_t task;
						  {
							  std::unique_lock lock(mutex);
							  cv.wait(lock, [this] { return stop or !tasks.empty(); });
							  if (stop && tasks.empty())
								  return;
							  task = std::move(tasks.front());
							  tasks.pop();
						  }
						  task();
					  }
				  });
			}
		}

		~threadpool() { join(); }

		void join()
		{
			{
				std::lock_guard lock(mutex);
				stop = true;
			}
			cv.notify_all();
			for (auto& t : workers)
				t.join();
		}

		template<typename F, typename... Args>
		auto enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>
		{
			using ret_t = std::invoke_result_t<F, Args...>;
			auto ntask   = std::make_shared<std::packaged_task<ret_t()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
			{
				std::lock_guard lock(mutex);
				tasks.emplace([ntask] { (*ntask)(); });
			}
			cv.notify_one();
			return ntask->get_future();
		}
	};

	export class taskpool
	{
	private:
		using function_t = std::function<void()>;


		std::vector<std::thread>            workers;
		std::vector<std::deque<function_t>> queues;
		std::queue<function_t>              global;
		std::mutex                          mutex;
		std::condition_variable             cv;
		bool                                stop;


	public:
		explicit taskpool(size_t n = std::thread::hardware_concurrency() - 2)
			: stop(false)
		{

			n = std::max(1ull, n);

			queues.resize(n);
			for (size_t i = 0; i < n; ++i)
			{
				workers.emplace_back(
				  [this, i]
				  {
					  thread::set_thread_name(std::format("deckard-pool-{}", i));

					  while (true)
					  {
						  function_t task;
						  if (pop_task(i, task) || steal_task(i, task) || pop_global(task))
						  {
							  task();
						  }
						  else
						  {
							  std::unique_lock lock(mutex);
							  cv.wait(lock, [this, i] { return stop || !queues[i].empty() || !global.empty(); });
							  if (stop && all_empty())
								  return;
						  }
					  }
				  });
			}
		}

		~taskpool() { close(); }

		void join()
		{
			{
				std::lock_guard lock(mutex);
				stop = true;
			}
			cv.notify_all();
			for (auto& t : workers)
				t.join();
		}

		void close() { join(); }

		template<typename F, typename... Args>
		auto enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>
		{
			using ret_t = std::invoke_result_t<F, Args...>;
			auto task   = std::make_shared<std::packaged_task<ret_t()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
			{
				std::lock_guard lock(mutex);
				global.emplace([task] { (*task)(); });
			}
			cv.notify_one();
			return task->get_future();
		}


	private:
		bool pop_task(size_t idx, function_t& task)
		{
			std::lock_guard lock(mutex);
			if (!queues[idx].empty())
			{
				task = std::move(queues[idx].front());
				queues[idx].pop_front();
				return true;
			}
			return false;
		}

		bool steal_task(size_t idx, function_t& task)
		{

			std::lock_guard lock(mutex);
			for (size_t i = 0; i < queues.size(); ++i)
			{
				if (i == idx)
					continue;
				if (!queues[i].empty())
				{
					task = std::move(queues[i].back());
					queues[i].pop_back();
					return true;
				}
			}
			return false;
		}

		bool pop_global(function_t& task)
		{
			std::lock_guard lock(mutex);
			if (!global.empty())
			{
				task = std::move(global.front());
				global.pop();
				return true;
			}
			return false;
		}

		bool all_empty()
		{

			for (const auto& q : queues)
				if (!q.empty())
					return false;
			return global.empty();
		}
	};


} // namespace deckard
