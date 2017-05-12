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
    Stat(): hits(0), misses(0), hit_cycles(0), miss_cycles(0) {}

    inline std::string stat_to_string(const std::string &prefix, const std::string &type)
    {
        std::stringstream out;
        uint64_t total_accesses = hits + misses;
        uint64_t total_cycles = hit_cycles + miss_cycles;

        out << prefix << std::setw(25) << std::left << (type + "-Hits")
                      << std::setw(15) << std::left << hits
                      << std::setw(15) << std::left << (100.0 * hits / total_accesses)
                      << std::setw(15) << std::left << hit_cycles
                      << std::setw(10) << std::left << (100.0 * hit_cycles / total_cycles)  << "\n";

        out << prefix << std::setw(25) << std::left << (type + "-Misses:")
                      << std::setw(15) << std::left << misses
                      << std::setw(15) << std::left << (100.0 * misses / total_accesses)
                      << std::setw(15) << std::left << miss_cycles
                      << std::setw(10) << std::left << (100.0 * miss_cycles / total_cycles) << "\n";

        out << prefix << std::setw(25) << std::left << (type + "-Total Accesses:")
                      << std::setw(15) << std::left << total_accesses
                      << std::setw(15) << std::left << 100.0
                      << std::setw(15) << std::left << total_cycles
                      << std::setw(10) << std::left << 100.0 << "\n\n";

        return out.str();
    }

public:
    uint64_t hits = 0;
    uint64_t misses = 0;
    uint64_t hit_cycles = 0;
    uint64_t miss_cycles = 0;
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


        out << load.stat_to_string(prefix, "Load") << store.stat_to_string(prefix, "Store");

        out << prefix << std::setw(25) << std::left << "Total-Hits:"
                      << std::setw(15) << std::left << total_hits
                      << std::setw(15) << std::left << (100.0 * total_hits / total_accesses)
                      << std::setw(15) << std::left << total_hit_cycles
                      << std::setw(10) << std::left << (100.0 * total_hit_cycles / total_cycles) << "\n";

        out << prefix << std::setw(25) << std::left << "Total-Misses:"
                      << std::setw(15) << std::left << total_misses
                      << std::setw(15) << std::left << (100.0 * total_misses / total_accesses)
                      << std::setw(15) << std::left << total_miss_cycles
                      << std::setw(10) << std::left << (100.0 * total_miss_cycles / total_cycles) << "\n\n";

        out << prefix << std::setw(25) << std::left << "Estimated Cost:"
                      << std::setw(15) << std::left << total_accesses
                      << std::setw(15) << std::left << 100.0
                      << std::setw(15) << std::left << total_cycles
                      << std::setw(10) << std::left << 100.0 << "\n\n";

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

    inline std::string line_stat_to_string()
    {
        std::stringstream out;
        out << "Memory Stats:\n";
        out << "\t\tAddr\tLoad Hit\tLoad Miss\tStore Hit\tStore Miss\n";
        for (const auto& p : _line_stat)
        {
            if (p.second.count <= THRESHOLD) {
                continue;
            }
            out << std::hex << p.first << std::dec << "\t\t"
               << p.second.load.hits << "\t" << p.second.load.misses << "\t"
               << p.second.store.hits << "\t"<< p.second.store.misses << "\n";
        }
        return out.str();
    }

    inline std::string stats_to_string()
    {
        std::stringstream out;
        // skip main process, starting from thread 1 since thread0 is the main application
        for (uint32_t pid = 0; pid < _num_processors; ++pid)
        {
            out << "+ Processor: " << pid << " L1 Data Cache" << std::endl
                << _profiles[pid].stat_to_string("+ ") << std::endl;
        }
        out << line_stat_to_string() << std::endl;
        return out.str();
    }

    inline void profile_cache_load(ACCESS_TYPE &type, uint32_t pid, uint64_t addr, int32_t cost)
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
    }

    inline void profile_cache_store(ACCESS_TYPE &type, uint32_t pid, uint64_t addr, int32_t cost)
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
    }

private:
    std::vector<Access_Stat> _profiles;
    std::unordered_map<uint64_t, Access_Stat> _line_stat;
    uint32_t _num_processors;
};
