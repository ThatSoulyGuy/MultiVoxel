#pragma once

#include <functional>
#include <queue>
#include <mutex>

namespace MultiVoxel::Independent::Thread
{
    class MainThreadInvoker
    {

    public:

        static void EnqueueTask(std::function<void()> task)
        {
            std::lock_guard lock(GetMutex());

            GetQueue().push(std::move(task));
        }

        static void ExecuteTasks()
        {
            auto& queue = GetQueue();

            std::lock_guard lock(GetMutex());

            while (!queue.empty())
            {
                queue.front()();
                queue.pop();
            }
        }

    private:

        static std::queue<std::function<void()>>& GetQueue()
        {
            static std::queue<std::function<void()>> queue;

            return queue;
        }

        static std::mutex& GetMutex()
        {
            static std::mutex mutex;

            return mutex;
        }
    };
}