#ifndef PARALLEL_FLUID_THREAD_POOL_H
#define PARALLEL_FLUID_THREAD_POOL_H

#include <iostream>
#include <queue>
#include <thread>
#include <chrono>
#include <mutex>
#include <future>
#include <unordered_set>
#include <atomic>

#include <vector>
#include <chrono>


class thread_pool {
public:
    /*!
     * @param threads_num number of threads in thread pool
     */
    thread_pool(size_t threads_num) {
        threads.reserve(threads_num);
        for (int i = 0; i < threads_num; ++i) {
            threads.emplace_back(&thread_pool::run, this);
        }
        quite = false;
        tasks_in_work = 0;
    }

    /*!
     * @details Adding task to the queue of thread pool
     * @param func function that should be executed in thread
     * @param args arguments for given function
     */
    template<typename Func, typename... Args>
    void add_task(const Func& func, Args&&... args) {
        std::lock_guard<std::mutex> lock(q_mtx);
        tasks_in_work++;
        tasks.emplace(std::async(std::launch::deferred, func, args...));
        q_cv.notify_one();
    }

    /*!
     * @details Wait finish of all tasks in all threads
     */
    void wait_all() {
        std::unique_lock<std::mutex> q_lock(q_mtx);
        finished_cv.wait(q_lock, [this]() -> bool {
            return tasks_in_work < 10 && tasks.empty();
        });
    }

    ~thread_pool() {
        wait_all();
        quite = true;
        for (auto & thread : threads) {
            q_cv.notify_all();
            thread.join();
        }
    }

private:
    /*!
     * @details Starts execution of tasks in thread pool tasks queue
     */
    void run() {
        while (!quite) {
            std::unique_lock<std::mutex> q_lock(q_mtx);
            q_cv.wait(q_lock, [this]() -> bool {
                return !tasks.empty() || quite;
            });

            if (!tasks.empty()) {
                auto task = std::move(tasks.front());
                tasks.pop();
                q_lock.unlock();

                task.get();
                tasks_in_work--;
                finished_cv.notify_all();
            }
        }
    }

    std::queue<std::future<void>> tasks;

    std::mutex q_mtx;
    std::condition_variable q_cv;
    std::condition_variable finished_cv;

    std::vector<std::thread> threads;
    std::atomic<bool> quite;

    std::atomic<int64_t> tasks_in_work = 0;
};

#endif //PARALLEL_FLUID_THREAD_POOL_H
