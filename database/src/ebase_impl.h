#ifndef __EBASE_IMPL_H__
#define __EBASE_IMPL_H__


#ifndef WIN32
#include <pthread.h>
#endif
//======================================================================
// CLASS EbaseDef
//  EbaseDef is a class that let apps to construct a definition
//  used to created Ebase database file.  It can also be used to 
//  query the structure of an existing Ebase database.
//
// IMPORTANT
//  Notice that in the current implementation the definition of an 
//  existing database is not modifiable.
//======================================================================
class EbaseDef {
    
    
    EbaseDef();
    ~EbaseDef();
    
    FieldDefinition *_fieldDef[3];
    
public:
    // -*-
    // Report the number of fields in the definition.
    //
    // Return :
    //          The number of fields.
    //
    EbUint32 NumberOfFields()   ;
    
    // -*-
    // Get the field name by field index.
    //
    // Para :
    //  [in] fieldId
    //      Must be in the range [0, numFields-1].
    //
    // Return :
    //          Pointer to the string that represents the field name.
    //          Note: Caller needs to copy the string to a separate place 
    //      if it wants to have its own copy.
    //
    // Exceptions :
    //      EBE_INVALID_FIELD
    //
    const char * GetFieldName(EbUint32 fieldId)   ;
    
    // -*-
    // Get the field data type.
    //
    // Para :
    //  [in] fieldId 
    //      Must be in the range [0, numFields-1].
    //
    // Return :
    //      Data type.
    // 
    // Exceptions :
    //      EBE_INVALID_FIELD
    //
    EbDataType GetFieldDataType(EbUint32 fieldId);
    
    // -*-
    // Get the field flag.
    //
    // Para :
    //  [in] fieldId
    //      Must be in the range [0, numFields-1].
    //
    // Return :
    //          field flag.
    //
    // Exceptions :
    //      EBE_INVALID_FIELD
    //
    EbUint32 GetFieldFlag(EbUint32 fieldId);
    
    // -*-
    // Find the field index by field name.
    //
    // Para :
    //  [in] fieldName
    //
    // Return :
    //          The field index of the field. Or < 0 if not found.
    //
    EbInt32 GetFieldId(const char *fieldName);
    
    // -*-
    // Remove all fields
    //
    
private:
    friend class Ebase;
    
    //
    // whether this instance is modifiable.  A definition owned
    // by an ebase object is not modifiable.
    //
    EbBoolean _modifiable;
};




class EbRecord {

 public:
    // -*-
    // Destructor.  
    //
    // Notice that there is no public constructor
    // for EbRecord becase application is not allowed to create a 
    // new EbRecord object.  Application can only obtain a pointer
    // to the EbRecord object through ebase.
    //
    ~EbRecord();

    
    // -*-
    // Generic (untyped) getfield by field name
    void * GetField(const char * fieldName)   ; 

    // -*-
    // Generic (untypeed) getfield by field name (alloc version)
    void * GetFieldAlloc(const char * fieldName);
    
    // -*-
    // Generic (untyped) getfield by field index
    void * GetField(EbUint32 fieldId)   ; 

    // -*-
    // Generic (untyped) getfield by field index (alloc version)
    void * GetFieldAlloc(EbUint32 fieldId);

    // -*-
    EbInt32 GetFieldInt32(const char *fieldName);

    // -*-
    EbInt32 GetFieldInt32(EbUint32 fieldId);


    // -*-
    // Generic version of querying field size (by field name).
    EbUint32 FieldSize(const char *fieldName, EbUint32 option=0);
    // -*-
    // Generic version of querying field size (by field index).
    EbUint32 FieldSize(EbUint32 fieldId, EbUint32 option=0);

    // -*-
    // Generic PutField by field name.
    void PutField(const char * fieldName, void * ptr); 
    // -*-
    // Generic PutField by field name (alloc version).
    void PutFieldAlloc(const char * fieldName, void * ptr);
    // -*-
    // Generic PutField by field index.
    void PutField(EbUint32 fieldId, void * ptr);
    // -*-
    // Generic PutField by field index (alloc version).
    void PutFieldAlloc(EbUint32 fieldId, void * ptr);
   
    // -*-
    void PutFieldInt32(const char *fieldName, EbInt32 number);
    // -*-
    void PutFieldInt32(EbUint32 fieldId, EbInt32 number);

    // -*-
    // Return the number of fields in the database.
    //
    EbUint32 NumberOfFields();

    // -*-
    // Return the pointer the database.  Each record, no matter which state
    // it is in, must belong to some database.
    //
    Ebase * GetEbase();

 private:
    friend class Ebase;
    friend class IndexDriver;

