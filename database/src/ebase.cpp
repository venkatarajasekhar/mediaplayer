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


#include "internal.h"
#include "datafile/include/datafile.h"
#include "fielddef.h"
#include "indexdriver.h"
#include "datadriver.h"

#include "ebarrays.h"
#include "hashtable.h"

#include <fcntl.h>
#include <sys/types.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>

#ifndef WIN32
#include <pthread.h>
#include <semaphore.h>
#endif

/***********************************************************************
 * Implements Ebase Class.
 ***********************************************************************
 */

// string constants
const char EB_RECORD_ID[] = "RecordId";

/***********************************************************************
 * constructor and destructor
 ***********************************************************************
 */


Ebase::Ebase()
: _pLock ( NULL )
{
    _hdr._major = EB_MAJOR_NUMBER;
    _hdr._minor = EB_MINOR_NUMBER;
    _hdr._dbDefHandle =
	_hdr._indexDefHandle =
        _hdr._indexEntryHandle = EB_INVALID_HANDLE;
    _hdr._numRecords = 0;

    _file = NULL;
    _lpDataDriver = NULL;

    _Int32IndexDef = new EbIndexDef(this, 0);
    if (_Int32IndexDef == NULL)
    {
        EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
    }
    _Int32IndexEntry = new EbInt32IndexEntry ();
    if (_Int32IndexEntry == NULL)
    {
        EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
    }
    _hashtable = new EbHashTable ();
    if (_hashtable == NULL)
    {
        EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
    }
    _dbDef = new EbaseDef ();
    if (_dbDef == NULL)
    {
        EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
    }

#ifndef WIN32
    sem_init (&_sem, 0, 1);
#endif
}

Ebase::~Ebase()
{
    // a database object should not be destroyed if it is not closed
    if (_file != NULL) {
	_file->Close();
	delete _file;
    }

    if (_lpDataDriver != NULL) {
	delete[] _lpDataDriver;
    }

    delete _dbDef;
    delete _hashtable;
    delete _Int32IndexEntry;
    delete _Int32IndexDef;

#ifndef WIN32
    sem_destroy (&_sem);
#endif
}

/***********************************************************************
 * A bunch of save functions
 * We don't expect those functions to fail as the space should have already
 * been allocated for them.
 ***********************************************************************
 */
void
Ebase::SaveHeader()
{
    // database object is always stored in the fixed datum, handle 0
    // we don't expect this will fail here.
    if(_file->isDirty())
        EB_CHECK(_file->WriteData(0, &_hdr, 0, sizeof(_hdr)) == sizeof(_hdr));
}

void
Ebase::SaveIndexEntries()
{
    if(_file->isDirty())
        _Int32IndexEntry->Save(*_file, _hdr._indexEntryHandle);
}

/***********************************************************************
 * Create database
 *     IN THIS VERSION, WE ASSUME NO ERROR DUE TO LACK OF RESOURCE
 *      WILL HAPPEN.
 *
 * Error detection is not complete.
 ***********************************************************************
 */
/* static */ int
Ebase::CreateDatabase(const char * dbFileName)
{

    int f = open (dbFileName, O_WRONLY | O_CREAT | O_EXCL,
        S_IREAD | S_IWRITE);

    if (f != -1) // success
    {
      //	close (f);
    }
    else
    {
      printf ("U are creating an existing Database file\n");
      return -1;
    }

	EbLock aLock(f);
    if (false == aLock.doLock())
    {
        printf ("U cannot re-created a locked database file\n");
        return -2;
    }



    // create a dummy ebase object
    Ebase db;

    // create the datafile file
    EB_CHECK(DataFile::Create(dbFileName) == EB_SUCCESS);
    EB_CHECK((db._file = new DataFile()) != NULL);
    EB_CHECK(db._file->Open(dbFileName) == EB_SUCCESS);

    // reserve space for storing the db header
    EB_CHECK(db._file->ResizeData(0, sizeof(db._hdr)) == EB_SUCCESS);

    // leave space in data file for saving db def
    db._hdr._dbDefHandle = db._file->NewData(0);

    EbHandleArray *ha = new EbHandleArray();
    ha->Init(8,0);
    db._Int32IndexEntry->Set(ha);

    // set the next record ID
    db._hdr._nextRecordID = FIRST_RECORD_ID;
    db._hdr._numRecords = 0;

    // leave space for index def
    db._hdr._indexDefHandle = db._file->NewData(0);

    // leave space for index entries
    db._hdr._indexEntryHandle = db._file->NewData(0);
    EB_CHECK(db._Int32IndexEntry->ReserveSpace(*db._file, db._hdr._indexEntryHandle)
          == EB_SUCCESS);

    // set other data in db
    db._pActiveRecord = NULL;

    // close the database, and it will save all the data to the file
    // except for the databae def (since it cannot be changed)
    db.CloseDatabase();

    aLock.releaseLock();
    return 0;
}

