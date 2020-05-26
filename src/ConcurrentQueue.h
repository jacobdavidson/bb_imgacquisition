// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>

template<typename T>
class ConcurrentQueue final
{
public:
    void push(const T& value)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _queue.push(value);
        lock.unlock();
        _cond.notify_one();
    }

    void push(T&& value)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _queue.push(std::move(value));
        lock.unlock();
        _cond.notify_one();
    }

    void pop(T& value)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        while (_queue.empty())
        {
            _cond.wait(lock);
        }
        std::swap(value, _queue.front());
        _queue.pop();
    }

    std::size_t size()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        return _queue.size();
    }

private:
    std::queue<T>           _queue;
    std::mutex              _mutex;
    std::condition_variable _cond;
};
