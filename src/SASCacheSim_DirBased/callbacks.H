// @ORIGINAL_AUTHOR: Gang-Ryung Uh
#pragma once

#include <assert.h>
#include <iostream>
#include <fstream>
#include <list>
#include <unordered_map>
#include <vector>

#include "pin.H"
#include "cache.H"

typedef struct
{
    INT32 num_sets;
    INT32 set_size;
    INT32 line_size;
    INT32 total_processors;
} CACHE_CONFIG;

VOID cache_load(UINT32 tid, ADDRINT addr);
VOID cache_store(UINT32 tid, ADDRINT addr);
VOID process_attach();
VOID process_detach();
VOID thread_attach();
VOID thread_detach();
