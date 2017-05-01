#include "coherence.H"

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
        cost += MEMORY_ACCESS;
        if (dir.state == CACHE_STATE::MODIFIED)
        {
            UINT32 owner = dir.owner(_num_processors);
            cost += data_write_back(owner, home);
        }
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
        cost += MEMORY_ACCESS;
        if (dir.state == CACHE_STATE::MODIFIED)
        {
            UINT32 owner = dir.owner(_num_processors);
            cost += data_write_back(owner, home);
        }
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
