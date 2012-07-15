#include "Utility/database/ebase.h"

EbTypedKey::EbTypedKey (int intkey)
{
    EbRawDataSize size = 4 + 4;
    
    rawkey = new char [size];
    if (rawkey == NULL) 
    {
        EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
    }
    
    *(EbRawDataSize*)rawkey = size;
    *(int*)((char*)rawkey + 4) = intkey;
    
}

EbTypedKey::EbTypedKey (const char *stringkey) 
{
    EbRawDataSize size = strlen(stringkey) + 4 + 1;
    
    rawkey = new char [size];
    if (rawkey == NULL) 
    {
        EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
    }
    
    *(EbRawDataSize*)rawkey = size;
    strcpy (rawkey + 4, stringkey);
    
    //            return res;
}

EbTypedKey::~EbTypedKey ()
{
    delete [] rawkey;
}

EbTypedKey::operator void * ()
{
    return rawkey;
}
