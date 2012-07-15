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


#include "../datadriver.h"

/***********************************************************************
 * This file defines and implements EbInt32 data driver.
 ***********************************************************************
 */
class Int32DataDriver : public DataDriver {
 public:
    
    //
    // Encode
    //
    void Encode(EbUint8 * data, 
                EbUint8 *& buffer,
                EbUint32 & size,
                EbBoolean alloc,
                DataFile *) {

        EB_ASSERT(((EbUint32)data & 3) == 0);	
        // source data should be aligned

        size = sizeof(EbInt32);
        if (alloc == EB_TRUE) {
            buffer = (EbUint8*) new EbInt32;
            if (buffer == NULL) 
            {
                EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
            }
        }
        EB_ASSERT(((EbUint32)buffer & 3) == 0);
        *(EbInt32*)buffer = *(EbInt32*)data;
    }

    //
    // Encoded data size
    //
    EbUint32 EncodedDataSize(EbUint8 *) {
        return sizeof(EbInt32);
    }

    //
    // Decode
    //
    void Decode(EbUint8 * data, 
                EbUint32 size, 
                EbUint8 *& buffer, 
                EbBoolean alloc,
                DataFile * df) {
        UNUSED(size);
        EB_ASSERT(size == sizeof(EbInt32));
        EB_ASSERT(((EbUint32)data & 3) == 0);
        if (alloc == EB_TRUE) {
            //..//..
			// buffer = (EbUint8*)new EbInt32;
			//
			buffer = new EbUint8 [sizeof (EbInt32)];
            if (buffer == NULL) 
            {
                EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
            }
        }
        EB_ASSERT(((EbUint32)buffer & 3) == 0);
        *(EbInt32*)buffer = *(EbInt32*)data;
    }

    //
    // Decoded data size
    //
    EbUint32 DecodedDataSize(EbUint8 *, EbUint32) {
        return sizeof(EbInt32);
    }

    EbUint8* CloneDecodedData(EbUint8 * data) {
        // check if data is aligned
        EB_ASSERT( ((EbUint32)data & 3) == 0);
        EbInt32* p = new EbInt32;
        if (p == NULL) 
        {
            EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
        }
        *p = *(EbInt32*)data;
        return (EbUint8*)p;
    }

    //
    // SetDefault - default is 0
    //
    void SetDefault(EbUint8 *& pBuffer) {
//        pBuffer = (EbUint8*)new EbInt32;
        pBuffer = (EbUint8*)new EbUint8 [sizeof (EbInt32)];
        if (pBuffer == NULL) 
        {
            EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
        }
        *(EbInt32*)pBuffer = 0;
    }

    //
    // CanHandleDataType
    //
    EbBoolean CanHandleDataType(EbDataType datatype, EbUint32 flag) {
        if (datatype == EB_INT32 && flag == 0) {
            return EB_TRUE;
        } else {
            return EB_FALSE;
        }
    }
};
 

/***********************************************************************
 * The only instance of EbInt32 data driver
 ***********************************************************************
 */
static Int32DataDriver theInt32DataDriver;

/* to force this code linked into image */
char undefTheInt32DataDriver;