/***********************************************************************
 * Open database
 *
 * Error detection is complete.
 * Error run-time reporting is not complete.
 * Error recovery is partial.
 *
 * return
 *     0  if success;
 *     -1 if the corresponding lock (of dbFileName) already exists;
 *
 ***********************************************************************
 */
int
Ebase::OpenDatabase(const char * dbFileName)
{
#ifndef WIN32
    EbSemLock aLock (&_sem);
    int trywait_ret = aLock.TryWait ();
    if (trywait_ret == EAGAIN)
    {
        printf ("sem_trywait () returns EAGAIN\n\n");
        return -10;
    }
    else if (trywait_ret != 0)
    {
        printf ("sem_trywait () returns a non-zero value\n\n");
        return -11;
    }
#endif

    return OpenDatabase(dbFileName, 0);
}

void Ebase::CheckForOpen (const char * dbFileName, EbLock *pLock)
{
    if ( false == pLock->isLockingFile ())
    {
        EB_ASSERT (0 == "EBE_DATABASE_FILE_NOT_LOCKED");
    }
    else if (true == pLock->getUsedFlag ())
    {
        EB_ASSERT (0 == "EBE_DATABASE_FILE_LOCK_USED_BY_OTHER_DB_OBJECT");
    }

}
int
Ebase::OpenDatabase(const char * dbFileName, EbLock *pLock)
{
    // the associated data file object should be null
    //EB_ASSERT(_file == NULL);
    if (_file != NULL)
    {
        return -3;
    }

    if (pLock) // backward compatibility
    {
        CheckForOpen(dbFileName, pLock);
        pLock->setUsedFlag ( true );
    }
    else
    {
    	int fd = open (dbFileName, O_WRONLY);
        _pLock = new EbLock (fd);
        if (false == _pLock->doLock())
        {
            printf ("U cannot open a locked database file\n");
            delete _pLock;
            _pLock = NULL;
            return -1;
        }

        // EB_ASSERT () inside
        CheckForOpen(dbFileName, _pLock);

        _pLock->setUsedFlag ( true );
    }

    strcpy (this->_dbFileName, dbFileName);

    EbUint32 i;


    // the database file should be a data file
    EB_CHECK((_file = new DataFile()) != NULL);
    if (_file->Open(dbFileName) != EB_SUCCESS) {
	    delete _file;
	    _file = NULL;
        _pLock->setUsedFlag(false);
        _pLock->releaseLock();
        delete _pLock;
        _pLock = NULL;
        return -2;
    }

    // read the header struct
    EB_VERIFY(_file->ReadData(0, &_hdr), == sizeof(_hdr));


    EB_CHECK(_Int32IndexEntry->Restore(*_file, _hdr._indexEntryHandle) == EB_SUCCESS);

    // check number of records
    EB_ASSERT(_Int32IndexEntry->IsValid() == EB_TRUE);
    EB_ASSERT(_hdr._numRecords == _Int32IndexEntry->GetHandleArray()->Size());

    // set modifiable flag in db def
    _dbDef->_modifiable = EB_FALSE;

    // we also need to find all data drivers
    _lpDataDriver = new DataDriver*[_dbDef->NumberOfFields()];
    if (_lpDataDriver == NULL)
    {
        _pLock->setUsedFlag(false);
        _pLock->releaseLock();
        EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
    }
    for (i=0; i< _dbDef->NumberOfFields(); i++) {
	_lpDataDriver[i] =
	    DataDriver::FindDataDriver(_dbDef->GetFieldDataType(i),
				       _dbDef->GetFieldFlag(i));
    }

    // init other transient data
    _pActiveRecord = NULL;

    // we should have exactly one index
    EB_ASSERT(_Int32IndexEntry->IsValid() == EB_TRUE);
    EB_ASSERT(EB_TRUE == _Int32IndexEntry->IsValid());

    // re-build the hashtable
    for (i = 0; i < _Int32IndexEntry->GetHandleArray()->Size (); ++i)
    {
        EbDataHandle handle = _Int32IndexEntry->GetHandleArray()->Get(i);
        EbRecord *pRecord = GetRecordByHandle(handle);
		if( pRecord == NULL )
			return -4;

        EB_ASSERT (pRecord != NULL);
        _hashtable->AddNode(pRecord->GetField("key"), handle);
    }
    return 0;
}

