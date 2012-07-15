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
#include "datadriver.h"
#include "indexdriver.h"
#include "record.h"

#include "ebarrays.h"
/***********************************************************************
 * This file implement EbRecord class and Ebase record related functions.
 ***********************************************************************
 */


//
// constants
//
const EbUint32 RECORD_ID_INDEX = 0;
const EbInt32 NEW_RECORD_ID = -1;

/*======================================================================
 *                  Record related functions in Ebase class
 *======================================================================
 */


/***********************************************************************
 * EbRecord * Ebase::NewRecord()
 ***********************************************************************
 */
EbRecord *
Ebase::NewRecord()
{
    EbUint32 i;
    EbRecord *newRecord=NULL;

    newRecord = new EbRecord(this);
    if (newRecord == NULL)
    {
        EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
    }
    if (newRecord->Init() == EB_FAILURE) {
        delete newRecord;
        EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
    }

    // initialize the new record with the default data
    // NOTE : we mark decoded flags so that field retrival can be
    // successful, even though the encoded record body is empty.
    for (i=0; i< _dbDef->NumberOfFields(); i++) {
        _lpDataDriver[i]->SetDefault(newRecord->_lField[i]._decodedData);

        // we set the changed flag to true, so that any query
        // made to the record won't go to the original record.
        // Plus, this is also required by CommitRecord()
        newRecord->_lField[i]._changed = EB_TRUE;
    }

    // init other data
    newRecord->_handle = EB_INVALID_HANDLE;
    newRecord->_pRecordData = NULL;

    // special treatment of record id field
    EB_ASSERT(*(EbInt32*)(newRecord->_lField[RECORD_ID_INDEX]._decodedData) == 0);
    *((EbInt32*)newRecord->_lField[RECORD_ID_INDEX]._decodedData) =
        NEW_RECORD_ID;

    // return
    return newRecord;
}

/***********************************************************************
 * void Ebase::CommitRecord()
 ***********************************************************************
 */

void
Ebase::CommitRecord(EbRecord *pRecord)
{
    EB_ASSERT(pRecord->_lField != NULL);

    // check if there is any modification to the record
    // NOTE for a new record, every field is marked modified
    EbBoolean modified = EB_FALSE;
    for (EbUint32 i=0; i< _dbDef->NumberOfFields(); i++) {
        if (pRecord->_lField[i]._changed == EB_TRUE) {
            modified = EB_TRUE;
        }
    }

    // return if nothing is modified
    if (modified == EB_FALSE) return;

    // check if it is a new record
    if (pRecord->_lField[RECORD_ID_INDEX]._changed == EB_TRUE &&
        *(EbInt32*)pRecord->_lField[RECORD_ID_INDEX]._decodedData ==
        NEW_RECORD_ID) {
        AddNewRecord(pRecord);
    } else {
        ModifyRecord(pRecord);
    }
}

