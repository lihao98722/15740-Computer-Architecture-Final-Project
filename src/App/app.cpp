
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <cstdint>
#include <mutex>

// #define DEBUG
#ifdef DEBUG

std::mutex iomutex;

#endif

#define DATA_T uint64_t
const DATA_T ROUND = 1e5;
const int SYNC = 1000;
std::atomic<DATA_T> data(0);

void produce()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(SYNC));
    for (DATA_T i = 0; i <= ROUND; ++i)
    {
        data = i;
    }

    #ifdef DEBUG

    std::lock_guard<std::mutex> iolock(iomutex);
    std::cout << "produce thread on CPU: " << sched_getcpu() << std::endl;

    #endif
}

void consume()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(SYNC));
    volatile DATA_T reader = 1;
    for (int i = 0; i < ROUND; ++i)
    {
        reader = data;
    }

    #ifdef DEBUG

    std::lock_guard<std::mutex> iolock(iomutex);
    std::cout << "consume thread on CPU: " << sched_getcpu() << std::endl;

    #endif
}

void pin(int i, std::thread &th)
{
    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    CPU_SET(i, &cpu_mask);

    pthread_setaffinity_np(th.native_handle(), sizeof(cpu_set_t), &cpu_mask);
}

int main()
{
    int nthreads = static_cast<int>(std::thread::hardware_concurrency() / 2);
    std::vector<std::thread> ths(nthreads);

    #ifdef DEBUG

    std::cout << "Launching " << nthreads << " threads" << std::endl;

    #endif

    ths[0] = std::thread{produce};
    pin(0, ths[0]);

    for (int i = 1; i < nthreads; ++i)
    {
        ths[i] = std::thread{consume};
        pin(i, ths[i]);
    }

    for (auto & th: ths)
    {
        th.join();
    }
}
