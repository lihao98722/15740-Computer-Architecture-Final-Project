#pragma once

#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>

// number of bits to shift for address elements
UINT32 offset_num_bits = 0;

// bit masks for address extraction
UINT64 tag_mask = ~0;
UINT64 offset_mask = ~0;

enum class CACHE_STATE
{
    UNCACHED,
    INVALID,
    SHARED,
    MODIFIED,
};

/* ===================================================================== */
/*  @brief Cache Line - cache tag                                        */
/* ===================================================================== */
class Cache_Line
{
public:
    Cache_Line(const UINT64 &addr_tag)
    {
        state = CACHE_STATE::UNCACHED;
        lru = 0;
        tag = addr_tag;
    }

public:
    CACHE_STATE  state;
    UINT64  lru;
    UINT64  tag;
};


/* ===================================================================== */
/*  @brief Cache Set - cache set with LRU replacement                    */
/* ===================================================================== */
class Cache_Set
{
public:
    Cache_Set(UINT32 associativity)
            : _associativity(associativity),
              _current_index(0)
    {
        _lines = std::vector<Cache_Line>(_associativity, Cache_Line(0));
    }

    /* ===================================================================== */
    CACHE_STATE find(UINT64 tag)
    {
        for (auto & line : _lines)
        {
            if (line.tag == tag)
            {
                ++line.lru;
                return line.state;
            }
        }
        return CACHE_STATE::UNCACHED;
    }

    /* ===================================================================== */
    void insert(UINT64 tag, CACHE_STATE state)
    {
        auto index = _current_index;
        if (index < 0)  // evict
        {
            index = evict();
        }
        else
        {
            _current_index = _current_index == _num_lines ? -1 : _current_index + 1;
        }

        _lines[index].state = state;
        _lines[index].lru = 0;
        _lines[index].tag = tag;
    }

private:
    INT32 evict()
    {
        UINT64 _min = _line[0].lru;
        INT32 _evict = 0;

        for (auto i = 1; i < _lines.size(); ++i)
        {
            if (_lines[i].lru < _min)
            {
                _evict = i;
                _min = _lines[i].lru;
            }
        }

        return _evict;
    }

private:
    INT32  _associativity;  // number of lines per cache set
    INT32  _current_index;  // first uncached index

    std::vector<Cache_Line>  _lines;
};


/* ===================================================================== */
/*  @brief Cache - cache                                                 */
/* ===================================================================== */
#include "coherence.H"
class Cache
{
public:
    Cache(UINT32 associativity, UINT32 num_sets)
        : _associativity(associativity),
          _num_sets(num_sets)
    {
        sets = std::vector<Cache_Set>(num_sets, associativity);
    }

public:
    std::vector<Cache_Set>  sets;

private:
    const UINT32  _associativity;
    const UINT32  _num_sets;

};


/* ===================================================================== */
/*  @brief Cache Controller - cache controller                           */
/* ===================================================================== */
#include "profile.H"
class Controller
{
public:
    Controller(UINT32       num_processors,
               UINT32       num_sets,
               UINT32       line_size,
               UINT32       associativity,
               UINT32       coherence_write,
               UINT32       coherence_protocol,
               UINT32       interconnection_type)
             : _num_processors(num_processors),
               _cache_size(cache_size),
               _line_size(line_size)
    {
        _cache_size = num_sets * associativity * line_size;
        cache = std::vector<Cache>(num_processors, Cache(associativity, num_sets));
        coherence = DIR_MSI<SET,Cache>(cache);
        profiles = std::vector<Profile>(num_processors, Profile(_cache_size, line_size, associativity));

        offset_num_bits = get_num_bits(line_size);
        set_index_num_bits = get_num_bits(num_sets);

        tag_mask = (tag_mask << (offset_num_bits + set_index_num_bits));
        offset_mask = ~(offset_mask << offset_num_bits);
    }

    /* ===================================================================== */
    void store_single_line(UINT64 addr, UINT32 pid)
    {
        UINT64 tag = get_tag(addr);
        UINT32 set_index = get_set_index(addr);
        Cache_Set set = _cache[pid].set[set_index];

        INT32 cost = 0;
        HIT_MISS_TYPES hit = coherence.process_write(set, tag, &cost);

        profiles[pid]._access[ACCESS_TYPE_STORE][hit]++;
        profiles[pid]._cycle[ACCESS_TYPE_STORE][hit] += cost;
        profiles[pid]._cost += cost;
    }

    /* ===================================================================== */
    void load_single_line(UINT64 addr, UINT32 pid)
    {
        UINT64 tag = get_tag(addr);
        UINT32 set_index = get_set_index(addr);
        Cache_Set set = _cache[pid].set[set_index];

        INT32 cost = 0;
        HIT_MISS_TYPES hit = coherence.process_read(set, tag, &cost);

        profiles[pid]._access[ACCESS_TYPE_LOAD][hit]++;
        profiles[pid]._cycle[ACCESS_TYPE_LOAD][hit] += cost;
        profiles[pid]._cost += cost;
    }

    std::string stat_to_string()
    {
        std::stringstream ss;
        // Skip main process, starting from thread 1 since thread0 is the main application
        for (auto i = 1; i < _num_processors; ++i)
        {
            ss << "+ Processor: " << i << " L1 Data Cache" << std::endl;
            ss << profiles[i].StatsLong("+ ") << std::endl;
        }

        return ss.str();
    }

private:
    // extract line information out of a memory address
    static inline UINT64 get_set_index(UINT64 addr)
    {
        return (addr & (~tag_mask)) >> offset_num_bits;
    }

    // extract tag infomation out of a memory address
    static inline UINT64 get_tag(UINT64 addr)
    {
        return addr & tag_mask;
    }

    // get the number of bits needed to represent 'num'
    static inline UINT32 get_num_bits(UINT64 num)
    {
        UINT32 num_bits = 0;
        for (; num > 0; num /= 2)
        {
            ++num_bits;
        }
        return num_bits;
    }

public:
    Coherence<Cache_Set, Cache>  coherence;
    std::vector<Cache>  cache;
    std::vector<Profile>   profiles;

private:
    UINT32  _num_processors;
    UINT32  _cache_size;
    UINT32  _line_size;
}