    // -*-
    // Constructor.  We make the constructor private so that nobody but
    // the Ebase object can create a record.
    //
    EbRecord(Ebase * eb);

    //
    // All records must be initialized before it is used.  Resource allocated
    // in Init() will be freed in the destructor.
    //
    // Return :
    //      SUCCESS or FAILURE
    //
    EbResult Init();
    
    //
    // CheckDataType.  A helper function.
    //
    // Para :
    //  [in]    fid :   field index
    //  [in]    type :  data type
    //
    // Exceptions :
    //      EBE_INVALID_FIELD
    //      EBE_FIELD_TYPE_MISMATCH
    //
    void CheckDataType(EbUint32 fid, EbDataType type);

    //
    // Locate encoded field.
    //
    EbUint8* LocateEncodedField(EbUint32 fid);

    //
    // Get encoded field size.
    //
    EbUint32 GetEncodedFieldSize(EbUint32 fid);

    //
    // Decode a field.  If the field is already modified or decoded,
    // this function does nothing.  Otherwise, it will create the
    // decoded data from the encoded data.
    //
    // Exceptions :
    //      EBE_OUT_OF_MEMORY
    //
    void DecodeField(EbUint32 fid);

    //
    // Field class contains field data in decoded form.
    //
    // Possible states of a field :
    //  !_decoded && !_changed : 
    //      _decodedData must be NULL;
    //      PutField will set _changed and _decodedData;
    //      ReadField will decode and set _decoded;
    //      CommitRecord will not skip this field and does change it state.
    //
    //  _decoded && !_changed :
    //      _decodedDatat must be meaningfull.  (Could be NULL if NULL is
    //          a legal value for the data type.)
    //      PutField will delete current _decodedData, set the new data,
    //          and set _changed flag;
    //      ReafField will return _decodedData.
    //      CommitRecord will not skip this field and does change it state.
    //      
    //  _changed :
    //      _decodedData must be meaningful.
    //      PutField will replace the current encoded data;
    //      ReadField will return the _decodedData;
    //      CommitRecord will encode this field and change state to
    //      _decoded && !_changed;
    //
    struct Field {
        EbUint8 * _decodedData;

        EbBoolean _decoded; // whether this field data is decoded
        // if _decoded is FALSE, _decodedData is NULL.

        EbBoolean _changed; // whether this field is modifed
        // if this field is modified, _decdedData contains new data.
        // for a new record, this flag is always set,
        // for an exiting record, it is set when this field is modified

        // constructor initialize the data
        // Memory occupied by a field is freed by the record
        Field(); 
    };

    //
    // an array of fields
    //
    Field *_lField;

    // 
    // pointer to the ebase object.
    //
    Ebase * _pEbase;

    //
    // the datahandle that holds the record content
    //
    EbDataHandle _handle;

    //
    // pointer to the encoded record.
    //
    EbUint8 * _pRecordData;

    //
    // Pointer to next active record.  All records of the same ebase are 
    // linked together.
    //
    EbRecord * _pNext;
};

//======================================================================
// CLASS EbIndexDef
// EbIndexDef allows applications to construct or query about an index
// definition in Ebase.
// In ebase, an index can be defined based on multiple fields.
// Applications cannot modify the index definition if the definition
// already exists in ebase.
//======================================================================
class EbIndexDef {
    
public:
    
    // -*-
    // Number of fields included in the def.
    //
    // Return :
    //          Number of fields.
    //
    EbUint32 NumberOfFields() { return _pd._numFields; }
    
    // -*-
    // Get field name.  The caller needs to copy the return string to make
    // a separate copy.
    // Para :
    //  [in] fieldId
    //      Must be in the range [0, numFields-1].
    //
    // Exceptions :
    //  [] EBE_INVALID_FIELD
    //      Field index is out of range
    //
    const char * GetFieldName(EbUint32 fieldIndex) ;
    
    // -*-
    // Get the field id in the ebase def.
    //
    // Exceptions :
    //      EBE_INVALID_FIELD
    //
    EbUint32 GetFieldId(EbUint32 fieldIndex)   ;
    
private:
    friend class Ebase;
    friend class EbIndexDefWrapper;
    friend class IndexDriver;
    
    //
    // Constructor.  We make it private so that one must obtain
    // a new one of an existing from an open database.
    // 
    EbIndexDef(Ebase *ebase, int identifier)
    {
        _pEbase = ebase;
        _pDriver = NULL;
        _modifiable = EB_FALSE;
        
        _pd._flag = 0;
        _pd._numFields = 1;
        _pd._fieldId[0] = identifier;
        
    }
    
    // run-time variables
    EbBoolean _modifiable;
    IndexDriver * _pDriver;
    Ebase * _pEbase;
    
