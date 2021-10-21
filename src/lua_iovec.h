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
#include "lauxhlib.h"

#if !defined(IOV_MAX)
# define IOV_MAX 1024
#endif

#define IOVEC_MT "iovec"

typedef struct {
    size_t nbyte;
    lua_State *th;
    int ref;
} lua_iovec_t;

LUALIB_API int luaopen_iovec(lua_State *L);

/**
 * lua_iovec_setv sets the buffers that held by *iov to *vec. *nvec must be set
 * to length of *vec, and if it is NULL or 0, it will return 0 without changing
 * anything.
 * offset specifies the starting position of the buffer.　if it is greater than
 * or equal to the number of bytes held by *iov, it returns 0 without changing
 * anything.
 * nbyte specifies the number of bytes to be set for *vec. if it is 0, it
 * returns 0 without changing anything.
 */
static inline size_t lua_iovec_setv(lua_iovec_t *iov, struct iovec *vec,
                                    int *nvec, size_t offset, size_t nbyte)
{
    int nv      = 0;
    size_t nb   = 0;
    int maxnvec = 0;
    int top     = lua_gettop(iov->th);
    int i       = 1;

    if (!nvec || *nvec <= 0 || offset >= iov->nbyte || !nbyte) {
        return 0;
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

static inline int lua_iovec_writev(lua_State *L, int fd, lua_iovec_t *iov,
                                   size_t offset, size_t nbyte)
{
    struct iovec vec[IOV_MAX];
    int nvec   = IOV_MAX;
    size_t nb  = lua_iovec_setv(iov, vec, &nvec, offset, nbyte);
    ssize_t rv = writev(fd, vec, nvec);

    switch (rv) {
    // got error
    case -1:
        // again
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            lua_pushinteger(L, 0);
            lua_pushnil(L);
            lua_pushboolean(L, 1);
            return 3;
        }
        // closed by peer
        else if (errno == EPIPE || errno == ECONNRESET) {
            return 0;
        }
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;

    default:
        lua_pushinteger(L, rv);
        lua_pushnil(L);
        lua_pushboolean(L, nb - (size_t)rv);
        return 3;
    }
}

static inline int lua_iovec_readv(lua_State *L, int fd, lua_iovec_t *iov,
                                  size_t offset, size_t nbyte)
{
    struct iovec vec[IOV_MAX];
    int nvec   = IOV_MAX;
    ssize_t rv = 0;

    lua_iovec_setv(iov, vec, &nvec, offset, nbyte);
    rv = readv(fd, vec, nvec);
    switch (rv) {
    // closed by peer
    case 0:
        return 0;

    // got error
    case -1:
        lua_pushnil(L);
        // again
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            lua_pushnil(L);
            lua_pushboolean(L, 1);
            return 3;
        }
        lua_pushstring(L, strerror(errno));
        return 2;

    default:
        lua_pushinteger(L, rv);
        return 1;
    }
}

#endif
