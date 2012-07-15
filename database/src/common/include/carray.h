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


#ifndef _carray_h_
#define _carray_h_

#include "../../internal.h"

/***********************************************************************
 * Common C routines used to implement various array classes
 ***********************************************************************
 */

// 
// Insert() assumes the needed space is already allocated (ppossibly
// through ArrayResize())
//
void 
ArrayInsert(EbUint8 *array, EbUint32 eleSize, EbUint32 numEles, EbUint8* ele, EbUint32 pos);

//
// resize
//
EbResult
ArrayResize(EbUint8 *& array, EbUint32 eleSize, EbUint32 numEles, EbUint32 newSize);

//
// delete an elemeent.  It does not resize the array.
//
void
ArrayDelete(EbUint8 * array, EbUint32 eleSize, EbUint32 numEles, EbUint32 pos);

//
// Move an element around.
// This function assumes that newPos was computed with the element in the old
// position.  Therefore, if the new pos is greater than the old pos, the new
// pos value is 1 greater than the final position of the element after the
// movement.
// For example, ArrayMove(..., 3, 5) really shuffles the 4th and the 5th 
// element.  ArrayMove(..., 3, 4) has no effect on the array.
//
void
ArrayMove(EbUint8 * array, EbUint32 eleSize, EbUint32 numEles, EbUint32 pos, EbUint32 newPos);

#endif
