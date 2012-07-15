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

#ifndef _fielddef_h_
#define _fielddef_h_

#include "Utility/database/ebglobal.h"

/***********************************************************************
 * Some constants related to field definition
 ***********************************************************************
 */

/***********************************************************************
 * Structure FieldDefinition defines a field in ebase.
 ***********************************************************************
 */
class FieldDefinition {
public:
    char _name[EB_MAX_FIELD_NAME+1];
    EbDataType _type;
    EbUint32 _flag;
    FieldDefinition () {}
    FieldDefinition (int index)
    {
        _flag = 0;
        switch (index)
        {
        case 0:
            _type = EB_INT32;
            strcpy (_name, EB_RECORD_ID);
            break;
        case 1:
//            _type = EB_INT32;
            _type = EB_RAW;
            strcpy (_name, "key");
            break;
        case 2:
//            _type = EB_STRING;
            _type = EB_RAW;
            strcpy (_name, "value");
            break;
        default:
            EB_ASSERT (0 == "EBE_INVALID_INDEX");
        }
    }
};

#endif
