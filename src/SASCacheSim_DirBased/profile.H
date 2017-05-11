// #pragma once

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

typedef enum
{
    LOAD = 0,
    STORE,
    ACCESS_TYPE_NUM
}ACCESS_TYPE;

typedef enum
{
    CACHE_MISS = 0,
    CACHE_HIT
}HIT_MISS_TYPES;

const uint64_t THRESHOLD = 100;

class Stat
{
public:
    Stat(): hit(0), miss(0), cycle(0) {}
    uint64_t hit = 0;
    uint64_t miss = 0;
    uint64_t cycle = 0;
};

class Access_Stat
{
public:
    Stat load;
    Stat store;
    uint64_t count = 0;
};

class Profile
{
public:
    Profile()
    {
        for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; ++accessType)
        {
            _access[accessType][CACHE_HIT] = 0;
            _access[accessType][CACHE_MISS] = 0;

            _cycle[accessType][CACHE_HIT] = 0;
            _cycle[accessType][CACHE_MISS] = 0;
        }

        _cost = 0;
    }

    /* ===================================================================== */
    UINT64 SumAccess(bool hit) const
    {
        UINT64 sum = 0;

        for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
        {
            sum += _access[accessType][hit];
        }
        return sum;
    }

    /* ===================================================================== */
    UINT64 SumCycle(HIT_MISS_TYPES type) const
    {
        UINT64 sum = 0;

        for (UINT32 accessType = 0; accessType < ACCESS_TYPE_NUM; accessType++)
        {
            sum += _cycle[accessType][type];
        }
        return sum;
    }

    /* ===================================================================== */
    inline UINT64 HitCycles(ACCESS_TYPE accessType) const
    {
        return _cycle[accessType][CACHE_HIT];
    }

    /* ===================================================================== */
    inline UINT64 MissCycles(ACCESS_TYPE accessType) const
    {
        return _cycle[accessType][CACHE_MISS];
    }

    /* ===================================================================== */
    inline UINT64 TotalCycles(ACCESS_TYPE accessType) const
    {
        return HitCycles(accessType) + MissCycles(accessType);
    }

    /* ===================================================================== */
    inline UINT64 TotalHitCycles() const
    {
        return SumCycle(CACHE_HIT);
    }

    /* ===================================================================== */
    inline UINT64 TotalMissCycles() const
    {
        return SumCycle(CACHE_MISS);
    }

    /* ===================================================================== */
    inline UINT64 Hits(ACCESS_TYPE accessType) const
    {
        return _access[accessType][CACHE_HIT];
    }

    /* ===================================================================== */
    inline UINT64 Misses(ACCESS_TYPE accessType) const
    {
        return _access[accessType][CACHE_MISS];
    }

    /* ===================================================================== */
    inline UINT64 Accesses(ACCESS_TYPE accessType) const
    {
        return Hits(accessType) + Misses(accessType);
    }

    /* ===================================================================== */
    inline UINT64 Hits() const
    {
        return SumAccess(true);
    }

    /* ===================================================================== */
    inline UINT64 Misses() const
    {
        return SumAccess(false);
    }

    /* ===================================================================== */
    inline UINT64 Accesses() const
    {
        return Hits() + Misses();
    }

    /* ===================================================================== */
    std::string stats_to_string(std::string prefix) const
    {
        std::stringstream out;
        out << prefix << std::setw(25) << std::left << " "
                      << std::setw(15) << std::left << "Count"
                      << std::setw(15) << std::left << "%"
                      << std::setw(15) << std::left << "Cycles"
                      << std::setw(10) << std::left << "%" << '\n';

        for (UINT32 i = 0; i < ACCESS_TYPE_NUM; i++)
        {
            const ACCESS_TYPE accessType = ACCESS_TYPE(i);

            std::string type(accessType == LOAD ? "Load" : "Store");

            out << prefix << std::setw(25) << std::left << (type + "-Hits")
                          << std::setw(15) << std::left << Hits(accessType)
                          << std::setw(15) << std::left << (100.0 * Hits(accessType) / Accesses(accessType))
                          << std::setw(15) << std::left << HitCycles(accessType)
                          << std::setw(10) << std::left <<  (100.0 * HitCycles(accessType) / TotalCycles(accessType))  << "\n";

            out << prefix << std::setw(25) << std::left << (type + "-Misses:")
                          << std::setw(15) << std::left << Misses(accessType)
                          << std::setw(15) << std::left << (100.0 * Misses(accessType) / Accesses(accessType))
                          << std::setw(15) << std::left << MissCycles(accessType)
                          << std::setw(10) << std::left << (100.0 * MissCycles(accessType) / TotalCycles(accessType)) << "\n";

            out << prefix << std::setw(25) << std::left << (type + "-Total Accesses:")
                          << std::setw(15) << std::left << Accesses(accessType)
                          << std::setw(15) << std::left << 100.0
                          << std::setw(15) << std::left << TotalCycles(accessType)
                          << std::setw(10) << std::left << 100.0 << "\n\n";
        }

        out << prefix << std::setw(25) << std::left << "Total-Hits:"
                      << std::setw(15) << std::left << Hits()
                      << std::setw(15) << std::left << (100.0 * Hits() / Accesses())
                      << std::setw(15) << std::left << TotalHitCycles()
                      << std::setw(10) << std::left << (100.0 * TotalHitCycles() / _cost) << "\n";

        out << prefix << std::setw(25) << std::left << "Total-Misses:"
                      << std::setw(15) << std::left << Misses()
                      << std::setw(15) << std::left << (100.0 * Misses() / Accesses())
                      << std::setw(15) << std::left << TotalMissCycles()
                      << std::setw(10) << std::left << (100.0 * TotalMissCycles() / _cost) << "\n\n";

        out << prefix << std::setw(25) << std::left << "Estimated Cost:"
                      << std::setw(15) << std::left << Accesses()
                      << std::setw(15) << std::left << 100.0
                      << std::setw(15) << std::left << _cost
                      << std::setw(10) << std::left << 100.0 << "\n\n";

        return out.str();
    }

public:
    UINT64  _access[ACCESS_TYPE_NUM][2];
    UINT64  _cycle[ACCESS_TYPE_NUM][2];
    UINT64  _cost;
};
