//
// @ORIGINAL_AUTHOR: Gang-Ryung Uh
// @EXTENDED: Adrian Perez (adrian.perez@hp.com)
//
/*! @file
 *  This file contains callback routines for cache simulator
 */

#include "callbacks.H"
#include "cache.H"
#include "coherence.H"
#include <assert.h>
#include <iostream>
#include <fstream>
#include <list>
#include <unordered_map>
#include <map>
#include <vector>

extern KNOB<string> KnobOutputFile;
extern KNOB<BOOL>   KnobNoSharedLibs;
extern CACHE_CONFIG l1_config;

class Stat
{
public:
    UINT64 loads = 0;
    UINT64 stores = 0;
};

std::unordered_map<ADDRINT, Stat> _mem;

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */
Controller controller;
std::map<UINT32, UINT32> t_map;                         // Mapping which Thread Belongs to Which processor
UINT32 active_threads = 0;
UINT32 nextPID = 0;
INT32 totalBitsToShift;
INT32 processorsMask;
UINT32 pow2processors;

int lock_id = 1;
PIN_LOCK mapLock;


/* ===================================================================== */
static inline INT32 floor_log2(UINT32 n)
{
    INT32 p = 0;

    if (n == 0) return -1;
    if (n & 0xffff0000) { p += 16; n >>= 16; }
    if (n & 0x0000ff00)	{ p +=  8; n >>=  8; }
    if (n & 0x000000f0) { p +=  4; n >>=  4; }
    if (n & 0x0000000c) { p +=  2; n >>=  2; }
    if (n & 0x00000002) { p +=  1; }

    return p;
}

/* ===================================================================== */
static inline INT32 ceil_log2(UINT32 n)
{
    return floor_log2(n - 1) + 1;
}

/* ===================================================================== */
UINT32 get_home_node(CACHE_TAG tag)
{
    return ((tag >> totalBitsToShift) & processorsMask);
}

/* ===================================================================== */
UINT32 get_pid(UINT32 tid)
{
    std::map<UINT32, UINT32>::iterator t_map_it = t_map.find(tid);
    return t_map_it->second;
}

/* ===================================================================== */
VOID cache_load(UINT32 tid, ADDRINT pin_addr)
{
    PIN_GetLock(&mapLock, lock_id++);
    UINT64 addr = reinterpret_cast<UINT64>(pin_addr);
    UINT32 pid = get_pid(tid);
    controller.cache[pid].load_single_line(addr, pid);
    increase_load(pin_addr);
    PIN_ReleaseLock(&mapLock);
}

/* ===================================================================== */
VOID cache_store(UINT32 tid, ADDRINT pin_addr)
{
    PIN_GetLock(&mapLock, lock_id++);
    UINT64 addr = reinterpret_cast<UINT64>(pin_addr);
    UINT32 pid = get_pid(tid);
    controller.cache[pid].store_single_line(addr, pid);
    increase_store(pin_addr);
    PIN_ReleaseLock(&mapLock);
}

/* ===================================================================== */
inline UINT32 get_current_tid()
{
    return PIN_ThreadId();
}

/* ===================================================================== */
inline UINT32 get_next_pid()
{
    // Round Robbin, condition typically faster than modulo
    return nextPID == pow2processors ? 0 : nextPID++;
}

/* ===================================================================== */
void process_attach()
{
    pow2processors = 1 << ceil_log2(catch_all_config.total_processors);
    processorsMask = pow2processors-1;
    // Creates Processors
    controller = Controller(catch_all_config.total_processors,
                            l1_config.num_sets,
                            l1_config.line_size,
                            l1_config.set_size,
                            l1_config.write,
                            l1_config.coherence,
                            l1_config.interconnect);

    PIN_GetLock(&mapLock, lock_id++);
    active_threads++;
    PIN_ReleaseLock(&mapLock);
}

/* ===================================================================== */
VOID process_detach()
{
    std::ofstream out(KnobOutputFile.Value().c_str());
    out << controller.stat_to_string();
    out << stat_to_string();
    out.close();
}

/* ===================================================================== */
VOID thread_attach()
{
    auto temp_tid = get_current_tid();
    auto temp_pid = get_next_pid();
    std::cout << "tid " << temp_tid << " -> " << "pid " << temp_pid << std::endl;
    (VOID)t_map.insert(std::pair<UINT32, UINT32>(temp_tid, temp_pid));
}

VOID thread_detach()
{
    auto tid = get_current_tid();
    std::map<UINT32, UINT32>::iterator t_map_it = t_map.find(tid);
    if (t_map_it != t_map.end())
    {
        (VOID)t_map.erase(t_map_it);
    }
}

void increaseLoad(ADDRINT addr)
{
    ++_mem[addr].loads;
}

void increaseStore(ADDRINT addr)
{
    ++_mem[addr].stores;
}

std::string stat_to_string()
{
    std::stringstream ss;
    ss << "Memory Stats:\n";
    ss << "Addr\tLoads\tStores\n";
    for (const auto& p : _mem)
    {
        ss << hexstr(p.first,8) << "\t" << p.second.loads << "\t" << p.second.stores << "\n";
    }
    return ss.str();
}
