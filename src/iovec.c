/**
 * Copyright (C) 2018 Masatoshi Teruya
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
 * iovec.c
 * lua-iovec
 *
 * Created by Masatoshi Teruya on 18/06/11.
 */

#include "lua_iovec.h"

static int readv_lua(lua_State *L)
{
    lua_iovec_t *iov = lauxh_checkudata(L, 1, IOVEC_MT);
    int fd           = lauxh_checkinteger(L, 2);
    uint64_t offset  = lauxh_optuint64(L, 3, 0);
    uint64_t nb      = lauxh_optuint64(L, 4, iov->nbyte);

    return lua_iovec_readv(L, fd, iov, offset, nb);
}

static int writev_lua(lua_State *L)
{
    lua_iovec_t *iov = lauxh_checkudata(L, 1, IOVEC_MT);
    int fd           = lauxh_checkinteger(L, 2);
    uint64_t offset  = lauxh_optuint64(L, 3, 0);
    uint64_t nb      = lauxh_optuint64(L, 4, iov->nbyte);

    return lua_iovec_writev(L, fd, iov, offset, nb);
}

static int consume_lua(lua_State *L)
{
    lua_iovec_t *iov = lauxh_checkudata(L, 1, IOVEC_MT);
    int top          = lua_gettop(iov->th);
    lua_Integer nb   = lauxh_checkinteger(L, 2);

    // check arguments
    if (nb < 0) {
        lua_pushinteger(L, iov->nbyte);
        return 1;
    } else if ((size_t)nb >= iov->nbyte) {
        lua_settop(iov->th, 0);
        iov->nbyte = 0;
        lua_pushinteger(L, iov->nbyte);
        return 1;
    }

    while (nb > 0 && top > 0) {
        char *data = lua_touserdata(iov->th, 1);
        size_t len = strlen(data);

        if ((size_t)nb < len) {
            size_t newlen = len - (size_t)nb;
            char *newdata = lua_newuserdata(iov->th, newlen + 1);
            memcpy(newdata, data + nb, newlen);
            newdata[newlen] = 0;
            lua_replace(iov->th, 1);
            iov->nbyte -= nb;
            break;
        }

        nb -= (lua_Integer)len;
        lua_remove(iov->th, 1);
        iov->nbyte -= len;
        top--;
    }
    lua_pushinteger(L, iov->nbyte);

    return 1;
}

static int concat_lua(lua_State *L)
{
    lua_iovec_t *iov   = lauxh_checkudata(L, 1, IOVEC_MT);
    lua_Integer offset = lauxh_optinteger(L, 2, 0);
    lua_Integer nb     = lauxh_optinteger(L, 3, iov->nbyte);
    int top            = lua_gettop(iov->th);
    int nvec           = 0;
    int i              = 1;

    // check arguments
    if (offset < 0) {
        offset = 0;
    }
    if (nb < 0) {
        nb = 0;
    }

    lua_settop(L, 0);
    if (!top || !nb || (size_t)offset > iov->nbyte) {
        lua_pushstring(L, "");
        return 1;
    } else if (!lua_checkstack(L, top)) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(ENOMEM));
        return 2;
    }

    // skip offset bytes from the beginning
    for (; offset && i <= top; i++) {
        char *data = lua_touserdata(iov->th, i);
        size_t len = strlen(data);

        if (offset >= len) {
            offset -= len;
            continue;
        }
        data += offset;
        len -= offset;
        offset = 0;
        if (nb >= len) {
            nb -= len;
            lua_pushlstring(L, data, len);
        } else {
            lua_pushlstring(L, data, nb);
            nb = 0;
        }
        nvec++;
    }

    for (; nb && i <= top; i++) {
        char *data = lua_touserdata(iov->th, i);
        size_t len = strlen(data);

        if (nb >= len) {
            nb -= len;
            lua_pushlstring(L, data, len);
        } else {
            lua_pushlstring(L, data, nb);
            nb = 0;
        }
        nvec++;
    }

    lua_concat(L, nvec);

    return 1;
}

static int del_lua(lua_State *L)
{
    lua_iovec_t *iov = lauxh_checkudata(L, 1, IOVEC_MT);
    lua_Integer idx  = lauxh_checkinteger(L, 2);
    char *data       = NULL;
    size_t len       = 0;

    if (idx < 1 || idx > lua_gettop(iov->th)) {
        lua_pushnil(L);
        return 1;
    }

    data = lua_touserdata(iov->th, idx);
    len  = strlen(data);
    lua_pushlstring(L, data, len);
    lua_remove(iov->th, idx);
    iov->nbyte -= len;

    return 1;
}

static int get_lua(lua_State *L)
{
    lua_iovec_t *iov = lauxh_checkudata(L, 1, IOVEC_MT);
    lua_Integer idx  = lauxh_checkinteger(L, 2);

    if (idx < 1 || idx > lua_gettop(iov->th)) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L, (char *)lua_touserdata(iov->th, idx));

    return 1;
}

