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


#ifndef _dfhandlearray_h_
#define _dfhandlearray_h_

#include "../../internal.h"

/***********************************************************************
 * HandleIndexArray stores an array of handle values in a continous block of
 * memory.  Instead of expand or shrink every time an element is
 * added or deleted, the array expand or shrinks in chunks.
 * It is used by DataFile, so it does not assumes the existence of DataFile.
 ***********************************************************************
 */
class HandleIndexArray {
public:

    //
    // constructor.
    //
    HandleIndexArray(EbUint32 chunkSize=10) {
        _chunkSize = chunkSize;
        _capacity = 0;
        _lHandle = NULL;
        _size = 0;
    }

    //
    // destructor.
    //
    ~HandleIndexArray() {
        //..//..
        // ArrayResize () uses realloc () to allocate the memory...
        // valgrind-2.4.0 will report "Mismatched free() / delete / delete []"
        //
        // delete _lHandle;
        if (_lHandle)
	{
            free (_lHandle);
            _lHandle = 0;
	}
    }

    //
    // Append a handle value at the end
    //
    EbResult Append(EbDataHandle handle) {
        return Insert(handle, Size());
    }

    //
    // Insert a handle value at the specified position.
    //
    EbResult Insert(EbDataHandle handle, EbUint32 pos);

    //
    // Delete an element by its index (position)
    //
    void Delete(EbUint32 pos);

    // 
    // Delete all elements.
    //
    void DeleteAll() {
        //..//..
        // ArrayResize () uses realloc () to allocate the memory...
        // valgrind-2.4.0 will report "Mismatched free() / delete / delete []"
        //
        // delete _lHandle;
        free (_lHandle);
        
        _lHandle = NULL;
        _capacity = 0;
        _size = 0;
    }
	
    //
    // Find a value in the array.
    // Return :
    //          The position of the found value
    //          Or < 0 if not found
    //  
    EbInt32 Find(EbDataHandle handle);

    //
    // Move an element from its current position to a new position.
    // Notice that the newPos specifies the position BEFORE the element
    // is removed from the old position.  In other words, this process
    // is equivalent to 1) add a new element with the same value at newPos;
    // and 2) delete the element which is used to be at the old position.
    //
    void Move(EbUint32 currPos, EbUint32 newPos);

    //
    // Get a value from the array by its index.
    //
    EbDataHandle Get(EbUint32 pos) {
        EB_ASSERT(pos < _size);
        return _lHandle[pos];
    }

    //
    // REturn the number of elements.
    //
    EbUint32 Size() { return _size; }

    //
    // Calculate the size when the array is serialized.
    // 
    EbUint32 SerializedSize() {
        return sizeof(_chunkSize) + sizeof(_size) + sizeof(_capacity) 
            +  _capacity * sizeof(EbDataHandle);
    }

    //
    // Serialize the array and store it to the file, starting from the 
    // specified offset.
    //
    EbResult Save(FILE *f, EbUint32 offset);

    //
    // Restore the value from a file, from the specified starting offset.
    //
    EbResult Restore(FILE *f, EbUint32 offset);

private:
    EbUint32 _chunkSize;
    EbUint32 _size;
    EbUint32 _capacity;
    EbDataHandle *_lHandle;
};


#endif
