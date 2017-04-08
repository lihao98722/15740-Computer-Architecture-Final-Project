#ifndef _COHERENCE_H_
#define _COHERENCE_H_

#include <sstream>
#include <iostream>
#include <fstream>
#include <map>
#include "cache.H"
#include "callbacks.H"

class DIRECTORY_LINE
{
  public:
    INT32 presenceBits;
    bool writeBit;

    DIRECTORY_LINE()
    {
        presenceBits = 0;
        writeBit = false;
    }
};
extern UINT32 active_threads;
extern UINT32 pow2processors;
extern int lock_id;
extern PIN_LOCK mapLock;
extern UINT32 getHomeNode(CACHE_TAG addr);
extern SIMULATION_CONFIG catch_all_config;
extern std::map<CACHE_TAG, DIRECTORY_LINE *> dir_map;

static INT32 COST_HIT = 1;
static INT32 COST_LOCAL_MISS = 30;
static INT32 COST_REMOTE_MISS = 60;
static INT32 COST_BLOCK_TRANSFER = 90;

/* ===================================================================== */
template <class SET, class CACHE>
class COHERENCE
{
  public:
    int mypid;
    UINT32 print_range;
    UINT32 limit;
    UINT64 process_transactions;
    CACHE **map;

    // constructors/destructors
    COHERENCE(CACHE **my_map, UINT32 pid)
    {
        map = my_map;
        mypid = pid;
        print_range = 100000;
        limit = 0;
    }
    virtual HIT_MISS_TYPES processRead(SET *set, CACHE_TAG tag, INT32 *cost)
    {
        return CACHE_MISS;
    }
virtual HIT_MISS_TYPES processWrite(SET *set, CACHE_TAG tag, INT32 *cost)
    {
        return CACHE_MISS;

    }

    virtual VOID ReadMiss(CACHE_TAG tag, UINT32 localNode){}
    virtual VOID WriteMiss(CACHE_TAG tag, UINT32 localNode){}
    virtual VOID Invalidate(SET *set, CACHE_TAG tag){}
    virtual VOID Fetch(SET *set, CACHE_TAG tag){}
    virtual VOID FetchAndInvalidate(SET *set, CACHE_TAG tag){}
    virtual VOID DataValueReply(){}
    virtual VOID DataWriteBack(){}
    virtual DIRECTORY_LINE *getDirLine(CACHE_TAG tag){return NULL;}


};




/* ===================================================================== */
template <class SET, class CACHE>
class UPDATE:public COHERENCE<SET,CACHE>
{
  private:

  public:
    UPDATE(CACHE **my_map, UINT32 pid):COHERENCE<SET, CACHE>(my_map, pid){}
    virtual VOID DataValueReply(){} // Dummy
    virtual VOID DataWriteBack (){} // Dummy

    /* ===================================================================== */
    virtual DIRECTORY_LINE *
    getDirLine(CACHE_TAG tag)
    {
        typedef std::pair<CACHE_TAG, DIRECTORY_LINE *> Ptr;
        std::map<CACHE_TAG, DIRECTORY_LINE *>::iterator dir_map_it;
        dir_map_it = dir_map.find(tag);
        if (dir_map_it == dir_map.end())
        {
            (VOID)dir_map.insert(Ptr(tag, new DIRECTORY_LINE()));
        }
        dir_map_it = dir_map.find(tag); // We should be able to simplify this code later
        return dir_map_it->second;
    }

    /* ===================================================================== */
    virtual VOID
    WriteMiss(CACHE_TAG tag, UINT32 localNode)
    {
        DIRECTORY_LINE *dirPtr = this->getDirLine(tag);
        INT32 tmp = dirPtr->presenceBits;
        if (dirPtr->writeBit == false)
        {
            for (UINT32 i = 0 ; i < pow2processors ; i++)
            {
                // Invalidation to all Nodes who had presence bit ON
                if (tmp & 1 && localNode != i)
                    this->dirTransaction(INVALIDATE, tag, i);
                tmp = tmp >> 1;
            }
            dirPtr->writeBit = true;
        }
        else
        {
            INT32 remoteNode = FloorLog2(dirPtr->presenceBits);
            this->dirTransaction(FETCH_INVALIDATE, tag, remoteNode);
        }
        dirPtr->presenceBits = 1 << localNode;
    }

    /* ===================================================================== */
    virtual VOID
    ReadMiss(CACHE_TAG tag, UINT32 localNode)
    {
        DIRECTORY_LINE *dirPtr = this->getDirLine(tag);
        if (dirPtr->writeBit == true)
        {
            INT32 remoteNode = FloorLog2(dirPtr->presenceBits);
            this->dirTransaction(FETCH, tag, remoteNode);
            dirPtr->writeBit = false;
        }
        // Turns Bit Presence On
        dirPtr->presenceBits = dirPtr->presenceBits ^ (1 << localNode);
        this->DataValueReply();
    }

    /* ===================================================================== */
    VOID
    Fetch(SET *set, CACHE_TAG tag)
    {
        // Dummy
    }