void
Ebase::AddNewRecord(EbRecord *pRecord)
{
    EB_ASSERT(_Int32IndexEntry->IsValid() == EB_TRUE);

    EbUint32 i;

    EbUint32 numFinishedFields, numFinishedIndices;

    EbErrorEnum error;

    // contain the encoded record data for the new record
    EbUint8 * pNewRecordData;

    // handle used to store the new record
    EbDataHandle newRecordHandle;

    EbUint32 numFields = pRecord->_pEbase->_dbDef->NumberOfFields();

    EbUint32 newRecordSize;

    // check if pRecord is created by this ebase
    if (pRecord->_pEbase != this)
    {
        EB_ASSERT (0 == "EBE_INVALID_RECORD");
    }

    // check if it is a new record
    EbInt32 *pRecordID = (EbInt32*)pRecord->_lField[RECORD_ID_INDEX]._decodedData;
    EB_ASSERT(*pRecordID == NEW_RECORD_ID);

    // set the record id
    *pRecordID = _hdr._nextRecordID;

    // calculate the new record size
    newRecordSize = PADDED_SIZE(numFields*sizeof(EncodedFieldSize), 4);
    for (i=0; i< numFields; i++) {
        EbUint32 size;
        DataDriver * pDriver = pRecord->_pEbase->_lpDataDriver[i];
        size = pDriver->EncodedDataSize(pRecord->_lField[i]._decodedData);
        EB_ASSERT(size <= EB_MAX_ENCODED_FIELD_SIZE);
        newRecordSize += PADDED_SIZE(size, 4);
    }

    // (res alloc #1) allocate memory to hold the new record
    pNewRecordData = new EbUint8[newRecordSize];
    if (pNewRecordData == NULL) {
        error = EBE_OUT_OF_MEMORY;
        goto bailout;
    }

    // (res alloc #2) allocate file space for the record
    // For existing record, we will re-use the same space if the new
    // record size is no greater than the old record size.
    newRecordHandle = _file->NewData(newRecordSize);
    if (newRecordHandle == EB_INVALID_HANDLE) {
        error = EBE_OUT_OF_FILE_SPACE;
        goto bailout_1;
    }

    //
    // (res alloc #3) encode data.  Some file space may be allocated in this
    //	step.
    //
    {
        // get the start of the field data area in the record data
        EbUint8 *pNewField =
            pNewRecordData +
            PADDED_SIZE(numFields*sizeof(EncodedFieldSize), 4);
//        EncodedFieldSize *& newEncodedFieldSize = pNewRecordData;
        EncodedFieldSize * pNewEncodedFieldSize = (EncodedFieldSize*)pNewRecordData;
        for (i=0; i< numFields; i++) {
            DataDriver * pDriver = pRecord->_pEbase->_lpDataDriver[i];
            EbUint32 size;
            pDriver->Encode(pRecord->_lField[i]._decodedData,
                            pNewField,
                            size,
                            EB_FALSE,
                            _file);
            EB_ASSERT(size <= EB_MAX_ENCODED_FIELD_SIZE);
//            newEncodedFieldSize[i] = (EncodedFieldSize)size;
            pNewEncodedFieldSize[i] = (EncodedFieldSize)size;

            // advance pField
            pNewField += PADDED_SIZE(size, 4);

        }
    }
    // for later error recovery
    numFinishedFields = numFields;

    //
    // (res alloc #4)
    // We sort the records.  For the new record case, we need to expand
    // index entry arrays.
    //

    // (res alloc #4_1) sort record id index - an optimization
    // A new record is always the last in the record ID index.
    numFinishedIndices = 0;
    if (_Int32IndexEntry->GetHandleArray()->Append(newRecordHandle) == EB_FAILURE) {
        error = EBE_OUT_OF_MEMORY;
        goto bailout_3;
    }

//    // (res alloc #4_2) for other indices

    numFinishedIndices = 1; //_Int32IndexEntry->IsValid() == EB_TRUE ? 1 : 0;

    //
    // (res alloc #5) reserve file space for the expanded index entry arrays
    //
    if (_Int32IndexEntry->ReserveSpace(*_file, _hdr._indexEntryHandle) == EB_FAILURE) {
        error = EBE_OUT_OF_FILE_SPACE;
        goto bailout_4;
    }

    // commit the record
    _file->WriteData(newRecordHandle, pNewRecordData);


    // adjust record state
    pRecord->_handle = newRecordHandle;
    pRecord->_pRecordData = pNewRecordData;
    for (i=0; i< numFields; i++) {
        EB_ASSERT(pRecord->_lField[i]._changed == EB_TRUE);
        pRecord->_lField[i]._changed = EB_FALSE;
        pRecord->_lField[i]._decoded = EB_TRUE;
    }

    // adjust ebase state
    _hdr._nextRecordID++;
    _hdr._numRecords++;

    // save & return
    // NOTE : we don't have to save index entries here since they can
    // be recovered.  But right now, we don't provide recovery and
    // we want to be a little convservative.
    SaveHeader();
    SaveIndexEntries();
    return;

bailout_4:
    //for (i=0; i< numFinishedIndices; i++) {
    if (numFinishedIndices == 1)
    {
        EbInt32 pos = _Int32IndexEntry->GetHandleArray()->Find(pRecord->_handle);
        EB_ASSERT(pos >= 0);
        _Int32IndexEntry->GetHandleArray()->Delete((EbUint32)pos);
    }

    // we just shrunk the array.  space reservation should be OK
    EB_VERIFY(_Int32IndexEntry->ReserveSpace(*_file, _hdr._indexEntryHandle),
           == EB_SUCCESS);

bailout_3:
    // free resources allocated during data encoding
    {
        EbUint8 *pNewField =
            pNewRecordData +
            PADDED_SIZE(numFields*sizeof(EncodedFieldSize), 4);
//        EncodedFieldSize *& newEncodedFieldSize = pNewRecordData;
        EncodedFieldSize * pNewEncodedFieldSize = (EncodedFieldSize*)pNewRecordData;
        for (EbUint32 j=0; j< numFinishedFields; j++) {
            if (pRecord->_lField[i]._changed == EB_TRUE) {
                DataDriver * pDriver = pRecord->_pEbase->_lpDataDriver[i];
                pDriver->FreeEncodedData(pNewField,
//                                         newEncodedFieldSize[i],
                                         pNewEncodedFieldSize[i],
                                         _file);
            }
//            pNewField += PADDED_SIZE(newEncodedFieldSize[i], 4);
            pNewField += PADDED_SIZE(pNewEncodedFieldSize[i], 4);
        }
    }

    /*bailout_2:*/
    _file->DeleteData(newRecordHandle);

bailout_1:
    delete[] pNewRecordData;

bailout:
    {
        EbError e(error);
        printf ("error: %s\n\n", e.GetStringName ());
        EB_ASSERT (0 == 1);
    }
}
//
// Modify an existing record :
//	1. calculate the new record size
//	2. allocate memory to hold the record
//	3. if the new size is bigger, we need to allocate a new data handle
//	4. encode all modified field data
//	5. re-sort indcies (no allocation is needed)
//	6. adjust record states, free old resources
//
void
Ebase::ModifyRecord(EbRecord *pRecord)
{
    // check the record is valid and is an existing record
    EB_ASSERT(pRecord != NULL);
    EB_ASSERT(pRecord->_lField[RECORD_ID_INDEX]._changed == EB_FALSE);
    EB_ASSERT(pRecord->_lField[RECORD_ID_INDEX]._decoded == EB_FALSE ||
           *(EbInt32*)pRecord->_lField[RECORD_ID_INDEX]._decodedData !=
           NEW_RECORD_ID);
    EB_ASSERT(pRecord->_handle != EB_INVALID_HANDLE);

    // check if pRecord is created by this ebase
    if (pRecord->_pEbase != this)
    {
        EB_ASSERT (0 == "EBE_INVALID_RECORD");
    }

    // local variables
    EbErrorEnum error;
    EbUint32 newRecordSize, oldRecordSize;
    EbUint8 * pNewRecordData;
    EbDataHandle newRecordHandle;
    EbUint32 numFinishedFields;
    EbUint32 i;
    EbUint32 numFields = _dbDef->NumberOfFields();
//    EncodedFieldSize *& oldFieldSize = pRecord->_pRecordData;
    EncodedFieldSize ** ppOldFieldSize = (EncodedFieldSize **)&(pRecord->_pRecordData);

    // get the old record size
    oldRecordSize = _file->DataSize(pRecord->_handle);

    // calculate the new record size
    newRecordSize = PADDED_SIZE(numFields*sizeof(EncodedFieldSize), 4);
    for (i=0; i< numFields; i++) {
        DataDriver * pDriver = pRecord->_pEbase->_lpDataDriver[i];
        if (pRecord->_lField[i]._changed == EB_TRUE) {
            // this is new data
            EbUint32 size =
                pDriver->EncodedDataSize(pRecord->_lField[i]._decodedData);
            EB_ASSERT(size <= EB_MAX_ENCODED_FIELD_SIZE);
            newRecordSize += PADDED_SIZE(size, 4);
        } else {
            // this the old data
//            newRecordSize += PADDED_SIZE(oldFieldSize[i], 4);
            newRecordSize += PADDED_SIZE((*ppOldFieldSize)[i], 4);
        }
    }

    // (res alloc #1) allocate memory to hold the new record
    pNewRecordData = new EbUint8[newRecordSize];
    if (pNewRecordData == NULL) {
        error = EBE_OUT_OF_MEMORY;
        goto bailout;
    }

    // (res alloc #2) allocate file space for the record
    // For existing record, we will re-use the same space if the new
    // record size is no greater than the old record size.
    if (newRecordSize > oldRecordSize) {
        newRecordHandle = _file->NewData(newRecordSize);
        if (newRecordHandle == EB_INVALID_HANDLE) {
            error = EBE_OUT_OF_FILE_SPACE;
            goto bailout_1;
        }
    } else {
        newRecordHandle = pRecord->_handle;
    }

    // (res alloc #3) Build the new record and encode any modified field
    // File space may be allocated when encoding field data
    {
        EbUint8 *pOldField =
            pRecord->_pRecordData +
            PADDED_SIZE(numFields*sizeof(EncodedFieldSize), 4);
        EbUint8 *pNewField =
            pNewRecordData +
            PADDED_SIZE(numFields*sizeof(EncodedFieldSize), 4);
//        EncodedFieldSize *& newFieldSize = pNewRecordData;
        EncodedFieldSize *pNewFieldSize = (EncodedFieldSize*)pNewRecordData;
        for (i=0; i< numFields; i++) {
            numFinishedFields = i;
            if (pRecord->_lField[i]._changed == EB_TRUE) {
                // this is new data
                DataDriver * pDriver = pRecord->_pEbase->_lpDataDriver[i];
                EbUint32 size;
                pDriver->Encode(pRecord->_lField[i]._decodedData,
                                pNewField,
                                size,
                                EB_FALSE,
                                _file);
                EB_ASSERT(size <= EB_MAX_ENCODED_FIELD_SIZE);
                pNewFieldSize[i] = (EncodedFieldSize)size;

                // advance pField
                pNewField += PADDED_SIZE(size, 4);
//                pOldField += PADDED_SIZE(oldFieldSize[i], 4);
                pOldField += PADDED_SIZE((*ppOldFieldSize)[i], 4);

            } else {
                // this is the old data.  We just copy it.
//                memcpy(pNewField, pOldField, oldFieldSize[i]);
//                newFieldSize[i] = oldFieldSize[i];
                memcpy(pNewField, pOldField, (*ppOldFieldSize)[i]);
                pNewFieldSize[i] = (*ppOldFieldSize)[i];

                // advance pField
//                pNewField += PADDED_SIZE(oldFieldSize[i], 4);
//                pOldField += PADDED_SIZE(oldFieldSize[i], 4);
                pNewField += PADDED_SIZE((*ppOldFieldSize)[i], 4);
                pOldField += PADDED_SIZE((*ppOldFieldSize)[i], 4);
            }
        }
    }

    numFinishedFields = numFields;

    // we still need to replace the old handle with
    // possibly a new handle
    {
        EbUint32 oldPos_0 = _Int32IndexEntry->GetHandleArray()->Find(pRecord->_handle);

        _Int32IndexEntry->Get(oldPos_0) = newRecordHandle;
    }

    // sort indices
    //  1. find the current position of the record
    //  2. find the new position of the record with modified fields
    //  3. move from old position to the new position.
    //
    // this step does not have any bailout handling.  so no step after
    // this can fail (for now).


    // commit writing the record.  This may make the original
    // record non-retrievable.  So nothing after this step
    // should cause failure.
    _file->WriteData(newRecordHandle, pNewRecordData);

    // free old resources
    {
        EbUint8 *pField =
            pRecord->_pRecordData +
            PADDED_SIZE(numFields*sizeof(EncodedFieldSize), 4);
        for (i=0; i< numFields; i++) {
            if (pRecord->_lField[i]._changed == EB_TRUE) {
                DataDriver * pDriver = pRecord->_pEbase->_lpDataDriver[i];
                pDriver->FreeEncodedData(pField,
//                                         oldFieldSize[i],
                                         (*ppOldFieldSize)[i],
                                         _file);
            }
//            pField += PADDED_SIZE(oldFieldSize[i], 4);
            pField += PADDED_SIZE((*ppOldFieldSize)[i], 4);
        }
    }
    if (pRecord->_handle != newRecordHandle) {
        _file->DeleteData(pRecord->_handle);
    }
    delete[] pRecord->_pRecordData;

    // adjust record state
    pRecord->_handle = newRecordHandle;
    pRecord->_pRecordData = pNewRecordData;
    for (i=0; i< numFields; i++) {
        if(pRecord->_lField[i]._changed == EB_TRUE) {
            pRecord->_lField[i]._changed = EB_FALSE;
            pRecord->_lField[i]._decoded = EB_TRUE;
        }
    }

    // save index entries and return
    SaveHeader();
    SaveIndexEntries();
    return;

//bailout_3:
    // free resources allocated during data encoding
    {
        EbUint8 *pNewField =
            pNewRecordData +
            PADDED_SIZE(numFields*sizeof(EncodedFieldSize), 4);
//        EncodedFieldSize *& newEncodedFieldSize = pNewRecordData;
        EncodedFieldSize *pNewEncodedFieldSize = (EncodedFieldSize*)pNewRecordData;
        for (EbUint32 j=0; j< numFinishedFields; j++) {
            if (pRecord->_lField[i]._changed == EB_TRUE) {
                DataDriver * pDriver = pRecord->_pEbase->_lpDataDriver[i];
                pDriver->FreeEncodedData(pNewField,
//                                         newEncodedFieldSize[i],
                                         pNewEncodedFieldSize[i],
                                         _file);
            }
//            pNewField += PADDED_SIZE(newEncodedFieldSize[i], 4);
            pNewField += PADDED_SIZE(pNewEncodedFieldSize[i], 4);
        }
    }

    /*bailout_2: */
    if (newRecordSize > oldRecordSize) {
        EB_ASSERT(newRecordHandle != pRecord->_handle);
        _file->DeleteData(newRecordHandle);
    }

bailout_1:
    delete[] pNewRecordData;

bailout:
    EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
}

