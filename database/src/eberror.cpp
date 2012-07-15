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

/***********************************************************************
 * This file implements EbError class
 ***********************************************************************/

#include "internal.h"

static struct {
    int _errorNumber;
    const char * _errorName;
} errorStringName[] = {
    {EBE_FAIL, "EBE_FAIL"}, 
    {EBE_OPEN_FILE, "EBE_OPEN_FILE"},
    {EBE_OUT_OF_MEMORY, "EBE_OUT_OF_MEMORY"},
    {EBE_OUT_OF_FILE_SPACE, "EBE_OUT_OF_FILE_SPACE"},
    {EBE_DATA_DRIVER_NOT_FOUND, "EBE_DATA_DRIVER_NOT_FOUND"},
    {EBE_FIELD_TYPE_MISMATCH, "EBE_FIELD_TYPE_MISMATCH"},
    {EBE_INVALID_FIELD, "EBE_INVALID_FIELD"},
    {EBE_INVALID_INDEX, "EBE_INVALID_INDEX"},
    {EBE_INVALID_RECORD, "EBE_INVALID_RECORD"},
    {EBE_ACCESS_VIOLATION, "EBE_ACCESS_VIOLATION"},
    {EBE_DATABASE_NOT_OPEN, "EBE_DATABASE_NOT_OPEN"},
    {EBE_DATABASE_FILE_NOT_LOCKED, "EBE_DATABASE_FILE_NOT_LOCKED"},
    {EBE_DATABASE_FILE_LOCK_USED_BY_OTHER_DB_OBJECT, "EBE_DATABASE_FILE_LOCK_USED_BY_OTHER_DB_OBJECT"},
    {EBE_DATABASE_FILE_LOCK_UNUSED_BY_DB_OBJECT, "EBE_DATABASE_FILE_LOCK_UNUSED_BY_DB_OBJECT"},
    {0, 0}
};

const char *
EbError::GetStringName()
{
    for(int i=0; errorStringName[i]._errorNumber != 0; i++) {
        if (_error == errorStringName[i]._errorNumber) {
            return errorStringName[i]._errorName;
        }
    }

    /* this is an internal error */
    EB_CHECK(1 == 0);
    return 0;
}
