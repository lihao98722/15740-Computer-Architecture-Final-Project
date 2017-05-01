#pragma once
// #ifndef CACHE_H
// #define CACHE_H

#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>

// #include "callbacks.H"
#include "coherence.H"


/* ===================================================================== */
/*  @brief Cache Line - cache tag                                        */
/* ===================================================================== */
class Cache_Line
{
public:
    Cache_Line()
    {
        lru = 0;
        tag = 0;
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
    Cache_Set(INT32 associativity)
            : _associativity(associativity),
              _current_index(0)
    {
        _lines = std::vector<Cache_Line>(associativity, Cache_Line());
        std::cout << "lines: " << _lines.size() << std::endl;
    }

    /* ===================================================================== */
    void fetch(UINT64 tag)
    {
        // find cache line
        for (auto & _line : _lines)
        {
            if (_line.tag == tag)
            {
                ++_line.lru;
                return;
            }
        }

        // cache miss
        auto index = _current_index;
        if (index < 0)  // evict
        {
            index = evict();
        }
        else // replace uncached line
        {
            if (++_current_index == _associativity)
            {
                _current_index = -1;
            }
        }

        #ifdef DEBUG
        std::cout<< "index: " << _current_index << std::endl;
        std::cout<< "tag: " << tag << std::endl;
        std::cout<< "size: " << _lines.size() << std::endl;
        std::cout<< "lru: " << _lines[index].lru << std::endl;
        #endif

        _lines[index].tag = tag;
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
        sets = std::vector<Cache_Set>(num_sets, Cache_Set(associativity));
        // std::cout << "sets : " << sets.size() << std::endl;
    }

    void fetch_cache_line(UINT64 tag, INT32 index)
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

        #ifdef DEBUG
        std::cout<< "offset_num_bits: " << offset_num_bits << std::endl;
        std::cout<< "set_index_num_bits: " << set_index_num_bits << std::endl;
        #endif
    }

    ~Controller()
    {
        delete coherence;
    }

    void store_single_line(UINT64 addr, UINT32 pid)
    {
        // std::cout << "store pid :" << pid << std::endl;
        UINT64 tag = get_tag(addr);
        UINT32 index = get_set_index(addr);

        cache[pid].fetch_cache_line(tag, index);
        coherence->process_write(pid, tag, index);
    }

    void load_single_line(UINT64 addr, UINT32 pid)
    {
        // std::cout << "load pid :" << pid << std::endl;
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
    inline UINT64 get_set_index(UINT64 addr)
    {
        return (addr & (~tag_mask)) >> offset_num_bits;
    }

    // extract tag infomation out of a memory address
    inline UINT64 get_tag(UINT64 addr)
    {
        return (addr & tag_mask) >> (offset_num_bits + set_index_num_bits);
    }

    // get the number of bits needed to represent 'num'
    inline UINT32 get_num_bits(UINT64 num)
    {
        UINT32 num_bits = 0;
        for (; num > 0; num /= 2, ++num_bits);
        return num_bits - 1;
    }

public:
    DIR_MSI * coherence;
    std::vector<Cache>  cache;

private:
    UINT32  _num_processors;
    UINT32  _cache_size;
    UINT32  _line_size;

    // number of bits to shift for address elements
    UINT32 offset_num_bits = 0;
    UINT32 set_index_num_bits = 0;

    // bit masks for address extraction
    UINT64 tag_mask = ~0;
    UINT64 offset_mask = ~0;
};