/***********************************************************************
 * void Ebase::DeleteRecord()
 ***********************************************************************
 */
void
Ebase::DeleteRecord(EbRecord *pRecord)
{
    EbUint32 i;

    // check record validity
    if (pRecord == NULL || pRecord->_pEbase != this) {
		EB_ASSERT (0 == "EBE_INVALID_RECORD");
    }

    // check if it is a new record status
    if (pRecord->_pRecordData == NULL) {
		EB_ASSERT(pRecord->_handle == EB_INVALID_HANDLE);
		EB_ASSERT(*((EbInt32*)pRecord->_lField[RECORD_ID_INDEX]._decodedData) ==
			   NEW_RECORD_ID);

		// free memory space occupied by the record
		delete pRecord;
		return;
    }

    // this is an existing record

    EB_ASSERT(pRecord->_handle != EB_INVALID_HANDLE);
    EB_ASSERT(pRecord->_lField[RECORD_ID_INDEX]._decodedData == NULL ||
		   *((EbInt32*)pRecord->_lField[RECORD_ID_INDEX]._decodedData) !=
		   NEW_RECORD_ID);

    // we need to free the file space it occupies

    // free file space occupied by field data
    EbUint8* pField = pRecord->_pRecordData +
		PADDED_SIZE(_dbDef->NumberOfFields()*sizeof(EncodedFieldSize), 4);
//    EncodedFieldSize *& fieldSize = pRecord->_pRecordData;
    EncodedFieldSize * pFieldSize = (EncodedFieldSize*)pRecord->_pRecordData;
    for(i=0; i< _dbDef->NumberOfFields(); i++) {
//		_lpDataDriver[i]->FreeEncodedData(pField, fieldSize[i],_file);
//		pField += PADDED_SIZE(fieldSize[i], 4);
        _lpDataDriver[i]->FreeEncodedData(pField, pFieldSize[i],_file);
        pField += PADDED_SIZE(pFieldSize[i], 4);
    }

    // remove from the index entry array
//    for(i=0; i< _indexDefWrapper->Size(); i++) {
    _Int32IndexEntry->GetHandleArray()->Delete(
        _Int32IndexEntry->GetHandleArray()->Find(pRecord->_handle));
//    }

    // free the file space occupied by the record
    _file->DeleteData(pRecord->_handle);

    // adjust ebase state
    _hdr._numRecords --;

    // save index entries
    SaveHeader();
    SaveIndexEntries();

    // free the memory
    delete pRecord;
}

