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

#include "indexdriver.h"

/***********************************************************************
 * Find an index driver by the index struct and db def
 ***********************************************************************
 */

/* static instance variable */
IndexDriver * IndexDriver::_pFirst = NULL;

/* constructor */
IndexDriver::IndexDriver() 
{
    _pNext = _pFirst;
    _pFirst = this;
}

/* static */ IndexDriver *
IndexDriver::FindIndexDriver(EbIndexDef * pIndex)
{
    for (IndexDriver *p = IndexDriver::_pFirst; p != NULL; p= p->_pNext) {
        if (p->HandleIndex(pIndex) == EB_TRUE) {
            return p;
        }
    }
    return NULL;
}

EbBoolean
IndexDriver::IsModificationRelavent(EbIndexDef * index, EbRecord * pRecord)
{
    // we only deal with generic drivers.  Specific drivers must provide
    // this function.
    EB_ASSERT(index->NumberOfFields() > 0);

    for (EbUint32 i=0; i< index->NumberOfFields(); i++) {
        if (RecordFieldIsChanged(pRecord, index->GetFieldId(i)) == EB_TRUE) {
            return EB_TRUE;
        }
    }
    return EB_FALSE;
}

/***********************************************************************
 * Some "smart" linker such as gcc won't link the individual driver
 * code into the image as nobody refer to them explicitly.  To overcome
 * this problem, the easiest and fool-proof way to refer to some variables
 * that are defined in the individual driver code.
 ***********************************************************************
 */
extern char undefTheInt32IndexDriver;
extern char undefTheStringIndexDriver;

void undefIndexDrivers() 
{
    undefTheInt32IndexDriver 
        = undefTheStringIndexDriver 
        = 0;
}
