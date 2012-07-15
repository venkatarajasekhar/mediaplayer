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
#ifndef WIN32
#include <unistd.h>
#endif

#include <assert.h>

#ifdef MS_MEM_DEBUG
#include <crtdbg.h>
#endif /* MS_MEM_DEBUG */

#define    TEST(E1) { if (E1) { printf("line %d : %s\n", __LINE__, #E1); } else  {printf("\nline %d : Dough! %s \nEnter infinite loop at %s:%d.\n\n", __LINE__, #E1, __FILE__, __LINE__); for(;;); }} 


void 
KeyValueTest ()
{
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

    unsigned int *key2 = 0;
    key2 = new unsigned int [512/sizeof (unsigned int)];
    memset (key2, 0xab, 512);

    int i;
    for (i = 0; i < 512; ++i)
    {
        unsigned char *ptr = (unsigned char*)key2 + i;
        assert (*ptr == 0xab);
    }
    key2[0] = 512;
    
    unsigned int *value2 = 0;
    value2 = new unsigned int [512/sizeof (unsigned int)];
    memset (value2, 0xcd, 512);

    for (i = 0; i < 512; ++i)
    {
        unsigned char *ptr = (unsigned char*)value2 + i;
        assert (*ptr == 0xcd);
    }
    value2[0] = 512;
    
    {
        Ebase db;

        // make sure there is no database file with the same name
        unlink ("junkdb");
        unlink ("junkdb.lock");

        // ok, the database file is created.
        TEST (0 == Ebase::CreateDatabase("junkdb"));
        
        // it fails to re-create an existing database file.
        TEST (-1 == Ebase::CreateDatabase ("junkdb"));
        
        // open an existing database file.
        TEST (0 == db.OpenDatabase("junkdb"));

        // close it.
        TEST (0 == db.CloseDatabase());

        // db could do nothing before opening a database file
        TEST (EB_FAILURE == db.CreateKey(key1));
        TEST (EB_FAILURE == db.DeleteKey(key1));
        TEST (EB_FAILURE == db.GetValue(key1, (void**)&tmp));
        TEST (EB_FAILURE == db.GetValue(key1, (void**)&tmp));
        
        // re-open it and create a key 
        TEST (0 == db.OpenDatabase("junkdb"));
        TEST (EB_SUCCESS == db.CreateKey(key1));
        TEST (0 == db.CloseDatabase());

        // re-open again to check the "default" value of created key
        TEST (0 == db.OpenDatabase("junkdb"));
        TEST (EB_SUCCESS == db.GetValue(key1, (void**)&tmp));
        TEST (tmp[0] == 0x4)
        TEST (tmp[1] == 0x0)
        TEST (tmp[2] == 0x0)
        TEST (tmp[3] == 0x0)
        TEST (0 == db.CloseDatabase());
    }
    {
        // a new Ebase object.
        // it's a different Ebase object from the previous one.
        Ebase db;
        
        // open the database file and put a value of key1
        TEST (0 == db.OpenDatabase("junkdb"));
        TEST (EB_SUCCESS == db.PutValue(key1, value1));
        TEST (0 == db.CloseDatabase());
        TEST (0 == db.OpenDatabase("junkdb"));
        TEST (EB_SUCCESS == db.GetValue(key1, (void**)&tmp));
        TEST (((unsigned int*)tmp)[0] == 0xC)
        TEST (((unsigned int*)tmp)[1] == 0xaaaabbbb)
        TEST (((unsigned int*)tmp)[2] == 0xccccdddd)
        TEST (0 == db.CloseDatabase());
    
        // re-open the database file and delete the key
        TEST (0 == db.OpenDatabase("junkdb"));
        TEST (0 == db.DeleteKey(key1));
        TEST (EB_FAILURE == db.GetValue(key1, (void**)&tmp));
        TEST (0 == db.CloseDatabase());
    }
    
    {
        // a new Ebase object.
        // it's a different Ebase object from the previous one.
        Ebase db;
        
        // open the database file and put a value of key2
        TEST (0 == db.OpenDatabase("junkdb"));
        TEST (EB_SUCCESS == db.CreateKey(key2));
        TEST (EB_SUCCESS == db.PutValue(key2, value2));
        TEST (0 == db.CloseDatabase());
        TEST (0 == db.OpenDatabase("junkdb"));
        TEST (EB_SUCCESS == db.GetValue(key2, (void**)&tmp));
        TEST (((unsigned int*)tmp)[0] == 512)
        
        for (i = 1; i < 512/sizeof (unsigned int); ++i)
        {
            TEST (((unsigned int*)tmp)[i] == 0xcdcdcdcd)
        }
        TEST (0 == db.CloseDatabase());
        
        // re-open the database file and delete the key
        TEST (0 == db.OpenDatabase("junkdb"));
        TEST (0 == db.DeleteKey(key2));
        TEST (EB_FAILURE == db.GetValue(key2, (void**)&tmp));
        TEST (0 == db.CloseDatabase());
    }

    {
        Ebase db;

        // Use class EbTypedKey to simplify the construction of a key.
        TEST (0 == db.OpenDatabase("junkdb"));
        TEST (EB_SUCCESS == db.CreateKey(EbTypedKey(2)));
        TEST (0 == db.CloseDatabase());
        
        TEST (0 == db.OpenDatabase("junkdb"));
        TEST (EB_SUCCESS == db.GetValue(EbTypedKey(2), (void**)&tmp));
        TEST (tmp[0] == 0x4)
        TEST (tmp[1] == 0x0)
        TEST (tmp[2] == 0x0)
        TEST (tmp[3] == 0x0)
        TEST (0 == db.CloseDatabase());
    }    
    {
        Ebase db;

        // Use a C-style string to create an EbTypedKey object
        EbTypedKey stringkey("teststringkey");
        
        TEST (0 == db.OpenDatabase("junkdb"));
        TEST (EB_SUCCESS == db.CreateKey(stringkey));
        TEST (0 == db.CloseDatabase());
        
        TEST (0 == db.OpenDatabase("junkdb"));
        TEST (EB_SUCCESS == db.GetValue(stringkey, (void**)&tmp));
        TEST (tmp[0] == 0x4)
        TEST (tmp[1] == 0x0)
        TEST (tmp[2] == 0x0)
        TEST (tmp[3] == 0x0)
        TEST  (0 == db.CloseDatabase());
    }
    //..//..
    {
        Ebase db;
        
        // Use a C-style string to create an EbTypedKey object
        EbTypedKey stringkey("teststringkey");
        
        TEST (0 == db.OpenDatabase("junkdb"));
        TEST (EB_SUCCESS == db.GetValue(stringkey, (void**)&tmp));
        TEST (tmp[0] == 0x4)
        TEST (tmp[1] == 0x0)
        TEST (tmp[2] == 0x0)
        TEST (tmp[3] == 0x0)

        // now we put a new value of STRINGKEY
        TEST (EB_SUCCESS == db.PutValue(stringkey, value1));

        // the previous result is not updated
        TEST (tmp[0] == 0x4)
        TEST (tmp[1] == 0x0)
        TEST (tmp[2] == 0x0)
        TEST (tmp[3] == 0x0)

        // re-get the value again
        TEST (EB_SUCCESS == db.GetValue(stringkey, (void**)&tmp));

        // we get the new value of STRINGKEY
        TEST (((unsigned int*)tmp)[0] == 0xC)
        TEST (((unsigned int*)tmp)[1] == 0xaaaabbbb)
        TEST (((unsigned int*)tmp)[2] == 0xccccdddd)
            
        TEST  (0 == db.CloseDatabase());
    }
    //..//..
    {
        Ebase db1, db2;

        // db1 opens junkdb successfully.
        TEST (0 == db1.OpenDatabase("junkdb"));

        // You cannot re-create a locked database file (junkdb is locked by db1)
        TEST (-2 == Ebase::CreateDatabase("junkdb"));

        // db1 cannot re-open junkdb if not closing it.
        // -3 indicates the Ebase object has opened some database file.
        TEST (-3 == db1.OpenDatabase("junkdb"));

        // db2 cannot re-open junkdb if db1 not closing it.
        // -1 indicates the database file has been opened by another Ebase object.
        TEST (-1 == db2.OpenDatabase("junkdb"));

        // db1 closes the db.
        TEST (0 == db1.CloseDatabase ());

        // -2 indicates the database file does not exist.
        TEST (-2 == db1.OpenDatabase("non-existing-db"));

    }
    {
        Ebase db;
        
        // db opens junkdb successfully.
        TEST (0 == db.OpenDatabase("junkdb"));

        // db closes it.
        TEST (0 == db.CloseDatabase ());
        
        // db re-closes it and fails.
        TEST (-1 == db.CloseDatabase ());
        
    }
    delete [] key2;
    delete [] value2;

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

   try {
       KeyValueTest();
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