/***********************************************************************
 * void Ebase::GetRecord()
 ***********************************************************************
 */
EbRecord *
Ebase::GetRecord(EbUint32 offset)
{
    if (offset >= NumberOfRecords())
    {
        EB_ASSERT (0 == "EBE_INVALID_RECORD");
    }

    // get record according to 0-th index.
    return GetRecordByHandle(_Int32IndexEntry->Get(offset));
}

EbRecord *
Ebase::GetRecordByHandle(EbDataHandle handle)
{
    EbRecord *p = new EbRecord(this);
    if (p == NULL)
    {
        EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
    }
    if (p->Init() == EB_FAILURE) {
	    delete p;
	    EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
    }

    // initialize the record data
    EB_ASSERT(_file->DataSize(handle) > 0);
	if( _file->DataSize(handle) <= 0 )
		return 0;

    p->_pRecordData = new EbUint8[_file->DataSize(handle)];

    if (p->_pRecordData == NULL) {
	    delete p;
	    EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
    }
    _file->ReadData(handle, p->_pRecordData);

    p->_handle = handle;

    return p;
}

/***********************************************************************
 * void Ebase::FreeRecord()
 ***********************************************************************
 */
void
Ebase::FreeRecord(EbRecord * pRecord)
{
    if (pRecord->_pEbase != this)
    {
        EB_ASSERT (0 == "EBE_INVALID_RECORD");
    }
    delete pRecord;
}

