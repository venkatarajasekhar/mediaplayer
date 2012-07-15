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


#include "../include/dfhandlearray.h"
#include "../../common/include/carray.h"

/***********************************************************************
 * the handle index array
 ***********************************************************************
 */
EbResult
HandleIndexArray::Insert(EbDataHandle handle, EbUint32 pos)
{
    // check capacity
    if (_capacity == _size) {
        if (ArrayResize((EbUint8*&)_lHandle, sizeof(EbDataHandle), _capacity,
                        _capacity + _chunkSize) == EB_FAILURE) {
            return EB_FAILURE;
        }
        _capacity += _chunkSize;
    }

    ArrayInsert((EbUint8*)_lHandle, sizeof(EbDataHandle), _size, 
                (EbUint8*)&handle, pos);
    _size ++;

    return EB_SUCCESS;
}

void
HandleIndexArray::Delete(EbUint32 pos)
{
    ArrayDelete((EbUint8*)_lHandle, sizeof(EbDataHandle), _size, pos);
    _size --;

    // check for capacity
    if (_capacity - _size > _chunkSize) {
        EB_VERIFY(ArrayResize((EbUint8*&)_lHandle, 
			   sizeof(EbDataHandle), 
			   _capacity,
                           _capacity - _chunkSize), 
               == EB_SUCCESS);
        _capacity -= _chunkSize;
    }
}

EbResult
HandleIndexArray::Save(FILE *f, EbUint32 offset)
{
    if (fseek(f, offset, SEEK_SET) != 0 ||
        fwrite(&_chunkSize, sizeof(_chunkSize), 1, f) != 1 ||
        fwrite(&_size, sizeof(_size), 1, f) != 1 ||
        fwrite(&_capacity, sizeof(_capacity), 1, f) != 1 ||
        fwrite(_lHandle, 
	       sizeof(EbDataHandle), 
	       _capacity, 
               f) != _capacity) {
        return EB_FAILURE;
    } else {
        return EB_SUCCESS;
    }
}

EbResult
HandleIndexArray::Restore(FILE *f, EbUint32 offset)
{
    //..//..
    // if (_lHandle != NULL) delete _lHandle;
    //
    if (_lHandle != NULL) 
        free (_lHandle);

    if (fseek(f, offset, SEEK_SET) != 0 ||
        fread(&_chunkSize, sizeof(_chunkSize), 1, f) != 1 ||
        fread(&_size, sizeof(_size), 1, f) != 1 ||
        fread(&_capacity, sizeof(_capacity), 1, f) != 1 ||
//        (_lHandle = new EbDataHandle[_capacity]) == NULL ||
        (_lHandle = (EbDataHandle *) malloc (sizeof (EbDataHandle)*_capacity)) == NULL ||
        fread(_lHandle, 
	      sizeof(EbDataHandle), 
	      _capacity, 
	      f) != _capacity) {
        //delete _lHandle;
        free (_lHandle);
        return EB_FAILURE;
    } else {
        return EB_SUCCESS;
    }
}

void
HandleIndexArray::Move(EbUint32 currPos, EbUint32 newPos)
{
    ArrayMove((EbUint8*)_lHandle, sizeof(EbDataHandle), _size, currPos, newPos);
}

EbInt32
HandleIndexArray::Find(EbDataHandle handle)
{
    for (EbUint32 i=0; i< _size; i++) {
        if (_lHandle[i] == handle) return i;
    }
    return -1;
}

