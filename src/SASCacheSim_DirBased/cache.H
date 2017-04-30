#pragma once

#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>

#include "coherence.H"

// number of bits to shift for address elements
UINT32 offset_num_bits = 0;
UINT32 set_index_num_bits = 0;

// bit masks for address extraction
UINT64 tag_mask = ~0;
UINT64 offset_mask = ~0;

/* ===================================================================== */
/*  @brief Cache Line - cache tag                                        */
/* ===================================================================== */
class Cache_Line
{
public:
    Cache_Line(const UINT64 &addr_tag)
    {
        lru = 0;
        tag = addr_tag;
    }

public:
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
    void fetch(UINT64 tag)
    {
        auto index = _current_index;
        if (index < 0)  // evict
        {
            index = evict();
            _lines[index].tag = tag;
        }
        else
        {
            _current_index = (_current_index == _associativity) ? -1 : _current_index + 1;
        }
        ++_lines[index].lru;
    }

private:
    INT32 evict()
    {
        UINT64 _min = _lines[0].lru;
        INT32 _evict = 0;

        for (UINT32 i = 1; i < _lines.size(); ++i)
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
class Cache
{
public:
    Cache(UINT32 associativity, UINT32 num_sets)
        : _associativity(associativity),
          _num_sets(num_sets)
    {
        sets = std::vector<Cache_Set>(num_sets, associativity);
    }

    void fetch_cache_line(UINT64 tag, UINT32 index)
    {
        sets[index].fetch(tag);
    }

public:
    std::vector<Cache_Set>  sets;

private:
    UINT32  _associativity;
    UINT32  _num_sets;

};

/* ===================================================================== */
/*  @brief Cache Controller - cache controller                           */
/* ===================================================================== */
class Controller
{
public:
    Controller(UINT32       num_processors,
               UINT32       num_sets,
               UINT32       line_size,
               UINT32       associativity)
             : _num_processors(num_processors),
               _line_size(line_size)
    {
        _cache_size = num_sets * associativity * line_size;
        cache = std::vector<Cache>(num_processors, Cache(associativity, num_sets));
        coherence = new DIR_MSI(num_processors);

        offset_num_bits = get_num_bits(line_size);
        set_index_num_bits = get_num_bits(num_sets);

        tag_mask = (tag_mask << (offset_num_bits + set_index_num_bits));
        offset_mask = ~(offset_mask << offset_num_bits);
    }

    ~Controller()
    {
        delete coherence;
    }

    void store_single_line(UINT64 addr, UINT32 pid)
    {
        UINT64 tag = get_tag(addr);
        UINT32 index = get_set_index(addr);
        cache[pid].fetch_cache_line(tag, index);
        coherence->process_write(pid, tag, index);
    }

    void load_single_line(UINT64 addr, UINT32 pid)
    {
        UINT64 tag = get_tag(addr);
        UINT32 index = get_set_index(addr);
        cache[pid].fetch_cache_line(tag, index);
        coherence->process_read(pid, tag, index);
    }

    std::string stat_to_string()
    {
        std::stringstream ss;
        // skip main process, starting from thread 1 since thread0 is the main application
        for (UINT32 i = 1; i < _num_processors; ++i)
        {
            ss << "+ Processor: " << i << " L1 Data Cache" << std::endl;
            ss << coherence->profiles[i].StatsLong("+ ") << std::endl;
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
        return (addr & tag_mask) >> (offset_num_bits + set_index_num_bits);
    }

    // get the number of bits needed to represent 'num'
    static inline UINT32 get_num_bits(UINT64 num)
    {
        UINT32 num_bits = 0;
        for (; num > 0; num /= 2, ++num_bits);
        return num_bits;
    }

public:
    Coherence * coherence;
    std::vector<Cache>  cache;

private:
    UINT32  _num_processors;
    UINT32  _cache_size;
    UINT32  _line_size;
};