/***********************************************************************
 * void Ebase::FreeAllRecords()
 ***********************************************************************
 */
void
Ebase::FreeAllRecords()
{
    for(; _pActiveRecord != NULL;) {
	// TRICKY : we need to copy _pActiveRecord because the destructor of
	// EbRecord will modify it.
	EbRecord *p = _pActiveRecord;
	delete p;
    }
}

/*======================================================================
 *                            EbRecord class
 *======================================================================
 */

//
// constructor
//
EbRecord::EbRecord(Ebase *eb)
{
    // remember the ebase object
    _pEbase = eb;
    _pRecordData = NULL;
    _lField = NULL;
    _handle = EB_INVALID_HANDLE;

    // link the new record to the active record list
    _pNext = eb->_pActiveRecord;
    eb->_pActiveRecord = this;

}

//
// Init
//
EbResult
EbRecord::Init()
{
    // allocate field data array
    _lField = new Field[NumberOfFields()];
    if (_lField == NULL) {
	return EB_FAILURE;
    } else {
	return EB_SUCCESS;
    }
}

//
// destructor - it only frees memory resources.
//
EbRecord::~EbRecord()
{
    EbUint32 i;
    if (_lField != NULL) {
        for (i=0; i < NumberOfFields(); i++) {
            // skip those fields whose _decodedData is not meaningfull
            if (_lField[i]._changed == EB_FALSE &&
                _lField[i]._decoded == EB_FALSE) {
                EB_ASSERT(_lField[i]._decodedData == NULL);
                continue;
            }

            DataDriver *pDriver = _pEbase->_lpDataDriver[i];
            pDriver->FreeDecodedData(_lField[i]._decodedData);
        }
        delete[] _lField;
    }

    // free other memory in the record
    if (_pRecordData != NULL) {
        delete[] _pRecordData;
        _pRecordData = NULL;
    }
    _handle = EB_INVALID_HANDLE;

    // remove myself from the active record list
    for (EbRecord ** pp = &_pEbase->_pActiveRecord;
         *pp != NULL;
         pp = &(*pp)->_pNext) {
        if (*pp == this) {
            *pp = (*pp)->_pNext;
            return;
        }
    }

    // this is weired, the deleted record is not on the list.
    EB_ASSERT(EB_FALSE);
}

