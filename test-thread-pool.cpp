#include <iostream>
#include <chrono>

#include "thread-pool.h"

void test_func(int& res, const std::vector<int>& arr) {
    using namespace std::chrono_literals;
    res = 0;
    for (int i = arr.size() - 1; i >= 0; --i) {
        for (int j = 0; j < arr.size(); ++j) {
            res += arr[i] + arr[j];
        }
    }
}


void thread_pool_test(std::vector<int> ans, std::vector<int> arr) {
    auto begin = std::chrono::high_resolution_clock::now();

    thread_pool t(6);
    for (int i = 0; i < ans.size(); ++i) {
        t.add_task(test_func, std::ref(ans[i]), std::ref(arr));
    }
    t.wait_all();

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "With thread pool: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << std::endl;
}

void without_thread_test(std::vector<int> ans, std::vector<int> arr) {

    auto begin = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ans.size(); ++i) {
        test_func(std::ref(ans[i]), std::ref(arr));
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Without thread pool: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << std::endl;
}

void run_test() {
    std::vector<int> ans(24);
    std::vector<int> arr1(10000);
    thread_pool_test(ans, arr1);
    without_thread_test(ans, arr1);
}

int main() {
    run_test();
}