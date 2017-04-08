
#include <iostream>
#include <thread>
#include <atomic>
#include <vector>
#include <cstdint>

#define DATA_T uint64_t

const DATA_T ROUND = 1e5;
std::atomic<DATA_T> data{0};


void produce()
{
    // std::cout << "produce" << std::endl;
    for (DATA_T i = 0; i <= ROUND; ++i)
    {
        data = i;
    }
}


void consume()
{
    // std::cout << "consume" << std::endl;
    volatile DATA_T reader = -1;
    for (int i = 0; i < ROUND; ++i)
    {
        reader = data;
    }
}


int main()
{
    int nthreads = std::thread::hardware_concurrency();

    std::vector<std::thread> ths;
    ths.push_back(std::thread{produce});
    for (int i = 1; i < nthreads; ++i)
    {
        ths.push_back(std::thread{consume});
    }

    for (auto & th: ths)
    {
        th.join();
    }
}
