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
    }

public:
    UINT32  sharer_vector;
    CACHE_STATE  state;
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
    }
    void process_read(UINT32 pid, UINT64 tag, UINT32 set_index);
    void process_write(UINT32 pid, UINT64 tag, UINT32 set_index);

    INT32 fetch(UINT32 pid, UINT32 home, UINT64 tag, HIT_MISS_TYPES &response);
    INT32 fetch_and_invalidate(UINT32 pid, UINT32 home, UINT64 tag, HIT_MISS_TYPES &response);
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

public:
    std::vector<Profile> profiles;
    std::unordered_map<UINT64, Directory_Line>  _directory;
    UINT32  _num_processors;
};
