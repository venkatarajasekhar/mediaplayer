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

#include "../indexdriver.h"


/***********************************************************************
 * Int32 generic index driver
 ***********************************************************************
 */
class Int32IndexDriver : public IndexDriver {

 public:
    EbBoolean HandleIndex(EbIndexDef * index) {
	if (index->NumberOfFields() == 1 &&
	    GetIndexFieldDataType(index, 0) == EB_INT32 &&
	    GetIndexFieldFlag(index, 0) == 0) {
	    return EB_TRUE;
	} else {
	    return EB_FALSE;
	}
    }

    EbResult Compare(EbIndexDef * index,
		   EbRecord * pRecord1,
		   EbRecord * pRecord2,
		   EbInt32& result) {

        EB_ASSERT(HandleIndex(index) == EB_TRUE);
	    EbUint32 fid = index->GetFieldId(0);

        if (pRecord1->GetFieldInt32(fid) < 
		    pRecord2->GetFieldInt32(fid)) {
		    result = -1;
	    } else if (pRecord1->GetFieldInt32(fid) == 
		           pRecord2->GetFieldInt32(fid)) {
		    result = 0;
	    } else {
		    result = 1;
	    }
	    return EB_SUCCESS;
	    
    }

};

static Int32IndexDriver theInt32IndexDriver;

/* force the code to be linked */
char undefTheInt32IndexDriver;
