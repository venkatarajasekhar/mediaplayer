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

#ifndef _datadriver_h_
#define _datadriver_h_

#include "internal.h"
#include "datafile/include/datafile.h"

/***********************************************************************
 * This file declares DataDriver class.
 * DataDriver handles data encoding (serialization) and decoding 
 * (unserialization).
 * This is the base class.  All sub-classes typically only have one
 * instances, and all instances are linked together.
 ***********************************************************************
 */
class DataDriver {

 public:

    //
    // The constructor will link this instance into a global link.
    //
    DataDriver();

    //
    // Encode a data into the pre-allocated buffer.
    //
    // Para :
    //		[in]data : pointer to the app data
    //		[in/out]buffer : the buffer that may hold the encoded data.
    //			Depending on the encoding option, it may point
    //			to a new buffer, a pre-allocated buffer or not
    //			be used at all.
    //		[out]size : the encoded data size
    //		[in]alloc : whether we allocate a buffer for the encoded data
    //		[in]df : the data file which driver may use to store extra data
    //
    // Exception :
    //		EBE_OUT_OF_MEMORY
    //
    //
    virtual void Encode(EbUint8 * data, 
			EbUint8 *& buffer,
			EbUint32 & size,
			EbBoolean alloc,
			DataFile * df=NULL)=0;

    //
    // Returns the size of encoded data.
    //
    // Para :
    //	[in]	data : the data in decoded form.
    //
    // Exceptions :
    //		This function really should not return any exceptions.
    //		However, a particular subclass may have to do so.
    //
    virtual EbUint32 EncodedDataSize(EbUint8 * data) = 0;

    //
    // Free encoded data.  It frees file space used to encode the data.
    // The base class implementation is to do nothing.
    //
    // Para :
    //	[in] 	data : 	encoded data
    //	[in]	size : 	encoded data size
    //	[in]	df :	data file
    //
    virtual void FreeEncodedData(EbUint8 * data,
				 EbUint32 size,
				 DataFile *df) {}
    //
    // Decode an encoded data into a buffer.
    //
    // Para :
    //		data : pointer to the encoded data
    //		size : the encoded data size
    //		buffer : pointer to the buffer that holds the decoded data
    //		alloc : whether to allocate a buffer to hold the decoded data
    //		df : the data file from which more data may be read.
    //
    // Exception :
    //		EBE_OUT_OF_MEMORY
    //
    virtual void Decode(EbUint8 * data, 
			EbUint32 size, 
			EbUint8 *& buffer, 
			EbBoolean alloc,
			DataFile *df = NULL) = 0;

    //
    // Get the decoded data size from the decoded data.
    // For simple, linear data, the option argument is ignored.
    // For complex, hiearchical data, the option has a type-specific meaning.
    //
    // Para :
    //		[in] data : the decoded data
    //		[in] option : the option used to determine the decoded size.
    //
    // Return :
    // 		the decoded data size.
    //
    virtual EbUint32 DecodedDataSize(EbUint8 * data, EbUint32 option=0) =0;

    //
    // Free a decoded data.  We use virtual function because the decoded
    // may be a complex hiearchical data structure which only data driver
    // understands.
    //
    // The base class implementaion is to delete the data;
    //
    // Para :
    //	[in]	data : the decoded data to be freed.
    //
    virtual void FreeDecodedData(EbUint8 * data) { delete[] data; }

    //
    // Clone a decoded data.  This is a deep copy.
    //
    // Para :
    // 	[in]	data : the decoded.
    //
    // Return :
    //		A new cloned data in decoded form.
    //
    // Exceptions :
    //		EBE_OUT_OF_MEMORY
    //
    virtual EbUint8 * CloneDecodedData(EbUint8 * data) { 
	EB_ASSERT(EB_TRUE == EB_FALSE); 
	return NULL;
    }

    //
    // Set initial default data.  Default data is the value a field has 
    // when it has never been set before.  Each type of data has its 
    // own default value.
    //
    // Memory is allocated to hold the default value.
    //
    // Para :
    //		pBuffer : create a default app data of this type.
    //
    // Exception :
    // 		EBE_OUT_OF_MEMORY
    //
    virtual void SetDefault(EbUint8 *& pBuffer)=0;

    //
    // Return true if this driver can handle the specified data.
    //
    virtual EbBoolean CanHandleDataType(EbDataType datatype, EbUint32 flag)=0;

    //
    // Find a data driver by the data type and flag.
    //
    // Para :
    //		datatype
    //		flag
    //
    // Return :
    // 		The matching data driver.
    //
    // Exception :
    // 		EBE_DATA_DRIVER_NOT_FOUND
    //
    static DataDriver * FindDataDriver(EbDataType datatype, EbUint32 flag);

 private:
    // pointer to link all drivers together
    DataDriver * _pNext;
    
    // pointer to the first driver
    static DataDriver *_pFirst;
};

#endif 	/* ifndef _datadriver_h_ */