/***********************************************************************
 * Close database
 ***********************************************************************
 */
int
Ebase::CloseDatabase()
{
#ifndef WIN32
    EbSemLock aLock (&_sem);
    int trywait_ret = aLock.TryWait ();
    if (trywait_ret == EAGAIN)
    {
        printf ("sem_trywait () returns EAGAIN\n\n");
        return -10;
    }
    else if (trywait_ret != 0)
    {
        printf ("sem_trywait () returns a non-zero value\n\n");
        return -11;
    }
#endif

    return CloseDatabase(0);
}

void
Ebase::CheckForClose(EbLock *pLock)
{
    if ( 0 != strcmp (pLock->_dbFileName, this->_dbFileName) || false == pLock->isLockingFile ())
    {
        EB_ASSERT (0 == "EBE_DATABASE_FILE_NOT_LOCKED");
    }
    else if (false == pLock->getUsedFlag ())
    {
        EB_ASSERT (0 == "EBE_DATABASE_FILE_LOCK_UNUSED_BY_DB_OBJECT");
    }
}
int
Ebase::CloseDatabase(EbLock *pLock)
{
    if (pLock) // backward compatibility
    {
        CheckForClose(pLock);
    }

    if (_pLock != 0)  // for the call CloseDatabse (0) from CreateDatabase ()
        CheckForClose(_pLock);

    if (_file == 0)
    {
        return -1;
    }
    // save index def and index entries
//    SaveIndexDef();
    SaveIndexEntries();

    // save header
    SaveHeader();

    // delete _file object, this will close the file too
    delete _file;
    _file = NULL;

    // remove active records - this should be done before
    // other persistent data clean up
    FreeAllRecords();

    // delete other objects
    _Int32IndexEntry->DeleteAll();

    // delete the data driver list
    delete [] _lpDataDriver;
    _lpDataDriver = NULL;

    // clean up the hashtable
    _hashtable->CleanUp ();

    // release the lock
    if (pLock)
    {
        pLock->setUsedFlag (false);
    }


    if (_pLock)  // it is a member
    {
        _pLock->setUsedFlag(false);
        _pLock->releaseLock();
        delete _pLock;
        _pLock = 0;
    }

    return 0;
}

/***********************************************************************
 * get database definition
 ***********************************************************************
 */
EbaseDef *
Ebase::GetEbaseDef()
{
    return _dbDef;
}

#ifdef EB_DEBUG
/***********************************************************************
 * Dump() :
 *  Display ebase content.
 ***********************************************************************
 */
void
Ebase::Dump()
{
    if (IsOpen() == EB_FALSE) {
        printf("Ebase is closed\n");
    }

    printf("--------------------------------------------------\n");
    printf("EbHeader : \n");
    printf("\tmajor = %d\n", _hdr._major);
    printf("\tminor = %d\n", _hdr._minor);
    printf("\tnextRecordID = %d\n", _hdr._nextRecordID);
    printf("\tnumRecords = %d\n", _hdr._numRecords);
    printf("\t_dbDefHandle = %d\n", _hdr._dbDefHandle);
    printf("\t_indexDefHandle = %d\n", _hdr._indexDefHandle);
    printf("\tindexEntryHandle = %d\n", _hdr._indexEntryHandle);

    _Int32IndexEntry->Dump();

    _file->Dump();
}
#endif

EbBoolean CompareRaw (void *a, void *b)
{
    EbUint32 size_a = *((EbUint32*)a);
    EbUint32 size_b = *((EbUint32*)b);
    EB_ASSERT ( size_a >= 4 );
    EB_ASSERT ( size_b >= 4 );

    int min = size_a >= size_b ? size_b : size_a ;
    for (int i = 0; i < min; ++i)
    {
        if ( ((EbUint8*)a)[i] != ((EbUint8*)b)[i] ) return EB_FALSE;
    }
    if (size_a != size_b) return EB_FALSE;
    return EB_TRUE;
}

//..//..
// This uses linear-search to find the record id
//..//..
EbInt32
Ebase::GetRecordOffsetFromKey (void * decodedKey)
{
    for (EbUint32 i = 0; i < NumberOfRecords (); ++i)
    {
        EbRecord *p = GetRecord (i);
        if (CompareRaw(p->GetField("key"), decodedKey) == EB_TRUE)
        {
            return i;
        }
    }
    return -1;
}

