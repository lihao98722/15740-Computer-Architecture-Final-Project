#include "coherence.H"
#include "cache.H"

// on a processor read with SHARED/MODIFIED
uint64_t DIR_MSI::fetch(uint32_t      pid,
                        uint32_t      home,
                        uint64_t      addr,
                        uint64_t      &hops,
                        ACCESS_TYPE   &response)
{
    uint64_t cost = get_directory_cost(pid, home, hops);
    Directory_Line &dir = get_directory_line(addr);
    assert(dir.state != CACHE_STATE::INVALID);

    // fetch up-to-date data
    if (dir.is_set(pid)) { // requesting node is sharer/owner, no state change
        response = ACCESS_TYPE::CACHE_HIT;
        cost += LOCAL_CACHE_ACCESS;
    } else { // requesting node is not sharer/owner
        response = ACCESS_TYPE::CACHE_MISS;
        cost += MEMORY_ACCESS;
        if (dir.state == CACHE_STATE::MODIFIED)
        {
            uint32_t owner = dir.owner(_num_processors);
            cost += data_write_back(owner, home, hops);
        }
        dir.set_sharer(pid);
        dir.state = CACHE_STATE::SHARED;
    }

    return cost;
}

// on cache eviction, invalidate directory line
void DIR_MSI::invalidate(uint32_t pid, uint64_t addr)
{
    Directory_Line &dir = get_directory_line(addr);
    dir.clear_sharer(pid);
    if (dir.sharer_vector == 0) { // no sharers
       dir.state = CACHE_STATE::INVALID;
    }
}

// on a processor write with SHARED/MODIFIED (without detector)
uint64_t DIR_MSI::fetch_and_invalidate(uint32_t     pid,
                                       uint32_t     home,
                                       uint64_t     addr,
                                       uint64_t     &hops,
                                       ACCESS_TYPE  &response)
{
    uint64_t cost = fetch(pid, home, addr, hops, response);
    Directory_Line &dir = get_directory_line(addr);
    assert(dir.state != CACHE_STATE::INVALID);

    // invalidate other sharers and claim ownership
    for (uint32_t i = 0; i < _num_processors; ++i)
    {
        if (dir.is_set(i) && i != pid) {  // NOTE: update hops
            get_directory_cost(i, home, hops);
        }
    }
    dir.sharer_vector = 0;
    dir.set_sharer(pid);
    dir.state = CACHE_STATE::MODIFIED;

    return cost;
}

// on a processor write with SHARED/MODIFIED (with detector), speculatively push data to qualified readers
uint64_t DIR_MSI::push_and_invalidate(uint32_t     pid,
                                      uint32_t     home,
                                      uint64_t     addr,
                                      uint64_t     &hops,
                                      ACCESS_TYPE  &response,
                                      Controller   *controller)
{
    uint64_t cost = get_directory_cost(pid, home, hops);
    Directory_Line &dir = get_directory_line(addr);
    assert(dir.state != CACHE_STATE::INVALID);

    // requesting node is the last writer
    if (dir.is_last_writer(pid) && dir.is_set(pid)) {
        for (uint32_t i = 0; i < _num_processors; ++i)
        {
            if (i != pid) {
                if (dir.qualified_reader(i)) {
                    dir.set_sharer(i);
                    controller->fetch_cache_line(i, addr, false);
                    cost += CACHE_TO_CACHE;
                    get_directory_cost(i, home, hops);  // NOTE: update hops
                } else {
                    dir.clear_sharer(i);
                }
            }
        }
        response = ACCESS_TYPE::CACHE_HIT;
        cost += LOCAL_CACHE_ACCESS;
        dir.state = CACHE_STATE::MODIFIED;
    } else {
        if (!dir.is_last_writer(pid)) {
            for (uint32_t i = 0; i < _num_processors; ++i)
            {
                if (i != pid && dir.qualified_reader(i)) {
                    dir.decrease_read_count(i);
                }
            }
            dir.update_last_writer(pid);
        }else {
            //NOTE: in case last writer is evicted
            controller->fetch_cache_line(pid, addr, true);
        }
        cost = fetch_and_invalidate(pid, home, addr, hops, response);
    }

    return cost;
}

// on a processor read with INVALID
uint64_t DIR_MSI::read_miss(uint32_t   pid,
                            uint32_t   home,
                            uint64_t   addr,
                            uint64_t   &hops)
{
    uint64_t cost = get_directory_cost(pid, home, hops) + MEMORY_ACCESS;
    Directory_Line &dir = get_directory_line(addr);
    assert(dir.state == CACHE_STATE::INVALID);
    dir.state = CACHE_STATE::SHARED;
    dir.set_sharer(pid);
    return cost;
}

// on a processor write with INVALID
uint64_t DIR_MSI::write_miss(uint32_t  pid,
                             uint32_t  home,
                             uint64_t  addr,
                             uint64_t  &hops)
{
    uint64_t cost = get_directory_cost(pid, home, hops) + MEMORY_ACCESS;
    Directory_Line &dir = get_directory_line(addr);
    assert(dir.state == CACHE_STATE::INVALID);
    dir.state = CACHE_STATE::MODIFIED;
    dir.set_sharer(pid);
    return cost;
}

// processor read handler
void DIR_MSI::process_read(uint32_t pid, uint64_t addr)
{
    uint64_t hops = 0;
    uint64_t cost = 0;
    ACCESS_TYPE response = ACCESS_TYPE::CACHE_MISS;

    uint32_t home = get_home_node(addr);
    auto &dir_line = get_directory_line(addr);
    CACHE_STATE state = dir_line.state;

    switch (state)
    {
        case CACHE_STATE::MODIFIED:
        case CACHE_STATE::SHARED:
            cost = fetch(pid, home, addr, hops, response);
            break;

        case CACHE_STATE::INVALID:
            cost = read_miss(pid, home, addr, hops);
            break;

        default:
            break;
    }

    if (response == ACCESS_TYPE::CACHE_MISS) {
        dir_line.increase_read_count(pid);
    }

    profiles->profile_cache_load(response, pid, addr, cost, hops);
};

// processor write handler
void DIR_MSI::process_write(uint32_t   pid,
                            uint64_t   addr,
                            Controller *controller)
{
    uint64_t hops = 0;
    uint64_t cost = 0;
    ACCESS_TYPE response = ACCESS_TYPE::CACHE_MISS;

    uint32_t home = get_home_node(addr);
    auto &dir_line = get_directory_line(addr);
    CACHE_STATE state = dir_line.state;

    switch (state)
    {
        case CACHE_STATE::MODIFIED:
        case CACHE_STATE::SHARED:
            response = ACCESS_TYPE::CACHE_HIT;
            if (detector) {
                cost = push_and_invalidate(pid, home,  addr, hops, response, controller);
            } else {
                cost = fetch_and_invalidate(pid, home, addr, hops, response);
            }
            break;

        case CACHE_STATE::INVALID:
            dir_line.update_last_writer(pid);
            cost = write_miss(pid, home, addr, hops);
            break;

        default:
            break;
    }

    profiles->profile_cache_store(response, pid, addr, cost, hops);
}
