#pragma once
#include <functional>
#include <queue>
#include <condition_variable>

class ThreadPool 
{
public:
	ThreadPool() = default;
	~ThreadPool() = default;
	void Init(size_t num_threads);
	void Terminate();
	template<class F>
	void EnqueueTask(F&& task)
	{
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			tasks.push(std::forward<F>(task));
		}

		condition_variable.notify_one();
	}
	bool isBusy();
private:
	void ThreadLoop();
private:
	std::mutex queue_mutex; //to make sure that the job queue is not corrupted.
	std::condition_variable condition_variable;
	std::vector<std::thread> threads;
	std::queue<std::function<void()>> tasks;
	bool terminate = false;
};