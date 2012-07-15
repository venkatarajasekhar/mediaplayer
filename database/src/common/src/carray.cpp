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


#include "../include/carray.h"

/***********************************************************************
 * Common C routines used to implement various array classes
 ***********************************************************************
 */
// 
// Insert() assumes the needed space is already allocated (ppossibly
// through ArrayResize())
//
void 
ArrayInsert(EbUint8 *array, EbUint32 eleSize, EbUint32 numEles, EbUint8* ele, EbUint32 pos)
{
    EB_ASSERT(array != NULL);
    EB_ASSERT(pos <= numEles);

    // move elements starting from pos one slot down towards the tail
    memmove(array + (pos+1)*eleSize,
            array + pos*eleSize,
            (numEles-pos) * eleSize);

    memcpy(array + pos * eleSize, ele, eleSize);
}

//
// resize
//
EbResult
ArrayResize(EbUint8 *& array, EbUint32 eleSize, EbUint32 numEles, EbUint32 newSize)
{
    EbUint8* temp = (EbUint8*)realloc(array, newSize * eleSize);
    if (temp == NULL && newSize != 0) {
        // we failed.  we must be expanding.
        EB_ASSERT(newSize > numEles);
        return EB_FAILURE;
    }

    array = temp;
    return  EB_SUCCESS;
}

//
// delete an elemeent.  It does not resize the array.
//
void
ArrayDelete(EbUint8 * array, EbUint32 eleSize, EbUint32 numEles, EbUint32 pos)
{
    EB_ASSERT(pos < numEles);

    memmove(array + pos*eleSize,
            array + (pos+1)*eleSize,
            (numEles - pos - 1)*eleSize);
}

//
// Move an element around.
//
void
ArrayMove(EbUint8 * array, EbUint32 eleSize, EbUint32 numEles, EbUint32 pos, EbUint32 newPos)
{
    static const EbInt32 BUF_SIZE = 100;
    static EbInt8 buf[BUF_SIZE];

    EB_ASSERT(pos < numEles);
    EB_ASSERT(newPos <= numEles);

    // check for trivial cases
    if (pos == newPos || pos == newPos-1) return;

    // copy the element to be moved
    EB_ASSERT(eleSize <= BUF_SIZE);
    memcpy(buf, array + pos*eleSize, eleSize);

    // shuffle elemens around
    if (newPos < pos) {
        memmove(array + (newPos+1)*eleSize,
                array + newPos*eleSize,
                (pos - newPos)*eleSize);
    } else {
        memmove(array + pos*eleSize,
                array + (pos+1)*eleSize,
                (newPos - pos -1)*eleSize);
        newPos --;
        // we need to decrement newPos since newPos is assumed
        // withe the element in the old pos
    }

    // copy the element to the new place
    memcpy(array + newPos*eleSize, buf, eleSize);
}


