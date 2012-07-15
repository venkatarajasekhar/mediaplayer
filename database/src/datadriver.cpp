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

#include "datadriver.h"

/***********************************************************************
 * This file implements non-virtual functions in DataDriver class.
 ***********************************************************************
 */

//
// define the static member data and set the initial value
//
DataDriver * DataDriver::_pFirst = NULL;

/***********************************************************************
 ***********************************************************************
 */
DataDriver::DataDriver()
{
    _pNext = _pFirst;
    _pFirst = this;
}

/***********************************************************************
 ***********************************************************************
 */
/* static */ DataDriver *
DataDriver::FindDataDriver(EbDataType datatype, EbUint32 flag)
{
    for (DataDriver *p = _pFirst; p != NULL; p = p->_pNext) {
	if (p->CanHandleDataType(datatype, flag) == EB_TRUE) {
	    return p;
	}
    }

    // for this version, we treat data driver not found as a fatal error.
    // This saves some effort in the caller function (for checking 
    // return value.)
    EB_ASSERT (0 == "EBE_DATA_DRIVER_NOT_FOUND");

    // it really does not make sense, but we have to add this to 
    // make compiler happy
    return NULL;
}


/***********************************************************************
 * Some "smart" linker such as gcc won't link the individual driver
 * code into the image as nobody refer to them explicitly.  To overcome
 * this problem, the easiest and fool-proof way to refer to some variables
 * that are defined in the individual driver code.
 ***********************************************************************
 */
extern char undefTheInt32DataDriver;
//extern char undefTheStringDataDriver;
//..//..
extern char undefTheRawDataDriver;
//..//..
void undefDataDrivers() 
{
    undefTheInt32DataDriver 
        = undefTheRawDataDriver
        = 0;
}
