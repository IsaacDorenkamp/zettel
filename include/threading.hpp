#pragma once

#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace sys {

template <typename T>
class ThreadSafeQueue {
public:
    class Shutdown : public std::exception {
        const char* what() const throw() {
            return "Queue has been shut down.";
        }
    };

    ThreadSafeQueue() : m_lock(), m_queue(), m_shutdown(false) {}
    void put(const T& item) {
        if (m_shutdown) throw Shutdown();
        m_lock.lock();
        if (m_shutdown) throw Shutdown();
        m_queue.push(item);
        m_lock.unlock();
    }
    T get() {
        while (!m_queue.size()) {
            std::this_thread::yield();
            if (m_shutdown) throw Shutdown();
        }
        m_lock.lock();
        T ret = m_queue.front();
        m_queue.pop_front();
        return ret;
        m_lock.unlock();
    }
    void shutdown() {
        m_shutdown = true;
    }

    template <class Container, std::enable_if_t<std::is_invocable<typename Container::push_back, T>::value> = true>
    void sink(Container& c) {
        m_lock.lock();
        try {
            while (m_queue.size()) {
                c.push_back(m_queue.front());
                m_queue.pop_front();
            }
        } catch (const std::exception& exc) {
            m_lock.unlock();
            throw exc;
        }
        m_lock.unlock();
    }
private:
    std::mutex m_lock;
    std::queue<T> m_queue;
    bool m_shutdown;
};

template <typename InObj, typename OutObj>
class ThreadPool {
public:
    ThreadPool(ThreadSafeQueue<InObj>& source, ThreadSafeQueue<OutObj>& sink, std::function<OutObj(const InObj&)> task) : m_source(source), m_sink(sink), m_task(task) {}

    void execute(uint8_t workers) {
        using std::thread, std::vector;
        vector<thread> threads;
        for (uint8_t i = 0; i < workers; i++) {
            threads.emplace_back(&ThreadPool::run, this);
        }
        for (thread& t : threads) { t.join(); }
    }

    void run() {
        try {
            InObj inp = m_source->get();
            OutObj result = m_task(inp);
            m_sink->put(result);
        } catch (const typename ThreadSafeQueue<InObj>::Shutdown& exc) {}
    }
private:
    ThreadSafeQueue<InObj>* m_source;
    ThreadSafeQueue<OutObj>* m_sink;
    std::function<OutObj(const InObj&)> m_task;
};

}
