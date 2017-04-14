
//
// @ORIGINAL_AUTHOR: Artur Klauser
// @EXTENDED: Gang-Ryung Uh
//

/*! @file
 *  This file contains a configurable cache class
 */

#ifndef PIN_CACHE_H
#define PIN_CACHE_H


#define KILO 1024
#define MEGA (KILO*KILO)
#define GIGA (KILO*MEGA)

typedef UINT64 CACHE_STATS; // type of cache hit/miss counters

#include <sstream>
#include <iostream>
#include <fstream>

/* ===================================================================== */
/*! RMR (rodric@gmail.com)
 *   - temporary work around because decstr()
 *     casts 64 bit ints to 32 bit ones
 */
/* ===================================================================== */
static string
mydecstr(UINT64 v, UINT32 w)
{
    ostringstream o;
    o.width(w);
    o << v;
    string str(o.str());
    return str;
}

/* ===================================================================== */
/*!
 *  @brief Checks if n is a power of 2.
 *  @returns true if n is power of 2
 */
/* ===================================================================== */
static inline
bool IsPower2(UINT32 n)
{
    return ((n & (n - 1)) == 0);
}

/* ===================================================================== */
/*!
 *  @brief Computes floor(log2(n))
 *  Works by finding position of MSB set.
 *  @returns -1 if n == 0.
 */
/* ===================================================================== */
static inline
INT32 FloorLog2(UINT32 n)
{
    INT32 p = 0;

    if (n == 0) return -1;

    if (n & 0xffff0000) { p += 16; n >>= 16; }
    if (n & 0x0000ff00)	{ p +=  8; n >>=  8; }
    if (n & 0x000000f0) { p +=  4; n >>=  4; }
    if (n & 0x0000000c) { p +=  2; n >>=  2; }
    if (n & 0x00000002) { p +=  1; }

    return p;
}

/* ===================================================================== */
/*!
 *  @brief Computes floor(log2(n))
 *  Works by finding position of MSB set.
 *  @returns -1 if n == 0.
 */
/* ===================================================================== */
static inline INT32
CeilLog2(UINT32 n)
{
    return FloorLog2(n - 1) + 1;
}

/* ===================================================================== */
namespace CACHE_STATES
{
   typedef enum
   {
     INVALID,
     SHARED,
     EXCLUSIVE,
     MODIFIED,
     VALID
   } CACHE_STATE;
} // CACHE_STATES

typedef enum
{
    CACHE_MISS,
    CACHE_HIT,
    COHERENCE_MISS,
    HIT_MISS_TYPES_NUM
} HIT_MISS_TYPES;


/* ===================================================================== */
/*!
 *  @brief Cache tag - self clearing on creation
 */
/* ===================================================================== */
class CACHE_TAG
{
  private:
    ADDRINT _tag;

  public:
    CACHE_TAG(ADDRINT tag = 0) { _tag = tag;}
    bool operator==(const CACHE_TAG &right) const { return _tag == right._tag; }
    operator ADDRINT() const { return _tag; }
};


/* ===================================================================== */
/*!
 * Everything related to cache sets
 */
/* ===================================================================== */
namespace CACHE_SET
{

/* ===================================================================== */
/*!
 *  @brief Cache set with LRU replacement
 */
/* ===================================================================== */
class TAG_MEMORY
{
  private:
    CACHE_TAG *_tags;
    INT32  *_lru;
    CACHE_STATES::CACHE_STATE *_valid;
    INT32 _tagsLastIndex;

  public:
    TAG_MEMORY(UINT32 associativity = MAX_ASSOCIATIVITY)
        : _tagsLastIndex(associativity - 1)
    {
        _tags = new CACHE_TAG[associativity];
        _lru = new INT32[associativity];
        _valid = new CACHE_STATES::CACHE_STATE[associativity];

        for (INT32 index = _tagsLastIndex; index >= 0; index--)
        {
            _tags[index] = CACHE_TAG(0);
            _lru[index] = 0;
            _valid[index] = CACHE_STATES::INVALID;
        }
    }

    /* ===================================================================== */
    inline VOID
    SetState(INT32 index, CACHE_STATES::CACHE_STATE state)
    {
        _valid[index] = state;
    }
    /* ===================================================================== */
    inline CACHE_STATES::CACHE_STATE
    GetState(INT32 index)
    {
        return _valid[index];
    }

    /* ===================================================================== */
    inline VOID
    SetAssociativity(UINT32 associativity)
    {
        _tagsLastIndex = associativity - 1;
    }

    /* ===================================================================== */
    inline INT32
    GetAssociativity(UINT32 associativity) {
        return _tagsLastIndex + 1;
    }

