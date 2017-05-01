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
Controller * controller = nullptr;
std::unordered_map<UINT32, UINT32> t_map;    // Mapping which Thread Belongs to Which processor
UINT32 active_threads = 0;
UINT32 nextPID = 0;
UINT32 pow2processors;

int lock_id = 1;
PIN_LOCK mapLock;



/* ===================================================================== */
UINT32 get_pid(UINT32 tid)
{
    auto t_map_it = t_map.find(tid);
    return t_map_it->second;
}

/* ===================================================================== */
VOID cache_load(UINT32 tid, ADDRINT pin_addr)
{
    // std::cout << "cache load" << std::endl;

    PIN_GetLock(&mapLock, lock_id++);
    UINT64 addr = reinterpret_cast<UINT64>(pin_addr);
    UINT32 pid = get_pid(tid);
    controller->load_single_line(addr, pid);
    increase_load(pin_addr);
    PIN_ReleaseLock(&mapLock);
}

/* ===================================================================== */
VOID cache_store(UINT32 tid, ADDRINT pin_addr)
{
    // std::cout << "cache store" << std::endl;

    PIN_GetLock(&mapLock, lock_id++);
    UINT64 addr = reinterpret_cast<UINT64>(pin_addr);
    UINT32 pid = get_pid(tid);
    controller->store_single_line(addr, pid);
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
    // std::cout << "process attach" << std::endl;

    pow2processors = l1_config.total_processors;
    controller = new Controller(l1_config.total_processors,
                                l1_config.num_sets,
                                l1_config.line_size,
                                l1_config.set_size);

    PIN_GetLock(&mapLock, lock_id++);
    active_threads++;
    PIN_ReleaseLock(&mapLock);
}

/* ===================================================================== */
VOID process_detach()
{
    std::ofstream out(KnobOutputFile.Value().c_str());
    out << controller->stat_to_string();
    out << stat_to_string();
    delete controller;
    out.close();
}

/* ===================================================================== */
VOID thread_attach()
{
    auto temp_tid = get_current_tid();
    auto temp_pid = get_next_pid();
    std::cout << "tid " << temp_tid << " -> " << "pid " << temp_pid << std::endl;
    t_map[temp_tid] = temp_pid;
}

VOID thread_detach()
{
    auto tid = get_current_tid();
    auto t_map_it = t_map.find(tid);
    if (t_map_it != t_map.end())
    {
        (VOID)t_map.erase(t_map_it);
    }
}

void increase_load(ADDRINT addr)
{
    ++_mem[addr].loads;
}

void increase_store(ADDRINT addr)
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
