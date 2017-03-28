//
// @ORIGINAL_AUTHOR: Artur Klauser
// @EXTENDED: Rodric Rabbah (rodric@gmail.com) 
// @EXTENDED: Gang-Ryung Uh (uh@cs.boisestate.edu)
// @EXTENDED: Adrian Eperez (adrian.perez@hp.com)

/*! @file
 *  This file contains an ISA-portable cache simulator
 */

#include "pin.H"
#include "profile.H"
#include "callbacks.H"
#include <assert.h>
#include <cstdio>
#include <sstream>
#include <iostream>
#include <fstream>
#include <set>


typedef enum
{
    COUNTER_MISS = 0,
    COUNTER_HIT = 1,
    COUNTER_NUM
} COUNTER;

typedef  COUNTER_ARRAY<UINT64, COUNTER_NUM> COUNTER_HIT_MISS;

// holds the counters with misses and hits
// conceptually this is an array indexed by instruction address
extern COMPRESSOR_COUNTER<ADDRINT, UINT32, COUNTER_HIT_MISS> profileData;
extern COMPRESSOR_COUNTER<ADDRINT, UINT32, COUNTER_HIT_MISS> profileInst;  


/* ===================================================================== */
/* Commandline Switches                                                  */
/* ===================================================================== */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,    "pintool", "o",
                            "cache.out", "specify dcache file name");

KNOB<string> KnobConfigFile(KNOB_MODE_WRITEONCE,    "pintool", "c",
                            "cache.config", "specify configuration file name");

KNOB<BOOL>   KnobNoSharedLibs(KNOB_MODE_WRITEONCE, "pintool", "no_shared_libs",
                              "0", "do not instrument shared libraries");

KNOB<UINT64> KnobInstructionCount(KNOB_MODE_WRITEONCE, "pintool",
                                  "l","5000000000000",
                                  "specify the number of instructions to profile");

#if defined (__DEBUG__)                           // <3>
LOCALVAR set<string,greater<string> > CheckRtns;
#endif

/* ===================================================================== */
/* Cache configurations and other simulation parameters                  */
/* ===================================================================== */
FILE *config;
CACHE_CONFIG l1_config, l2_config, l3_config;
SIMULATION_CONFIG catch_all_config;

