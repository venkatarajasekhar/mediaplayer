#include "../datadriver.h"

typedef EbUint32 EbRawDataSize;


class RawDataDriver : public DataDriver {
public:
    //
    // Encode
    //
    void Encode(EbUint8 * data, 
        EbUint8 *& buffer,
        EbUint32 & size,
        EbBoolean alloc,
        DataFile *) 
    {
        size = *(EbRawDataSize*)data;
        if (alloc == EB_TRUE)
        {
            buffer = new EbUint8[size];
            if (buffer == NULL) 
            {
                EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
            }
        }
        
        memcpy (buffer, data, size);    
    }

    //
    // Encoded data size
    //
    EbUint32 EncodedDataSize(EbUint8 *data)
    {
        return *(EbRawDataSize*)data;
    }
    
    void FreeEncodedData(EbUint8 * data,
        EbUint32 size,
        DataFile *df) {}

    void Decode(EbUint8 * data, 
        EbUint32 size, 
        EbUint8 *& buffer, 
        EbBoolean alloc,
        DataFile *df = NULL)
    {
        EB_ASSERT (size == *(EbRawDataSize*)data);
        if (alloc == EB_TRUE)
        {
            buffer = new EbUint8[size];
            if (buffer == NULL) 
            {
                EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
            }
        }

        memcpy (buffer, data, size);
    }

    EbUint32 DecodedDataSize(EbUint8 * data, EbUint32 option=0)
    {
        return *(EbRawDataSize*)data;
    }

    void FreeDecodedData(EbUint8 * data) { delete[] data; }

    EbUint8 * CloneDecodedData(EbUint8 * data) { 
        EbUint32 size = DecodedDataSize(data, 0);
        EbUint8 *tmp = new EbUint8[size];
        if (tmp == NULL) 
        {
            EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
        }
        memcpy (tmp, data, size);
        return tmp;
    }
    
    void SetDefault(EbUint8 *& pBuffer)
    {
        pBuffer = new EbUint8 [sizeof(EbRawDataSize)];
        if (pBuffer == NULL) 
        {
            EB_ASSERT (0 == "EBE_OUT_OF_MEMORY");
        }
        *(EbRawDataSize*)pBuffer = (EbRawDataSize)4;
    }

    EbBoolean CanHandleDataType(EbDataType datatype, EbUint32 flag)
    {
        if (datatype == EB_RAW && flag == 0) {
            return EB_TRUE;
        } else {
            return EB_FALSE;
        }            
    }
};

static RawDataDriver theRawDataDriver;

char undefTheRawDataDriver;
