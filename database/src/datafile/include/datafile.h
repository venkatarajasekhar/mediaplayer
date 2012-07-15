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


#ifndef _datafile_h_
#define _datafile_h_

#include "dfhandlearray.h"


/***********************************************************************
 * A DataFile object is associated with an open file.  It maps the file
 * to a list of variable sized data.  It provides the following 
 * functionalities :
 *  . query the offset set and size of a data.  retrieve a data.
 *  . add a new data.
 *  . replace a data.
 *  . delete a data.
 *
 *  Optionally, it may provide the following advanced support :
 *  . retrieve partial data.
 *  . modify partial data.
 *  . resize data.
 *
 *  Each data is identified by its handle.  Handle 0 is a special
 *  handle which always exists.  Initially, it has 0 size. Other handles are
 *  created/deleted/recyled dynamically.
 ***********************************************************************
 */
class DataFile {

 public:   
    //
    // Constructor.
    //
    DataFile();

    //
    // destructor. Responsible for closing file.
    //
    ~DataFile();

    //
    // Return true if the object is associated with an open file
    //
    EbBoolean IsOpen() {
        return _file != NULL ? EB_TRUE : EB_FALSE;
    }

    //
    // Create an empty data file.
    //
    static EbResult Create(const char *fname);

    //
    // Initialization with an opened file handle
    // Para : 
    // 		f - pointer to file handle
    // Return :
    //		SUCCESS if the file is a data file.
    //          FAILURE otherwise
    // 
    EbResult Open(FILE *f);

    //
    // Initialize with a file name
    // Para : 
    //		fname - pointer to file name.
    // Return :
    //		SUCCESS, if file is successfully opened
    //		FAILURE, if file cannot be opened
    //
    EbResult Open(const char * fname);

    //
    // Close an opened data file.
    //
    void Close();

    //
    // Query the offset of a data by its handle.
    // Para :
    //		handle - 
    // Return :
    //		offset
    //          (EbUint32)-1 if the handle is invalid
    //
    EbUint32 DataOffset(EbDataHandle handle);

    //
    // Query the size of a data by its handle.
    // Para :
    //		handle -
    // Return :
    //		size;
    //          <0 if the handle is invalid.
    //
    EbInt32 DataSize(EbDataHandle handle);

    //
    // Add a data for future write.
    // Para :
    //		size  - The size of the data.
    // Return :
    //		the handle for this data.
    //		INVALID_HANDLE is returned if it fails.
    //
    EbDataHandle NewData(EbUint32 size=0);

    //
    // Delete a data. 
    // Para :
    //		handle - the handle for the data to be deleted.
    //			It should not be 0.
    //
    void DeleteData(EbDataHandle handle);

    //
    // Resize a data.  The data must have already existed.
    // Para :
    //          [in] handle
    //          [in] size       the new data size
    // Return :
    //          SUCCESS or FAILURE
    //
    EbResult ResizeData(EbDataHandle handle, EbUint32 size);

    //
    // Read the whole or partial data.
    // Para :
    //          [in] handle     the data handle
    //          [out]*buf       the data read into the buf
    //          [in]offset      the starting byte from the beginning 
    //                          of the data
    //          [in]size        how many bytes to read.  If negative, the 
    //                          remaining data starting from the offset is
    //                          read in.
    // Return :
    //          the bytes that are read
    //          < 0 to indicate an error
    //
    EbInt32 ReadData(EbDataHandle handle, 
                   void *buf, 
                   EbUint32 offset=0, 
                   EbInt32 size=-1);

    //
    // Write the whole or partial data.
    // Para :
    //          [in]size        how many bytes to read.  If negative, the 
    //                          remaining data starting from the offset is
    //                          read in.
    // Return :
    //          the bytes that are written.
    //          < 0 to indicate an error.
    //
    EbInt32 WriteData(EbDataHandle handle, 
                    void *data, 
                    EbUint32 offset=0, 
                    EbInt32 size=-1);

#ifdef EB_DEBUG
    //
    // Print out the content for debugging purpose
    //
    void Dump();
#endif
    bool isDirty() {return m_isDirty;}
 private:
    bool m_isDirty;
    //
    // Allocate a free handle slot.  It will expand handle table and 
    // re-allocate file space for the handle table if necessary.
    // Return :
    //		the index to the free handle
    //		< 0 if it failed.
    //
    EbDataHandle AllocateHandle();

    //
    // FreeHandle marks the handle to be a free handle, and link it
    // to the free handle list.
    //
    // Para :
    //	[in]	handle  	The handle to be freed
    //
    void FreeHandle(EbDataHandle handle);

    //
    // A low-level space alloaction routine.  It will find a space
    // that is big enough to satify the request.  It will expand the
    // file size as the last resort.
    // Para :
    //          [in] size
    //          [out] pos       The position of the new space in handle index
    // Return :
    //          the offset of the found space
    //          or < 0 if failed to find any space or expand the file
    //
    EbInt32 AllocateSpace(EbUint32 size, EbUint32 & pos);
    
    
    // every datafile will have a header like this
    struct Header {
        char _magicWord[EB_MAGIC_WORD_LENGTH];
        EbUint8 _flag;		// not used for now
        EbUint8 _major;
        EbUint8 _minor;
        EbUint32 _numHandles;
        EbUint32 _handleOffset;
        EbDataHandle _freeHandle;
    } _hdr;

    // each Handle is a structure
    struct Handle {
        union {
            EbUint32 _offset;
            EbDataHandle _nextHandle;             // for free handle list
        };
        EbUint32 _flag : 1;
        EbUint32 _size : 31;
    } * _lHandle;

    // all slots in use are sorted
    HandleIndexArray _lHandleIndex;

    // the associated file
    FILE *_file;
};   
    
#endif