    /* ===================================================================== */
    INT32
    Find(CACHE_TAG tag)
    {
        INT32 index = -1;
        for (INT32 i = _tagsLastIndex; i >= 0; i--) {
            _lru[i]++;
            if(_tags[i] == tag) {
                _lru[i] = 0;
                index = i;
            }
        }
        return index;
    }

    /* ===================================================================== */
    INT32
    Replace(CACHE_TAG tag)
    {
        INT32 maxlru = 0;
        int pick = 0;

        for (INT32 i = 0; i < _tagsLastIndex; i++)
        {
            if (_valid[i] == CACHE_STATES::INVALID)
            {
                pick = i;
                break;
            }

            if (_lru[i] > maxlru)
            {
                maxlru = _lru[i];
                pick = i;
            }
        }
        _tags[pick]  = tag;
        _valid[pick] = CACHE_STATES::VALID;
        _lru[pick]   = 0;
        return pick;
    }
};

}; // namespace CACHE_SET


/* ===================================================================== */
/*!
 *  @brief Generic cache base class;
 *  no allocate specialization, no cache set specialization
 */
/* ===================================================================== */
class CACHE_BASE
{
  public:
    // types, constants
    typedef enum
    {
        ACCESS_TYPE_LOAD,
        ACCESS_TYPE_STORE,
        ACCESS_TYPE_NUM
    } ACCESS_TYPE;

    typedef enum
    {
        CACHE_TYPE_ICACHE,
        CACHE_TYPE_DCACHE,
        CACHE_TYPE_NUM
    } CACHE_TYPE;

  protected:
    static const UINT32 HIT_MISS_NUM = 3;
    CACHE_STATS _access[ACCESS_TYPE_NUM][HIT_MISS_NUM];
    CACHE_STATS _fetch[HIT_MISS_NUM];  // instruction cache
    CACHE_STATS _cost;

  private:    // input params
    const std::string _name;
    const UINT32 _cacheSize;
    const UINT32 _lineSize;
    const UINT32 _associativity;

    // computed params
    const UINT32 _lineShift;
    const UINT32 _setIndexMask;

    /* ===================================================================== */
    CACHE_STATS
    SumAccess(bool hit) const
    {
        CACHE_STATS sum = 0;

        for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
        {
            sum += _access[accessType][hit];
        }

        return sum;
    }

    /* ===================================================================== */
    /** Gang-Ryung Uh: The following is for instruction cache */
    /* ===================================================================== */
    inline CACHE_STATS
    SumFetchAccess(bool hit) const
    {
        return _fetch[hit];
    }

  protected:
    UINT32 NumSets() const { return _setIndexMask + 1; }

  public:
    // constructors/destructors
    CACHE_BASE(std::string name, UINT32 cacheSize, UINT32 lineSize, UINT32 associativity);

    // accessors
    /* ===================================================================== */
    inline UINT32
    CacheSize() const {
        return _cacheSize;
    }
    /* ===================================================================== */
    inline UINT32
    LineSize() const {
        return _lineSize;
    }
    /* ===================================================================== */
    inline UINT32
    Associativity() const {
        return _associativity;
    }

    /* ===================================================================== */
    inline CACHE_STATS
    Hits(ACCESS_TYPE accessType) const {
        return _access[accessType][CACHE_HIT];
    }
    /* ===================================================================== */
    inline CACHE_STATS
    Misses(ACCESS_TYPE accessType) const {
        return _access[accessType][CACHE_MISS];
    }
    /* ===================================================================== */
    inline CACHE_STATS
    CoherenceMisses(ACCESS_TYPE accessType) const {
        return _access[accessType][COHERENCE_MISS];
    } // Adrian Perez
    /* ===================================================================== */
    inline CACHE_STATS
    Accesses(ACCESS_TYPE accessType) const {
        return Hits(accessType) + Misses(accessType) + CoherenceMisses(accessType);
    }
    /* ===================================================================== */
    inline CACHE_STATS
    Hits() const {
        return SumAccess(true);
    }
    /* ===================================================================== */
    inline CACHE_STATS
    Misses() const {
        return SumAccess(false);
    }
    /* ===================================================================== */
    inline CACHE_STATS
    Accesses() const {
        return Hits() + Misses();
    }
    /* ===================================================================== */
    inline CACHE_STATS
    IHits() const {
        return SumFetchAccess(true);
    }   // Gang-Ryung Uh
    /* ===================================================================== */
    inline CACHE_STATS
    IMisses() const {
        return SumFetchAccess(false);
    }// Gang-Ryung Uh
    /* ===================================================================== */
    inline CACHE_STATS
    ICoherenceMisses() const {
        return SumFetchAccess(false);
    }// Adrian Perez

