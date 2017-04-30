// @ORIGINAL_AUTHOR: Gang-Ryung Uh
#pragma once

#include <assert.h>
#include <iostream>
#include <fstream>
#include <list>
#include <unordered_map>
#include <map>
#include <vector>

#include "pin.H"
#include "cache.H"
// #include "coherence.H"

#define THREAD_ATTACH   1
#define THREAD_DETACH   2
#define PROCESS_ATTACH  3
#define PROCESS_DETACH  4

typedef struct
{
    INT32 num_sets;
    INT32 set_size;
    INT32 line_size;
    INT32 total_processors;
} CACHE_CONFIG;

typedef struct
{
    INT32  simulate_inst_cache;
    INT32  track_insts;
    INT32  track_loads;
    INT32  track_stores;
    INT32  threshold_hit;
    INT32  threshold_miss;
    INT32  total_processors;
} SIMULATION_CONFIG;

VOID cache_load(UINT32 tid, ADDRINT addr);
VOID cache_store(UINT32 tid, ADDRINT addr);
VOID process_attach();
VOID process_detach();
VOID thread_attach();
VOID thread_detach();

void increase_load(ADDRINT addr);
void increase_store(ADDRINT addr);
std::string stat_to_string();