    /* ===================================================================== */
    VOID
    Invalidate(SET *set, CACHE_TAG tag)
    {
        set->SetState(set->Find(tag),CACHE_STATES::INVALID);
    }

    /* ===================================================================== */
    VOID
    FetchAndInvalidate(SET *set, CACHE_TAG tag)
    {
        set->SetState(set->Find(tag),CACHE_STATES::INVALID);
    }

    /* ===================================================================== */
    INT32
    dirTransaction(DIR_ACTION type, CACHE_TAG tag, INT32 destination)
    {
        INT32 cost = 0;
        if ( this->mypid == destination)
            cost = COST_LOCAL_MISS;
        else
            cost = COST_REMOTE_MISS;

        SET *set = this->map[destination]->getSET(tag);
        switch (type)
        {
          case READ_MISS:
            this->map[destination]->coherence->ReadMiss(tag, this->mypid);
            break;

          case WRITE_MISS:
            this->map[destination]->coherence->WriteMiss(tag, this->mypid);
            break;

          case INVALIDATE:
            this->map[destination]->coherence->Invalidate(set, tag);
            break;

          case FETCH:
            this->map[destination]->coherence->Fetch(set, tag);
            break;


          case FETCH_INVALIDATE:
            this->map[destination]->coherence->FetchAndInvalidate(set, tag);
            break;

          default:
            break;
        }
        return cost;
    }


    /* ===================================================================== */
    /* Default for Parent Class is Valid Invalid Protocol                    */
    /* ===================================================================== */
    virtual HIT_MISS_TYPES
    processRead(SET *set, CACHE_TAG tag, INT32 *cost){
        UINT32 destination = getHomeNode(tag);
        HIT_MISS_TYPES result = CACHE_MISS;
        INT32 index = set->Find(tag);
        if (index >= 0) // Tag Found
        {
            switch (set->GetState(index)) // Check Cache Line State
            {
              case CACHE_STATES::VALID:
                result = CACHE_HIT;
                *cost = COST_HIT;
                break;

              case CACHE_STATES::INVALID:
                result = COHERENCE_MISS;
                *cost = COST_LOCAL_MISS;
                *cost += this->dirTransaction(READ_MISS, tag, destination);
                set->SetState(index,CACHE_STATES::VALID);
                break;

              default:
                break;
            }
        }
        else // Tag Not Found
        {
            *cost = COST_BLOCK_TRANSFER;
            *cost += this->dirTransaction(READ_MISS, tag, destination);
            INT32 newIndex = set->Replace(tag);
            set->SetState(newIndex,CACHE_STATES::VALID);
        }
        return result;
    }

    /* ===================================================================== */
    /* Deafult for Parent Class is Valid Invalid Protocol                    */
    /* ===================================================================== */
    virtual HIT_MISS_TYPES
    processWrite(SET *set, CACHE_TAG tag, INT32 *cost)
    {
        HIT_MISS_TYPES result = CACHE_MISS;
        UINT32 destination = getHomeNode(tag);
        INT32 index = set->Find(tag);
        if (index >= 0) // Tag Found
        {
            switch (set->GetState(index)) // Check Cache Line State
            {
              case CACHE_STATES::VALID:
                result = CACHE_HIT;
                *cost = COST_HIT;
                this->dirTransaction(WRITE_MISS, tag, destination);
                break; // No State Change No Bus Action Needed

              case CACHE_STATES::INVALID:
                *cost = COST_REMOTE_MISS;
                result= COHERENCE_MISS;
                this->dirTransaction(WRITE_MISS, tag, destination);
                break;

              default:
                break;
            }
        }
        return result;
    }

};
/* ===================================================================== */
/*!
 *  @brief Cache Coherence Protocol MSI
 */
/* ===================================================================== */
template <class SET, class CACHE>
class DIR_MSI:public UPDATE<SET,CACHE>
{
  private:
  public:
    DIR_MSI(CACHE **my_map, UINT32 pid):UPDATE<SET,CACHE>(my_map, pid){}
    VOID Fetch(SET *set, CACHE_TAG tag);
    VOID ReadMiss(CACHE_TAG tag, UINT32 localNode);
    VOID WriteMiss(CACHE_TAG tag, UINT32 localNode);
    VOID Invalidate(SET *set, CACHE_TAG tag);
    VOID DataWriteBack();
    VOID DataValueReply();
    VOID FetchAndInvalidate(SET *set, CACHE_TAG tag);
    HIT_MISS_TYPES processRead(SET *set, CACHE_TAG tag, INT32 *cost);
    HIT_MISS_TYPES processWrite(SET *set, CACHE_TAG tag, INT32 *cost);

};

