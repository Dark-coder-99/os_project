#include "ThreadPool.h"
#include <iostream>
#include <chrono>
#include <random>
#include <vector>
#include <mutex>
#include <iomanip>

// A mutex to ensure console output doesn't get scrambled when multiple threads print
std::mutex cout_mutex;

// Function to simulate some heavy computational work
int perform_heavy_computation(int id) {
    // Generate a random delay between 100 and 500 ms to simulate un-even workload
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(100, 500);
    int sleep_time = distrib(gen);

    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));

    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Task " << std::setw(3) << id 
                  << " executed on thread " << std::this_thread::get_id() 
                  << " (took " << sleep_time << "ms)\n";
    }

    // Return a calculation result so we can demonstrate futures perfectly
    return id * id; 
}

int main() {
    std::cout << "=================================================\n";
    std::cout << " Starting Scalable Thread Management Example\n";
    std::cout << "=================================================\n\n";
    
    // Initialize ThreadPool. It will automatically use the optimal number of hardware threads.
    ThreadPool pool;
    std::cout << "[INFO] ThreadPool instantiated gracefully with " << pool.get_worker_count() << " background workers.\n\n";

    // Vector to store future results
    std::vector<std::future<int>> results;

    int num_tasks = 25;
    std::cout << "[INFO] Enqueuing " << num_tasks << " independent heavy tasks...\n\n";
    
    auto start_time = std::chrono::high_resolution_clock::now();

    // Push all tasks to the pool immediately without blocking the main thread
    for (int i = 1; i <= num_tasks; ++i) {
        // enqueue returns a std::future where the final result will be produced
        results.push_back(
            pool.enqueue([i] {
                return perform_heavy_computation(i);
            })
        );
    }

    std::cout << "[INFO] All tasks dynamically queued. Main thread is free while pool works...\n\n";

    // We can do other things here on the main thread while the pool computes.

    // Finally, collect the results safely. future.get() blocks if a specific task is not yet completely done.
    int expected_sum = 0;
    int actual_sum = 0;
    
    for (int i = 1; i <= num_tasks; ++i) {
        actual_sum += results[i-1].get();  // Blocks if not yet computed
        expected_sum += (i * i);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << "\n=================================================\n";
    std::cout << " All Computations Concluded.\n";
    std::cout << " Expected Total Sum: " << expected_sum << "\n";
    std::cout << " Actual Computed Sum: " << actual_sum << "\n";
    std::cout << " Total Time Taken: " << elapsed.count() << " seconds\n";
    std::cout << "=================================================\n";

    if (expected_sum == actual_sum) {
        std::cout << "[SUCCESS] Library operated flawlessly without race-conditions!\n";
    } else {
        std::cout << "[ERROR] Data mismatch detected!\n";
    }

    std::cout << "[INFO] Pool destructor being invoked. Cleanly joining all worker threads...\n";

    return 0; // Returning destroys the scope, which kills `pool`, joining all threads cleanly.
}
