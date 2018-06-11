/*
 *  Copyright (C) 2018 Masatoshi Teruya
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 *
 *
 *  lua_iovec.h
 *  lua-iovec
 *
 *  Created by Masatoshi Teruya on 18/06/11.
 */


#ifndef lua_iovec_h
#define lua_iovec_h

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/types.h>
// lualib
#include "lauxhlib.h"


#if !defined(IOV_MAX)
#define IOV_MAX 1024
#endif

#define IOVEC_MT    "iovec"

typedef struct {
    int nvec;
    int used;
    size_t bytes;
    struct iovec *data;
    int *refs;
    size_t *lens;
} lua_iovec_t;


LUALIB_API int luaopen_iovec( lua_State *L );


#endif