    /* ===================================================================== */
    inline CACHE_STATS
    IAccesses() const {
        return IHits() + IMisses() + ICoherenceMisses();
    }// Gang-Ryung Uh


    /* ===================================================================== */
    inline CACHE_STATS
    Cost() const {
        return _cost;
    }

    /* ===================================================================== */
    inline UINT32
    GetIndex(CACHE_TAG tag) const
    {
        return (tag & _setIndexMask);
    }

    /* ===================================================================== */
    inline VOID
    SplitAddress(const ADDRINT addr, CACHE_TAG & tag, UINT32 & setIndex) const
    {
        tag = addr >> _lineShift;
        setIndex = tag & _setIndexMask;
    }

    /* ===================================================================== */
    inline VOID
    SplitAddress(const ADDRINT addr, CACHE_TAG & tag, UINT32 & setIndex, UINT32 & lineIndex) const
    {
        const UINT32 lineMask = _lineSize - 1;
        lineIndex = addr & lineMask;
        SplitAddress(addr, tag, setIndex);
    }

    string StatsLong(string prefix = "", CACHE_TYPE = CACHE_TYPE_DCACHE) const;
};

/* ===================================================================== */
CACHE_BASE::CACHE_BASE(std::string name, UINT32 cacheSize, UINT32 lineSize, UINT32 associativity)
  : _name(name),
    _cacheSize(cacheSize),
    _lineSize(lineSize),
    _associativity(associativity),
    _lineShift(FloorLog2(lineSize)),
    _setIndexMask((cacheSize / (associativity * lineSize)) - 1)
{
    ASSERTX(IsPower2(_lineSize));
    ASSERTX(IsPower2(_setIndexMask + 1));

    for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
    {
        _access[accessType][CACHE_HIT] = 0;
        _access[accessType][CACHE_MISS] = 0;
        _access[accessType][COHERENCE_MISS] = 0;
    }

    _fetch[false] = 0; // Gang-Ryung Uh: for instruction cache
    _fetch[true]  = 0; // Gang-Ryung Uh: for instruction cache
    _cost = 0;// Adrian Perez
}

/* ===================================================================== */
/*!
 *  @brief Stats output method
 */
/* ===================================================================== */
string
CACHE_BASE::StatsLong(string prefix, CACHE_TYPE cache_type) const
{
    const UINT32 headerWidth = 19;
    const UINT32 numberWidth = 12;

    string out;

    out += prefix + _name + "\n";

    if (cache_type != CACHE_TYPE_ICACHE) {

        for (UINT32 i = 0; i < ACCESS_TYPE_NUM; i++)
        {
            const ACCESS_TYPE accessType = ACCESS_TYPE(i);

            std::string type(accessType == ACCESS_TYPE_LOAD ? "Load" : "Store");

            out += prefix + ljstr(type + "-Hits:                ", headerWidth)
                   + mydecstr(Hits(accessType), numberWidth)  +
                   "  " +fltstr(100.0 * Hits(accessType) / Accesses(accessType), 2, 6) + "%\n";

            out += prefix + ljstr(type + "-Misses:              ", headerWidth)
                   + mydecstr(Misses(accessType), numberWidth) +
                   "  " +fltstr(100.0 * Misses(accessType) / Accesses(accessType), 2, 6) + "%\n";

            out += prefix + ljstr(type + "-Coherence Misses:    ", headerWidth)
                   + mydecstr(CoherenceMisses(accessType), numberWidth) +
                   "  " +fltstr(100.0 * CoherenceMisses(accessType) / Accesses(accessType), 2, 6) + "%\n";


            out += prefix + ljstr(type + "-Accesses:            ", headerWidth)
                   + mydecstr(Accesses(accessType), numberWidth) +
                   "  " +fltstr(100.0 * Accesses(accessType) / Accesses(accessType), 2, 6) + "%\n";

            out += prefix + "\n";

        }

        out += prefix + ljstr("Total-Hits:            ", headerWidth)
               + mydecstr(Hits(), numberWidth) +
               "  " +fltstr(100.0 * Hits() / Accesses(), 2, 6) + "%\n";

        out += prefix + ljstr("Total-Misses:          ", headerWidth)
               + mydecstr(Misses(), numberWidth) +
               "  " +fltstr(100.0 * Misses() / Accesses(), 2, 6) + "%\n";

        out += prefix + ljstr("Total-Accesses:        ", headerWidth)
               + mydecstr(Accesses(), numberWidth) +
               "  " +fltstr(100.0 * Accesses() / Accesses(), 2, 6) + "%\n";
        out += "\n";

        out += prefix + ljstr("Estimated Cost:        ", headerWidth)
               + mydecstr(Cost(), numberWidth) + " Cycles\n";
        out += "\n";

    }
    else
    {
        // Instruction cache

        out += prefix + ljstr("Total-Hits:                      ", headerWidth)
               + mydecstr(IHits(), numberWidth) +
               "  " +fltstr(100.0 * IHits() / IAccesses(), 2, 6) + "%\n";

        out += prefix + ljstr("Total-Misses:                   ", headerWidth)
               + mydecstr(IMisses(), numberWidth) +
               "  " +fltstr(100.0 * IMisses() / IAccesses(), 2, 6) + "%\n";

        out += prefix + ljstr("Total-Coherence Misses:         ", headerWidth)
               + mydecstr(ICoherenceMisses(), numberWidth) +
               "  " +fltstr(100.0 * ICoherenceMisses() / IAccesses(), 2, 6) + "%\n";


        out += prefix + ljstr("Total-Accesses:                ", headerWidth)
               + mydecstr(IAccesses(), numberWidth) +
               "  " +fltstr(100.0 * IAccesses() / IAccesses(), 2, 6) + "%\n";
        out += "\n";

        out += prefix + ljstr("Estimated Cost:       ", headerWidth)
               + mydecstr(Cost(), numberWidth) +
               "  " +fltstr(100.0 * Accesses() / Accesses(), 2, 6) + " Cycles\n";
        out += "\n";


    }
    return out;
}

