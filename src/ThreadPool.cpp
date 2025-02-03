#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t num_threads) 
{
	for (size_t i = 0; i < num_threads; ++i) 
	{
		threads.emplace_back(std::thread(&ThreadPool::ThreadLoop, this));
	}

}

bool ThreadPool::isBusy() 
{
	bool isBusy = false;
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		isBusy = !tasks.empty();
	}
	return isBusy;
}

void ThreadPool::EnqueueTask(const std::function<void()>& task) 
{
	{
		std::unique_lock<std::mutex> lock(queue_mutex);

		tasks.push(std::move(task));
	}


	condition_variable.notify_one();

}


void ThreadPool::ThreadLoop() 
{
	while (true) 
	{
		std::function<void()> func;

		{
			std::unique_lock<std::mutex> lock(queue_mutex);

			condition_variable.wait(lock, [this] {
				return !tasks.empty() || terminate;
			});

			if (terminate && tasks.empty()) 
			{
				return;
			}


			func = std::move(tasks.front());
			tasks.pop();

		}

		func();
	}

}


void ThreadPool::Terminate() {
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		terminate = true;
	}

	condition_variable.notify_all();
	
	for (std::thread& active_thread : threads) {
		active_thread.join();
	}


	threads.clear();
}

ThreadPool::~ThreadPool() 
{
	ThreadPool::Terminate();
}