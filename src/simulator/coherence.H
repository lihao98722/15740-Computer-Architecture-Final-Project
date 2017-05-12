#pragma once

#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <assert.h>

#include "profile.H"

typedef enum
{
    LOCAL_CACHE_ACCESS = 3,
    REMOTE_CACHE_ACCESS = 7,
    CACHE_TO_CACHE = 4,   // amortized by the numebr of pushed processors
    MEMORY_ACCESS = 100
}COST;

enum class CACHE_STATE
{
    INVALID,
    SHARED,
    MODIFIED
};

/* ===================================================================== */
/*  @brief Directory_Line                                                */
/* ===================================================================== */
class Directory_Line
{
public:
    Directory_Line()
    {
        sharer_vector = 0;
        read_count_vector = 0;
        last_writer = ~0;
        state = CACHE_STATE::INVALID;
    }

    inline bool is_set(uint32_t pid)
    {
        return sharer_vector & (1 << pid);
    }

    inline uint32_t owner(uint32_t num_processors)
    {
        uint32_t pid = 0;
        for (; !is_set(pid) && (pid < num_processors); ++pid);
        assert(pid != num_processors);
        assert(state == CACHE_STATE::MODIFIED);
        return pid;
    }

    inline void set_sharer(uint32_t pid)
    {
        sharer_vector |= (1 << pid);
    }

    inline void clear_sharer(uint32_t pid)
    {
        sharer_vector &= (~(1 << pid));
    }

    inline void clear_read_count(uint32_t pid)
    {
        read_count_vector &= ~(3 << (pid * 2));
    }

    inline void increase_read_count(UINT32 pid)
    {
        assert(pid < 64);
        auto count = (read_count_vector >> (pid * 2)) & 3;
        if (count < 3){
            read_count_vector += (1 << (pid * 2));
        }
    }

    inline void decrease_read_count(UINT32 pid)
    {
        assert(pid < 64);
        auto count = (read_count_vector >> (pid * 2)) & 3;
        if (count > 0){
            read_count_vector -= (1 << (pid * 2));
        }
    }

    inline bool qualified_reader(UINT32 pid)
    {
        return read_count_vector & (1 << (2 * pid + 1));
    }

    inline bool is_last_writer(UINT32 pid)
    {
        return pid == last_writer;
    }

    inline void update_last_writer(UINT32 pid)
    {
        last_writer = pid;
        set_sharer(pid);
        read_count_vector = 0;
    }

public:
    uint32_t  sharer_vector;
    CACHE_STATE  state;

    // detector
    uint32_t last_writer;
    uint64_t read_count_vector; //NOTE: reverse
};

/* ===================================================================== */
/*  @brief Cache Coherence Protocol MSI                                  */
/* ===================================================================== */
class Controller;

class DIR_MSI
{
public:
    DIR_MSI(uint32_t num_processors): _num_processors(num_processors)
    {
        profiles = new Profile(num_processors);
        detector = true;
    }

    ~DIR_MSI()
    {
        delete profiles;
    }

    void process_read(uint32_t pid, uint64_t addr);
    void process_write(uint32_t pid, uint64_t addr, Controller *controller);
    void invalidate(uint32_t pid, uint64_t addr);

    uint64_t fetch(uint32_t pid, uint32_t home, uint64_t addr, uint64_t &hops, ACCESS_TYPE &response);
    uint64_t fetch_and_invalidate(uint32_t pid, uint32_t home, uint64_t addr, uint64_t &hops, ACCESS_TYPE &response);
    uint64_t push_and_invalidate(uint32_t pid, uint32_t home, uint64_t addr, uint64_t &hops, ACCESS_TYPE &response, Controller *controller);
    uint64_t read_miss(uint32_t pid, uint32_t home, uint64_t addr, uint64_t &hops);
    uint64_t write_miss(uint32_t pid, uint32_t home, uint64_t addr, uint64_t &hops);

    inline uint64_t data_write_back(uint32_t pid, uint32_t home, uint64_t &hops)
    {
        return get_directory_cost(pid, home, hops) + MEMORY_ACCESS;
    }

    inline uint32_t get_home_node(uint64_t addr)
    {
        return addr & (_num_processors - 1);
    }

    inline uint64_t get_directory_cost(uint32_t src, uint32_t dest, uint64_t &hops)
    {
        if (src != dest) {  // request -> reply
            hops += 2;
        }
        return (src == dest) ? LOCAL_CACHE_ACCESS : REMOTE_CACHE_ACCESS;
    }

    inline Directory_Line & get_directory_line(uint64_t addr)
    {
        auto it = _directory.find(addr);
        if (it == _directory.end())
        {
            _directory[addr] = Directory_Line();
        }
        return _directory[addr];
    }

public:
    Profile *profiles;
    std::unordered_map<uint64_t, Directory_Line>  _directory; //NOTE: addr -> dir_line
    uint32_t  _num_processors;
    bool detector;
};
