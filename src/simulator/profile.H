// #pragma once

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <unordered_map>

typedef enum class
{
    CACHE_HIT,
    CACHE_MISS
}ACCESS_TYPE;

const uint64_t THRESHOLD = 100;

class Stat
{
public:
    Stat(): hits(0), misses(0), hit_cycles(0), miss_cycles(0), hops(0) {}

    inline std::string stat_to_string(const std::string &prefix, const std::string &type)
    {
        std::stringstream out;
        uint64_t total_accesses = hits + misses;
        uint64_t total_cycles = hit_cycles + miss_cycles;

        out << prefix << std::setw(25) << std::left << (type + "-Hits")
                      << std::setw(15) << std::left << hits
                      << std::setw(15) << std::left << (100.0 * hits / total_accesses)
                      << std::setw(15) << std::left << hit_cycles
                      << std::setw(10) << std::left << (100.0 * hit_cycles / total_cycles) << std::endl;

        out << prefix << std::setw(25) << std::left << (type + "-Misses:")
                      << std::setw(15) << std::left << misses
                      << std::setw(15) << std::left << (100.0 * misses / total_accesses)
                      << std::setw(15) << std::left << miss_cycles
                      << std::setw(10) << std::left << (100.0 * miss_cycles / total_cycles) << std::endl;

        out << prefix << std::setw(25) << std::left << (type + "-Total Accesses:")
                      << std::setw(15) << std::left << total_accesses
                      << std::setw(15) << std::left << 100.0
                      << std::setw(15) << std::left << total_cycles
                      << std::setw(10) << std::left << 100.0 << std::endl << std::endl;

        return out.str();
    }

public:
    uint64_t hits;
    uint64_t misses;
    uint64_t hit_cycles;
    uint64_t miss_cycles;
    uint64_t hops;
};

class Access_Stat
{
public:
    Access_Stat() : count(0) {}

    inline std::string stat_to_string(const std::string &prefix)
    {
        std::stringstream out;
        uint64_t total_hits = load.hits + store.hits;
        uint64_t total_misses = load.misses + store.misses;
        uint64_t total_accesses = total_hits + total_misses;

        uint64_t total_hit_cycles = load.hit_cycles + store.hit_cycles;
        uint64_t total_miss_cycles = load.miss_cycles + store.miss_cycles;
        uint64_t total_cycles = total_hit_cycles + total_miss_cycles;

        uint64_t total_hops = load.hops + store.hops;

        out << load.stat_to_string(prefix, "Load") << store.stat_to_string(prefix, "Store");

        out << prefix << std::setw(25) << std::left << "Total-Hits:"
                      << std::setw(15) << std::left << total_hits
                      << std::setw(15) << std::left << (100.0 * total_hits / total_accesses)
                      << std::setw(15) << std::left << total_hit_cycles
                      << std::setw(10) << std::left << (100.0 * total_hit_cycles / total_cycles) << std::endl;

        out << prefix << std::setw(25) << std::left << "Total-Misses:"
                      << std::setw(15) << std::left << total_misses
                      << std::setw(15) << std::left << (100.0 * total_misses / total_accesses)
                      << std::setw(15) << std::left << total_miss_cycles
                      << std::setw(10) << std::left << (100.0 * total_miss_cycles / total_cycles) << std::endl << std::endl;

        out << prefix << std::setw(25) << std::left << "Estimated-Cost:"
                      << std::setw(15) << std::left << total_accesses
                      << std::setw(15) << std::left << 100.0
                      << std::setw(15) << std::left << total_cycles
                      << std::setw(10) << std::left << 100.0 << std::endl;

        out << prefix << std::setw(25) << std::left << ("Load-Network-Msg:")
                      << std::setw(15) << std::left << load.hops
                      << std::setw(15) << std::left << (100.0 * load.hops / total_hops) << std::endl
            << prefix << std::setw(25) << std::left << ("Store-Network-Msg:")
                      << std::setw(15) << std::left << store.hops
                      << std::setw(10) << std::left << (100.0 * store.hops / total_hops) << std::endl
            << prefix << std::setw(25) << std::left << ("Total-Network-Msg:")
                      << std::setw(15) << std::left << total_hops
                      << std::setw(10) << std::left << 100.0  << std::endl << std::endl;

        return out.str();
    }

public:
    Stat load;
    Stat store;
    uint64_t count;
};

class Profile
{
public:
    Profile(uint32_t num_processors) : _num_processors(num_processors)
    {
        _profiles = std::vector<Access_Stat>(num_processors, Access_Stat());
    }

    inline void profile_cache_load(ACCESS_TYPE  &type,
                                   uint32_t     pid,
                                   uint64_t     addr,
                                   uint64_t     cost,
                                   uint64_t     hops)
    {
        if (type == ACCESS_TYPE::CACHE_HIT) {
            ++_line_stat[addr].load.hits;
            ++_profiles[pid].load.hits;
            _profiles[pid].load.hit_cycles += cost;
        } else {
            ++_line_stat[addr].load.misses;
            ++_profiles[pid].load.misses;
            _profiles[pid].load.miss_cycles += cost;
        }
        ++_line_stat[addr].count;
        _profiles[pid].load.hops += hops;
    }