EbUint32
EbRecord::FieldSize(EbUint32 fid, EbUint32 option)
{
    // check fid
    if (fid >= NumberOfFields()) {
        EB_ASSERT (0 == "EBE_INVALID_FIELD");
    }

    DataDriver *pDriver = _pEbase->_lpDataDriver[fid];
    DecodeField(fid);
    return pDriver->DecodedDataSize(_lField[fid]._decodedData);
}

void *
EbRecord::GetField(EbUint32 fid)
{
    // check fid
    EB_ASSERT(fid < NumberOfFields());

    DecodeField(fid);
    return _lField[fid]._decodedData;
}

void *
EbRecord::GetFieldAlloc(EbUint32 fid)
{
    // check fid
    EB_ASSERT(fid < NumberOfFields());

    void * newData;
    DataDriver *pDriver = _pEbase->_lpDataDriver[fid];
    if (_lField[fid]._changed == EB_TRUE) {
	// this field is modified, we need to clone the decoded data
	newData = pDriver->CloneDecodedData(_lField[fid]._decodedData);
    } else {
	DecodeField(fid);
	EB_ASSERT(_lField[fid]._decoded == EB_TRUE);

	// this field is now decoded, we simple "steal" the  data.
	newData = _lField[fid]._decodedData;
	_lField[fid]._decodedData = NULL;
	_lField[fid]._decoded = EB_FALSE;
    }

    return newData;
}

