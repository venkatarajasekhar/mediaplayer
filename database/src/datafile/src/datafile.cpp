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

#include "../include/datafile.h"


/***********************************************************************
 * handle slots used internally by data file 
 ***********************************************************************
 */

const EbInt32 NUM_INIT_HANDLES = 5;

// begin and end slot are empty slots to mark beginning and ending
// of the data file.  They are always in use and appear in handle index.
const EbDataHandle BEGIN_HANDLE = 1;
const EbDataHandle END_HANDLE = 2;
const EbDataHandle HANDLE_HANDLE = 3;
const EbDataHandle HANDLE_INDEX_HANDLE = 4;

// 0 is reserved by API requirement

/***********************************************************************
 * increment used when resizing handle table
 ***********************************************************************
 */
const EbInt32 HANDLE_TABLE_CHUNK = 8;

/***********************************************************************
 * file layout
 ***********************************************************************
 */

const char MAGIC_WORD[] = "abee";
const EbUint8 MAJOR_NUMBER = 1;
const EbUint8 MINOR_NUMBER = 1;
const EbUint32 FLAG = 0;

/***********************************************************************
 * constructor and destructor
 ***********************************************************************
 */
DataFile::DataFile() 
{
    _file = NULL;
    m_isDirty=false;
}

DataFile::~DataFile()
{
    if (IsOpen() == EB_TRUE) {
        Close();
    }
}

/***********************************************************************
 * create a data file
 ***********************************************************************
 */
EbResult
DataFile::Create(const char *fname)
{
    // create the file
    FILE *f = fopen(fname, "wb");	// read, writie and binary
    if (f == NULL) return EB_FAILURE;

    // create a data file instance
    DataFile *df = new DataFile();
    EB_ASSERT(df != NULL);
    df->_hdr._numHandles = NUM_INIT_HANDLES;
    
    //..//.. 
    // valgrind-2.4.0 will report "Mismatched free() / delete / delete []"
    //
    //df->_lHandle = new Handle[NUM_INIT_HANDLES];
    df->_lHandle = (Handle*) malloc (sizeof (Handle)*NUM_INIT_HANDLES);
    EB_ASSERT(df->_lHandle != NULL);
    memset (df->_lHandle, 0, sizeof (Handle)*NUM_INIT_HANDLES);
	
    // init slot index data
    EB_VERIFY(df->_lHandleIndex.Append(BEGIN_HANDLE), == EB_SUCCESS);
    EB_VERIFY(df->_lHandleIndex.Append(0), == EB_SUCCESS);
    EB_VERIFY(df->_lHandleIndex.Append(HANDLE_HANDLE), == EB_SUCCESS);
    EB_VERIFY(df->_lHandleIndex.Append(HANDLE_INDEX_HANDLE), == EB_SUCCESS);
    EB_VERIFY(df->_lHandleIndex.Append(END_HANDLE), == EB_SUCCESS);

    // init slot data
    for(EbInt32 i=0; i< NUM_INIT_HANDLES; i++) {
	df->_lHandle[i]._flag = 1;
    }

    df->_lHandle[0]._offset = sizeof(Header);
    df->_lHandle[0]._size = 0;

    df->_lHandle[BEGIN_HANDLE]._offset = sizeof(Header);
    df->_lHandle[BEGIN_HANDLE]._size = 0;

    df->_hdr._handleOffset = 
        df->_lHandle[HANDLE_HANDLE]._offset = sizeof(Header);
    df->_lHandle[HANDLE_HANDLE]._size = NUM_INIT_HANDLES * sizeof(Handle);

    df->_lHandle[HANDLE_INDEX_HANDLE]._offset = 
	df->_lHandle[HANDLE_HANDLE]._offset +
	df->_lHandle[HANDLE_HANDLE]._size;
    df->_lHandle[HANDLE_INDEX_HANDLE]._size = 
        df->_lHandleIndex.SerializedSize();

    df->_lHandle[END_HANDLE]._offset =
	df->_lHandle[HANDLE_INDEX_HANDLE]._offset + 
	df->_lHandle[HANDLE_INDEX_HANDLE]._size;
    df->_lHandle[END_HANDLE]._size = 0;

    // init other data
    strncpy(df->_hdr._magicWord, MAGIC_WORD,EB_MAGIC_WORD_LENGTH);
    df->_hdr._flag = FLAG;
    df->_hdr._major = MAJOR_NUMBER;
    df->_hdr._minor = MINOR_NUMBER;
    df->_hdr._numHandles = NUM_INIT_HANDLES;
    df->_hdr._freeHandle = EB_INVALID_HANDLE;
    df->_file = f;

    // write the file header
    df->m_isDirty=true;
    df->Close();
    delete df;

    // return
    return EB_SUCCESS;
}

