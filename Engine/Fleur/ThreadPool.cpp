#include "ThreadPool.h"

Fleur::ThreadPool::ThreadPool()
    : m_NumWorkers(std::thread::hardware_concurrency())
    , m_IsRunning(true)
{
}

void Fleur::ThreadPool::OnInit()
{
    m_Workers.reserve(m_NumWorkers);
    for (size_t i = 0; i < m_NumWorkers; i++)
    {
        m_Workers.emplace_back(
            [this]()
            {
                while (m_IsRunning)
                {
                    std::unique_lock<std::mutex> ul(m_QueueMutex);
                    m_Condition.wait(ul, [this]() { return !m_Tasks.empty() || !m_IsRunning; });
                    if (!m_IsRunning)
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
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        m_IsRunning = false;
    }
    m_Condition.notify_all();
    for (size_t i = 0; i < m_NumWorkers; i++)
    {
        std::thread& thread = m_Workers[i];
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
    m_Workers.clear();
    m_Workers.shrink_to_fit();
}

Fleur::ThreadPool::Task Fleur::ThreadPool::GetTask()
{
    Task task = std::move(m_Tasks.front());
    m_Tasks.pop();
    return task;
}

std::string Fleur::ThreadPool::PrintThreadID(size_t thread_id) const
{
    std::ostringstream ss;
    ss << m_Workers[thread_id].get_id();
    return ss.str();
}
