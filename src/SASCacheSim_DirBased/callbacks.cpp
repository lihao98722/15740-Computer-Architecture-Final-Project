//
// @ORIGINAL_AUTHOR: Gang-Ryung Uh
// @EXTENDED: Adrian Perez (adrian.perez@hp.com)
//
/*! @file
 *  This file contains callback routines for cache simulator
 */

#include "callbacks.H"
#include "profile.H"
#include "cache.H"
#include "coherence.H"
#include <assert.h>
#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include <vector>

// wrap configuration constants into their own name space to avoid name clashes

typedef enum
{
    COUNTER_MISS = 0,
    COUNTER_HIT = 1,
    COUNTER_NUM
} COUNTER;

typedef  COUNTER_ARRAY<UINT64, COUNTER_NUM> COUNTER_HIT_MISS;

extern KNOB<string> KnobOutputFile;
extern KNOB<BOOL>   KnobNoSharedLibs;
extern SIMULATION_CONFIG catch_all_config;
extern CACHE_CONFIG l1_config;


/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */
CACHE<CACHE_SET::TAG_MEMORY>* p_array[MAX_PROCESSORS]; // Array of Processors being Simulated
std::map<UINT32, UINT32> t_map;                      // Mapping which Thread Belongs to Which processor
std::map<CACHE_TAG, DIRECTORY_LINE *> dir_map;         // Directory Based Map
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

// holds the counters with misses and hits
// conceptually this is an array indexed by instruction address

COMPRESSOR_COUNTER<ADDRINT, UINT32, COUNTER_HIT_MISS> profileData;
COMPRESSOR_COUNTER<ADDRINT, UINT32, COUNTER_HIT_MISS> profileInst;


/* ===================================================================== */
UINT32
getHomeNode(CACHE_TAG tag)
{
    return ((tag >> totalBitsToShift) & processorsMask);
}

/* ===================================================================== */
UINT32
getPID(UINT32 tid)
{
    std::map<UINT32, UINT32>::iterator t_map_it = t_map.find(tid);
    return t_map_it->second;
}
/* ============================================================== */
VOID
FetchSingle(UINT32 tid, ADDRINT addr, UINT32 instId)
{
    PIN_GetLock(&mapLock, lock_id++);
    HIT_MISS_TYPES il1Hit = p_array[getPID(tid)]->FetchSingleLine(addr);
    const COUNTER counter = il1Hit ? COUNTER_HIT : COUNTER_MISS;
    profileInst[instId][counter]++;
    lock_id = PIN_ReleaseLock(&mapLock);
}


/* ============================================================== */
VOID
FetchSingleFast(UINT32 tid, ADDRINT addr)
{
    PIN_GetLock(&mapLock, lock_id++);
    p_array[getPID(tid)]->FetchSingleLine(addr);
    lock_id = PIN_ReleaseLock(&mapLock);
}

/* ============================================================== */
VOID
FetchMulti(UINT32 tid, ADDRINT addr, UINT32 size, UINT32 instId)
{
    PIN_GetLock(&mapLock, lock_id++);
    HIT_MISS_TYPES il1Hit = p_array[getPID(tid)]->Fetch(addr,size);
    const COUNTER counter = il1Hit ? COUNTER_HIT : COUNTER_MISS;
    profileInst[instId][counter]++;
    lock_id = PIN_ReleaseLock(&mapLock);
}

/* =============================================================== */
VOID
FetchMultiFast(UINT32 tid, ADDRINT addr, UINT size)
{
    PIN_GetLock(&mapLock, lock_id++);
    p_array[getPID(tid)]->Fetch(addr, size);
    lock_id = PIN_ReleaseLock(&mapLock);
}

/* ===================================================================== */

VOID LoadSingle(UINT32 tid, ADDRINT addr, UINT32 instId)
{
#if defined (__DEBUG__)
    if (DataAddrs.find(addr) == DataAddrs.end())
    {
        return;
    }
#endif // __DEBUG__

    PIN_GetLock(&mapLock, lock_id++);
    HIT_MISS_TYPES dl1Hit = p_array[getPID(tid)]->LoadSingleLine(addr);
    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    profileData[instId][counter]++;
    lock_id = PIN_ReleaseLock(&mapLock);
}

/* ===================================================================== */

VOID LoadSingleFast(UINT32 tid, ADDRINT addr)
{
#if defined (__DEBUG__)
    if (DataAddrs.find(addr) == DataAddrs.end())
    {
        return;
    }
//    cerr << "data addr:" << hex << addr << endl;
#endif // __DEBUG__

    PIN_GetLock(&mapLock, lock_id++);
    p_array[getPID(tid)]->LoadSingleLine(addr);
    lock_id = PIN_ReleaseLock(&mapLock);
}

/* ===================================================================== */

VOID LoadMulti(UINT32 tid, ADDRINT addr, UINT32 size, UINT32 instId)
{
#if defined (__DEBUG__)
    if (DataAddrs.find(addr) == DataAddrs.end())
    {
        return;
    }
#endif // __DEBUG__

    PIN_GetLock(&mapLock, lock_id++);
    HIT_MISS_TYPES dl1Hit = p_array[getPID(tid)]->Load(addr,size);
    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    profileData[instId][counter]++;
    lock_id = PIN_ReleaseLock(&mapLock);
}

