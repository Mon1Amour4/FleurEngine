#pragma once

#include <functional>
#include <future>
#include <queue>

#include "Services/ServiceInterfaces.hpp"

template <class T, class... Args>
using Result = std::invoke_result<T, Args...>::type;

namespace Fleur
{

class ThreadPool : public Service<ThreadPool>
{
    friend struct Service<ThreadPool>;

public:
    struct Task
    {
        template <typename F>
        Task(F&& func)
            : f(std::forward<F>(func)){};

        Task(Task&& other) noexcept
            : f(std::move(other.f)) {};

        void operator()() const
        {
            f();
        };
        ~Task() = default;

    private:
        std::function<void()> f;
    };

    ThreadPool();

    template <class Func, class... Args>
    auto Submit(Func&& f, Args&&... args) -> std::future<std::invoke_result_t<Func, Args...>>
    {
        using return_type = std::invoke_result_t<Func, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<Func>(f), std::forward<Args>(args)...));

        std::future<return_type> future = task->get_future();

        std::lock_guard<std::mutex> lock(m_QueueMutex);
        m_Tasks.emplace([task]() { (*task)(); });
        m_Condition.notify_one();
        return future;
    }

private:
    uint16_t m_NumWorkers;
    std::vector<std::thread> m_Workers;
    std::mutex m_QueueMutex;
    std::condition_variable m_Condition;

    std::queue<Task> m_Tasks;

    bool m_IsRunning;

    void Release();
    [[nodiscard]] Task GetTask();

    std::string PrintThreadID(size_t threadID) const;

protected:
    void OnInit();
    void OnShutdown();
};
}  // namespace Fleur
