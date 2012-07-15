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

#ifndef _ebarrays_h_
#define _ebarrays_h_

#include "Utility/database/ebglobal.h"

// forward declaration
class Ebase;
class DataFile;
class EbIndexDef;

//======================================================================
// class EbArray (not to document it in html docs)
// 	A base class for all array classes. <i>Internal Class</i> 
// 
// DESCRIPTION
//  This array class allocates 
// 	a block of memory to contain an array of elements.  Elements are
// 	copied to the block through shallow copy.  Blocks are realloc'ed
// 	each time it needs different size.  Performance is optimized 
// 	by resizing the block in chunks of elements rather than each 
// 	element.
//======================================================================
class EbArray {
 public:
    // 
    // Constructor.
    //
    // Para :
    //	[in] eleSize
    //		The element size.
    //
    EbArray(EbUint32 eleSize);

    // 
    // Destrcutor.
    //
    virtual ~EbArray();

    // 
    // Initialize the array.
    //
    // Para :
    //	[in] chunkSize	
    //      The chunk size
    //  [in] initCapacity
    //      Initial capacity
    //
    EbResult Init(EbUint32 chunkSize=8, EbUint32 initCapacity=0);

    // 
    // Make a clone with possibly different chunk size and
    // a capacity delta from the current one.
    //
    // Return :
    //      Pointer to the cloned array.  Returns NULL if not 
    //      enough memory.
    //
    EbArray * Clone(EbUint32 chunkSize=8, EbUint32 capacityDelta=0);

    // 
    // Insert an element.  A memory block with the same size as the element
    // size is allocated inside the array at the specified position.  
    // A shallow memory copy copies the element content, as pointed by
    // the argument, to the newly allocated memory block inside the array.  
    //
    // Para :
    //  [in] ele
    //      Pointer to the element to be inserted.
    //  [in] pos
    //      The position at which the element is inserted.
    //
    EbResult Insert(void * ele, EbUint32 pos);

    // 
    // Append an element.
    //
    EbResult Append(void * ele);

    // 
    // Delete an element.
    //
    // Para :
    //  [in] pos
    //      The position where the element is deleted.
    //
    void Delete(EbUint32 pos);

    // 
    // Delete all elements.
    //
    void DeleteAll();

    // 
    // Move an eleement from on epositin to another. This function assumes 
    // that newPos was computed with the moving element in the current
    // position.  Therefore, if the new pos is greater than the old pos, 
    // the new pos value is really one greater than the final position of 
    // the element after the movement.
    // For example, Move(3, 5) really shuffles the 4th and the 5th 
    // element.  Move(..., 3, 4) has no effect on the array.
    //
    // Para :
    //  [in] oldPos 
    //      The position of the element to be moved.
    //  [in] newPos
    //      The position where the element is moved to.  The new position
    //      is calculated assuming that the element were still at the 
    //      current position.
    //          
    void Move(EbUint32 oldPos, EbUint32 newPos);

    // 
    // Get a pointer to the specified element
    //
    // Para :
    //  [in] pos
    //      The position of the element to be retrieved.
    //
    void * Get(EbUint32 pos);

    // 
    // Get the number of elements.
    //
    EbUint32 Size();

    // 
    // Reserve a space in the data file for future serialization.
    //
    // Para :
    //  [in] f
    //      The data file.
    //  [in] handle
    //      The data handle that is allocated for storing the array.
    //      The handle must be created before and must be valid.
    //
    EbResult ReserveSpace(DataFile & f, EbDataHandle handle);

    // 
    // Save the array to the data file.  It should be saved to 
    // the handle where it has previously reserved.  No failure
    // should happen.
    //
    // Para :
    //  [in] f
    //      The data file.
    //  [in] handle
    //      The data handle that is allocated for storing the array.
    //      The handle should be resized properly through ReserveSpace()
    //      so that enough space is reserved to store the array.
    //
    void Save(DataFile & f, EbDataHandle handle);

    // 
    // Restore an array from the data file.
    //
    // Para :
    //  [in] f
    //      The data file.
    //  [in] handle
    //      The data handle that is allocated for storing the array.
    //      The behavior is unpredictable if the handle represents
    //      a non-array datum.
    //
    EbResult Restore(DataFile & f, EbDataHandle handle);

 private:
    //
    // the size when array is serialzied
    //
    EbUint32 SerializedSize();

    // instance variables
    EbUint32 _size;
    EbUint32 _capacity;
    EbUint32 _chunkSize;
    EbUint32 _eleSize;
    EbUint8 *_buf;
};
    

//======================================================================
// class EbHandleArray (not to document it in html docs)
//  EbHandleArray is an array of data handles in a data file.
//======================================================================
class EbHandleArray : private EbArray {
 public:

