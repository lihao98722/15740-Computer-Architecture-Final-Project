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
#include <vector>

extern KNOB<string> KnobOutputFile;
extern KNOB<BOOL>   KnobNoSharedLibs;
extern SIMULATION_CONFIG catch_all_config;
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
CACHE<CACHE_SET::TAG_MEMORY>* p_array[MAX_PROCESSORS];  // Array of Processors being Simulated
std::map<UINT32, UINT32> t_map;                         // Mapping which Thread Belongs to Which processor
std::map<CACHE_TAG, DIRECTORY_LINE *> dir_map;          // Directory Based Map
UINT32 active_threads = 0;
UINT32 nextPID = 0;
INT32 totalBitsToShift;
INT32 processorsMask;
UINT32 pow2processors;

#if defined (__DEBUG__)                           // <3>
#include <set>
set<UINT32> DataAddrs;
#endif

int lock_id = 1;
PIN_LOCK mapLock;


/* ===================================================================== */
UINT32 getHomeNode(CACHE_TAG tag)
{
    return ((tag >> totalBitsToShift) & processorsMask);
}

/* ===================================================================== */
UINT32 getPID(UINT32 tid)
{
    std::map<UINT32, UINT32>::iterator t_map_it = t_map.find(tid);
    return t_map_it->second;
}

/* ===================================================================== */

VOID CacheLoad(UINT32 tid, ADDRINT addr)
{
#if defined (__DEBUG__)
    if (DataAddrs.find(addr) == DataAddrs.end()) {return;}
#endif

    PIN_GetLock(&mapLock, lock_id++);
    HIT_MISS_TYPES dl1Hit = p_array[getPID(tid)]->LoadSingleLine(addr);
    IncreaseLoad(addr);
    PIN_ReleaseLock(&mapLock);
}

/* ===================================================================== */

VOID CacheStore(UINT32 tid, ADDRINT addr)
{
#if defined (__DEBUG__)
    if (DataAddrs.find(addr) == DataAddrs.end()) {return;}
#endif

    PIN_GetLock(&mapLock, lock_id++);
    HIT_MISS_TYPES dl1Hit = p_array[getPID(tid)]->StoreSingleLine(addr);
    IncreaseStore(addr);
    PIN_ReleaseLock(&mapLock);
}

/* ===================================================================== */
VOID setProcessorsArray(UINT32 totalProcessors)
{
    char  buf[512];
    for (UINT32 i=0 ; i<totalProcessors; i++)
    {
        sprintf(buf, "%s: %d %s\n", "Processor", i, "L1 Data Cache");
        string s1 = string(buf);
        int size = l1_config.num_sets * l1_config.set_size *
                   l1_config.line_size;

        CACHE<CACHE_SET::TAG_MEMORY>* ptr1;

        ptr1 = new CACHE<CACHE_SET::TAG_MEMORY>
                     (s1, size, l1_config.line_size, l1_config.set_size, l1_config.write,
                      l1_config.coherence, l1_config.interconnect, p_array, i);
        p_array[i] = ptr1;
    }

    PIN_GetLock(&mapLock, lock_id++);
    active_threads++;
    lock_id = PIN_ReleaseLock(&mapLock);
}

/* ===================================================================== */
VOID unsetProcessorsArray(UINT32 totalProcessors)
{
    for (UINT32 i=0 ; i<totalProcessors; i++)
    {
        assert(p_array[i]->valid == true);
        p_array[i]->valid = false;
    }
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
VOID Fini()
{
    std::ofstream out(KnobOutputFile.Value().c_str());
    // Starting from thread 1 since thread0 is the main application
    for (UINT32 i=0; i<pow2processors; i++)
    {
        out << p_array[i]->StatsLong("+ " , CACHE_BASE::CACHE_TYPE_DCACHE) << endl;
    }
    if (catch_all_config.track_loads || catch_all_config.track_stores)
    {
        out << StatToString();
    }


    out.close();
}

// Debug
UINT32 temp_tid = 0;
UINT32 temp_pid = 0;
/* ===================================================================== */
VOID SMPMain(int reason)
{
    UINT32 tid;
    string s;

    switch (reason)
    {
      case PROCESS_ATTACH:
        totalBitsToShift = 16 - FloorLog2(l1_config.line_size);
        pow2processors = 1 << CeilLog2(catch_all_config.total_processors);
        processorsMask = pow2processors-1;
        // Creates Processors
        setProcessorsArray(pow2processors);
        break;

      case PROCESS_DETACH:
        unsetProcessorsArray(pow2processors);
        Fini();
        break;

      case THREAD_ATTACH:
        temp_tid = get_current_tid();
        temp_pid = get_next_pid();
        std::cout << "tid " << temp_tid << " -> " << "pid " << temp_pid << std::endl;
        (VOID)t_map.insert(std::pair<UINT32, UINT32>(temp_tid, temp_pid));
        break;

      case THREAD_DETACH:
        tid = get_current_tid();
        std::map<UINT32, UINT32>::iterator t_map_it = t_map.find(tid);
        if (t_map_it != t_map.end())
        {
            (VOID)t_map.erase(t_map_it);
        }
        break;
    }
}

void IncreaseLoad(ADDRINT addr)
{
    ++_mem[addr].loads;
}

void IncreaseStore(ADDRINT addr)
{
    ++_mem[addr].stores;
}

std::string StatToString()
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
