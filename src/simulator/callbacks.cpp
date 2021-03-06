#include "callbacks.H"

extern KNOB<string> KnobOutputFile;
extern KNOB<BOOL>   KnobNoSharedLibs;
extern CACHE_CONFIG l1_config;

int32_t lock_id = 1;
uint32_t next_pid = 0;
uint32_t num_processors;  // NOTE: number of processor is power of 2

Controller * controller = nullptr;
std::unordered_map<uint32_t, uint32_t> t_map;  // thread id -> processor id
PIN_LOCK mapLock;

inline uint32_t get_pid(uint32_t tid)
{
    assert(t_map.find(tid) != t_map.end());
    return t_map[tid];
}

inline uint32_t get_current_tid()
{
    return PIN_ThreadId();
}

inline uint32_t get_next_pid()
{
    return next_pid == num_processors ? 0 : next_pid++;
}

void cache_load(UINT32 tid, ADDRINT pin_addr)
{
    PIN_GetLock(&mapLock, lock_id++);
    uint64_t addr = reinterpret_cast<UINT64>(pin_addr);
    uint32_t pid = get_pid(tid);
    controller->load_single_line(addr, pid);
    PIN_ReleaseLock(&mapLock);
}

void cache_store(UINT32 tid, ADDRINT pin_addr)
{
    PIN_GetLock(&mapLock, lock_id++);
    uint64_t addr = reinterpret_cast<UINT64>(pin_addr);
    uint32_t pid = get_pid(tid);
    controller->store_single_line(addr, pid);
    PIN_ReleaseLock(&mapLock);
}

void process_attach()
{
    PIN_GetLock(&mapLock, lock_id++);
    num_processors = l1_config.total_processors;
    controller = new Controller(l1_config.total_processors,
                                l1_config.num_sets,
                                l1_config.line_size,
                                l1_config.set_size);
    PIN_ReleaseLock(&mapLock);
}

void process_detach()
{
    PIN_GetLock(&mapLock, lock_id++);
    std::ofstream out(KnobOutputFile.Value().c_str());
    out << controller->stats_to_string();
    delete controller;
    out.close();
    PIN_ReleaseLock(&mapLock);
}

void thread_attach()
{
    PIN_GetLock(&mapLock, lock_id++);
    auto temp_tid = get_current_tid();
    auto temp_pid = get_next_pid();
    std::cout << "tid " << temp_tid << " -> " << "pid " << temp_pid << std::endl;
    t_map[temp_tid] = temp_pid;
    PIN_ReleaseLock(&mapLock);
}

void thread_detach()
{
    PIN_GetLock(&mapLock, lock_id++);
    uint32_t tid = get_current_tid();
    assert(t_map.find(tid) != t_map.end());
    t_map.erase(t_map.find(tid));
    PIN_ReleaseLock(&mapLock);
}
