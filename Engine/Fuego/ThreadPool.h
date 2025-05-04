#pragma once

#include <functional>
#include <future>
#include <queue>

#include "singleton.hpp"

namespace Fuego
{

struct Task
{
    Task(std::weak_ptr<std::packaged_task<void()>> f)
        : f(f) {};

    Task(Task&& other) noexcept
        : f(std::move(other.f))
    {
    }

private:
    std::weak_ptr<std::packaged_task<void()>> f;
};

class ThreadPool : public singleton<ThreadPool>
{
    friend class singletone;

public:
    ThreadPool(uint16_t workers);

    void Init();

    void Stop();

    void Push(Task&& task);


private:
    uint16_t num_workers;
    std::vector<std::thread> workers;
    std::vector<char> available_workers;
    std::mutex mutex;

    std::queue<Task> tasks;

    bool running;

    void thread_pool__main();
    void Release();
    std::thread* GetFreeWorker() const;
};
}  // namespace Fuego