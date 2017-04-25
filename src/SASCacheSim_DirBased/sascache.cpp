//
// @ORIGINAL_AUTHOR: Artur Klauser
// @EXTENDED: Rodric Rabbah (rodric@gmail.com)
// @EXTENDED: Gang-Ryung Uh (uh@cs.boisestate.edu)
// @EXTENDED: Adrian Eperez (adrian.perez@hp.com)

/*! @file
 *  This file contains an ISA-portable cache simulator
 */

#include "pin.H"
#include "callbacks.H"
#include <assert.h>
#include <cstdio>
#include <sstream>
#include <iostream>
#include <fstream>
#include <set>

/* ===================================================================== */
/* Commandline Switches                                                  */
/* ===================================================================== */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,
                            "pintool",
                            "o",
                            "cache.out",
                            "specify dcache file name");

KNOB<string> KnobConfigFile(KNOB_MODE_WRITEONCE,
                            "pintool",
                            "c",
                            "cache.config",
                            "specify configuration file name");

KNOB<BOOL> KnobNoSharedLibs(KNOB_MODE_WRITEONCE,
                            "pintool",
                            "no_shared_libs",
                            "0",
                            "do not instrument shared libraries");

KNOB<UINT64> KnobInstructionCount(KNOB_MODE_WRITEONCE,
                                  "pintool",
                                  "l","5000000000000",
                                  "specify the number of instructions to profile");

/* ===================================================================== */
/* Cache configurations and other simulation parameters                  */
/* ===================================================================== */
FILE *config;
CACHE_CONFIG l1_config, l2_config, l3_config;
SIMULATION_CONFIG catch_all_config;

/* ===================================================================== */
INT32 Usage()
{
    cerr <<
        "This tool represents a cache simulator.\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}

/* ===================================================================== */
std::string cache_config_string(CACHE_CONFIG cache)
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

    std::string out;
    out += "      number of set: " + decstr(cache.num_sets, 1)        + "\n" +
           "      associativity: " + decstr(cache.set_size, 1)        + "\n" +
           "          line size: " + decstr(cache.line_size, 1)       + "\n" +
           "     write_strategy: " + write_string[cache.write]        + "\n" +
           "          coherence: " + coherence_string[cache.coherence]    + "\n" +
           "       interconnect: " + "Directory" + "\n";

    return out;
}

/* ===================================================================== */
std::string config_string()
{
    std::string out;

    out += "L1:\n";
    out += cache_config_string(l1_config);

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

/********************************************
  Cache Coherence Protocol:
      0: Valid-Invalid
      1: Modified-Shared-Invalid
      2: Modified-Exclusive-Shared-Invalid
      3: Dragon
*********************************************/
/* ===================================================================== */
LOCALFUN VOID init_configuration()
{
    /* L1 cache config */
    if (fscanf(config, "L1: %i: %i: %i: %i: %i\n",
               &l1_config.num_sets, &l1_config.set_size, &l1_config.line_size,
               &l1_config.write, &l1_config.coherence) != 5)
    {
        perror("fscanf: cannot access config param for L1 cache");
        exit(-1);
    }

    ASSERTX((l1_config.num_sets <= MAX_SETS) && (l1_config.set_size <= MAX_ASSOCIATIVITY));

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
LOCALFUN VOID Initialization()
{
    if (((config = fopen((KnobConfigFile.Value()).c_str(), "r" )) == NULL))
    {
        cerr << "Cannot open configuration file : " << KnobConfigFile.Value() << "\n";
        Usage();
        exit(-1);
    }

    // initialize cache configuration and other simulation modes
    // based on the specification from the input file
    init_configuration();
}

/* =====================================================================
  Called for every read and write
  Learned from https://software.intel.com/sites/landingpage/pintool/docs/76991/Pin/html/index.html#MAddressTrace
 ===================================================================== */
VOID Instruction(INS ins, VOID *v)
{
    UINT32 memOperands = INS_MemoryOperandCount(ins);

    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            if( catch_all_config.track_loads )
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE, (AFUNPTR) CacheLoad,
                    IARG_THREAD_ID,
                    IARG_MEMORYREAD_EA,
                    IARG_END);
            }
        } // End memory read

        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            if( catch_all_config.track_stores )
            {
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE, (AFUNPTR) CacheStore,
                    IARG_THREAD_ID,
                    IARG_MEMORYWRITE_EA,
                    IARG_END);
            }
        } // End memory write
    } // End memOp fFor loop
} // End outer For

/* ===================================================================== */
VOID ThreadStart(THREADID threadIndex, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    SMPMain(THREAD_ATTACH);
}

/* ===================================================================== */
VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    SMPMain(THREAD_DETACH);
}

/* ===================================================================== */
VOID Fini(int code, VOID * v)
{
    SMPMain(PROCESS_DETACH);
}

/* ===================================================================== */
int main(int argc, char *argv[])
{
    PIN_InitSymbols();

    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }

    Initialization();

    // Register Trace to be called when each instruction is loaded.
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    // Register thr_begin/thr_end to be called when a thread begins/ends
    PIN_AddThreadStartFunction(ThreadStart, (VOID *) 0);
    PIN_AddThreadFiniFunction(ThreadFini, (VOID *) 0);

    // Initialize main process
    SMPMain(PROCESS_ATTACH);

    // Never returns
    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
