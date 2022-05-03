/**
 * Copyright (C) 2018 Masatoshi Fukunaga
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 *
 * lua_iovec.h
 * lua-iovec
 *
 * Created by Masatoshi Teruya on 18/06/11.
 */

#ifndef lua_iovec_h
#define lua_iovec_h

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
// lualib
#include <lauxhlib.h>

#if !defined(IOV_MAX)
# define IOV_MAX 1024
#endif

#define IOVEC_MT "iovec"

typedef struct {
    size_t nbyte;
    lua_State *th;
    int ref;
} lua_iovec_t;

static inline void lua_iovec_loadlib(lua_State *L)
{
    int top = lua_gettop(L);

    // load iovec module
    luaL_getmetatable(L, IOVEC_MT);
    if (lua_isnil(L, -1)) {
        luaL_loadstring(L, "require('iovec')");
        lua_call(L, 0, 0);
    }
    lua_settop(L, top);
}

static inline int lua_iovec_new(lua_State *L)
{
    lua_iovec_t *iov = lua_newuserdata(L, sizeof(lua_iovec_t));

    iov->nbyte = 0;
    iov->th    = lua_newthread(L);
    iov->ref   = lauxh_ref(L);
    lauxh_setmetatable(L, IOVEC_MT);

    return 1;
}

/**
 * lua_iovec_setv sets the buffers that held by 'iov' to 'vec' and returns the
 * number of bytes set.
 *
 * - 'nvec' must be set to length of 'vec', and if it is NULL or 0, it will
 * return 0 without changing anything.
 * - 'offset' specifies the starting position of the buffer.　if it is greater
 * than or equal to the number of bytes held by 'iov', it returns 0 without
 * changing anything.
 * - 'nbyte' specifies the number of bytes to be set for 'vec'. if it is 0, set
 * all data.
 */
static inline size_t lua_iovec_setv(lua_iovec_t *iov, struct iovec *vec,
                                    int *nvec, size_t offset, size_t nbyte)
{
    int nv      = 0;
    size_t nb   = 0;
    int maxnvec = 0;
    int top     = lua_gettop(iov->th);
    int i       = 1;

    if (!nvec || *nvec <= 0 || offset >= iov->nbyte) {
        return 0;
    } else if (nbyte == 0) {
        nbyte = iov->nbyte;
    }
    maxnvec = *nvec;

    // skip offset bytes from the beginning
    for (; offset && i <= top; i++) {
        struct iovec *v = &vec[nv];
        char *data      = lua_touserdata(iov->th, i);
        size_t len      = strlen(data);

        if (offset >= len) {
            offset -= len;
            continue;
        }

        data += offset;
        len -= offset;
        offset      = 0;
        v->iov_base = data;
        if (nbyte >= len) {
            v->iov_len = len;
            nbyte -= len;
        } else {
            v->iov_len = nbyte;
            nbyte      = 0;
        }
        nb += v->iov_len;
        nv++;
    }

    for (; nv < maxnvec && nbyte && i <= top; i++) {
        struct iovec *v = &vec[nv];
        char *data      = lua_touserdata(iov->th, i);
        size_t len      = strlen(data);

        v->iov_base = data;
        if (nbyte >= len) {
            v->iov_len = len;
            nbyte -= len;
        } else {
            v->iov_len = nbyte;
            nbyte      = 0;
        }
        nb += v->iov_len;
        nv++;
    }

    *nvec = nv;

    return nb;
}

#endif
