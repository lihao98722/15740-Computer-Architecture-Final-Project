#pragma once

#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>

#include "coherence.H"

const uint64_t DEFAULT_TAG = 0xFFFFFFFFFFFFFFFF;

enum class LOCAL_STATUS
{
    UNCACHED,
    VALID,
    INVALID
};

/* ===================================================================== */
/*  @brief Cache Line - cache tag                                        */
/* ===================================================================== */
class Cache_Line
{
public:
    Cache_Line(): lru(0), tag(DEFAULT_TAG), addr(0), status(LOCAL_STATUS::UNCACHED){}

public:
    uint64_t lru;
    uint64_t tag;
    uint64_t addr;
    LOCAL_STATUS status;
};

/* ===================================================================== */
/*  @brief Cache Set - cache set with LRU replacement                    */
/* ===================================================================== */
class Cache_Set
{
public:
    Cache_Set(int32_t associativity) : _associativity(associativity)
    {
        _lines = std::vector<Cache_Line>(associativity, Cache_Line());
    }

    // find cache line, return line status
    inline LOCAL_STATUS fetch(uint64_t tag, DIR_MSI *coherence, bool replace)
    {
        for (auto & _line : _lines)
        {
            if (_line.tag == tag)
            {
                if (replace) {
                    ++_line.lru;
                }
                return _line.status;
            }
        }
        return LOCAL_STATUS::UNCACHED;
    }

    // simulate fetching single cache line, return victim tag if evict triggered
    inline void fetch_single_line(uint32_t  pid,
                                  uint64_t  tag,
                                  uint64_t  addr,
                                  DIR_MSI   *coherence,
                                  bool      replace)
    {
        LOCAL_STATUS status = fetch(tag, coherence, replace);
        if (status == LOCAL_STATUS::UNCACHED)
        {
            int32_t index = evict(pid, coherence);
            if (replace) {
                ++_lines[index].lru;
            }
            _lines[index].tag = tag;
            _lines[index].addr = addr;
            _lines[index].status = LOCAL_STATUS::VALID;
        }

        #ifdef DEBUG
        std::cout<< "index: " << _current_index << std::endl;
        std::cout<< "tag: " << tag << std::endl;
        std::cout<< "size: " << _lines.size() << std::endl;
        std::cout<< "lru: " << _lines[index].lru << std::endl;
        #endif
    }

private:
    // evict cache line and update directory
    inline int32_t evict(uint32_t pid, DIR_MSI* coherence)
    {
        uint64_t _min = _lines[0].lru;
        uint64_t _evict_addr = 0;
        int32_t _evict = 0;

        for (uint32_t i = 1; i < _lines.size(); ++i)
        {
            if (_lines[i].lru < _min)
            {
                _evict = i;
                _evict_addr = _lines[i].addr;
                _min = _lines[i].lru;
            }
        }
        if (_lines[_evict].status != LOCAL_STATUS::UNCACHED)
        {
            coherence->invalidate(pid, _evict_addr);
        }
        return _evict;
    }

private:
    int32_t   _associativity;  // number of lines per cache set
    std::vector<Cache_Line>  _lines;

};

/* ===================================================================== */
/*  @brief Cache - cache                                                 */
/* ===================================================================== */
class Cache
{
public:
    Cache(uint32_t associativity, uint32_t num_sets)
        : _associativity(associativity),
          _num_sets(num_sets)
    {
        sets = std::vector<Cache_Set>(num_sets, Cache_Set(associativity));
    }

public:
    std::vector<Cache_Set>  sets;

private:
    uint32_t  _associativity;
    uint32_t  _num_sets;

};

/* ===================================================================== */
/*  @brief Cache Controller - cache controller                           */
/* ===================================================================== */
class Controller
{
public:
    Controller(uint32_t       num_processors,
               uint32_t       num_sets,
               uint32_t       line_size,
               uint32_t       associativity)
             : _num_processors(num_processors),
               _line_size(line_size)
    {
        _cache_size = num_sets * associativity * line_size;
        cache = std::vector<Cache>(num_processors, Cache(associativity, num_sets));
        coherence = new DIR_MSI(num_processors);
        offset_num_bits = get_num_bits(line_size);
        set_index_num_bits = get_num_bits(num_sets);

        tag_mask = (0xFFFFFFFFFFFFFFFF << (offset_num_bits + set_index_num_bits));
        offset_mask = ~(0xFFFFFFFFFFFFFFFF << offset_num_bits);

        #ifdef DEBUG
        std::cout << "offset_num_bits: " << offset_num_bits << std::endl;
        std::cout << "set_index_num_bits: " << set_index_num_bits << std::endl;
        #endif
    }

    ~Controller()
    {
        delete coherence;
    }

    inline void store_single_line(uint64_t addr, uint32_t pid)
    {
        fetch_cache_line(pid, addr, true);
        coherence->process_write(pid, addr, this);
    }

    inline void load_single_line(uint64_t addr, uint32_t pid)
    {
        fetch_cache_line(pid, addr, true);
        coherence->process_read(pid, addr);
    }

    inline void fetch_cache_line(uint32_t pid, uint64_t addr, bool replace)
    {
        uint64_t tag = get_tag(addr);
        uint32_t index = get_set_index(addr);
        cache[pid].sets[index].fetch_single_line(pid, tag, addr, coherence, replace);
    }

    inline std::string stat_to_string()
    {
        std::stringstream ss;
        // skip main process, starting from thread 1 since thread0 is the main application
        for (uint32_t i = 0; i < _num_processors; ++i)
        {
            ss << "+ Processor: " << i << " L1 Data Cache" << std::endl;
            ss << coherence->profiles[i].stats_to_string("+ ") << std::endl;
        }
        ss << coherence->stat_to_string() << std::endl;
        return ss.str();
    }

private:
    // extract line information out of a memory address
    inline uint64_t get_set_index(uint64_t addr)
    {
        return (addr & (~tag_mask)) >> (offset_num_bits);
    }

    // extract tag infomation out of a memory address
    inline uint64_t get_tag(uint64_t addr)
    {
        return (addr & tag_mask) >> (offset_num_bits + set_index_num_bits);
    }

    // get the number of bits needed to represent 'num', NOTE: num is power of 2
    inline uint32_t get_num_bits(uint64_t num)
    {
        uint32_t num_bits = 0;
        for (; num > 0; num /= 2, ++num_bits);
        return num_bits - 1;
    }

public:
    DIR_MSI * coherence;
    std::vector<Cache>  cache;

private:
    uint32_t  _num_processors;
    uint32_t  _cache_size;
    uint32_t  _line_size;

    // number of bits to shift for address elements
    uint32_t offset_num_bits;
    uint32_t set_index_num_bits;

    // bit masks for address extraction
    uint64_t tag_mask;
    uint64_t offset_mask;
};