/***********************************************************************
 * open a data file
 ***********************************************************************
 */
EbResult
DataFile::Open(FILE *f)
{
    EB_ASSERT(f != NULL);

    // read header
    EB_VERIFY(fseek(f, 0, SEEK_SET), == 0);
    if (fread((void*)&_hdr, sizeof(_hdr), 1, f) != 1) {
	return EB_FAILURE;
    }

    // check header data
    if (strncmp(_hdr._magicWord, MAGIC_WORD, EB_MAGIC_WORD_LENGTH) != 0) {
	return EB_FAILURE;
    }
    if (_hdr._major != MAJOR_NUMBER || _hdr._minor != MINOR_NUMBER) {
	return EB_FAILURE;
    }

    // read in slots, this should not fail
    if(fseek(f, _hdr._handleOffset, SEEK_SET) != 0) {
	return EB_FAILURE;
    }
    
    //..//..
    // _lHandle = new Handle[_hdr._numHandles];
    //
    _lHandle = (Handle*) malloc (sizeof (Handle)*_hdr._numHandles);
    if (_lHandle == NULL) {
	return EB_FAILURE;
    }
	memset (_lHandle, 0, sizeof (Handle)*_hdr._numHandles);
    if (fread((void*)_lHandle, sizeof(Handle), _hdr._numHandles, f) != 
	_hdr._numHandles) {
	//..//..
    // delete[] _lHandle;
    //
    free (_lHandle);
	return EB_FAILURE;
    }

    // read in handle index
    if (_lHandleIndex.Restore(f, _lHandle[HANDLE_INDEX_HANDLE]._offset) !=
	EB_SUCCESS) {
	//..//..
    // delete[] _lHandle;
    //
    free (_lHandle);
	return EB_FAILURE;
    }

    // sanity check
    EB_ASSERT(_lHandleIndex.Size() <= (EbInt32)_hdr._numHandles);
    EB_ASSERT(_lHandleIndex.Size() >= NUM_INIT_HANDLES);
    EB_ASSERT(_lHandle[HANDLE_HANDLE]._offset == _hdr._handleOffset);

    // remember the file
    _file = f;

    return EB_SUCCESS;
}

EbResult
DataFile::Open(const char *fname)
{
    FILE *f = fopen(fname, "r+b");
    if (f == NULL) {
	return EB_FAILURE;
    }

    return Open(f);
}

/***********************************************************************
 * close a data file
 *   We need to save some cached data back to the file.  All required
 *   file space should have been allocated before.
 ***********************************************************************
 */
void
DataFile::Close()
{
    // check if data is consistent
    if(m_isDirty)
    {
        EB_ASSERT(_lHandle[HANDLE_HANDLE]._offset == _hdr._handleOffset);

        // check if required space is already allocated
        EB_ASSERT(_lHandle[HANDLE_HANDLE]._size >= sizeof(Handle)*_hdr._numHandles);
        EB_ASSERT(_lHandle[HANDLE_INDEX_HANDLE]._size >=
    	   _lHandleIndex.SerializedSize());

        // save handle and handle index to files
        EB_VERIFY(fseek(_file, _lHandle[HANDLE_HANDLE]._offset, SEEK_SET), == 0);
        EB_VERIFY(fwrite(_lHandle, sizeof(Handle), _hdr._numHandles, _file),
    	   == _hdr._numHandles);
        EB_VERIFY(_lHandleIndex.Save(_file, _lHandle[HANDLE_INDEX_HANDLE]._offset),
    	   == EB_SUCCESS);

        // save the header
        EB_VERIFY(fseek(_file, 0, SEEK_SET), == 0);
        EB_VERIFY(fwrite(&_hdr, sizeof(_hdr), 1, _file), == 1);
    }

    // close file
    fclose(_file);
    _file = NULL;

    // free memory
    //..//..
    // delete[] _lHandle;
    //
    free (_lHandle);
    _lHandle = NULL;

    _lHandleIndex.DeleteAll();
}