static int set_lua(lua_State *L)
{
    lua_iovec_t *iov = lauxh_checkudata(L, 1, IOVEC_MT);
    size_t len       = 0;
    const char *str  = lauxh_checklstring(L, 2, &len);
    lua_Integer idx  = lauxh_checkinteger(L, 3);
    size_t idxlen    = 0;
    char *data       = NULL;

    if (idx < 1 || idx > lua_gettop(iov->th)) {
        lua_pushboolean(L, 0);
        return 1;
    } else if (!lua_checkstack(iov->th, 1)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, strerror(ENOMEM));
        return 2;
    }

    idxlen = strlen((char *)lua_touserdata(iov->th, idx));
    data   = lua_newuserdata(iov->th, len + 1);
    memcpy(data, str, len);
    data[len] = 0;
    lua_replace(iov->th, idx);
    iov->nbyte = iov->nbyte - idxlen + len;
    lua_pushboolean(L, 1);

    return 1;
}

static int addn_lua(lua_State *L)
{
    lua_iovec_t *iov = lauxh_checkudata(L, 1, IOVEC_MT);
    lua_Integer n    = lauxh_checkinteger(L, 2);
    int top          = lua_gettop(iov->th);
    char *data       = NULL;

    // check argument
    lauxh_argcheck(L, n > 0, 2, "1 or more integer expected, got less than 1");

    if (top >= IOV_MAX) {
        lua_pushboolean(L, 0);
        return 1;
    } else if (!lua_checkstack(iov->th, 1)) {
        lua_pushboolean(L, 0);
        lua_pushinteger(L, 0);
        lua_pushstring(L, strerror(ENOMEM));
        return 3;
    }

    // create data filled with spaces
    data = lua_newuserdata(iov->th, (size_t)n + 1);
    memset(data, ' ', n);
    data[n] = 0;
    iov->nbyte += n;
    lua_pushboolean(L, 1);
    lua_pushinteger(L, top + 1);

    return 2;
}

static int add_lua(lua_State *L)
{
    lua_iovec_t *iov = lauxh_checkudata(L, 1, IOVEC_MT);
    size_t len       = 0;
    const char *str  = lauxh_checklstring(L, 2, &len);
    int top          = lua_gettop(iov->th);
    char *data       = NULL;

    if (len == 0 || top >= IOV_MAX) {
        lua_pushboolean(L, 0);
        return 1;
    } else if (!lua_checkstack(iov->th, 1)) {
        lua_pushboolean(L, 0);
        lua_pushinteger(L, 0);
        lua_pushstring(L, strerror(ENOMEM));
        return 3;
    }

    // create data filled with string argument
    data = lua_newuserdata(iov->th, len + 1);
    memcpy(data, str, len);
    data[len] = 0;
    iov->nbyte += len;
    lua_pushboolean(L, 1);
    lua_pushinteger(L, top + 1);

    return 2;
}

static int bytes_lua(lua_State *L)
{
    lua_iovec_t *iov = lauxh_checkudata(L, 1, IOVEC_MT);
    lua_pushinteger(L, iov->nbyte);
    return 1;
}

static int len_lua(lua_State *L)
{
    lua_iovec_t *iov = lauxh_checkudata(L, 1, IOVEC_MT);
    lua_pushinteger(L, lua_gettop(iov->th));
    return 1;
}

static int tostring_lua(lua_State *L)
{
    lua_pushfstring(L, IOVEC_MT ": %p", lua_touserdata(L, 1));
    return 1;
}

static int gc_lua(lua_State *L)
{
    lua_iovec_t *iov = lua_touserdata(L, 1);
    iov->ref         = lauxh_unref(L, iov->ref);
    return 0;
}

static int new_lua(lua_State *L)
{
    lua_iovec_t *iov = lua_newuserdata(L, sizeof(lua_iovec_t));

    iov->nbyte = 0;
    iov->th    = lua_newthread(L);
    iov->ref   = lauxh_ref(L);
    lauxh_setmetatable(L, IOVEC_MT);

    return 1;
}

LUALIB_API int luaopen_iovec(lua_State *L)
{
    // create metatable
    if (luaL_newmetatable(L, IOVEC_MT)) {
        struct luaL_Reg mmethod[] = {
            {"__gc",       gc_lua      },
            {"__tostring", tostring_lua},
            {"__len",      len_lua     },
            {NULL,         NULL        }
        };
        struct luaL_Reg method[] = {
            {"bytes",   bytes_lua  },
            {"add",     add_lua    },
            {"addn",    addn_lua   },
            {"set",     set_lua    },
            {"get",     get_lua    },
            {"del",     del_lua    },
            {"concat",  concat_lua },
            {"consume", consume_lua},
            {"writev",  writev_lua },
            {"readv",   readv_lua  },
            {NULL,      NULL       }
        };
        struct luaL_Reg *ptr = mmethod;

        // metamethods
        do {
            lauxh_pushfn2tbl(L, ptr->name, ptr->func);
            ptr++;
        } while (ptr->name);
        // methods
        lua_pushstring(L, "__index");
        lua_newtable(L);
        ptr = method;
        do {
            lauxh_pushfn2tbl(L, ptr->name, ptr->func);
            ptr++;
        } while (ptr->name);
        lua_rawset(L, -3);
    }
    lua_pop(L, 1);

    // create module table
    lua_newtable(L);
    lauxh_pushfn2tbl(L, "new", new_lua);
    lauxh_pushnum2tbl(L, "IOV_MAX", IOV_MAX);

    return 1;
}