void
EbRecord::PutField(EbUint32 fid, void * ptr)
{
    // check fid
    EB_ASSERT(fid < NumberOfFields());

    // one cannot modify record id field
    if (fid == RECORD_ID_INDEX)
    {
        EB_ASSERT (0 == "EBE_INVALID_FIELD");
    }

    // free the existing data
    if (_lField[fid]._changed == EB_TRUE || _lField[fid]._decoded == EB_TRUE) {
    	_pEbase->_lpDataDriver[fid]->FreeDecodedData(_lField[fid]._decodedData);
    }

    // take over the ownership of the data
    _lField[fid]._decodedData = (EbUint8*)ptr;
    _lField[fid]._changed = EB_TRUE;
}

void
EbRecord::PutFieldAlloc(EbUint32 fid, void *ptr)
{
    // check fid
    EB_ASSERT(fid < NumberOfFields());

    // one cannot modify record id field
    if (fid == RECORD_ID_INDEX)
    {
        EB_ASSERT (0 == "EBE_INVALID_FIELD");
    }

    // make a copy of the new data
    EbUint8* newData =
	_pEbase->_lpDataDriver[fid]->CloneDecodedData((EbUint8*)ptr);

    // free the existing data
    if (_lField[fid]._changed == EB_TRUE || _lField[fid]._decoded == EB_TRUE) {
	    _pEbase->_lpDataDriver[fid]->FreeDecodedData(_lField[fid]._decodedData);
    }

    // set the new data
    _lField[fid]._decodedData = newData;
    _lField[fid]._changed = EB_TRUE;
}

void
EbRecord::DecodeField(EbUint32 fid)
{
    // check fid
    EB_ASSERT(fid < NumberOfFields());

    // do nothing if the field is modified or already decoded
    if (_lField[fid]._changed == EB_TRUE || _lField[fid]._decoded == EB_TRUE) {
	return;
    }

    /* the field is not change nor decoded, we have to decode it */
    EB_ASSERT(_lField[fid]._decodedData == NULL);
    DataDriver *pDriver = _pEbase->_lpDataDriver[fid];
    pDriver->Decode(LocateEncodedField(fid),
		    GetEncodedFieldSize(fid),
		    _lField[fid]._decodedData,
		    EB_TRUE,
		    _pEbase->_file);
    _lField[fid]._decoded = EB_TRUE;
}

EbUint8 *
EbRecord::LocateEncodedField(EbUint32 fid)
{
    // check fid
    EB_ASSERT(fid < NumberOfFields());
    EB_ASSERT(_pRecordData != NULL);

    EbUint8 * field = _pRecordData +
	PADDED_SIZE(NumberOfFields()*sizeof(EncodedFieldSize), 4);
    for (EbUint32 i=0; i< fid; i++) {
	field += PADDED_SIZE(((EncodedFieldSize*)_pRecordData)[i], 4);
    }
    return field;
}

EbUint32
EbRecord::GetEncodedFieldSize(EbUint32 fid)
{
    // check fid
    EB_ASSERT(fid < NumberOfFields());
    EB_ASSERT(_pRecordData != NULL);

    return ((EncodedFieldSize*)_pRecordData)[fid];
}

/*======================================================================
 *                         EbRecord::Field struct
 *======================================================================
 */

EbRecord::Field::Field()
{
    _decodedData = NULL;
    _decoded = EB_FALSE;
    _changed = EB_FALSE;
}

