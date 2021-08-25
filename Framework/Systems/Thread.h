#pragma once


class Task final
{
public:
	typedef std::function<void()> Process;
public:
	explicit Task(Process&& process) 
	{
		this->process = std::forward<Process>(process);
	}
	~Task() = default;

	Task(const Task&) = delete;
	Task& operator=(const Task&) = delete;

	void Execute() 
	{
		process();
	}
private:
	Process process;
};

class Thread final 
{
public:
	explicit Thread();
	~Thread();
	Thread(const Thread&) = delete;
	Thread& operator=(const Thread&) = delete;

public:
	inline static Thread* Get() {return instance;}
	static void Create();
	static void Delete();
public:

    void Initialize();
	void Invoke();
	template<typename Process>
	void AddTask(Process&& process);
	void SetStop(bool bStop) { this->bStop = bStop; }
private:
	static Thread* instance;
	std::vector<std::thread> threads;
	std::queue<std::shared_ptr<Task>> tasks;
	std::mutex taskMutex;
	std::condition_variable conditionVar; 
	uint threadCount;
	bool bStop;

};

template<typename Process>
inline void Thread::AddTask(Process && process)
{
	if (threads.empty())
	{
		//LOG_WARNING("THREAD::AddTask:: No available threads");
		process();
		return;
	}
	
	std::unique_lock<std::mutex> lock(taskMutex);
	
	
	tasks.push(std::make_shared<Task>(std::bind(std::forward<Process>(process))));
	lock.unlock();

	conditionVar.notify_one(); 
	
}
