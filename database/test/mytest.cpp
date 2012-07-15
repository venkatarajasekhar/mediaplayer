/***********************************************************************
 * ebase library - an embedded C++ database library                    *
 * Copyright (C) 2001, Jun Sun, jsun@mvista.com or jsun@junsun.net.    *
 *                                                                     *
 * This library is free software; you can redistribute it and/or       *
 * modify it under the terms of the GNU Lesser General Public          *
 * License as published by the Free Software Foundation; either        *
 * version 2.1 of the License, or (at your option) any later version.  *
 ***********************************************************************
 */
#include "Utility/database/ebase.h"
#include "../src/hashtable.h"
#include "../src/ebase_impl.h"
#ifndef WIN32
#include <unistd.h>
#endif


#ifdef MS_MEM_DEBUG
#include <crtdbg.h>
#endif /* MS_MEM_DEBUG */

#define    TEST(E1) { if (E1) { printf("line %d : %s\n", __LINE__, #E1); } else  {printf("\nline %d : Dough! %s \nEnter infinite loop at %s:%d.\n\n", __LINE__, #E1, __FILE__, __LINE__); for(;;); }} 

//Ebase db;

class EbaseBatchTest
{
public:
    static void DisplayDB (Ebase *db);
    static void batch ();
    static void batch2 ();
    static EbInt32 GetRecordOffsetFromKey (Ebase &db, void *key)
    {
        return db.GetRecordOffsetFromKey (key);
    }
    static void EbaseBatchTest::KeyValueWithLockTest ();
};

void 
EbaseBatchTest::DisplayDB(Ebase *db)
{
    EbaseDef *pdef = db->GetEbaseDef();
    EbUint32 i,j;
    printf("Display db according to index 0\n");
    for (i=0; i< 78; i++) printf("-");  printf("\n");
    for (i=0; i< pdef->NumberOfFields(); i++) {
        printf("%s", pdef->GetFieldName(i));
        if (i != 0) printf("\t");
    }
    printf("\n");
    for (i=0; i< 78; i++) printf("-");  printf("\n");
    for (i=0; i< db->NumberOfRecords(); i++) {
        EbRecord *pRecord = db->GetRecord(i);
        for (j=0; j< pdef->NumberOfFields(); j++) {
            switch (pdef->GetFieldDataType(j)) {
             case EB_INT32 : 
                printf(" %d\t", pRecord->GetFieldInt32(j));
                break;
//             case EB_STRING :
            //            printf("%s\t", pRecord->GetFieldString(j));
            //            break;
            
             default :
                break;
            }
        }
        db->FreeRecord(pRecord);
        printf("\n");
    }
    for (i=0; i< 78; i++) printf("-");  printf("\n");
}


void 
HashTableTest ()
{
    EbHashTable hashtable;
    unsigned int key1 [] = { 0x8,  0xaaaabbbb,  };
    unsigned int key2 [] = { 0xC,  0x99998888, 0x77776666, };
    unsigned int key3 [] = { 0x10, 0x11112222, 0x33334444, 0x55556666, };
    
    hashtable.AddNode(key1, (EbUint32)0xaaaa);
    hashtable.AddNode(key2, (EbUint32)0xbbbb);
    //hashtable.AddNode(key3, (EbDataHandle)0xcccc);
 

    {
        EbDataHandle handle1 = hashtable.key2handle(key1);
        TEST (handle1 == 0x0000aaaa);
        EbDataHandle handle2 = hashtable.key2handle(key2);
        TEST (handle2 == 0x0000bbbb);
        EbDataHandle handle3 = hashtable.key2handle(key3);
        TEST (handle3 == EB_INVALID_HANDLE);
    }
    hashtable.RemoveNode(key2);
    {
        EbDataHandle handle1 = hashtable.key2handle(key1);
        TEST (handle1 == 0x0000aaaa);
        EbDataHandle handle2 = hashtable.key2handle(key2);
        TEST (handle2 == EB_INVALID_HANDLE);
        EbDataHandle handle3 = hashtable.key2handle(key3);
        TEST (handle3 == EB_INVALID_HANDLE);
    }
}

