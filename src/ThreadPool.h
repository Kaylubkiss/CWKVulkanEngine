#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <functional>
#include <queue>
#include <condition_variable>

class ThreadPool 
{
public:
	ThreadPool() = default;
	~ThreadPool() = default;
	void Init(size_t num_threads);
	//MUST CALL THIS BEFORE EXITING PROGRAM.
	void Terminate();
	void EnqueueTask(std::function<void()>& task);
private:
	void ThreadLoop();
private:
	std::mutex queue_mutex; //to make sure that the job queue is not corrupted.
	std::condition_variable condition_variable;
	std::vector<std::thread> threads;
	std::queue<std::function<void()>> tasks;
	bool terminate = false;
};

#endif