/* ===================================================================== */
/*!
 *  @brief Templated cache class with specific cache set allocation policies
 *
 *  All that remains to be done here is allocate and deallocate the right
 *  type of cache sets.
 */
/* ===================================================================== */
#include "coherence.H"
template <class SET>
class CACHE : public CACHE_BASE
{
  private:
    SET * _sets;
    UINT32 interconnection;
    UINT32 write;

  public:
    BOOL valid;
    UINT32 mypid;
    CACHE **mymap;

    COHERENCE<SET,CACHE> *coherence;

    // constructors/destructors
    CACHE(std::string name,     UINT32 cacheSize,       UINT32 lineSize,
          UINT32 associativity, UINT32 coherence_write, UINT32 coherence_protocol,
          UINT32 interconnection_type, CACHE ** map, UINT32 pid)
      : CACHE_BASE(name, cacheSize, lineSize, associativity)
    {
        _sets = new SET[NumSets()];

        for (UINT32 i = 0; i < NumSets(); i++)
        {
            _sets[i].SetAssociativity(associativity);
        }
        valid = true;
        mypid = pid;
        mymap = map;
        if (coherence_write == WRITE_THROUGH_NO_ALLOCATE && coherence_protocol == PROTOCOL_VI)
            coherence = new UPDATE<SET,CACHE>(map, mypid);
        else if (coherence_write == WRITE_BACK_ALLOCATE && coherence_protocol == PROTOCOL_MSI)
            coherence = new DIR_MSI<SET,CACHE>(map, mypid);
        else
        {
            perror("CACHE_BASE: Invalid combination of cache write strategy and coherence protocol");
            exit(1);
        }

        interconnection = interconnection_type;
    }

    ~CACHE()
    {
        delete[] _sets;
    }

    // modifiers
    /// Cache access from addr to addr+size-1
    HIT_MISS_TYPES Load(ADDRINT addr, UINT32 size);
    HIT_MISS_TYPES Store(ADDRINT addr, UINT32 size);

    /// Cache access at addr that does not span cache lines
    HIT_MISS_TYPES LoadSingleLine(ADDRINT addr);
    HIT_MISS_TYPES StoreSingleLine(ADDRINT addr);

    /// Cache access from addr to addr+size-1
    HIT_MISS_TYPES Fetch(ADDRINT addr, UINT32 size);

    /// Cache access at addr that does not span cache lines
    HIT_MISS_TYPES FetchSingleLine(ADDRINT addr);

    /// Return the Cache Line in set
    SET *getSET(CACHE_TAG tag);
};

/* ===================================================================== */
template <class SET>
SET*
CACHE<SET>::getSET(CACHE_TAG tag)
{
    UINT32 setIndex = GetIndex(tag);
    return &_sets[setIndex];
}


/* ===================================================================== */
/*!
 *  @return true if all accessed cache lines hit
 */
/* ===================================================================== */

