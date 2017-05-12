//
// @ORIGINAL_AUTHOR: Gang-Ryung Uh
// @EXTENDED: Adrian Perez (adrian.perez@hp.com)
//
/*! @file
 *  This file contains callback routines for cache simulator
 */
#include "callbacks.H"

extern KNOB<string> KnobOutputFile;
extern KNOB<BOOL>   KnobNoSharedLibs;
extern CACHE_CONFIG l1_config;

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */
Controller * controller = nullptr;
std::unordered_map<UINT32, UINT32> t_map;    // Mapping which Thread Belongs to Which processor
// UINT32 active_threads = 0;
uint32_t nextPID = 0;
uint32_t pow2processors;

int lock_id = 1;
PIN_LOCK mapLock;


uint32_t get_pid(UINT32 tid)
{
    auto t_map_it = t_map.find(tid);
    return t_map_it->second;
}

void cache_load(UINT32 tid, ADDRINT pin_addr)
{
    // std::cout << "cache load" << std::endl;
    PIN_GetLock(&mapLock, lock_id++);
    uint64_t addr = reinterpret_cast<UINT64>(pin_addr);
    uint32_t pid = get_pid(tid);
    controller->load_single_line(addr, pid);
    PIN_ReleaseLock(&mapLock);
}

void cache_store(UINT32 tid, ADDRINT pin_addr)
{
    // std::cout << "cache store" << std::endl;
    PIN_GetLock(&mapLock, lock_id++);
    uint64_t addr = reinterpret_cast<UINT64>(pin_addr);
    uint32_t pid = get_pid(tid);
    controller->store_single_line(addr, pid);
    PIN_ReleaseLock(&mapLock);
}

inline uint32_t get_current_tid()
{
    return PIN_ThreadId();
}

inline uint32_t get_next_pid()
{
    // Round Robbin, condition typically faster than modulo
    return nextPID == pow2processors ? 0 : nextPID++;
}

void process_attach()
{
    PIN_GetLock(&mapLock, lock_id++);
    pow2processors = l1_config.total_processors;
    controller = new Controller(l1_config.total_processors,
                                l1_config.num_sets,
                                l1_config.line_size,
                                l1_config.set_size);
    // active_threads++;
    PIN_ReleaseLock(&mapLock);
}

void process_detach()
{
    std::ofstream out(KnobOutputFile.Value().c_str());
    out << controller->stat_to_string();
    delete controller;
    out.close();
}

void thread_attach()
{
    auto temp_tid = get_current_tid();
    auto temp_pid = get_next_pid();
    std::cout << "tid " << temp_tid << " -> " << "pid " << temp_pid << std::endl;
    t_map[temp_tid] = temp_pid;
}

void thread_detach()
{
    auto tid = get_current_tid();
    auto t_map_it = t_map.find(tid);
    if (t_map_it != t_map.end())
    {
        (void)t_map.erase(t_map_it);
    }
}