/***********************************************************************
 * return the offset of a data
 ***********************************************************************
 */
EbUint32 
DataFile::DataOffset(EbDataHandle h)
{
    EB_ASSERT(h >=0 && h< _hdr._numHandles);

    // caller cannot query internal handles
    EB_ASSERT(h == 0 || h >= NUM_INIT_HANDLES);

    if (_lHandle[h]._flag == 1) {
        return _lHandle[h]._offset;
    } else {
        return (EbUint32)-1;
    }
}

/***********************************************************************
 * return the size of a data
 ***********************************************************************
 */
EbInt32 
DataFile::DataSize(EbDataHandle h)
{
    EB_ASSERT(h >=0 && h< _hdr._numHandles);

    // caller cannot query internal handles
    EB_ASSERT(h == 0 || h >= NUM_INIT_HANDLES);

    if(_lHandle[h]._flag == 1) {
        return (EbInt32)_lHandle[h]._size;
    } else {
        return -1;
    }
}

/***********************************************************************
 * New a data.  File space is allocated immediately.  
 * This may cause _handle array and handle index to be expanded both in 
 * memory and in file.
 ***********************************************************************
 */
EbDataHandle
DataFile::NewData(EbUint32 size)
{

    // get a free handle slot
    EbInt32 freeHandle = AllocateHandle();
    if (freeHandle < 0) {
	return EB_INVALID_HANDLE;
    }
    EB_ASSERT(_lHandle[freeHandle]._flag == 0);

    // find a free space big enough in the file 
    EbUint32 pos;
    EbInt32 offset = AllocateSpace(size, pos);
    if (offset < 0) {
        return EB_INVALID_HANDLE;
    }

    // adjust the index first
    if (_lHandleIndex.Insert(freeHandle, pos) != EB_SUCCESS) {
	return EB_INVALID_HANDLE;
    }

    // set the handle
    _lHandle[freeHandle]._offset = offset;
    _lHandle[freeHandle]._flag = 1;
    _lHandle[freeHandle]._size = size;

    // we also need to expand some space to hold the handle index
    if (ResizeData(HANDLE_INDEX_HANDLE, _lHandleIndex.SerializedSize()) 
	== EB_FAILURE) {
	FreeHandle(freeHandle);
	_lHandleIndex.Delete(pos);
	return EB_INVALID_HANDLE;
    }
    m_isDirty=true;
    return freeHandle;
}

/***********************************************************************
 * Delet a data.
 ***********************************************************************
 */
void 
DataFile::DeleteData(EbDataHandle handle)
{
    // assert the handle is in use
    EB_ASSERT(_lHandle[handle]._flag == 1);

    // one cannot delete internal handles and 0 handle
    EB_ASSERT(handle >= NUM_INIT_HANDLES);

    // delete the handle from the handle index
    _lHandleIndex.Delete(_lHandleIndex.Find(handle));

    // free the handle,  this returns the handle back to the
    // free handle pool
    m_isDirty=true;
    FreeHandle(handle);
}

/***********************************************************************
 * Resize an existing data
 ***********************************************************************
 */
