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

#include "Utility/database/ebglobal.h"

void EbAssertFail(const char *fname,
                int line,
                const char * msg) {
    fprintf(stderr, "Assertion failed at %s:%d - %s\n", fname, line, msg);
    exit(-1);
}
