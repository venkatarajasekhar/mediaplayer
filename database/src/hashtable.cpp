#include "hashtable.h"

TableEntry::TableEntry(EbDataHandle handle, void *key, TableEntry *next) {
    this->handle = handle;
    EbUint32 size = *(EbRawDataSize*)key;
    EB_ASSERT (size >= 0);
    this->key = new EbUint8 [size];
    memcpy (this->key, key, size);
    this->next   = next;
}

EbHashTable::EbHashTable() 
{   
    memset (array, 0, sizeof(array));
}

EbHashTable::~EbHashTable () 
{
    CleanUp ();
}

void 
EbHashTable::CleanUp ()
{
    TableEntry *p = (TableEntry*)array[0];
    for (EbUint32 i = 0; i < EB_MAX_NUM_HASH_TABLE_ENTRIES; ++i )
    {
        for (TableEntry *ptr = (TableEntry*)array[i]; ptr != 0; )
        {
            TableEntry *tmp = ptr->next;
            delete [] ((EbUint8*)(ptr->key));
            delete ptr;
            ptr = tmp;
        }
    }
    memset (array, 0, sizeof(array));
}

void 
EbHashTable::AddNode (void *key, EbDataHandle handle)
{
    EbUint32 hash = getHash (key);
    TableEntry *p = new TableEntry (handle, key, array[hash]);
    array[hash] = p;
}

EbUint32 
EbHashTable::getHash(void *key)
{
    EbRawDataSize size = *(EbRawDataSize*)key;
    EbUint32 result = 0;
    
    for (EbUint32 i = 0; i < size; ++i)
    {
        result += ((unsigned char*)key)[i];
    }
    return result % EB_MAX_NUM_HASH_TABLE_ENTRIES;
}

TableEntry *
EbHashTable::key2entry (void *key)
{
    EbUint32 hash = getHash (key);
    for (TableEntry *ptr = (TableEntry *)array[hash]; ptr != 0; ptr = ptr->next)
    {
        if (CompareRaw(key, ptr->key) == EB_TRUE)
            return ptr;
    }
    return 0;
}

EbDataHandle 
EbHashTable::key2handle (void *key)
{
    if (TableEntry * p = key2entry(key)) return p->handle;
    return EB_INVALID_HANDLE;
}


EbResult 
EbHashTable::RemoveNode (void *key)
{
    EbUint32 hash = getHash (key);
    TableEntry *prev = 0, *ptr = 0;
    
    for (ptr = (TableEntry *)array[hash]; ptr != 0; prev = ptr, ptr = ptr->next)
    {
        if (CompareRaw(key, ptr->key) == EB_TRUE)
        {
            if (prev == 0)
            {
                array[hash] = ptr->next;
            }
            else
            {
                prev->next = ptr->next;
            }
            delete [] ((EbUint8*)(ptr->key));
            delete ptr;
            return EB_SUCCESS;
        }
    }
    return EB_FAILURE;
}