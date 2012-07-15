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

#ifndef _indexdriver_h_
#define _indexdriver_h_

// internal header file

#include "internal.h"
#include "ebase_impl.h"

/***********************************************************************
 * A abstract base class for index drivers.
 ***********************************************************************
 */
class IndexDriver {

 public:
    //
    // Constructor will link all index drivers together as a list.
    //
    IndexDriver();

    //
    // Check if this driver handles the index which is defined by
    // an index struct and a db def.
    //
    virtual EbBoolean HandleIndex(EbIndexDef * index) = 0;

    //
    // Compare two records.  The Index struct provided is one that
    // the driver declared to handle it previously.
    //
    virtual EbResult Compare(EbIndexDef * index,
			   EbRecord * pRecord1,
			   EbRecord * pRecord2,
			   EbInt32& result) = 0;

    //
    // Check if any fields relavent to the specified index are 
    // modified in the record.
    // The default implementation is applicable for generic drivers,
    // which checks whether any field defined in the index struct
    // is modified.
    //
    virtual EbBoolean IsModificationRelavent(EbIndexDef * index,
					   EbRecord * pRecord);

    //
    // Find an index driver that handles the index.
    //
    static IndexDriver * FindIndexDriver(EbIndexDef * pIndex);

 protected:
    // services to subclasses
    EbBoolean RecordFieldIsChanged(EbRecord *pRecord, EbUint32 fid) {
	EB_ASSERT(fid < pRecord->NumberOfFields());
	return pRecord->_lField[fid]._changed;
    }

    EbDataType GetIndexFieldDataType(EbIndexDef *index, EbUint32 i) {
	EB_ASSERT(i < index->NumberOfFields());
	return index->_pEbase->_dbDef->GetFieldDataType(index->_pd._fieldId[i]);
    }

    EbUint32 GetIndexFieldFlag(EbIndexDef *index, EbUint32 i) {
	EB_ASSERT(i < index->NumberOfFields());
	return index->_pEbase->_dbDef->GetFieldFlag(index->_pd._fieldId[i]);
    }

 private:
    IndexDriver * _pNext;
    static IndexDriver * _pFirst;
};

#endif
