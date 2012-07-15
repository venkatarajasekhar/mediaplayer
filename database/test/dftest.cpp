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
#include "../src/datafile/include/datafile.h"

void main()
{
    DataFile df;
    char buf[100];

    // test environment
    EB_ASSERT(sizeof(EbInt8) == 1);
    EB_ASSERT(sizeof(EbUint8) == 1);
    EB_ASSERT(sizeof(EbInt16) == 2);
    EB_ASSERT(sizeof(EbInt32) == 4);

    // test create
    EB_VERIFY(DataFile::Create("testdf.df"), == EB_SUCCESS);
    EB_VERIFY(df.Open("testdf.df"), == EB_SUCCESS);
    EB_VERIFY(df.DataSize(0), == 0);
    EB_VERIFY(df.ResizeData(0, 10), == EB_SUCCESS);

    // test write & read data
    EB_VERIFY(df.WriteData(0, "12345", 0, 6), == 6);
    EB_VERIFY(df.WriteData(0, "555", 5, 4), == 4);
    EB_VERIFY(df.ReadData(0, buf), > 0);
    EB_VERIFY(strcmp(buf, "12345555"), == 0);

    // test close and open
    df.Close();
    EB_VERIFY(df.Open("testdf.df"), == EB_SUCCESS);
    EB_VERIFY(df.ReadData(0, buf), > 0);
    EB_VERIFY(strcmp(buf, "12345555"), == 0);

    // test add new data
    EbDataHandle h = df.NewData(20);
    EB_VERIFY(h, != EB_INVALID_HANDLE);
    EB_VERIFY(df.WriteData(h, buf), == 20);
    buf[0] = 0; buf[1] = 0;
    EB_VERIFY(df.ReadData(h, buf), == 20);
    EB_VERIFY(strcmp(buf, "12345555"), == 0);

    // test the new data across open/close session
    EB_VERIFY(df.WriteData(0, &h, 0, sizeof(h)), == sizeof(h));
    df.Close();
    EB_VERIFY(df.Open("testdf.df"), == EB_SUCCESS);
    EB_VERIFY(df.ReadData(0, &h, 0, sizeof(h)), == sizeof(h));
    EB_VERIFY(df.ReadData(h, buf), == 20);
    EB_VERIFY(strcmp(buf, "12345555"), == 0);

    // test delete data
    df.DeleteData(h);
    EB_VERIFY(df.DataSize(h), < 0);

    df.Close();

} 