/* ===================================================================== */
INT32
Usage()
{
    cerr <<
        "This tool represents a cache simulator.\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}

/* ===================================================================== */
std::string
cache_config_string(CACHE_CONFIG cache) 
{
    std::string write_string[] = {
        "WRITE_THROUGH_NO_ALLOCATE",
        "WRITE_THROUGH_ALLOCATE",
        "WRITE_BACK_NO_ALLOCATE",
        "WRITE_BACK_ALLOCATE",
        "NONE"        
    };

    std::string coherence_string[] = {
        "VALID_INVALID",
        "MSI",
        "MESI",
        "DRAGON",
        "NONE"        
    };

    std::string interconnect_string[] = {
        "BUS",
        "DIRECTORY",
        "NONE"        
    };

    std::string out;
    out += "      number of set: " + decstr(cache.num_sets, 1)        + "\n" +
           "      associativity: " + decstr(cache.set_size, 1)        + "\n" +
           "          line size: " + decstr(cache.line_size, 1)       + "\n" +
           "     write_strategy: " + write_string[cache.write]        + "\n" +
           "          coherence: " + coherence_string[cache.coherence]    + "\n" +
           "       interconnect: " + interconnect_string[cache.interconnect] + "\n";

    return out;
}

/* ===================================================================== */
std::string
config_string() 
{
    std::string out;

    out += "L1:\n";
    out += cache_config_string(l1_config);
    out += "L2:\n";    
    out += cache_config_string(l2_config);
    out += "L3:\n";        
    out += cache_config_string(l3_config);

    // other configuration parameters 
    out += "\n    Simulate I cache:    " +
           decstr(catch_all_config.simulate_inst_cache) + "\n" +
             "    Track Instructions:  " +
           decstr(catch_all_config.track_insts)      + "\n" +
           "    Track loads:         " +
           decstr(catch_all_config.track_loads)      + "\n" +
             "    Track stores:        " +
           decstr(catch_all_config.track_stores)     + "\n" +
             "    Threshold_hit:       "  +
           decstr(catch_all_config.threshold_hit)  + "\n" +
             "    Threshold_miss:      " +
           decstr(catch_all_config.threshold_miss) + "\n" +
             "    Total Processors:       " +
           decstr(catch_all_config.total_processors) + "\n";

    return out;
}


/* ===================================================================== */
LOCALFUN VOID
init_configuration() 
{
    /********************************************
      Cache Coherence Protocol: 
          0: Valid-Invalid
          1: Modified-Shared-Invalid
          2: Modified-Exclusive-Shared-Invalid
          3: Dragon

       Interconnect:
          0: BUS
          1: DIRECTORY
    *********************************************/


    /* L1 cache config */
    if (fscanf(config, "L1: %i: %i: %i: %i: %i: %i\n",
               &l1_config.num_sets, &l1_config.set_size, &l1_config.line_size,
               &l1_config.write, &l1_config.coherence,
               &l1_config.interconnect) != 6) 
    {
        perror("fscanf: cannot access config param for L1 cache");
        exit(-1);
    }
    ASSERTX((l1_config.num_sets <= MAX_SETS) && (l1_config.set_size <= MAX_ASSOCIATIVITY));
        
    /* L2 cache config */    
    if (fscanf(config, "L2: %i: %i: %i: %i: %i: %i\n",
               &l2_config.num_sets, &l2_config.set_size, &l2_config.line_size,
               &l2_config.write, &l2_config.coherence,
               &l2_config.interconnect) != 6) 
    {
        perror("fscanf: cannot access config param for L2 cache");
        exit(-1);
    }
    ASSERTX((l2_config.num_sets <= MAX_SETS) && (l2_config.set_size <= MAX_ASSOCIATIVITY));
        
    /* L3 cache config */    
    if (fscanf(config, "L3: %i: %i: %i: %i: %i: %i\n",
               &l3_config.num_sets, &l3_config.set_size, &l3_config.line_size,
               &l3_config.write, &l3_config.coherence,
               &l3_config.interconnect) != 6) 
    {
        perror("fscanf: cannot access config param for L2 cache");
        exit(-1);
    }
    ASSERTX((l3_config.num_sets <= MAX_SETS) && (l3_config.set_size <= MAX_ASSOCIATIVITY));
    
    /* Other call-all simulation params */
    if (fscanf(config, "I: %i: TI: %i: TL: %i: TS: %i: H: %i: M: %i: P: %i\n",
               &catch_all_config.simulate_inst_cache,
               &catch_all_config.track_insts,               
               &catch_all_config.track_loads,
               &catch_all_config.track_stores,
               &catch_all_config.threshold_hit,
               &catch_all_config.threshold_miss,
               &catch_all_config.total_processors) != 7)
    {
        perror("fscanf: cannot access other catch-all config params");
        exit(-1);
    }

    cerr << config_string() << endl;
    
}

/* ===================================================================== */
LOCALFUN VOID
Initialization() 
{
    if(((config = fopen( (KnobConfigFile.Value()).c_str(), "r" ))
        == NULL)  )
    {
        cerr << "Cannot open configuration file : " << KnobConfigFile.Value() << "\n";
        Usage();
        exit(-1);
    }

    // initialize cache configuration and other simulation modes
    // based on the specification from the input file
    init_configuration();
}

/* ===================================================================== */
LOCALFUN VOID
Trace(TRACE trace, VOID *v) 
{
#if defined TARGET_LINUX
    if (KnobNoSharedLibs.Value() &&
        IMG_Type(SEC_Img(RTN_Sec(TRACE_Rtn(trace)))) == IMG_TYPE_SHAREDLIB)
        return;

    RTN rtn = TRACE_Rtn(trace);

#if defined (__DEBUG__)
    if (RTN_Valid(rtn))
        if (CheckRtns.find(RTN_Name(rtn)) == CheckRtns.end()) 
        {
            // cerr << "Ignored Routine Name : " << RTN_Name(rtn) << "\n";            
            return;
        }
#endif // __DEBUG__

#endif // TARGET_LINUX

#if defined (__DEBUG__)
//    cerr << "Routine Name to be inspected : " << RTN_Name(rtn) << "\n";            
#endif // __DEBUG__
    
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {

        INS ins = BBL_InsHead(bbl);
        for (; INS_Valid(ins); ins = INS_Next(ins)) {

            const ADDRINT iaddr = INS_Address(ins);

            if (catch_all_config.simulate_inst_cache) 
            {
                const UINT32 instId = profileInst.Map(iaddr);
                INT32 size = INS_Size(ins);
                
                switch (size) 
                {
                  case 1:
                  case 2:
                  case 3:
                  case 4:
                    if (catch_all_config.track_insts) 
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) FetchSingle,
                                       IARG_THREAD_ID, 
                                       IARG_UINT32, iaddr,
                                       IARG_UINT32, instId, 
                                       IARG_END);
                    else
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) FetchSingleFast,
                                       IARG_THREAD_ID, 
                                       IARG_UINT32, iaddr,
                                       IARG_UINT32, instId, 
                                       IARG_END);
                    break;
                    
                  default:
                    if (catch_all_config.track_insts)                     
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) FetchMulti,
                                       IARG_THREAD_ID, 
                                       IARG_UINT32, iaddr, 
                                       IARG_UINT32,
                                       size, 
                                       IARG_UINT32, instId, 
                                       IARG_END);
                    else
                        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) FetchMultiFast,
                                       IARG_THREAD_ID, IARG_UINT32, iaddr, IARG_UINT32,
                                       size, IARG_UINT32, instId, IARG_END); 
                    break;
                } // End Switch
            }
            

            if (INS_IsMemoryRead(ins))
            {
                
                // map sparse INS addresses to dense IDs
                const ADDRINT iaddr = INS_Address(ins);
                const UINT32 instId = profileData.Map(iaddr);
                
                const UINT32 size = INS_MemoryReadSize(ins);
                const BOOL   single = (size <= 4);
                if( catch_all_config.track_loads )
                {
                    if( single )
                    {
                        INS_InsertPredicatedCall(
                            ins, IPOINT_BEFORE, (AFUNPTR) LoadSingle,
                            IARG_THREAD_ID,
                            IARG_MEMORYREAD_EA,
                            IARG_UINT32, instId,
                            IARG_END);
                    }
                    else
                    {
                        INS_InsertPredicatedCall(
                            ins, IPOINT_BEFORE,  (AFUNPTR) LoadMulti,
                            IARG_THREAD_ID,
                            IARG_MEMORYREAD_EA,
                            IARG_MEMORYREAD_SIZE,
                            IARG_UINT32, instId,
                            IARG_END);
                    }
                }
                else
                {
                    if( single )
                    {
                        INS_InsertPredicatedCall(
                            ins, IPOINT_BEFORE,  (AFUNPTR) LoadSingleFast,
                            IARG_THREAD_ID,
                            IARG_MEMORYREAD_EA,
                            IARG_END);
                            
                    }
                    else
                    {
                        INS_InsertPredicatedCall(
                            ins, IPOINT_BEFORE,  (AFUNPTR) LoadMultiFast,
                            IARG_THREAD_ID,
                            IARG_MEMORYREAD_EA,
                            IARG_MEMORYREAD_SIZE,
                            IARG_END);
                    }
                }
                
            } // Memory Read
        
            if ( INS_IsMemoryWrite(ins) )
            {
                // map sparse INS addresses to dense IDs
                const ADDRINT iaddr = INS_Address(ins);
                const UINT32 instId = profileData.Map(iaddr);
                
                const UINT32 size = INS_MemoryWriteSize(ins);
                
                const BOOL   single = (size <= 4);
                    
                if( catch_all_config.track_stores )
                {
                    if( single )
                    {
                        INS_InsertPredicatedCall(
                            ins, IPOINT_BEFORE,  (AFUNPTR) StoreSingle,
                            IARG_THREAD_ID,
                            IARG_MEMORYWRITE_EA,
                            IARG_UINT32, instId,
                            IARG_END);
                    }
                    else
                    {
                        INS_InsertPredicatedCall(
                            ins, IPOINT_BEFORE,  (AFUNPTR) StoreMulti,
                            IARG_THREAD_ID,
                            IARG_MEMORYWRITE_EA,
                            IARG_MEMORYWRITE_SIZE,
                            IARG_UINT32, instId,
                            IARG_END);
                    }
                        
                }
                else
                {
                    if( single )
                    {
                        INS_InsertPredicatedCall(
                            ins, IPOINT_BEFORE,  (AFUNPTR) StoreSingleFast,
                            IARG_THREAD_ID,
                            IARG_MEMORYWRITE_EA,
                            IARG_END);
                            
                    }
                    else
                    {
                        INS_InsertPredicatedCall(
                            ins, IPOINT_BEFORE,  (AFUNPTR) StoreMultiFast,
                            IARG_THREAD_ID,
                            IARG_MEMORYWRITE_EA,
                            IARG_MEMORYWRITE_SIZE,
                            IARG_END);
                    }
                }
            } // End Memory Write
            //} // End Debug by Instrumenting only the Test Method 
            
	} // End Ins For
        
    } // End outer For   
}