void 
KeyValueTest ()
{
    EbResult res;
    unsigned int key1[] = {
        0x8,
            0xaabbccdd,
    };
    unsigned int value1[] = 
    {
        0xC,
            0xaaaabbbb, 
            0xccccdddd, 
    };
    char *tmp;
    {
        Ebase db;
    
        if ( -1 == Ebase::CreateDatabase("anotherdb"))
        {
          unlink ("anotherdb");
          Ebase::CreateDatabase("anotherdb");
        }

        db.OpenDatabase("anotherdb");
        db.CloseDatabase();

        db.OpenDatabase("anotherdb");
        res = db.CreateKey(key1);
        db.CloseDatabase();

        db.OpenDatabase("anotherdb");
        res = db.GetValue(key1, (void**)&tmp);
        TEST (tmp[0] == 0x4)
        TEST (tmp[1] == 0x0)
        TEST (tmp[2] == 0x0)
        TEST (tmp[3] == 0x0)
        db.CloseDatabase();
    }
    {
        Ebase db;
        db.OpenDatabase("anotherdb");
        res = db.PutValue(key1, value1);
        db.CloseDatabase();

        db.OpenDatabase("anotherdb");
        res = db.GetValue(key1, (void**)&tmp);
        TEST (((unsigned int*)tmp)[0] == 0xC)
        TEST (((unsigned int*)tmp)[1] == 0xaaaabbbb)
        TEST (((unsigned int*)tmp)[2] == 0xccccdddd)
        db.CloseDatabase();
    
        db.OpenDatabase("anotherdb");
        EbInt32 a = EbaseBatchTest::GetRecordOffsetFromKey(db, key1);
        TEST (a == 0)
        db.DeleteKey(key1);
        EbInt32 b = EbaseBatchTest::GetRecordOffsetFromKey(db, key1);
        TEST (b == -1)
        db.CloseDatabase();

        db.OpenDatabase("anotherdb");
        EbInt32 c = EbaseBatchTest::GetRecordOffsetFromKey(db, key1);
        TEST (c == -1)
        db.CloseDatabase();
    }
    {
        Ebase db;
        db.OpenDatabase("anotherdb");
        res = db.CreateKey(EbTypedKey(2));
        db.CloseDatabase();
        
        db.OpenDatabase("anotherdb");
        res = db.GetValue(EbTypedKey(2), (void**)&tmp);
        TEST (tmp[0] == 0x4)
            TEST (tmp[1] == 0x0)
            TEST (tmp[2] == 0x0)
            TEST (tmp[3] == 0x0)
            db.CloseDatabase();
    }    
    {
        Ebase db;
        db.OpenDatabase("anotherdb");
        res = db.CreateKey(EbTypedKey("teststringkey"));
        db.CloseDatabase();
        
        db.OpenDatabase("anotherdb");
        res = db.GetValue(EbTypedKey("teststringkey"), (void**)&tmp);
        TEST (tmp[0] == 0x4)
            TEST (tmp[1] == 0x0)
            TEST (tmp[2] == 0x0)
            TEST (tmp[3] == 0x0)
            db.CloseDatabase();
    }
}
void 
EbaseBatchTest::KeyValueWithLockTest ()
{
    EbResult res;
    unsigned int key1[] = {
        0x8,
            0xaabbccdd,
    };
    unsigned int value1[] = 
    {
        0xC,
            0xaaaabbbb, 
            0xccccdddd, 
    };
    char *tmp;
    {
        Ebase db;
        EbLock aLock ("anotherdb");
        if ( -1 == Ebase::CreateDatabase("anotherdb"))
        {
            unlink ("anotherdb");
            Ebase::CreateDatabase("anotherdb");
        }

        try
        {
          db.OpenDatabase("anotherdb", &aLock);
          TEST (1 == 0);
        }
        catch (EbError &e)
        {
            TEST(e.GetError() == EBE_DATABASE_FILE_NOT_LOCKED);
            TEST(strcmp(e.GetStringName(), "EBE_DATABASE_FILE_NOT_LOCKED") == 0);
        }
        TEST (aLock.doLock () == true);
        db.OpenDatabase("anotherdb", &aLock);
        TEST (aLock.releaseLock () == false);
        db.CloseDatabase(&aLock);
        TEST (aLock.releaseLock () == true);

        EbLock anotherLock ("anotherdb");
        TEST (anotherLock.doLock () == true);
        try
        {
          db.OpenDatabase("anotherdb", &aLock);
          TEST (1==0);
        }
        catch (EbError &e)
        {
            TEST(e.GetError() == EBE_DATABASE_FILE_NOT_LOCKED);
            TEST(strcmp(e.GetStringName(), "EBE_DATABASE_FILE_NOT_LOCKED") == 0);
        }
        db.OpenDatabase("anotherdb", &anotherLock);
        res = db.CreateKey(key1);
        db.CloseDatabase(&anotherLock);

        db.OpenDatabase("anotherdb", &anotherLock);
        res = db.GetValue(key1, (void**)&tmp);
        TEST (tmp[0] == 0x4)
        TEST (tmp[1] == 0x0)
        TEST (tmp[2] == 0x0)
        TEST (tmp[3] == 0x0)
        db.CloseDatabase(&anotherLock);
        anotherLock.releaseLock ();
    }
    {
        // two EbLock objects
        EbLock aLock("anotherdb"), anotherLock ("anotherdb");  
        // "anotherdb" is locked by the first EbLock object.
        // so the second object cannot lock the file again.
        TEST (aLock.doLock () == true);
        TEST (anotherLock.doLock () == false);
        // but if the second object has to lock the file,
        // it has the power to release the lock.
        //
        // After lock released bye the second EbLock object, 
        // the first EbLock object does not "own" the lock.
        TEST (aLock.isLockingFile () == true);
        TEST (anotherLock.releaseLock () == true);
        TEST (aLock.isLockingFile () == false);
        TEST (aLock.releaseLock () == false);
        
        // the second EbLock object lock the file.
        TEST (anotherLock.doLock ());
        TEST (anotherLock.isLockingFile () == true);
        
        // Now second EbLock is not locking the file.
        TEST (aLock.isLockingFile () == false);
        
        
        Ebase db;
//         try
//         { 
//           db.OpenDatabase("anotherdb", &aLock);
//           TEST (1 == 0);
//         }
//         catch (EbError &e)
//         {
//             TEST(e.GetError() == EBE_DATABASE_FILE_NOT_LOCKED);
//             TEST(strcmp(e.GetStringName(), "EBE_DATABASE_FILE_NOT_LOCKED") == 0);
//         }
        
        db.OpenDatabase ("anotherdb", &anotherLock);
        res = db.CreateKey(EbTypedKey(2));
        db.CloseDatabase(&anotherLock);
        
        
        db.OpenDatabase("anotherdb", &anotherLock);
        res = db.GetValue(EbTypedKey(2), (void**)&tmp);
        TEST (tmp[0] == 0x4)
        TEST (tmp[1] == 0x0)
        TEST (tmp[2] == 0x0)
        TEST (tmp[3] == 0x0)
        db.CloseDatabase(&anotherLock);
        anotherLock.releaseLock ();
    }    
    {
        Ebase db;
        EbLock unmatchedLock("unmatchedname"), matchedLock("anotherdb");
        
        unmatchedLock.doLock (); 
        matchedLock.doLock ();
        
        try
        {
          db.OpenDatabase("anotherdb", &unmatchedLock);
          TEST (0==1);
        }
        catch (EbError &e)
        {
            TEST(e.GetError() == EBE_DATABASE_FILE_NOT_LOCKED);
            TEST(strcmp(e.GetStringName(), "EBE_DATABASE_FILE_NOT_LOCKED") == 0);
        }
        db.OpenDatabase("anotherdb", &matchedLock);
        res = db.CreateKey(EbTypedKey("teststringkey"));
        db.CloseDatabase(&matchedLock);
        
        db.OpenDatabase("anotherdb", &matchedLock);
        res = db.GetValue(EbTypedKey("teststringkey"), (void**)&tmp);
        TEST (tmp[0] == 0x4)
        TEST (tmp[1] == 0x0)
        TEST (tmp[2] == 0x0)
        TEST (tmp[3] == 0x0)
        
        try
        {
          db.CloseDatabase (&unmatchedLock);
          TEST (0==1);
        }
        catch (EbError &e)
        {
            TEST(e.GetError() == EBE_DATABASE_FILE_NOT_LOCKED);
            TEST(strcmp(e.GetStringName(), "EBE_DATABASE_FILE_NOT_LOCKED") == 0);
        }
        db.CloseDatabase(&matchedLock);
        
        unmatchedLock.releaseLock ();
        matchedLock.releaseLock ();
    }
}

