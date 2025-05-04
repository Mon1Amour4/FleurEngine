#include "ThreadPool.h"

Fuego::ThreadPool::ThreadPool(uint16_t workers)
    : num_workers(workers)
    , running(true)
{
}

void Fuego::ThreadPool::Init()
{
    workers.reserve(num_workers);
    available_workers.assign((num_workers + 7) / 8, 0);
    for (size_t i = 0; i < num_workers; i++)
    {
        workers.emplace_back();
    }

    thread_pool__main();
}

void Fuego::ThreadPool::Stop()
{
    running = false;
}

void Fuego::ThreadPool::Push(Task&& task)
{
    std::lock_guard guard(mutex);
    tasks.push(std::move(task));
}

void Fuego::ThreadPool::thread_pool__main()
{
    while (running)
    {
        if (!tasks.empty())
        {
            auto worker = GetFreeWorker();
            if (worker)
            {
            }
        }
    };

    Release();
}

void Fuego::ThreadPool::Release()
{
    for (size_t i = 0; i < num_workers; i++)
    {
        workers[i].join();
    }
    workers.clear();
    workers.shrink_to_fit();
    available_workers.clear();
}

std::thread* Fuego::ThreadPool::GetFreeWorker()
{
    const char* sequence = available_workers.data();
    for (size_t i = 0; i < num_workers; i++)
    {
        size_t byte_index = i / 8;
        size_t bit_index = i % 8;

        if ((sequence[byte_index] & (1 << bit_index)) == 0)
        {
            return &workers[i];
        }
    }
    return nullptr;
}