template <class SET>
HIT_MISS_TYPES
CACHE<SET>::Store(ADDRINT addr, UINT32 size)
{
    const ADDRINT highAddr = addr + size;
    HIT_MISS_TYPES allHit = CACHE_HIT;
    const ADDRINT lineSize = LineSize();
    const ADDRINT notLineMask = ~(lineSize - 1);
    INT32 cost;
    do
    {
        CACHE_TAG tag;
        UINT32 setIndex;
        SplitAddress(addr, tag, setIndex);
        SET & set = _sets[setIndex];
        HIT_MISS_TYPES localHit = coherence->processWrite(&set, tag, &cost);
	allHit = localHit;
        addr = (addr & notLineMask) + lineSize; // start of next cache line
    }
    while (addr < highAddr);

    _access[ACCESS_TYPE_STORE][allHit]++;
    _cost += cost;
    return allHit;
}
/* ===================================================================== */
/*!
 *  @return true if all accessed cache lines hit
 */
/* ===================================================================== */

template <class SET>
HIT_MISS_TYPES
CACHE<SET>::Load(ADDRINT addr, UINT32 size)
{
    const ADDRINT highAddr = addr + size;
    HIT_MISS_TYPES allHit = CACHE_HIT;

    const ADDRINT lineSize = LineSize();
    const ADDRINT notLineMask = ~(lineSize - 1);
    INT32 cost;

    do
    {
        CACHE_TAG tag;
        UINT32 setIndex;
        SplitAddress(addr, tag, setIndex);
        SET & set = _sets[setIndex];
        HIT_MISS_TYPES localHit = coherence->processRead(&set, tag, &cost);
	allHit = localHit;
        addr = (addr & notLineMask) + lineSize; // start of next cache line
    }
    while (addr < highAddr);

    _access[ACCESS_TYPE_LOAD][allHit]++;
    _cost += cost;

    return allHit;
}

/* ===================================================================== */
/*!
 *  @return true if accessed cache line hits
 */
/* ===================================================================== */
template <class SET>
HIT_MISS_TYPES
CACHE<SET>::LoadSingleLine(ADDRINT addr)
{
    INT32 cost;
    CACHE_TAG tag;
    UINT32 setIndex;
    SplitAddress(addr, tag, setIndex);
    SET & set = _sets[setIndex];
    HIT_MISS_TYPES hit = coherence->processRead(&set, tag, &cost);
    _access[ACCESS_TYPE_LOAD][hit]++;
    _cost += cost;
    return hit;
}

/* ===================================================================== */
/*!
 *  @return true if accessed cache line hits
 */
/* ===================================================================== */
template <class SET>
HIT_MISS_TYPES
CACHE<SET>::StoreSingleLine(ADDRINT addr)
{
    INT32 cost;
    CACHE_TAG tag;
    UINT32 setIndex;
    SplitAddress(addr, tag, setIndex);
    SET & set = _sets[setIndex];
    HIT_MISS_TYPES hit = coherence->processWrite(&set, tag, &cost);
    _access[ACCESS_TYPE_STORE][hit]++;
    _cost += cost;
    return hit;
}

/* ===================================================================== */
/*!
 *  @return true if accessed instruction cache line hits
 */
/* ===================================================================== */
template <class SET>
HIT_MISS_TYPES
CACHE<SET>::Fetch(ADDRINT addr, UINT32 size)
{
    const ADDRINT highAddr = addr + size;
    HIT_MISS_TYPES allHit = CACHE_HIT;
    const ADDRINT lineSize = LineSize();
    const ADDRINT notLineMask = ~(lineSize - 1);
    INT32 cost;

    do
    {
        CACHE_TAG tag;
        UINT32 setIndex;
        SplitAddress(addr, tag, setIndex);
        SET & set = _sets[setIndex];
	HIT_MISS_TYPES localHit = coherence->processRead(&set, tag, &cost);
        allHit = localHit;
        addr = (addr & notLineMask) + lineSize; // start of next cache line
    }
    while (addr < highAddr);
    _fetch[allHit]++;
    _cost += cost;
    return allHit;
}

/* ===================================================================== */
/*!
 *  @return true if accessed instruction cache line hits
 */
/* ===================================================================== */
template <class SET>
HIT_MISS_TYPES
CACHE<SET>::FetchSingleLine(ADDRINT addr)
{
    INT32 cost;
    CACHE_TAG tag;
    UINT32 setIndex;
    SplitAddress(addr, tag, setIndex);
    SET  &set = _sets[setIndex];
    HIT_MISS_TYPES hit = coherence->processRead(&set, tag, &cost);
    _fetch[hit]++;
    _cost += cost;
    return hit;
}

#endif // PIN_CACHE_H
