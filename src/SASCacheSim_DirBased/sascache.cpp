//
// @ORIGINAL_AUTHOR: Artur Klauser
// @EXTENDED: Rodric Rabbah (rodric@gmail.com)
// @EXTENDED: Gang-Ryung Uh (uh@cs.boisestate.edu)
// @EXTENDED: Adrian Eperez (adrian.perez@hp.com)

/*! @file
 *  This file contains an ISA-portable cache simulator
 */

#include <assert.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <set>

#include "pin.H"
#include "callbacks.H"


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
CACHE_CONFIG l1_config;

/* ===================================================================== */
INT32 Usage()
{
    cerr << "This tool represents a cache simulator.\n\n"
         << KNOB_BASE::StringKnobSummary()
         << '\n';
    return -1;
}

/* ===================================================================== */
std::string cache_config_string(CACHE_CONFIG cache)
{
    std::stringstream out;
    out << std::setw(20) << "number of set: "   << cache.num_sets                    << "\n"
        << std::setw(20) << "associativity: "   << cache.set_size                    << "\n"
        << std::setw(20) << "line size: "       << cache.line_size                   << "\n"
        << std::setw(20) << "write_strategy: "  << "WRITE_BACK_ALLOCATE"             << "\n"
        << std::setw(20) << "coherence: "       << "MSI"                             << "\n"
        << std::setw(20) << "interconnect: "    << "Directory"                       << "\n"
        << std::setw(20) << "Total Processors: "<< l1_config.total_processors << "\n";
    return out.str();
}


LOCALFUN VOID init_configuration()
{
    /* L1 cache config */
    if (fscanf(config, "L1 Data Cache: #Processors=%i #Sets=%i Associativity=%i LineSize=%i\n",
               &l1_config.total_processors, &l1_config.num_sets, &l1_config.set_size, &l1_config.line_size) != 4)
    {
        perror("fscanf: cannot access config param for L1 cache");
        exit(-1);
    }

    ASSERTX((l1_config.num_sets <= MAX_SETS) && (l1_config.set_size <= MAX_ASSOCIATIVITY));
    cerr << cache_config_string(l1_config) << endl;
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
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR) CacheLoad,
                IARG_THREAD_ID,
                IARG_MEMORYREAD_EA,
                IARG_END);
        } // End memory read

        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR) CacheStore,
                IARG_THREAD_ID,
                IARG_MEMORYWRITE_EA,
                IARG_END);
        } // End memory write
    } // End memOp fFor loop
} // End outer For

/* ===================================================================== */
VOID ThreadStart(THREADID threadIndex, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    ThreadAttach();
}

/* ===================================================================== */
VOID ThreadFini(THREADID threadid, const CONTEXT *ctxt, INT32 code, VOID *v)
{
    ThreadDetach();
}

/* ===================================================================== */
VOID Fini(int code, VOID * v)
{
    ProcessDetach();
}

/* ===================================================================== */
int main(int argc, char *argv[])
{
    PIN_InitSymbols();

    if(PIN_Init(argc,argv))
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
    ProcessAttach();

    // Never returns
    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