    inline void profile_cache_store(ACCESS_TYPE   &type,
                                    uint32_t      pid,
                                    uint64_t      addr,
                                    uint64_t      cost,
                                    uint64_t      hops)
    {
        if (type == ACCESS_TYPE::CACHE_HIT) {
            ++_line_stat[addr].store.hits;
            ++_profiles[pid].store.hits;
            _profiles[pid].store.hit_cycles += cost;
        } else {
            ++_line_stat[addr].store.misses;
            ++_profiles[pid].store.misses;
            _profiles[pid].store.miss_cycles += cost;
        }
        ++_line_stat[addr].count;
        _profiles[pid].store.hops += hops;
    }

    inline std::string stats_to_string()
    {
        std::stringstream out;
        uint64_t all_hits = 0;
        uint64_t all_misses = 0;

        uint64_t all_hit_cycles = 0;
        uint64_t all_miss_cycles = 0;

        uint64_t all_loads = 0;
        uint64_t all_load_hops = 0;

        uint64_t all_hops = 0;
        uint64_t all_cycels = 0;

        // NOTE: starting from thread 1 if you want to skip main process
        for (uint32_t pid = 0; pid < _num_processors; ++pid)
        {
            all_hits += _profiles[pid].load.hits + _profiles[pid].store.hits;
            all_misses += _profiles[pid].load.misses + _profiles[pid].store.misses;

            all_loads += _profiles[pid].load.hits + _profiles[pid].load.misses;
            all_load_hops += _profiles[pid].load.hops;

            all_hit_cycles += _profiles[pid].load.hit_cycles + _profiles[pid].store.hit_cycles;
            all_miss_cycles += _profiles[pid].load.miss_cycles + _profiles[pid].store.miss_cycles;

            all_hops += _profiles[pid].load.hops + _profiles[pid].store.hops;

            out << "+ Processor: " << pid << " L1 Data Cache" << std::endl
                << _profiles[pid].stat_to_string("+ ") << std::endl;
        }
        all_cycels += all_hit_cycles + all_miss_cycles;

        out << std::setw(25) << std::left << "+ All-Hits:"
            << std::setw(10) << std::right << all_hits
            << std::setw(10) << std::right << (100.0 * all_hits / (all_hits + all_misses)) << "%" << std::endl
            << std::setw(25) << std::left << "+ All-Misses:"
            << std::setw(10) << std::right << all_misses
            << std::setw(10) << std::right << (100.0 * all_misses / (all_hits + all_misses)) << "%" << std::endl
            << std::setw(25) << std::left << "+ All-Hit-Cycles:"
            << std::setw(10) << std::right << all_hit_cycles
            << std::setw(10) << std::right << (100.0 * all_hit_cycles / all_cycels) << "%" << std::endl
            << std::setw(25) << std::left << "+ All-Miss-Cycles:"
            << std::setw(10) << std::right << all_miss_cycles
            << std::setw(10) << std::right << (100.0 * all_miss_cycles / all_cycels) << "%" << std::endl
            << std::setw(25) << std::left << "+ All-Cycles:"
            << std::setw(10) << std::right << all_cycels << std::endl
            << std::setw(25) << std::left << "+ Avg-Network-Msg-Load:"
            << std::setw(10) << std::right << (1.0 * all_load_hops / all_loads) << std::endl
            << std::setw(25) << std::left << "+ All-Hops:"
            << std::setw(10) << std::right << all_hops << std::endl << std::endl;

        out << line_stat_to_string() << std::endl;
        return out.str();
    }

private:
    inline std::string line_stat_to_string()
    {
        std::stringstream out;
        out << "Memory Stats:" << std::endl;
        out << std::setw(15) << std::left << "Addr"
            << std::setw(10) << std::left << "Load Hit" << std::setw(10) << std::left << "Load Miss"
            << std::setw(10) << std::left << "Store Hit" << std::setw(10) << std::left << "Store Miss" << std::endl;

        for (const auto & p : _line_stat)
        {
            if (p.second.count <= THRESHOLD) {
                continue;
            }
            out << std::setw(15) << std::left << std::hex << p.first << std::dec
                << std::setw(10) << std::left << p.second.load.hits
                << std::setw(10) << std::left << p.second.load.misses
                << std::setw(10) << std::left << p.second.store.hits
                << std::setw(10) << std::left << p.second.store.misses << std::endl;
        }
        return out.str();
    }

private:
    std::vector<Access_Stat> _profiles;
    std::unordered_map<uint64_t, Access_Stat> _line_stat;
    uint32_t _num_processors;
};
