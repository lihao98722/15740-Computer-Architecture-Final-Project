// @ORIGINAL_AUTHOR: Gang-Ryung Uh

#pragma once

#include <unordered_map>
#include "pin.H"


#define THREAD_ATTACH   1
#define THREAD_DETACH   2
#define PROCESS_ATTACH  3
#define PROCESS_DETACH  4

#define MAX_PROCESSORS     1024
#define MAX_THREADS         256
#define MAX_SETS           4096 // cacheSize / (lineSize * associativity);
#define MAX_ASSOCIATIVITY   256 // associativity;

typedef enum
{
    NO_BUS_ACTION,
    BUS_READ,
    BUS_READ_EXCLUSIVE,
    BUS_WRITE,
    BUS_ACTIONS_NUM
} BUS_ACTION;

typedef enum
{
    NO_DIR_ACTION,
    READ_MISS,
    WRITE_MISS,
    INVALIDATE,
    FETCH,
    FETCH_INVALIDATE,
    DATA_VALUE_REPLY,
    DATA_WRITE_BACK,
    DIR_ACTIONS_NUM
} DIR_ACTION;

typedef struct
{
    INT32 num_sets;
    INT32 set_size;
    INT32 line_size;
    INT32 total_processors;
} CACHE_CONFIG;

VOID CacheLoad(UINT32 tid, ADDRINT addr);
VOID CacheStore(UINT32 tid, ADDRINT addr);
VOID ProcessAttach();
VOID ProcessDetach();
VOID ThreadAttach();
VOID ThreadDetach();

void IncreaseLoad(ADDRINT addr);
void IncreaseStore(ADDRINT addr);
std::string StatToString();
