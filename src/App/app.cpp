
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

using DATA_T = uint64_t;
const uint32_t ROUND = 1e6;
const int SYNC = 100;

std::mutex mx;
DATA_T *data = nullptr;

static inline void local_irq_disable()
{
  __asm__ volatile("cli:":::"memory");
}

void produce()
{
    // local_irq_disable();
    // std::this_thread::sleep_for(std::chrono::milliseconds(SYNC));

    for (auto i = 0; i < ROUND; ++i)
    {
        std::lock_guard<std::mutex> lock{mx};
        *data = i;
    }

    #ifdef DEBUG
    std::lock_guard<std::mutex> iolock(iomutex);
    std::cout << "produce thread on CPU: " << sched_getcpu() << std::endl;
    #endif
}

void consume()
{
    // local_irq_disable();
    // std::this_thread::sleep_for(std::chrono::milliseconds(SYNC));
    volatile DATA_T reader = 1;

    for (auto i = 0; i < ROUND; ++i)
    {
        std::lock_guard<std::mutex> lock{mx};
        reader = *data;
    }

    #ifdef DEBUG
    std::lock_guard<std::mutex> iolock(iomutex);
    std::cout << "consume thread on CPU: " << sched_getcpu() << std::endl;
    #endif
}

inline void pin(int i, std::thread &th)
{
    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    CPU_SET(i, &cpu_mask);

    pthread_setaffinity_np(th.native_handle(), sizeof(cpu_set_t), &cpu_mask);
}

int main()
{
    // int nthreads = static_cast<int>(std::thread::hardware_concurrency() / 2 - 1);
    // std::cout << "app start" << std::endl;
    data = new DATA_T(100999986);
    int nthreads = 3;
    std::vector<std::thread> ths(nthreads);

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

    std::cout << "Memory Addr: " << std::hex << data << std::endl;
    delete data;
}
