#include "coherence.H"

// on a processor read with SHARED/MODIFIED
int32_t DIR_MSI::fetch(uint32_t pid, uint32_t home, uint64_t tag, HIT_MISS_TYPES &response)
{
    int32_t cost = get_directory_cost(pid, home);
    Directory_Line &dir = get_directory_line(tag);
    assert(dir.state != CACHE_STATE::INVALID);

    // fetch up-to-date data
    if (dir.is_set(pid))  // requesting node is sharer/owner, no state change
    {
        response = CACHE_HIT;
        cost += LOCAL_CACHE_ACCESS;
    }
    else  // requesting node is not sharer/owner
    {
        response = CACHE_MISS;
        cost += MEMORY_ACCESS;
        if (dir.state == CACHE_STATE::MODIFIED)
        {
            uint32_t owner = dir.owner(_num_processors);
            cost += data_write_back(owner, home);
        }
    }

    dir.set_sharer(pid);
    dir.state = CACHE_STATE::SHARED;

    return cost;
}

void DIR_MSI::invalidate(uint32_t pid, uint64_t tag)
{
    Directory_Line &dir = get_directory_line(tag);
    dir.clear_sharer(pid);
    if (dir.sharer_vector == 0) // no sharers
    {
        dir.state = CACHE_STATE::INVALID;
    }
}

// on a processor write with SHARED/MODIFIED
int32_t DIR_MSI::fetch_and_invalidate(uint32_t pid, uint32_t home, uint64_t tag, HIT_MISS_TYPES &response)
{
    int32_t cost = fetch(pid, home, tag, response);
    Directory_Line &dir = get_directory_line(tag);
    assert(dir.state != CACHE_STATE::INVALID);

    if (proposed)
    {
        for (uint32_t i = 0; i < _num_processors; ++i)
        {
            dir.set_sharer(i);
        }
        dir.state = CACHE_STATE::SHARED;
    }
    else
    {
        // invalidate other sharers and claim ownership
        dir.sharer_vector = 0;
        dir.set_sharer(pid);
        dir.state = CACHE_STATE::MODIFIED;
    }
    return cost;
}

// on a processor read with INVALID
int32_t DIR_MSI::read_miss(uint32_t pid, uint32_t home, UINT64 tag)
{
    INT32 cost = get_directory_cost(pid, home) + MEMORY_ACCESS;
    Directory_Line &dir = get_directory_line(tag);
    assert(dir.state == CACHE_STATE::INVALID);

    dir.state = CACHE_STATE::SHARED;
    dir.set_sharer(pid);

    return cost;
}

// on a processor write with INVALID
int32_t DIR_MSI::write_miss(uint32_t pid, uint32_t home, uint64_t tag)
{
    int32_t cost = get_directory_cost(pid, home) + MEMORY_ACCESS;
    Directory_Line &dir = get_directory_line(tag);
    assert(dir.state == CACHE_STATE::INVALID);

    if (proposed)
    {
        for (uint32_t i = 0; i < _num_processors; ++i)
        {
            dir.set_sharer(i);
        }
        dir.state = CACHE_STATE::SHARED;
    }
    else
    {
        dir.sharer_vector = 0;
        dir.set_sharer(pid);
        dir.state = CACHE_STATE::MODIFIED;
    }
    return cost;
}

// processor read handler
void DIR_MSI::process_read(uint32_t  pid, uint64_t addr, uint64_t tag)
{
    int32_t cost = 0;
    HIT_MISS_TYPES response = CACHE_MISS;

    uint32_t home = get_home_node(tag);
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
    if (response == CACHE_MISS) {
        ++line_stat[addr].load.miss;
    }
    else
    {
        ++line_stat[addr].load.hit;
    }
    ++line_stat[addr].count;
};

// processor write handler
void DIR_MSI::process_write(uint32_t  pid, uint64_t addr, uint64_t tag)
{
    int32_t cost = 0;
    HIT_MISS_TYPES response = CACHE_MISS;

    uint32_t home = get_home_node(tag);
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
    if (response == CACHE_MISS) {
        ++line_stat[addr].store.miss;
    }
    else
    {
        ++line_stat[addr].store.hit;
    }
    ++line_stat[addr].count;
}