/* ===================================================================== */

VOID LoadMultiFast(UINT32 tid, ADDRINT addr, UINT32 size)
{
#if defined (__DEBUG__)
    if (DataAddrs.find(addr) == DataAddrs.end())
    {
        return;
    }
//    cerr << "data addr:" << hex << addr << endl;
#endif // __DEBUG__

    PIN_GetLock(&mapLock, lock_id++);
    p_array[getPID(tid)]->Load(addr, size);
    lock_id = PIN_ReleaseLock(&mapLock);
}


/* ===================================================================== */

VOID StoreSingle(UINT32 tid, ADDRINT addr, UINT32 instId)
{
#if defined (__DEBUG__)
    if (DataAddrs.find(addr) == DataAddrs.end())
    {
        return;
    }
#endif // __DEBUG__

    PIN_GetLock(&mapLock, lock_id++);
    HIT_MISS_TYPES dl1Hit = p_array[getPID(tid)]->StoreSingleLine(addr);
    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    profileData[instId][counter]++;
    lock_id = PIN_ReleaseLock(&mapLock);
}
/* ===================================================================== */

VOID StoreSingleFast(UINT32 tid, ADDRINT addr)
{
#if defined (__DEBUG__)
    if (DataAddrs.find(addr) == DataAddrs.end())
    {
        return;
    }
#endif // __DEBUG__
    PIN_GetLock(&mapLock, lock_id++);
    p_array[getPID(tid)]->StoreSingleLine(addr);
    lock_id = PIN_ReleaseLock(&mapLock);
}

/* ===================================================================== */

VOID StoreMulti(UINT32 tid, ADDRINT addr, UINT32 size, UINT32 instId)
{
#if defined (__DEBUG__)
    if (DataAddrs.find(addr) == DataAddrs.end())
    {
        return;
    }
#endif // __DEBUG__

    PIN_GetLock(&mapLock, lock_id++);
    const HIT_MISS_TYPES dl1Hit = p_array[getPID(tid)]->Store(addr,size);
    const COUNTER counter = dl1Hit ? COUNTER_HIT : COUNTER_MISS;
    profileData[instId][counter]++;
    lock_id = PIN_ReleaseLock(&mapLock);
}

/* ===================================================================== */
VOID
StoreMultiFast(UINT32 tid, ADDRINT addr, UINT32 size)
{
#if defined (__DEBUG__)
    if (DataAddrs.find(addr) == DataAddrs.end())
    {
        return;
    }
#endif // __DEBUG__

    PIN_GetLock(&mapLock, lock_id++);
    cout << "Load Multi Fast: ";
    p_array[getPID(tid)]->Store(addr, size);
    cout << "End" << endl;
    lock_id = PIN_ReleaseLock(&mapLock);
}

/* ===================================================================== */
VOID
setProcessorsArray(UINT32 totalProcessors)
{
    char  buf[512];
    // extern CACHE_CONFIG l1_config, l2_config, l3_config;
    for (UINT32 i=0 ; i<totalProcessors; i++)
    {
        //       cout << " Creating processor: " << i << endl;
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
VOID
unsetProcessorsArray(UINT32 totalProcessors)
{
    for (UINT32 i=0 ; i<totalProcessors; i++)
    {
        assert(p_array[i]->valid == true);
        p_array[i]->valid = false;
        //cout << "Unset Processor: " << i << endl;
    }
}

/* ===================================================================== */
inline
UINT32
get_current_tid()
{
    return PIN_ThreadId();
}
/* ===================================================================== */
inline
UINT32
get_next_pid()
{
    // Round Robbin, condition typically faster than modulo
    return nextPID == pow2processors ? 0 : nextPID++;
}
/* ===================================================================== */
VOID
Fini()
{
    std::ofstream out(KnobOutputFile.Value().c_str());
    // Starting from thread 1 since thread0 is the main application
    for (UINT32 i=0; i<pow2processors; i++)
    {
        //cout << p_array[i]->StatsLong("+ " , CACHE_BASE::CACHE_TYPE_DCACHE) << endl;
        out << p_array[i]->StatsLong("+ " , CACHE_BASE::CACHE_TYPE_DCACHE) << endl;
    }
    if (catch_all_config.track_loads || catch_all_config.track_stores)
    {
        out <<
            "#\n"
            "# LOAD stats\n"
            "#\n";
        out << profileData.StringLong();
    }


    out.close();
}

// Debug
UINT32 temp_tid = 0;
UINT32 temp_pid = 0;
/* ===================================================================== */
VOID
SMPMain(int reason)
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

        // Adding Main Thread
        // (VOID)t_map.insert(std::pair<UINT32, UINT32>(get_current_tid(), get_next_pid()));
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
        // typedef std::pair<UINT32, UINT32> Ptr;
        std::map<UINT32, UINT32>::iterator t_map_it = t_map.find(tid);
        if (t_map_it != t_map.end())
        {
            (VOID)t_map.erase(t_map_it);
        }
        break;
    }
}