/* ===================================================================== */
VOID
thr_begin(UINT32 threadIndex, VOID *sp, int flags, VOID *v) 
{
    SMPMain(THREAD_ATTACH);
}

/* ===================================================================== */
VOID
thr_end(UINT32 threadIndex, INT32 code, VOID *v) 
{
    SMPMain(THREAD_DETACH);
}

/* ===================================================================== */
VOID
Fini(int code, VOID * v)
{
    SMPMain(PROCESS_DETACH);
}

/* ===================================================================== */
int
main(int argc, char *argv[])
{
#if defined (__DEBUG__)
    extern set<UINT32> DataAddrs;
#endif
    
    PIN_InitSymbols();

    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }

    Initialization();

#if defined (__DEBUG__)                           // <3>
    CheckRtns.insert("_Z4tst1Pv");
    CheckRtns.insert("_Z5pmainiPPc");    
    //CheckRtns.insert("_Z4tst2Pv");
    //CheckRtns.insert("_Z4tst3Pv");

    DataAddrs.insert(0x0805AA84);
    //DataAddrs.insert(0x0805A988);
    //DataAddrs.insert(0x0805A98C);        
#endif
    COUNTER_HIT_MISS threshold;

    threshold[COUNTER_HIT]  = catch_all_config.threshold_hit;
    threshold[COUNTER_MISS] = catch_all_config.threshold_miss ;
    
    profileData.SetThreshold( threshold );
     
    TRACE_AddInstrumentFunction(Trace, 0);
    PIN_AddFiniFunction(Fini, 0);
    PIN_AddThreadBeginFunction(thr_begin, (VOID *) 0);
    PIN_AddThreadEndFunction(thr_end, (VOID *) 0);    

    SMPMain(PROCESS_ATTACH);

    // Never returns
    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