#define USE_HASHTABLE
#ifndef USE_HASHTABLE
//..//.
// key not already existed
// Do you prefer HASHTABLE here ?
//..//..
EbResult
Ebase::CreateKey (void *key, int *errorCode)
{
#ifndef WIN32
    EbSemLock aLock (&_sem);
    int trywait_ret = aLock.TryWait ();
    if (trywait_ret == EAGAIN)
    {
        printf ("sem_trywait () returns EAGAIN\n\n");
        if (errorCode) *errorCode = -10;
        return EB_FAILURE;
    }
    else if (trywait_ret != 0)
    {
        printf ("sem_trywait () returns a non-zero value\n\n");
        if (errorCode) *errorCode = -11;
        return EB_FAILURE;
    }
#endif

    int offset = GetRecordOffsetFromKey(key);
    if (offset >= 0)
    {
        printf ("the key exists\n\n");
        if (errorCode) *errorCode = -12;
        return EB_FAILURE;
    }

    EbRecord *pRecord = NewRecord();
    pRecord->PutFieldAlloc("key", key);
    CommitRecord(pRecord);

//    _hashtable->AddNode(key, pRecord->GetField(EB_RECORD_ID));
    return EB_SUCCESS;
}


// key not already existed
EbResult
Ebase::DeleteKey (void *key, int *errorCode)
{
#ifndef WIN32
    EbSemLock aLock (&_sem);
    int trywait_ret = aLock.TryWait ();
    if (trywait_ret == EAGAIN)
    {
        printf ("sem_trywait () returns EAGAIN\n\n");
        if (errorCode) *errorCode = -10;
        return EB_FAILURE;
    }
    else if (trywait_ret != 0)
    {
        printf ("sem_trywait () returns a non-zero value\n\n");
        if (errorCode) *errorCode = -11;
        return EB_FAILURE;
    }
#endif

    int offset = GetRecordOffsetFromKey(key);
    if (offset < 0)
    {
        printf ("the key does not exist\n\n");
        if (errorCode) *errorCode = -12;
        return EB_FAILURE;
    }

    EbRecord *pRecord = GetRecord (offset);
    DeleteRecord(pRecord);
    return EB_SUCCESS;
}

//..//..
// key already existed
EbResult
Ebase::PutValue (void *key, void *value, int *errorCode)
{
#ifndef WIN32
    EbSemLock aLock (&_sem);
    int trywait_ret = aLock.TryWait ();
    if (trywait_ret == EAGAIN)
    {
        printf ("sem_trywait () returns EAGAIN\n\n");
        if (errorCode) *errorCode = -10;
        return EB_FAILURE;
    }
    else if (trywait_ret != 0)
    {
        printf ("sem_trywait () returns a non-zero value\n\n");
        if (errorCode) *errorCode = -11;
        return EB_FAILURE;
    }
#endif

    int offset = GetRecordOffsetFromKey(key);
    if (offset < 0)
    {
        printf ("the key does not exist\n\n");
        if (errorCode) *errorCode = -12;
        return EB_FAILURE;
    }

    EbRecord *p = GetRecord (offset);
    p->PutFieldAlloc("value", value);
    CommitRecord(p);
    return EB_SUCCESS;
}

// key already existed
EbResult
Ebase::GetValue (void *key, void **value, int *errorCode)
{
#ifndef WIN32
    EbSemLock aLock (&_sem);
    int trywait_ret = aLock.TryWait ();
    if (trywait_ret == EAGAIN)
    {
        printf ("sem_trywait () returns EAGAIN\n\n");
        if (errorCode) *errorCode = -10;
        return EB_FAILURE;
    }
    else if (trywait_ret != 0)
    {
        printf ("sem_trywait () returns a non-zero value\n\n");
        if (errorCode) *errorCode = -11;
        return EB_FAILURE;
    }
#endif
    int offset = GetRecordOffsetFromKey(key);
    if (offset < 0)
    {
        printf ("the key does not exist\n\n");
        if (errorCode) *errorCode = -12;
        return EB_FAILURE;
    }

    EbRecord *p = GetRecord (offset);
    *value = p->GetField("value");
    return EB_SUCCESS;
}

#else // USE_HASHTABLE

EbResult
Ebase::CreateKey (void *key, int *errorCode)
{
#ifndef WIN32
    EbSemLock aLock (&_sem);
    int trywait_ret = aLock.TryWait ();
    if (trywait_ret == EAGAIN)
    {
        printf ("sem_trywait () returns EAGAIN\n\n");
        if (errorCode) *errorCode = -10;
        return EB_FAILURE;
    }
    else if (trywait_ret != 0)
    {
        printf ("sem_trywait () returns a non-zero value\n\n");
        if (errorCode) *errorCode = -11;
        return EB_FAILURE;
    }
#endif

    if (!_file)
    {
        if (errorCode) *errorCode = -12;
        return EB_FAILURE;
    }

    EbDataHandle handle = _hashtable->key2handle(key);
    if (handle != EB_INVALID_HANDLE)
    {
        if (errorCode) *errorCode = -13;
        return EB_FAILURE;
    }
    else
    {
        EbRecord *pRecord = NewRecord();
        pRecord->PutFieldAlloc("key", key);
        CommitRecord(pRecord);
        _hashtable->AddNode(key, pRecord->_handle);
        return EB_SUCCESS;
    }
}