EbResult 
DataFile::ResizeData(EbDataHandle handle, EbUint32 size)
{
    // assert the handle is in use
    EB_ASSERT(_lHandle[handle]._flag == 1);

    // shortcut for shrinking
    if (size <= _lHandle[handle]._size) {
        _lHandle[handle]._size = size;
        m_isDirty=true;
        return EB_SUCCESS;
    }

    // collect any space before and after this handle
    EbInt32 pos = _lHandleIndex.Find(handle);

    // we are guranteed that the pos is not the first nor the last
    // since we have two special handles for the first and the last
    EB_ASSERT(pos > 0 && pos < (EbInt32)_lHandleIndex.Size() - 1);

    EbDataHandle prev = _lHandleIndex.Get(pos-1);
    EbDataHandle next = _lHandleIndex.Get(pos+1);
    EB_ASSERT(_lHandle[prev]._flag == 1);
    EB_ASSERT(_lHandle[next]._flag == 1);

    EbInt32 startOffset = _lHandle[prev]._offset + _lHandle[prev]._size;
    if (_lHandle[next]._offset - startOffset > size) {
        // we find space in neiborhood
        // the index remains the same
        _lHandle[handle]._offset = startOffset;
	_lHandle[handle]._size = size;
        m_isDirty=true;
        return EB_SUCCESS;
    }

    // we have to find the space in some place else
    EbUint32 newPos;
    startOffset = AllocateSpace(size, newPos);
    if (startOffset < 0) {
        return EB_FAILURE;
    }

    // update the index
    _lHandleIndex.Move(pos, newPos);

    // update the handle
    _lHandle[handle]._offset = startOffset;
    _lHandle[handle]._size = size;

    m_isDirty=true;
    return EB_SUCCESS;
}

/***********************************************************************
 * read a data
 ***********************************************************************
 */
EbInt32 
DataFile::ReadData(EbDataHandle handle, void *buf, EbUint32 offset, EbInt32 size)
{
    // assert the handle is in use
    EB_ASSERT(_lHandle[handle]._flag == 1);

    // a one cannot read internal handles
    EB_ASSERT(handle >= NUM_INIT_HANDLES || handle == 0);

    // check offset
    if (offset > _lHandle[handle]._size) {
        return -1;
    }

    // calculate size
    if (size < 0) {
        size = _lHandle[handle]._size - offset;
    }

    // read data
    EB_VERIFY(fseek(_file, _lHandle[handle]._offset + offset, SEEK_SET),
           == 0);
    EB_VERIFY((EbInt32)fread(buf, 1, size, _file), == size);
    
    return size;
}

/***********************************************************************
 * write a data
 ***********************************************************************
 */
EbInt32 
DataFile::WriteData(EbDataHandle handle, void *buf, EbUint32 offset, EbInt32 size)
{
//jsprintf("---------------- ebase write data %d----------------jason\n",size);
    // assert the handle is in use
    EB_ASSERT(_lHandle[handle]._flag == 1);

    // a one cannot read internal handles
    EB_ASSERT(handle >= NUM_INIT_HANDLES || handle == 0);

    // check offset
    if (offset > _lHandle[handle]._size) {
        return -1;
    }

    // calculate size
    if (size < 0) {
        size = _lHandle[handle]._size - offset;
    }

    // write data
    EB_VERIFY(fseek(_file, _lHandle[handle]._offset + offset, SEEK_SET),
           == 0);
    EB_VERIFY((EbInt32)fwrite(buf, 1, size, _file), == size);
    m_isDirty=true;
    return size;
}

/***********************************************************************
 * Allocate a space.  Does not touch any data structure except
 * _lHandle[END_HANDLE] when it needs to expand the file.
 ***********************************************************************
 */
EbInt32
DataFile::AllocateSpace(EbUint32 size, EbUint32 & pos)
{
    EbUint32 i,j;

    for (i=0; i< _lHandleIndex.Size()-1; i++) {
	EbDataHandle j = _lHandleIndex.Get(i);
        EbDataHandle k = _lHandleIndex.Get(i+1);

	EB_ASSERT(_lHandle[j]._flag == 1);
	EB_ASSERT(_lHandle[k]._flag == 1);
        EB_ASSERT(_lHandle[k]._offset >= _lHandle[j]._offset+_lHandle[j]._size);

	EbUint32 gap = 
            _lHandle[k]._offset - _lHandle[j]._offset - _lHandle[j]._size;
	if ( size <= gap) {
            // we found it
            pos = i+1;
            return _lHandle[j]._offset + _lHandle[j]._size;
	}
    }

    // we did not find space inside the file
    // we need to expand the file.
    i = _lHandleIndex.Size()-2;
    j = _lHandleIndex.Get(i);
    EbInt32 newEnd = _lHandle[j]._offset + _lHandle[j]._size + size;
    if (fseek(_file, newEnd, SEEK_SET) != 0) {
        return -1;
    }
    // we write one byte to mark the real end of file
    if (fwrite(&i, 1, 1, _file) != 1) {
        return -1;
    }
//printf("---------------- ebase AllocateSpace %d----------------jason\n",size);
    _lHandle[END_HANDLE]._offset = newEnd;

    pos = i+1;
    m_isDirty=true;
    return _lHandle[j]._offset + _lHandle[j]._size;
}

