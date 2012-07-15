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


#include "internal.h"
#include "fielddef.h"

#include "ebase_impl.h"

EbaseDef::EbaseDef() 
//    : EbArray(sizeof(FieldDefinition))
{
    // by default, the definition is modifiable
    _modifiable = EB_FALSE;

    _fieldDef[0] = new FieldDefinition(0);
    _fieldDef[1] = new FieldDefinition(1);
    _fieldDef[2] = new FieldDefinition(2);
}

EbaseDef::~EbaseDef ()
{
    delete _fieldDef[0];
    delete _fieldDef[1];
    delete _fieldDef[2];
}

    
EbUint32
EbaseDef::NumberOfFields() 
{
    return 3;
}

const char *
EbaseDef::GetFieldName(EbUint32 fieldIndex) 
{
    if (fieldIndex >= NumberOfFields()) 
    {
        EB_ASSERT (0 == "EBE_INVALID_FIELD");
    }
    return _fieldDef[fieldIndex]->_name;
}

EbDataType
EbaseDef::GetFieldDataType(EbUint32 fieldIndex)
{
    if (fieldIndex >= NumberOfFields()) 
    {
        EB_ASSERT (0 == "EBE_INVALID_FIELD");
    }
    return _fieldDef[fieldIndex]->_type;
} 

EbUint32
EbaseDef::GetFieldFlag(EbUint32 fieldIndex)
{
    if (fieldIndex >= NumberOfFields()) 
    {
        EB_ASSERT (0 == "EBE_INVALID_FIELD");
    }
    return _fieldDef[fieldIndex]->_flag;
} 


//FieldDefinition &
//EbaseDef::Get(EbUint32 i)
//{
//    return *_fieldDef[i];
//}

EbInt32
EbaseDef::GetFieldId(const char *fieldName)
{
    for (EbUint32 i=0; i< NumberOfFields(); i++) {
        if (strcmp(_fieldDef[i]->_name, fieldName) == 0) {
            return i;
        }
    }
    return -1;
}
