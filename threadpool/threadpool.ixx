export module deckard.taskpool;

import std;
import deckard.function_ref;

namespace deckard
{
	export class threadpool
	{
	private:
		// using function_t = deckard::function_ref<void()>;
		using function_t = std::function<void()>;


		std::vector<std::thread>            workers;
		std::vector<std::deque<function_t>> queues;
		std::queue<function_t>              global;
		std::mutex                          mutex;
		std::condition_variable             cv;
		bool                                stop;


	public:
		explicit threadpool(size_t n = std::thread::hardware_concurrency())
			: stop(false)
		{

			n = std::max(1ull, n);

			queues.resize(n);
			for (size_t i = 0; i < n; ++i)
			{
				workers.emplace_back(
				  [this, i, n]
				  {
					  while (true)
					  {
						  function_t task; // = []{};
						  if (pop_task(i, task) or steal_task(i, task) or pop_global(task))
						  {
							  task();
						  }
						  else
						  {
							  std::unique_lock lock(mutex);
							  cv.wait(lock, [this, i] { return stop or !queues[i].empty() or !global.empty(); });
							  if (stop && all_empty())
								  return;
						  }
					  }
				  });
			}
		}

		~threadpool() { close(); }

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


} // namespace deckard::taskpool