void 
EbaseBatchTest::batch()
{
    Ebase db;
    if ( -1 == Ebase::CreateDatabase("junkdb"))
    {
        unlink ("junkdb");
        Ebase::CreateDatabase("junkdb");
    }
    
    // open & close
    db.OpenDatabase("junkdb");


    db.CloseDatabase();

    // get db def
    {
        db.OpenDatabase("junkdb");
        EbaseDef *def = db.GetEbaseDef();
        TEST(def != NULL);
   
        EbInt32 fid = def->GetFieldId("key");
        TEST(fid >= 0);
        TEST(strcmp(def->GetFieldName(fid), "key") == 0);
        TEST(def->GetFieldDataType(fid) == EB_RAW);
        
        fid = def->GetFieldId("value");
        TEST(fid >= 0);
        TEST(strcmp(def->GetFieldName(fid), "value") == 0);
        TEST(def->GetFieldDataType(fid) == EB_RAW);
        
        db.CloseDatabase();    
    }

    
    // test new record and commit new record
    {
        db.OpenDatabase("junkdb");

        // test basics
        TEST(db.NumberOfRecords() == 0);

        // add a new record
        EbRecord *pRecord = db.NewRecord();
        TEST(pRecord != NULL);
        TEST(pRecord->GetFieldInt32(EB_RECORD_ID) == -1);
        db.CommitRecord(pRecord);
        TEST(db.NumberOfRecords() == 1);
        TEST(pRecord->GetFieldInt32(EB_RECORD_ID) == 1);

        // test if the new record survive close and re-open
        db.CloseDatabase();
        db.OpenDatabase("junkdb");

        TEST(db.NumberOfRecords() == 1);
        pRecord = db.GetRecord(0);

        TEST(pRecord != NULL);
        TEST(pRecord->GetFieldInt32(EB_RECORD_ID) == 1);

        // this should not do anything
        db.CommitRecord(pRecord);

        // test other fields
        char *ptr = (char*)pRecord->GetField("key");
        TEST (ptr[0] == 0x4);
        TEST (ptr[1] == 0x0);
        TEST (ptr[2] == 0x0);
        TEST (ptr[3] == 0x0);
        
        ptr = (char*)pRecord->GetField("value");
        TEST (ptr[0] == 0x4);
        TEST (ptr[1] == 0x0);
        TEST (ptr[2] == 0x0);
        TEST (ptr[3] == 0x0);

        db.CloseDatabase();
    }
    // test modifying fields
    {
        db.OpenDatabase("junkdb");
        EbRecord *pRecord = db.GetRecord(0);

        // test other fields
        char *ptr;
        TEST(pRecord->GetFieldInt32(EB_RECORD_ID) == 1);
        ptr = (char*)pRecord->GetField("key");
        TEST (ptr[0] == 0x4);
        TEST (ptr[1] == 0x0);
        TEST (ptr[2] == 0x0);
        TEST (ptr[3] == 0x0);
        
        ptr = (char*)pRecord->GetField("value");
        TEST (ptr[0] == 0x4);
        TEST (ptr[1] == 0x0);
        TEST (ptr[2] == 0x0);
        TEST (ptr[3] == 0x0);
        
        try {
            pRecord->PutFieldInt32(EB_RECORD_ID, 3);
            TEST(1 == 0);
        } catch (EbError e) {
            TEST(e.GetError() == EBE_INVALID_FIELD);
            TEST(strcmp(e.GetStringName(), "EBE_INVALID_FIELD") == 0);
        }

        pRecord = db.NewRecord();

        // put field
        const int testkey_length = 2;
        unsigned int *testkey = new unsigned int [testkey_length];
        testkey[0] = 0x8;
        testkey[1] = 0xaabbccdd;

        const int testvalue_length = 3;
        //unsigned int *testvalue = new unsigned int [testvalue_length];
        //testvalue[0] = 0xC;
        //testvalue[1] = 0x11223344;
        //testvalue[2] = 0x55667788;
        //delete [] testvalue;
        
        pRecord->PutField("key", (void *)testkey);
        char *p = (char*)pRecord->GetField("key");
        int i ;
		for (i = 0; i < testkey_length*4; ++i)
        {
            TEST (p[i] == ((char*)testkey)[i]);
        }
    
        unsigned int testvalue2 [] = 
        {
            0xC,
            0x99999999,
            0x88888888,
        };

        pRecord->PutFieldAlloc("value", (void*)testvalue2);
        p = 0;
        p = (char*)pRecord->GetField("value");
		for (i = 0; i < sizeof(testvalue2); ++i)
        {
            TEST (p[i] == ((char*)testvalue2)[i]);
        }

        char * temp;
        temp = (char*)pRecord->GetFieldAlloc("value");
        for (i = 0; i < testvalue_length*4; ++i)
        {
            TEST (temp[i] == ((char*)testvalue2)[i]);
        }
        delete[] temp;

        // commit record
        db.CommitRecord(pRecord);

        // check fields again
        TEST(db.NumberOfRecords() == 2);
        p = 0;
        p = (char*)pRecord->GetField("key");
        for (i = 0; i < testkey_length*4; ++i)
        {
            TEST (p[i] == ((char*)testkey)[i]);
        }
        

        p = 0;
        p = (char*)pRecord->GetField("value");
        for (i = 0; i < testvalue_length*4; ++i)
        {
            TEST (p[i] == ((char*)testvalue2)[i]);
        }

        // check fields again from file
        pRecord = db.GetRecord(1);
//        TEST(pRecord->GetFieldInt32("key") == 5);
        p = 0;
        p = (char*)pRecord->GetField("key");
        for (i = 0; i < testkey_length*4; ++i)
        {
            TEST (p[i] == ((char*)testkey)[i]);
        }
        p = 0;
        p = (char*)pRecord->GetField("value");
        for (i = 0; i < testvalue_length*4; ++i)
        {
            TEST (p[i] == ((char*)testvalue2)[i]);
        }

        //..//..
        EbInt32 x = db.GetRecordOffsetFromKey (testkey);
        //..//..

            //        // test big variable
        //        for (i=0; i< 255; i++) {
        //            buf[i] = 'a';
        //        }
        //        pRecord->PutFieldString("value", buf);
        //        db.CommitRecord(pRecord);
        //        TEST(buf == pRecord->GetFieldStringAlloc("value"));
        //        TEST(strcmp(buf, pRecord->GetFieldString("value")) == 0);
        

        // test persistency of big variable
        db.CloseDatabase();
        db.OpenDatabase("junkdb");
        pRecord = db.GetRecord(1);
        p = 0;
        p = (char*)pRecord->GetField("value");
        for (i = 0; i < testvalue_length*4; ++i)
        {
            TEST (p[i] == ((char*)testvalue2)[i]);
        }
        
        db.CloseDatabase();
    }

    // test deleting record
    {
        db.OpenDatabase("junkdb");
        EbRecord *pRecord = db.GetRecord(0);
        TEST(db.NumberOfRecords() == 2);
        db.DeleteRecord(pRecord);
        TEST(db.NumberOfRecords() == 1);
        db.CloseDatabase();

        db.OpenDatabase("junkdb");
        pRecord = db.GetRecord(0);
        TEST(pRecord->GetFieldInt32((EbUint32)0) == 2);
        DisplayDB ( &db);

        db.CloseDatabase();
    }

    // we succeed
    printf("batch test is successful!\n");
    
}


