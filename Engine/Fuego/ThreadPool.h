#pragma once

#include <functional>
#include <future>
#include <queue>

#include "singleton.hpp"

namespace Fuego
{
class ThreadPool : public singleton<ThreadPool>
{
    friend class singletone;

public:
    struct Task
    {
        Task(std::packaged_task<void()> func)
            : f(std::move(func)) {};

        Task(Task&& other) noexcept
            : f(std::move(other.f)) {};

        void operator()()
        {
            f();
        };

    private:
        std::packaged_task<void()> f;
    };

    ThreadPool();

    void Init();

    void Shutdown();

    void Push(Task&& task);


private:
    uint16_t num_workers;
    std::vector<std::thread> workers;
    std::mutex queue_mutex;
    std::condition_variable condition;

    std::queue<Task> tasks;

    bool running;

    void Release();
    Task GetTask();

    std::string PrintThreadID(size_t thread_id) const;
};
}  // namespace Fuego