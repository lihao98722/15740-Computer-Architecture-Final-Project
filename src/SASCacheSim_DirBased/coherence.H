#ifndef _COHERENCE_H_
#define _COHERENCE_H_
// #pragma once

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
        assert(pid != num_processors && state == CACHE_STATE::MODIFIED);
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
/*  @brief Cache Coherence Directory Based                               */
/* ===================================================================== */
class Coherence
{
public:
    Coherence(UINT32 num_processors) : _num_processors(num_processors)
    {
        profiles = std::vector<Profile>(num_processors, Profile());
    }
    virtual ~Coherence();
    virtual void process_read(UINT32 pid, UINT64 tag, UINT32 set_index) {}
    virtual void process_write(UINT32 pid, UINT64 tag, UINT32 set_index) {}

    virtual INT32 fetch(UINT32 pid, UINT32 home, UINT64 tag, HIT_MISS_TYPES &response) {return 0;}
    virtual INT32 fetch_and_invalidate(UINT32 pid, UINT32 home, UINT64 tag, HIT_MISS_TYPES &response) {return 0;}
    virtual INT32 read_miss(UINT32 pid, UINT32 home, UINT64 tag) {return 0;}
    virtual INT32 write_miss(UINT32 pid, UINT32 home, UINT64 tag) {return 0;}
    virtual INT32 data_write_back(UINT32 pid, UINT32 home) {return 0;}

public:
    std::vector<Profile> profiles;
    std::unordered_map<UINT64, Directory_Line>  _directory;
    UINT32  _num_processors;
};

/* ===================================================================== */
/*  @brief Cache Coherence Protocol MSI                                  */
/* ===================================================================== */
class DIR_MSI : public Coherence
{
public:
    DIR_MSI(UINT32 num_processors): Coherence(num_processors) {}
    virtual void process_read(UINT32 pid, UINT64 tag, UINT32 set_index) override;
    virtual void process_write(UINT32 pid, UINT64 tag, UINT32 set_index) override;

    virtual INT32 fetch(UINT32 pid, UINT32 home, UINT64 tag, HIT_MISS_TYPES &response) override;
    virtual INT32 fetch_and_invalidate(UINT32 pid, UINT32 home, UINT64 tag, HIT_MISS_TYPES &response) override;
    virtual INT32 read_miss(UINT32 pid, UINT32 home, UINT64 tag) override;
    virtual INT32 write_miss(UINT32 pid, UINT32 home, UINT64 tag) override;
    virtual INT32 data_write_back(UINT32 pid, UINT32 home) override
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
};

// on a processor read with SHARED/MODIFIED
INT32 DIR_MSI::fetch(UINT32 pid, UINT32 home, UINT64 tag, HIT_MISS_TYPES &response)
{
    INT32 cost = get_directory_cost(pid, home);
    Directory_Line &dir = get_directory_line(tag);
    assert(dir.state != CACHE_STATE::INVALID);

    if (dir.is_set(pid))  // requesting node is sharer/owner, no state change
    {
        response = CACHE_HIT;
        cost += LOCAL_CACHE_ACCESS;
    }
    else  // requesting node is not sharer/owner
    {
        response = CACHE_MISS;
        UINT32 owner = dir.owner(_num_processors);
        cost += MEMORY_ACCESS + (dir.state == CACHE_STATE::MODIFIED ? data_write_back(owner, home) : 0);
        dir.set_sharer(pid);
        dir.state = CACHE_STATE::SHARED;
    }

    return cost;
}

// on a processor write with SHARED/MODIFIED
INT32 DIR_MSI::fetch_and_invalidate(UINT32 pid, UINT32 home, UINT64 tag, HIT_MISS_TYPES &response)
{
    INT32 cost = get_directory_cost(pid, home);
    Directory_Line &dir = get_directory_line(tag);
    assert(dir.state != CACHE_STATE::INVALID);

    // fetch up-to-date data
    if (dir.is_set(pid))  // requesting node is sharer/owner, state changes to MODIFIED
    {
        response = CACHE_HIT;
        cost += LOCAL_CACHE_ACCESS;
    }
    else  // requesting node is not sharer/owner
    {
        response = CACHE_MISS;
        UINT32 owner = dir.owner(_num_processors);
        cost += MEMORY_ACCESS + (dir.state == CACHE_STATE::MODIFIED ? data_write_back(owner, home) : 0);
    }

    // invalidate other sharers and claim ownership
    dir.sharer_vector = 0;
    dir.set_sharer(pid);
    dir.state = CACHE_STATE::MODIFIED;

    return cost;
}

// on a processor read with INVALID
INT32 DIR_MSI::read_miss(UINT32 pid, UINT32 home, UINT64 tag)
{
    INT32 cost = get_directory_cost(pid, home) + MEMORY_ACCESS;
    Directory_Line &dir = get_directory_line(tag);
    assert(dir.state == CACHE_STATE::INVALID);

    dir.state = CACHE_STATE::SHARED;
    dir.set_sharer(pid);

    return cost;
}

// on a processor write with INVALID
INT32 DIR_MSI::write_miss(UINT32 pid, UINT32 home, UINT64 tag)
{
    INT32 cost = get_directory_cost(pid, home) + MEMORY_ACCESS;
    Directory_Line &dir = get_directory_line(tag);
    assert(dir.state == CACHE_STATE::INVALID);

    dir.state = CACHE_STATE::MODIFIED;
    dir.set_sharer(pid);

    return cost;
}

// processor read handler
void DIR_MSI::process_read(UINT32  pid, UINT64 tag, UINT32 set_index)
{
    INT32 cost = 0;
    HIT_MISS_TYPES response = CACHE_MISS;

    UINT32 home = get_home_node(tag);
    CACHE_STATE state = get_directory_line(tag).state;

    switch (state)
    {
        case CACHE_STATE::MODIFIED:
        case CACHE_STATE::SHARED:
            cost = fetch(pid, home, tag, response);
            break;

        case CACHE_STATE::INVALID:
            cost = read_miss(pid, home, tag);
            break;
        default:
            break;
    }

    profiles[pid]._access[0][response]++;
    profiles[pid]._cycle[0][response] += cost;
    profiles[pid]._cost += cost;
};

// processor write handler
void DIR_MSI::process_write(UINT32  pid, UINT64  tag, UINT32  set_index)
{
    INT32 cost = 0;
    HIT_MISS_TYPES response = CACHE_MISS;

    UINT32 home = get_home_node(tag);
    CACHE_STATE state = get_directory_line(tag).state;

    switch (state)
    {
        case CACHE_STATE::MODIFIED:
        case CACHE_STATE::SHARED:
            response = CACHE_HIT;
            cost = fetch_and_invalidate(pid, home, tag, response);
            break;

        case CACHE_STATE::INVALID:
            cost = write_miss(pid, home, tag);
            break;
        default:
            break;
    }

    profiles[pid]._access[1][response]++;
    profiles[pid]._cycle[1][response] += cost;
    profiles[pid]._cost += cost;
}

#endif // _COHERENCE_H_