    // 
    // Constructor
    //
    EbHandleArray() : EbArray(sizeof(EbDataHandle)) {}

    // 
    // Set the initial capacity and the chunk size.  Handle array is
    // resized in chunks.  In addition, it is resized when it has
    // a numtiple number of chunkSize PLUS the initial capacity.
    // This way, several handle arrays may have different initial
    // capacities, and they will be resized at different times, even
    // if they expand or shrink at the same step.
    //
    EbResult Init(EbUint32 chunkSize, EbUint32 initCapacity) {
        return EbArray::Init(chunkSize, initCapacity);
    }

    // 
    //  
    EbUint32 Size() { 
        return EbArray::Size(); 
    }

    // 
    //
    EbDataHandle& Get(EbUint32 pos) { 
        return *(EbDataHandle *) EbArray::Get(pos); 
    }

    // 
    //
    EbResult Insert(EbDataHandle h, EbUint32 pos) {
        return EbArray::Insert(&h, pos);
    }

    // 
    //
    EbResult Append(EbDataHandle h) { 
        return EbArray::Append(&h); 
    }

    // 
    //
    void Delete(EbUint32 pos) { 
        EbArray::Delete(pos); 
    }

    // 
    // Delete all elements.  If Init() is ever called on this object,
    // this function will bring the object back to the same state as
    // Init() was just called.
    //
    void DeleteAll() { EbArray::DeleteAll(); }

    // 
    //
    void Move(EbUint32 oldPos, EbUint32 newPos) {
        EbArray::Move(oldPos, newPos);
    }

    // 
    // Find the position of a handle in the array
    //
    // Para :
    //  [in] handle
    //      The handle value that is searched for.
    //
    // Return :
    //      The position of the found handle. Or -1 if not found.
    //
    EbInt32 Find(EbDataHandle handle);

    // 
    //
    EbResult ReserveSpace(DataFile &f, EbDataHandle h) {
        return EbArray::ReserveSpace(f, h);
    }

    // 
    //
    void Save(DataFile &f, EbDataHandle h) {
        EbArray::Save(f, h);
    }

    // 
    //
    EbResult Restore(DataFile &f, EbDataHandle h) {
        return EbArray::Restore(f, h);
    }

    // 
    //
    EbHandleArray * Clone(EbUint32 chunkSize, EbUint32 capacityDelta) {
        return (EbHandleArray*)EbArray::Clone(chunkSize, capacityDelta);
    }

    //
    // For debugging purpose
    //
    void Dump();
};

//======================================================================
// class EbInt32IndexEntry (not to document it in html docs)
// EbInt32IndexEntry is an array of ponters to handle arrays. 
//
// DESCRIPTION
//  EbInt32IndexEntry is essentially an array of record sorting orders.
//  Each element in this array is a list of record IDs, representing
//  a sorting of records according one index.
//======================================================================
class EbInt32IndexEntry
{
private:
    EbHandleArray *_pHandleArray;

 public:

    // 
    // Constructor.
    //
    EbInt32IndexEntry();

    // 
    //
    EbBoolean IsValid ()
    {
        return (_pHandleArray) ? EB_TRUE : EB_FALSE;
    }

    // 
    //
    EbHandleArray *& GetHandleArray() {
        return _pHandleArray;
    }

    // 
    // A short cut to a handle value in a handle array.
    //
    EbDataHandle& Get(EbUint32 pos) {
        return GetHandleArray()->Get(pos);
    }

    // 
    // Set an new handle array.  This function will "own" the
    // the EbHandleArray object hereafter.
    //
    EbResult Set(EbHandleArray * ha);

    // 
    // Delete an element.  We need to pass the data file parameter
    // because this funtion will remove the reserved space for the
    // delete entry array.
    //
    // Note that the memory occupied by the handle array is not freed
    //
    void Delete(EbUint32 pos, DataFile &f);

    // 
    // Delete all elements.  Unlike Delete(), DeleteAll() will clear
    // all memory resource but not file space.  (it is mainly for
    // reusing this array, as it is required due to the resuability
    // of Ebase object.)
    //
    void DeleteAll();

    // 
    //
    EbResult ReserveSpace(DataFile &f, EbDataHandle h);

    // 
    //
    void Save(DataFile &f, EbDataHandle h);

    // 
    //
    EbResult Restore(DataFile &f, EbDataHandle h);

    //
    // For debugging
    //
    void Dump();

 private:
    // a list of data handles that store the index entry arrays
    //EbHandleArray _lEntryArrayHandle;
    //EbHandleArray _theOnlyOneIndexEntryArray;
};

#endif  // _ebarrays_h_
