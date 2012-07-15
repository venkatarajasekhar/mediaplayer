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
#include "common/include/carray.h"
#include "datafile/include/datafile.h"
#include "ebarrays.h"

/*======================================================================
 * A base class for all array classes.
 *======================================================================
 */
EbArray::EbArray(EbUint32 eleSize)
{
    EB_ASSERT(eleSize > 0);
    _eleSize = eleSize;
    _buf = NULL;
    _chunkSize = 1;
    _size = 0;
    _capacity = 0;
}

/* virtual */
EbArray::~EbArray()
{
    //..//..
    //  if (_buf != NULL) delete _buf;
    //
    if (_buf != NULL) free (_buf);

    
}

EbResult
EbArray::Init(EbUint32 chunkSize, EbUint32 initCapacity)
{
    // no space should have been allocated yet
    EB_ASSERT(_buf == NULL);
    
    //..//..
    // _buf = new EbUint8[_eleSize * initCapacity];
    //
    // Note: for uclib, _buf may be 0 if initCapacity is 0
    if (initCapacity > 0)
    {
        _buf = (EbUint8*) malloc ( sizeof (EbUint8) * _eleSize * initCapacity );
    
        if (_buf == NULL) return EB_FAILURE;
    }
    EB_ASSERT(chunkSize > 0);
    _chunkSize = chunkSize;
    _capacity = initCapacity;

    return EB_SUCCESS;
}

EbArray * 
EbArray::Clone(EbUint32 chunkSize, EbUint32 capacityDelta)
{
    EbArray * a = new EbArray(_eleSize);
    EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");

    if (a->Init(_chunkSize, _capacity + capacityDelta) ==
	EB_FAILURE) {
	    delete a;
	    EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
    }

    // do a memory copy
    memcpy(a->_buf, _buf, _eleSize*_size);
    a->_size = _size;
    return a;
}

EbResult
EbArray::Insert(void *ele, EbUint32 pos)
{
    EB_ASSERT(pos <= _size);

    // let us see if we need to expand the array
    if (_capacity == _size) {
	if (ArrayResize(_buf,
			_eleSize,
			_capacity,
			_capacity + _chunkSize) != EB_SUCCESS) {
	    return EB_FAILURE;
	}
	_capacity += _chunkSize;
    }

    // insertion
    ArrayInsert(_buf,
		_eleSize,
		_size,
		(EbUint8*)ele,
		pos);
    _size ++;

    // return
    return EB_SUCCESS;
}

EbResult
EbArray::Append(void *ele)
{
    return Insert(ele, _size);
}

void
EbArray::Delete(EbUint32 pos)
{
    // deletion
    ArrayDelete(_buf, _eleSize, _size, pos);

    // check if we need to shrink the array
    // the policy is that if there are two chunk free, we return one
    _size--;
    if (_capacity - _size >= 2 * _chunkSize) {
	EB_VERIFY(ArrayResize(_buf,
			   _eleSize,
			   _capacity,
			   _capacity - _chunkSize), == EB_SUCCESS);
	_capacity -= _chunkSize;
    }
}

void
EbArray::DeleteAll()
{
    // we shrink the buffer capacity back to the initial capacity
    EbInt32 initCapacity = _capacity % _chunkSize;
    EB_VERIFY(ArrayResize(_buf,
                       _eleSize,
                       _capacity,
                       initCapacity), == EB_SUCCESS);
    _capacity = initCapacity;

    _size = 0;
}

void
EbArray::Move(EbUint32 oldPos, EbUint32 newPos)
{
    ArrayMove(_buf, _eleSize, _size, oldPos, newPos);
}

void *
EbArray::Get(EbUint32 pos)
{
    EB_ASSERT(pos < _size);
    return _buf + _eleSize * pos;
}

EbUint32
EbArray::Size()
{
    return _size;
}

EbResult
EbArray::ReserveSpace(DataFile &f, EbDataHandle handle)
{
    return f.ResizeData(handle, SerializedSize());
}

void
EbArray::Save(DataFile &f, EbDataHandle handle)
{
    EB_CHECK(f.DataSize(handle) >= (EbInt32)SerializedSize());
    EB_CHECK(f.WriteData(handle, &_size, 0, sizeof(_size)) == sizeof(_size));
    EB_CHECK(f.WriteData(handle, &_capacity, sizeof(_size), sizeof(_capacity)) ==
	  sizeof(_capacity));
    EB_CHECK(f.WriteData(handle, &_chunkSize, sizeof(_size)+sizeof(_capacity),
		      sizeof(_chunkSize)) == sizeof(_chunkSize));
    EB_CHECK(f.WriteData(handle, &_eleSize, sizeof(_size)+sizeof(_capacity)+sizeof(_chunkSize), sizeof(_eleSize)) == sizeof(_eleSize));
    EB_CHECK((EbUint32)f.WriteData(handle, _buf, sizeof(_size)+sizeof(_capacity)+sizeof(_chunkSize)+sizeof(_eleSize), _capacity * _eleSize) == _capacity*_eleSize);
}