/* ALLOCATION HOOK FUNCTION
   -------------------------
   An allocation hook function can have many, many different
   uses. This one simply logs each allocation operation in a file.
*/
#ifdef MS_MEM_DEBUG
extern "C" {
int __cdecl MyAllocHook(
   int      nAllocType,
   void   * pvData,
   size_t   nSize,
   int      nBlockUse,
   long     lRequest,
   const unsigned char * szFileName,
   int      nLine
   )
{
   char *operation[] = { "", "allocating", "re-allocating", "freeing" };
   char *blockType[] = { "Free", "Normal", "CRT", "Ignore", "Client" };

   if ( nBlockUse == _CRT_BLOCK )   // Ignore internal C runtime library allocations
      return( 1 );

   _ASSERT( ( nAllocType > 0 ) && ( nAllocType < 4 ) );
   _ASSERT( ( nBlockUse >= 0 ) && ( nBlockUse < 5 ) );

   fprintf( stdout, 
            "Memory operation in %s, line %d: %s a %d-byte '%s' block (# %ld)\n",
            szFileName, nLine, operation[nAllocType], nSize, 
            blockType[nBlockUse], lRequest );
   if ( pvData != NULL )
      fprintf( stdout, " at %X", pvData );

   return( 1 );         // Allow the memory operation to proceed
}
}

