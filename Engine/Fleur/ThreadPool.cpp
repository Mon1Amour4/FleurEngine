#include "ThreadPool.h"

Fleur::ThreadPool::ThreadPool()
    : num_workers(std::thread::hardware_concurrency())
    , running(true)
{
}

void Fleur::ThreadPool::OnInit()
{
    workers.reserve(num_workers);
    for (size_t i = 0; i < num_workers; i++)
    {
        workers.emplace_back(
            [this]()
            {
                while (running)
                {
                    std::unique_lock<std::mutex> ul(queue_mutex);
                    condition.wait(ul, [this]() { return !tasks.empty() || !running; });
                    if (!running)
                        return;
                    Task task = GetTask();
                    ul.unlock();
                    task();
                }
            });

        FL_CORE_TRACE("[ThreadPool] Thread created: id: {0}", PrintThreadID(i));
    }
}

void Fleur::ThreadPool::OnShutdown()
{
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        running = false;
    }
    condition.notify_all();
    for (size_t i = 0; i < num_workers; i++)
    {
        std::thread& thread = workers[i];
        if (thread.joinable())
        {
            FL_CORE_TRACE("[ThreadPool] Thread closed: id: {0}", PrintThreadID(i));
            thread.join();
        }
    }
    Release();
}

void Fleur::ThreadPool::Release()
{
    workers.clear();
    workers.shrink_to_fit();
}

Fleur::ThreadPool::Task Fleur::ThreadPool::GetTask()
{
    Task task = std::move(tasks.front());
    tasks.pop();
    return task;
}

std::string Fleur::ThreadPool::PrintThreadID(size_t thread_id) const
{
    std::ostringstream ss;
    ss << workers[thread_id].get_id();
    return ss.str();
}
