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
            auto& q = GetQueue();

            std::lock_guard lock(GetMutex());

            while (!q.empty())
            {
                q.front()();
                q.pop();
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
            static std::mutex m;
            return m;
        }
    };
}