EbResult
EbArray::Restore(DataFile &f, EbDataHandle h)
{
    // check against the minimum size
    EB_ASSERT(f.DataSize(h) >= 0);
    if (f.DataSize(h) < sizeof(_size)+sizeof(_eleSize)+sizeof(_capacity)+sizeof(_chunkSize)) {
	return EB_FAILURE;
    }

    // read variables
    EB_CHECK(f.ReadData(h, &_size, 0, sizeof(_size)) == sizeof(_size));
    EB_CHECK(f.ReadData(h, &_capacity, sizeof(_size), sizeof(_capacity)) == sizeof(_capacity));
    EB_CHECK(f.ReadData(h, &_chunkSize, sizeof(_size)+sizeof(_capacity), sizeof(_chunkSize)) == sizeof(_chunkSize));
    EB_CHECK(f.ReadData(h, &_eleSize, sizeof(_size)+sizeof(_capacity)+sizeof(_chunkSize), sizeof(_eleSize)) == sizeof(_eleSize));
   
    // check size again
    if (f.DataSize(h) < (EbInt32)SerializedSize()) {
	return EB_FAILURE;
    }

    // free current buffer
    //..//..
    // if (_buf != NULL) delete _buf;
    //
    if (_buf != NULL) free (_buf);

    _buf = 0;
    // allocate the buffer and read in the array
    // if ((_buf = new EbUint8[_eleSize*_capacity]) == NULL) {
	//     return EB_FAILURE;
    // }
    //
    // Note: for uclib, _buf may be 0 if initCapacity is 0
    if (_capacity > 0)
    {
        if ((_buf = (EbUint8*) malloc (sizeof (EbUint8)*_eleSize*_capacity)) == NULL)
        {
            return EB_FAILURE;
        }
    }

    EB_CHECK((EbUint32)f.ReadData(h, _buf, sizeof(_size)+sizeof(_capacity)+sizeof(_chunkSize)+sizeof(_eleSize), _capacity * _eleSize) == _capacity*_eleSize);

    return EB_SUCCESS;
}

EbUint32 
EbArray::SerializedSize()
{
    return sizeof(_size)+sizeof(_capacity)+sizeof(_chunkSize)+sizeof(_eleSize) + _capacity * _eleSize;
}

/*======================================================================
 * EbHandleArray
 *======================================================================
 */
EbInt32 
EbHandleArray::Find(EbDataHandle handle)
{
    for (EbUint32 i=0; i< Size(); i++) {
        if (Get(i) == handle) return (EbInt32)i;
    }
    return -1;
}

void
EbHandleArray::Dump()
{
    for (EbUint32 i=0; i< Size(); i++) {
        printf("%d, ", Get(i));
    }
}

/*======================================================================
 * EbInt32IndexEntry is an array of ponters to handle arrays.
 *======================================================================
 */

EbInt32IndexEntry::EbInt32IndexEntry()
: _pHandleArray ( 0 )
{
    
}

EbResult
EbInt32IndexEntry::Set(EbHandleArray *ha)
{
    EB_ASSERT (_pHandleArray == 0);
    _pHandleArray = ha;
    
    return EB_SUCCESS;
}

void
EbInt32IndexEntry::Delete(EbUint32 pos, DataFile &f)
{
    delete _pHandleArray; 
    _pHandleArray = 0;
}

void
EbInt32IndexEntry::DeleteAll()
{
    delete _pHandleArray;
    _pHandleArray = 0;
}

EbResult
EbInt32IndexEntry::ReserveSpace(DataFile &f, EbDataHandle h)
{
    if (IsValid () == EB_FALSE)
    {
        return EB_SUCCESS;
    }
    else
    {
        return _pHandleArray->ReserveSpace(f, h);
    }
}

void 
EbInt32IndexEntry::Save(DataFile &f, EbDataHandle h)
{
    if (IsValid () == EB_FALSE)
    {
        return ;
    }
    else
    {
        _pHandleArray->Save(f, h);
    }
}

EbResult
EbInt32IndexEntry::Restore(DataFile &f, EbDataHandle h)
{
    EB_ASSERT(IsValid() == EB_FALSE);

    EbHandleArray *ha = new EbHandleArray();
    EB_CHECK(ha->Init(8, 0) == EB_SUCCESS);
    EB_CHECK(ha->Restore(f, h) == EB_SUCCESS);
    _pHandleArray = ha;
    return EB_SUCCESS;
}

//
// For debugging purpose
//
void
EbInt32IndexEntry::Dump()
{
    printf("--------------------------------------------------\n");
    printf("Dump record index entries (2D) \n");
    if (IsValid()) 
    {
        printf("index 0 (the integer identifier): \n");
        GetHandleArray()->Dump();
        printf("\n");
    }
}
    