#endif /* MS_MEM_DEBUG */

// Macros for setting or clearing bits in the CRT debug flag 
#ifdef _DEBUG
#define  SET_CRT_DEBUG_FIELD(a)   _CrtSetDbgFlag((a) | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))
#define  CLEAR_CRT_DEBUG_FIELD(a) _CrtSetDbgFlag(~(a) & _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))
#else
#define  SET_CRT_DEBUG_FIELD(a)   ((void) 0)
#define  CLEAR_CRT_DEBUG_FIELD(a) ((void) 0)
#endif 

// extern void CheckAddress(void *ptr);


int main()
{
#ifdef MS_MEM_DEBUG
    // Send all reports to STDOUT
    _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
    _CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
    _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
    _CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT );
    _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
    _CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDOUT );
   SET_CRT_DEBUG_FIELD( _CRTDBG_LEAK_CHECK_DF | _CRTDBG_DELAY_FREE_MEM_DF );
   // _CrtSetAllocHook( &MyAllocHook );
#endif /* MS_MEM_DEBUG */

   // CheckAddress((void*)0x447760);

   try {
       EbaseBatchTest::batch ();
       KeyValueTest();
       HashTableTest();
       EbaseBatchTest::KeyValueWithLockTest ();
    }
//   try { EbaseBatchTest::batch2(); }
   catch (EbError e) {
    printf("caught exception : %d, %s\n", e.GetError(), e.GetStringName());
    }

#ifdef MS_MEM_DEBUG
    _CrtMemDumpAllObjectsSince( NULL );
#endif /* MS_MEM_DEBUG */
    return 0;
}