/***********************************************************************
 * Allocate a new handle.
 ***********************************************************************
 */
EbDataHandle 
DataFile::AllocateHandle()
{
    // Check if there is any free handle around
    if (_hdr._freeHandle == EB_INVALID_HANDLE) {

        // now we need to expand handle table in memory.
        // and we need to reserve a bigger space in file for the table.
        EbInt32 newSize = sizeof(Handle)*(_hdr._numHandles+HANDLE_TABLE_CHUNK);
        Handle *temp = (Handle*)realloc(_lHandle, newSize);
        if (temp == NULL) {
            return EB_INVALID_HANDLE;
        }
        _lHandle = temp;

        // re-allocate space in file
        if (ResizeData(HANDLE_HANDLE, newSize) != EB_SUCCESS) {
            // we do not shrik the handle table in memory, 'cuase it does 
            // not matter
            return EB_INVALID_HANDLE;
        }

        // we proceed to finalize the deal
        for (EbDataHandle i=(EbDataHandle)_hdr._numHandles;
	     i < (EbDataHandle)(_hdr._numHandles+ HANDLE_TABLE_CHUNK);
             i ++) {
            _lHandle[i]._nextHandle = i+1;
            _lHandle[i]._flag = 0;
        }
        _hdr._freeHandle = _hdr._numHandles;
        _hdr._numHandles += HANDLE_TABLE_CHUNK;
        _hdr._handleOffset = _lHandle[HANDLE_HANDLE]._offset;
        _lHandle[_hdr._numHandles-1]._nextHandle = EB_INVALID_HANDLE;
        m_isDirty=true;
    }

    EbDataHandle temp = _hdr._freeHandle;
    _hdr._freeHandle = _lHandle[temp]._nextHandle;
    EB_ASSERT(_lHandle[temp]._flag == 0);
    return temp;
}

/***********************************************************************
 * free a handle.  This function returns an in-use handle back to
 * the free handle pool.
 ***********************************************************************
 */
void 
DataFile::FreeHandle(EbDataHandle handle)
{
    EB_ASSERT(_lHandle[handle]._flag == 1);

    _lHandle[handle]._flag = 0;
    _lHandle[handle]._nextHandle = _hdr._freeHandle;
    _hdr._freeHandle = handle;
    m_isDirty=true;
}

#ifdef EB_DEBUG

/***********************************************************************
 * dump the state for debugging
 ***********************************************************************
 */
void
DataFile::Dump()
{
    printf("--------------------------------------------------\n");
    printf("Dump data file header \n");
    printf("\tnumHandles = %d\n", _hdr._numHandles);
    printf("\thandleOffset = %d\n", _hdr._handleOffset);
    printf("\tfreeHandle = %d\n", _hdr._freeHandle);

    printf("\n");
    printf("In-use data handles \n");
    for(EbUint32 i=0; i< _lHandleIndex.Size(); i++) {
        printf("\t%d:\t%d\t%d\t%d\t%d\t%d\n", 
               i, 
               _lHandleIndex.Get(i),
               _lHandle[_lHandleIndex.Get(i)]._flag,
               _lHandle[_lHandleIndex.Get(i)]._offset,
               _lHandle[_lHandleIndex.Get(i)]._size,
               _lHandle[_lHandleIndex.Get(i)]._offset+
               _lHandle[_lHandleIndex.Get(i)]._size);
    }

    printf("\n");
    printf("Free data handles \n");
    EbDataHandle curr = _hdr._freeHandle;
    for(;curr != EB_INVALID_HANDLE;) {
        printf("%d(%d); ", curr, _lHandle[curr]._flag);
        curr = _lHandle[curr]._nextHandle;
    }
    printf("\n");
}
#endif