EbResult
Ebase::DeleteKey(void *key, int *errorCode)
{
#ifndef WIN32
    EbSemLock aLock (&_sem);
    int trywait_ret = aLock.TryWait ();
    if (trywait_ret == EAGAIN)
    {
        printf ("sem_trywait () returns EAGAIN\n\n");
        if (errorCode) *errorCode = -10;
        return EB_FAILURE;
    }
    else if (trywait_ret != 0)
    {
        printf ("sem_trywait () returns a non-zero value\n\n");
        if (errorCode) *errorCode = -11;
        return EB_FAILURE;
    }
#endif

    if (!_file)
    {
        if (errorCode) *errorCode = -12;
        return EB_FAILURE;
    }

    EbDataHandle handle = _hashtable->key2handle(key);
    if (handle == EB_INVALID_HANDLE)
    {
        if (errorCode) *errorCode = -13;
        return EB_FAILURE;
    }
    else
    {
        EbRecord *pRecord = GetRecordByHandle(handle);
        DeleteRecord(pRecord);
        _hashtable->RemoveNode(key);
        return EB_SUCCESS;
    }
}

EbResult
Ebase::PutValue(void *key, void *value, int *errorCode)
{
#ifndef WIN32
    EbSemLock aLock (&_sem);
    int trywait_ret = aLock.TryWait ();
    if (trywait_ret == EAGAIN)
    {
        printf ("sem_trywait () returns EAGAIN\n\n");
        if (errorCode) *errorCode = -10;
        return EB_FAILURE;
    }
    else if (trywait_ret != 0)
    {
        printf ("sem_trywait () returns a non-zero value\n\n");
        if (errorCode) *errorCode = -11;
        return EB_FAILURE;
    }
#endif

    if (!_file)
    {
        if (errorCode) *errorCode = -12;
        return EB_FAILURE;
    }

    TableEntry * pEntry = _hashtable->key2entry(key);
    if (pEntry == NULL)
    {
        if (errorCode) *errorCode = -13;
        return EB_FAILURE;
    }
    else
    {
        EbRecord *pRecord = GetRecordByHandle(pEntry->handle);
        pRecord->PutFieldAlloc("value", value);
        CommitRecord(pRecord);
        pEntry->handle = pRecord->_handle;
        return EB_SUCCESS;
    }
}

EbResult
Ebase::GetValue (void *key, void **value, int *errorCode)
{
#ifndef WIN32
    EbSemLock aLock (&_sem);
    int trywait_ret = aLock.TryWait ();
    if (trywait_ret == EAGAIN)
    {
        printf ("sem_trywait () returns EAGAIN\n\n");
        if (errorCode) *errorCode = -10;
        return EB_FAILURE;
    }
    else if (trywait_ret != 0)
    {
        printf ("sem_trywait () returns a non-zero value\n\n");
        if (errorCode) *errorCode = -11;
        return EB_FAILURE;
    }
#endif

    if (!_file)
    {
        if (errorCode) *errorCode = -12;
        return EB_FAILURE;
    }

    EbDataHandle handle = _hashtable->key2handle(key);
    if (handle == EB_INVALID_HANDLE)
    {
        if (errorCode) *errorCode = -13;
        return EB_FAILURE;
    }
    else
    {
        EbRecord *pRecord = GetRecordByHandle(handle);
        *value = pRecord->GetField("value");
        return EB_SUCCESS;
    }
}

#endif // USE_HASHTABLE

//EbResult
//Ebase::CreateStringKey (char *stringkey)
//{
//    EbRawDataSize size = strlen(stringkey) + 4 + 1;
//
//    char *rawkey = new char [size];
//    if (rawkey == NULL) return EB_FAILURE;
//
//    *(EbRawDataSize*)rawkey = size;
//    strcpy (rawkey + 4, stringkey);
//
//    EbResult res = CreateKey(rawkey);
//
//    delete [] rawkey;
//
//    return res;
//}
//
//
//EbResult
//Ebase::GetValueByStringKey (char *stringkey, void **value)
//{
//}
