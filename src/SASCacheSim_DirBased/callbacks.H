//
// @ORIGINAL_AUTHOR: Gang-Ryung Uh
//

/*! @file
 *  This header file contains definition for callback routines
 */

#ifndef _CALLBACKS__H_
#define _CALLBACKS__H_

#include "pin.H"
#include <unordered_map>


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


typedef enum
{
    PROTOCOL_VI,
    PROTOCOL_MSI,
    PROTOCOL_MESI,
    PROTOCOL_DRAGON,
    NONE
} COHERENCE_PROTOCOL;

typedef enum
{
    WRITE_THROUGH_NO_ALLOCATE = 0,
    WRITE_THROUGH_ALLOCATE,
    WRITE_BACK_NO_ALLOCATE,
    WRITE_BACK_ALLOCATE,
    WRITE_NUM
} WRITE_STRATEGY;


typedef struct
{
    INT32 num_sets;
    INT32 set_size;
    INT32 line_size;
    INT32 write;
    INT32 coherence;
    INT32 interconnect;
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

VOID CacheLoad(UINT32 tid, ADDRINT addr);
VOID CacheStore(UINT32 tid, ADDRINT addr);
VOID Fini(int code, VOID * v);
VOID SMPMain(int reason);


void IncreaseLoad(ADDRINT addr);
void IncreaseStore(ADDRINT addr);
std::string StatToString();

#endif // _CALLBACKS__H_