/* ===================================================================== */
template <class SET, class CACHE>
HIT_MISS_TYPES
DIR_MSI<SET,CACHE>::processRead(SET *set, CACHE_TAG tag, INT32 *cost){
    UINT32 destination = getHomeNode(tag);
    HIT_MISS_TYPES result = CACHE_MISS;
    INT32 index = set->Find(tag);
    if (index >= 0) // Tag Found
    {
        switch (set->GetState(index)) // Check Cache Line State
        {
          case CACHE_STATES::MODIFIED:
          case CACHE_STATES::SHARED:
            result = CACHE_HIT;
            *cost = COST_HIT;
            break; // No State Change or Bus Action Required in these two cases

          case CACHE_STATES::INVALID:
            result = COHERENCE_MISS;
            *cost = COST_LOCAL_MISS;
            *cost += this->dirTransaction(READ_MISS, tag, destination);
            set->SetState(index,CACHE_STATES::SHARED);
            break;

          default:
            break;
        }
    }
    else // Tag Not Found
    {
        *cost = COST_BLOCK_TRANSFER;
        *cost += this->dirTransaction(READ_MISS, tag, destination);
        INT32 newIndex = set->Replace(tag);
        set->SetState(newIndex,CACHE_STATES::SHARED);
    }
    return result;
};


/* ===================================================================== */
template <class SET, class CACHE>
HIT_MISS_TYPES
DIR_MSI<SET,CACHE>::processWrite(SET *set, CACHE_TAG tag, INT32 *cost){
    HIT_MISS_TYPES result = CACHE_MISS;
    UINT32 destination = getHomeNode(tag);
    INT32 index = set->Find(tag);
    if (index >= 0) // Tag Found
    {
        switch (set->GetState(index)) // Check Cache Line State
        {
          case CACHE_STATES::MODIFIED:
            result = CACHE_HIT;
            *cost = COST_HIT;
            break; // No State Change No Bus Action Needed

          case CACHE_STATES::SHARED:
            result = CACHE_HIT;
            *cost = COST_REMOTE_MISS;
            this->dirTransaction(WRITE_MISS, tag, destination);
            set->SetState(index,CACHE_STATES::MODIFIED);
            break;

          case CACHE_STATES::INVALID:
            *cost = COST_REMOTE_MISS;
            result= COHERENCE_MISS;
            this->dirTransaction(WRITE_MISS, tag, destination);
            set->SetState(index,CACHE_STATES::MODIFIED);
            break;

          default:
            break;
        }
    }
    else // On Write we always allocate for this protocol
    {
        *cost = COST_BLOCK_TRANSFER;
        set->Replace(tag);
        this->dirTransaction(WRITE_MISS, tag, destination);
        set->SetState(index,CACHE_STATES::MODIFIED);
    }
    return result;
}

/* ===================================================================== */
template <class SET, class CACHE>
VOID
DIR_MSI<SET,CACHE>::ReadMiss(CACHE_TAG tag, UINT32 localNode)
{
    DIRECTORY_LINE *dirPtr = this->getDirLine(tag);
    if (dirPtr->writeBit == true)
    {
        INT32 remoteNode = FloorLog2(dirPtr->presenceBits);
        this->dirTransaction(FETCH, tag, remoteNode);
        dirPtr->writeBit = false;
    }
    // Turns Bit Presence On
    dirPtr->presenceBits = dirPtr->presenceBits ^ (1 << localNode);
    this->DataValueReply();
}
/* ===================================================================== */
template <class SET, class CACHE>
VOID
DIR_MSI<SET,CACHE>::DataValueReply()
{
    // Dummy
}
/* ===================================================================== */
template <class SET, class CACHE>
VOID
DIR_MSI<SET,CACHE>::DataWriteBack()
{
    // Dummy
}
/* ===================================================================== */
template <class SET, class CACHE>
VOID
DIR_MSI<SET,CACHE>::Fetch(SET *set, CACHE_TAG tag)
{
    set->SetState(set->Find(tag),CACHE_STATES::SHARED);
}
/* ===================================================================== */
template <class SET, class CACHE>
VOID
DIR_MSI<SET,CACHE>::FetchAndInvalidate(SET *set, CACHE_TAG tag)
{
    set->SetState(set->Find(tag),CACHE_STATES::INVALID);
}
/* ===================================================================== */
template <class SET, class CACHE>
VOID
DIR_MSI<SET,CACHE>::Invalidate(SET *set, CACHE_TAG tag)
{
    set->SetState(set->Find(tag),CACHE_STATES::INVALID);
}

/* ===================================================================== */
template <class SET, class CACHE>
VOID
DIR_MSI<SET,CACHE>::WriteMiss(CACHE_TAG tag, UINT32 localNode)
{
    DIRECTORY_LINE *dirPtr = this->getDirLine(tag);
    INT32 tmp = dirPtr->presenceBits;
    if (dirPtr->writeBit == false)
    {
        for (UINT32 i = 0 ; i < pow2processors ; i++)
        {
            // Invalidation to all Nodes who had presence bit ON
            if (tmp & 1 && localNode != i)
                this->dirTransaction(INVALIDATE, tag, i);
            tmp = tmp >> 1;
        }
        dirPtr->writeBit = true;
    }
    else
    {
        INT32 remoteNode = FloorLog2(dirPtr->presenceBits);
        this->dirTransaction(FETCH_INVALIDATE, tag, remoteNode);
    }
    dirPtr->presenceBits = 1 << localNode;
}

#endif // _COHERENCE_H_
