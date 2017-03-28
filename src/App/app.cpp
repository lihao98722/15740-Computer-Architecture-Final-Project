
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

const int ROUND = 10000;

std::mutex m;
int data = -1;

void produce()
{
    // std::cout << "produce" << std::endl;
    for (int i = 0; i < ROUND; ++i)
    {
        std::lock_guard<std::mutex> lock(m);
        data = i;
    }
}


void consume()
{
    // std::cout << "consume" << std::endl;
    volatile int reader = -1;
    for (int i = 0; i < ROUND; ++i)
    {
        std::lock_guard<std::mutex> lock(m);
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