    // persistent data
    struct PD {
        EbUint32 _flag;
        EbUint32 _numFields;
        EbUint8 _fieldId[EB_MAX_NUM_INDEX_FIELDS];
    } _pd;
};

class EbLock
{
    friend class Ebase;
    
private:
    bool usedByDB;
    char _dbFileName[EB_MAX_FILE_NAME];
    inline void setUsedFlag (bool f);
    inline bool getUsedFlag ();
    
public:
    EbLock (int fd);
    
    bool doLock ();
    
    bool isLockingFile ();
    
    bool releaseLock ();
    
    char _lockfile[EB_MAX_FILE_NAME];
    
    FILE *_file;
	int  lock_fd;
};

//////////////////////////////////////////////////////////////////////
//                      IN-LINE FUNCTIONS                           //
//////////////////////////////////////////////////////////////////////

// ======================================================================
//      IN-LINE FUNCTIONS FOR EbIndexDef
// ======================================================================
inline EbUint32 
EbIndexDef::GetFieldId(EbUint32 fieldIndex)  
{
    if (fieldIndex >= _pd._numFields) 
    {
        EB_ASSERT (0 == "EBE_INVALID_FIELD");
    }
    return _pd._fieldId[fieldIndex];
}

inline const char * 
EbIndexDef::GetFieldName(EbUint32 fieldIndex)  
{
    return _pEbase->_dbDef->GetFieldName(GetFieldId(fieldIndex));
}

// ======================================================================
//      IN-LINE FUNCTIONS FOR EbRecord
// ======================================================================

inline Ebase *
EbRecord::GetEbase()
{
    return _pEbase;
}

inline EbUint32
EbRecord::NumberOfFields()
{
    return _pEbase->_dbDef->NumberOfFields();
}

inline void *
EbRecord::GetField(const char * fieldName)  
{
    return GetField(_pEbase->_dbDef->GetFieldId(fieldName));
}

inline void * 
EbRecord::GetFieldAlloc(const char * fieldName)
{
    return GetFieldAlloc(_pEbase->_dbDef->GetFieldId(fieldName));
}

inline EbUint32 
EbRecord::FieldSize(const char * fieldName, EbUint32 option)
{
    return FieldSize(_pEbase->_dbDef->GetFieldId(fieldName), option);
}

inline void
EbRecord::CheckDataType(EbUint32 fid, EbDataType type)
{
    if (fid >= NumberOfFields()) { 
        EB_ASSERT (0 == "EBE_INVALID_FIELD");
    } 
    if (_pEbase->_dbDef->GetFieldDataType(fid) != type) { 
        EB_ASSERT (0 == "EBE_FIELD_TYPE_MISMATCH"); 
    }
}

inline EbInt32 
EbRecord::GetFieldInt32(EbUint32 fieldId)
{
    CheckDataType(fieldId, EB_INT32);
    return *(EbInt32*)GetField(fieldId);
}

inline EbInt32 
EbRecord::GetFieldInt32(const char *fieldName)
{
    return GetFieldInt32((EbUint32)_pEbase->_dbDef->GetFieldId(fieldName));
}

inline void 
EbRecord::PutField(const char * fieldName, void * ptr)
{
    PutField(_pEbase->_dbDef->GetFieldId(fieldName), ptr);
}

inline void 
EbRecord::PutFieldAlloc(const char * fieldName, void * ptr)
{
    PutFieldAlloc(_pEbase->_dbDef->GetFieldId(fieldName), ptr);
}

inline void 
EbRecord::PutFieldInt32(EbUint32 fieldId, EbInt32 number)
{
    CheckDataType(fieldId, EB_INT32);
    PutFieldAlloc(fieldId, &number);
}

inline void 
EbRecord::PutFieldInt32(const char *fieldName, EbInt32 number)
{
    PutFieldInt32(_pEbase->_dbDef->GetFieldId(fieldName), number);
}


// ======================================================================
//      IN-LINE FUNCTIONS FOR Ebase
// ======================================================================
inline EbUint32
Ebase::NumberOfRecords()
{
    return _hdr._numRecords;
}

inline EbBoolean
Ebase::IsOpen()
{
    if (_file != NULL) return EB_TRUE;
    else return EB_FALSE;
}

// ======================================================================
//      IN-LINE FUNCTIONS FOR EbLock
// ======================================================================
inline void 
EbLock::setUsedFlag (bool f)
{
    usedByDB = f;
}

inline bool 
EbLock::getUsedFlag ()
{
    return usedByDB;
}

#ifndef WIN32
class EbSemLock
{
public:
    EbSemLock (sem_t *p);
    int TryWait ();
    ~EbSemLock ();
private:
    bool locked;
    sem_t *_sem;
};
#endif

#endif 


