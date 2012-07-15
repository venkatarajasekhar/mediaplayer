#ifndef _hashtable_h_
#define _hashtable_h_

#include "Utility/database/ebglobal.h"

class TableEntry
{
public:
    void *key;
    EbDataHandle handle;
    TableEntry *next;
    TableEntry (EbDataHandle handle, void *key, TableEntry *next);
    
};

extern EbBoolean CompareRaw (void *a, void *b);

class EbHashTable
{
public:
    //EbArray array;
    TableEntry *array[EB_MAX_NUM_HASH_TABLE_ENTRIES];

    EbHashTable();
    
    ~EbHashTable ();
    
    void CleanUp ();

    void AddNode (void *key, EbDataHandle handle);
    
    EbUint32 getHash(void *key);
    
    TableEntry *key2entry (void *key);
    
    EbDataHandle key2handle (void *key);

    EbResult RemoveNode (void *key);

};



#endif


