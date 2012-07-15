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

#ifndef _internal_h_
#define _internal_h_

#include "Utility/database/ebase.h"
//#include "memdbg.h"

/***********************************************************************
 * This re-defines several global types and constants to more programmer
 * friendly name.  This should make future source code reuse easier.
 *
 * This idea here is the separation of public API and internal 
 * programming environment.  Public API is named such that it is 
 * friendly for co-existence with other libraries.  The internal
 * programming environment is named such that it makes the best sense
 * (and make name short) for the programmers.
 **********************************************************************
 */

//typedef		EbResult	Result;
//#define		SUCCESS		EB_SUCCESS
//#define		FAILURE		EB_FAILURE

//typedef		EbBoolean	Boolean;
//#define		TRUE		EB_TRUE
//#define		FALSE		EB_FALSE




//#define		VERIFY(e, c)	EB_VERIFY(e, c)
//#define		CHECK(e)	EB_CHECK(e)

//typedef		EbInt8		int8;
//typedef		EbUint8		EbUint8;
//typedef		EbInt16		int16;
//typedef		EbUint16	uint16;
//typedef		EbInt32		int32;
//typedef		EbUint32	uint32;

//typedef		EbDataHandle	DataHandle;

//#define		IndexDef	EbIndexDef
//#define		IndexDefArray	EbIndexDefArray

//#define		INVALID_HANDLE	EB_INVALID_HANDLE
//#define		MAGIC_WORD_LENGTH  EB_MAGIC_WORD_LENGTH
//#define		MAX_FIELD_NAME  EB_MAX_FIELD_NAME
//#define		MAX_ENCODED_FIELD_SIZE EB_MAX_ENCODED_FIELD_SIZE
//#define		MAX_NUM_INDICES	EB_MAX_NUM_INDICES
//#define		MAX_NUM_INDEX_FIELDS  EB_MAX_NUM_INDEX_FIELDS

//typedef		EbIndexId	IndexId;
//typedef		EbDataType	DataType;


/***********************************************************************
 * Some additional macros, typedefs
 **********************************************************************
 */

// UNUSED() is applied to unused formal arguments so that the compiler
// cannot complain about unused arguments
#define		UNUSED(x)		(void*)x

// PADDED_SIZE is the size plus extra bytes to make the whole size be 
// a multiple of the second argument
#define		PADDED_SIZE(x, y)	( ((x)  + (y) - 1) & ~((y)-1) )

// the field size originally was EbUint8
// the change of typedef ENCODEDFIELDSIZE was also made in database/src/record.cpp
//typedef		EbUint8		EncodedFieldSize;
typedef		EbUint32		EncodedFieldSize;

//
// The first record id 
//
#define		FIRST_RECORD_ID		1

#endif 	// _interna_h_
