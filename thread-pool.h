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
    explicit thread_pool(size_t threads_num) {
        threads.reserve(threads_num);
        for (int i = 0; i < threads_num; ++i) {
            threads.emplace_back(&thread_pool::run, this);
        }
        quite = false;
        last_idx = 0;
    }


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

                task.first.get();

                std::lock_guard<std::mutex> set_lock(finished_tasks_ids_mtx);
                finished_tasks_ids.insert(task.second);
                finished_tasks_cv.notify_all();
            }
        }
    }


    /*!
     * @details Adding task to the queue of thread pool
     * @param func function that should be executed in thread
     * @param args arguments for given function
     * @return index of task in thread pool
     */
    template<typename Func, typename... Args>
    int64_t add_task(const Func& func, Args&&... args) {
        int64_t task_idx = last_idx++;
        std::lock_guard<std::mutex> lock(q_mtx);
        tasks.emplace(std::async(std::launch::deferred, func, args...), task_idx);
        q_cv.notify_one();
        return task_idx;
    }

    /*!
     * @details Wait finish of all tasks in all threads
     */
    void wait_all() {
        std::unique_lock<std::mutex> q_lock(q_mtx);
        finished_tasks_cv.wait(q_lock, [this]() -> bool {
            std::lock_guard<std::mutex> set_lock(finished_tasks_ids_mtx);
            return finished_tasks_ids.size() == last_idx && tasks.empty();
        });
    }

    ~thread_pool() {
        wait_all();
        quite = true;
        q_cv.notify_all();
        for (auto & thread : threads) {
            thread.join();
        }
    }

private:
    std::queue<std::pair<std::future<void>, int64_t>> tasks;

    std::mutex q_mtx;
    std::condition_variable q_cv;

    std::mutex finished_tasks_ids_mtx;
    std::condition_variable finished_tasks_cv;

    std::vector<std::thread> threads;
    std::atomic<bool> quite;
    std::atomic<int64_t> last_idx;

    std::unordered_set<int64_t> finished_tasks_ids;
};

#endif //PARALLEL_FLUID_THREAD_POOL_H
