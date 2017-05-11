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
    CACHE_TO_CACHE = 8,
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

    bool is_set(UINT32 pid)
    {
        return sharer_vector & (1 << pid);
    }

    UINT32 owner(UINT32 num_processors)
    {
        UINT32 pid = 0;
        for (; !is_set(pid) && (pid < num_processors); ++pid);
        assert(pid != num_processors);
        assert(state == CACHE_STATE::MODIFIED);
        return pid;
    }

    void set_sharer(UINT32 pid)
    {
        sharer_vector |= (1 << pid);
    }

    void clear_sharer(UINT32 pid)
    {
        sharer_vector &= (~(1 << pid));
        // clear read counts as well
        read_count_vector &= ~(3 << (pid*2));
    }

    void increase_read_count(UINT32 pid)
    {
        assert(pid < 64);
        auto count = (read_count_vector >> (pid * 2)) & 3;
        if (count < 3){
            read_count_vector += (1 << (pid * 2));
        }
    }

    void decrease_read_count(UINT32 pid)
    {
        assert(pid < 64);
        auto count = (read_count_vector >> (pid * 2)) & 3;
        if (count > 0){
            read_count_vector -= (1 << (pid * 2));
        }
    }

    bool qualified_reader(UINT32 pid)
    {
        return read_count_vector & (1 << (pid+1));
    }

    bool is_last_writer(UINT32 pid)
    {
        return pid == last_writer;
    }

    void update_last_writer(UINT32 pid)
    {
        last_writer = pid;
        set_sharer(pid);
        read_count_vector = 0;
    }

public:
    UINT32  sharer_vector;
    CACHE_STATE  state;

    // detector
    UINT32 last_writer;
    UINT64 read_count_vector; //NOTE: reverse
};

/* ===================================================================== */
/*  @brief Cache Coherence Protocol MSI                                  */
/* ===================================================================== */
class DIR_MSI
{
public:
    DIR_MSI(UINT32 num_processors): _num_processors(num_processors)
    {
        profiles = std::vector<Profile>(num_processors, Profile());
        detector = false;
    }
    void process_read(UINT32 pid, UINT64 addr, UINT64 tag, UINT32 set_index);
    void process_write(UINT32 pid, UINT64 addr, UINT64 tag, UINT32 set_index);

    INT32 fetch(UINT32 pid, UINT32 home, UINT64 tag, HIT_MISS_TYPES &response);
    INT32 fetch_and_invalidate(UINT32 pid, UINT32 home, UINT64 tag, HIT_MISS_TYPES &response);
    // speculatively push data to qualified readers
    INT32 push_and_invalidate(UINT32 pid, UINT32 home, UINT64 tag, HIT_MISS_TYPES &response);
    INT32 read_miss(UINT32 pid, UINT32 home, UINT64 tag);
    INT32 write_miss(UINT32 pid, UINT32 home, UINT64 tag);
    INT32 data_write_back(UINT32 pid, UINT32 home)
    {
        return get_directory_cost(pid, home) + MEMORY_ACCESS;
    }

    inline UINT32 get_home_node(UINT64 tag)
    {
        return tag & (_num_processors -1);
    }

    inline INT32 get_directory_cost(UINT32 src, UINT32 dest)
    {
        return (src == dest) ? LOCAL_CACHE_ACCESS : REMOTE_CACHE_ACCESS;
    }

    inline Directory_Line & get_directory_line(UINT64 tag)
    {
        auto it = _directory.find(tag);
        if (it == _directory.end())
        {
            _directory[tag] = Directory_Line();
        }
        return _directory[tag];
    }

    inline std::string stat_to_string()
    {
        std::stringstream ss;
        ss << "Memory Stats:\n";
        ss << "\t\tAddr\tLoad Hit\tLoad Miss\tStore Hit\tStore Miss\n";
        for (const auto& p : line_stat)
        {
            if (p.second.count <= THRESHOLD) {continue;}
            ss << std::hex << p.first << std::dec << "\t"
               << p.second.load.hit << "\t" << p.second.load.miss << "\t"
               << p.second.store.hit << "\t"<< p.second.store.miss << "\n";
        }
        return ss.str();
    }

public:
    std::vector<Profile> profiles;
    std::unordered_map<uint64_t, Access_Stat> line_stat;
    std::unordered_map<UINT64, Directory_Line>  _directory;
    UINT32  _num_processors;
    bool detector;
};
