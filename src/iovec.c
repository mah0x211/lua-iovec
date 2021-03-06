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
 *  iovec.c
 *  lua-iovec
 *
 *  Created by Masatoshi Teruya on 18/06/11.
 */

#include "lua_iovec.h"


static int writev_lua( lua_State *L )
{
    lua_iovec_t *iov = lauxh_checkudata( L, 1, IOVEC_MT );
    int fd = lauxh_checkinteger( L, 2 );
    uint64_t offset = lauxh_optuint64( L, 3, 0 );
    size_t len = iov->bytes - offset;
    ssize_t rv = 0;

    if( offset < iov->bytes )
    {
        struct iovec *vec = iov->data;
        struct iovec src = *vec;
        int nvec = iov->used;

        if( offset )
        {
            int idx = 0;

            for(; idx < iov->used; idx++ )
            {
                if( vec[0].iov_len > offset ){
                    src = vec[0];
                    vec[0].iov_base += offset;
                    vec[0].iov_len -= offset;
                    break;
                }
                offset -= vec[0].iov_len;
                vec++;
                nvec--;
            }
        }

        rv = writev( fd, vec, nvec );
        *vec = src;
    }
    else {
        struct iovec empty_iov = {
            .iov_base = NULL,
            .iov_len = 0
        };

        len = 0;
        rv = writev( fd, &empty_iov, 1 );
    }

    switch( rv )
    {
        // got error
        case -1:
            // again
            if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ){
                lua_pushinteger( L, 0 );
                lua_pushnil( L );
                lua_pushboolean( L, 1 );
                return 3;
            }
            // closed by peer
            else if( errno == EPIPE || errno == ECONNRESET ){
                return 0;
            }
            // got error
            lua_pushnil( L );
            lua_pushstring( L, strerror( errno ) );
            return 2;

        default:
            lua_pushinteger( L, rv );
            lua_pushnil( L );
            lua_pushboolean( L, len - (size_t)rv );
            return 3;
    }
}



static int del_lua( lua_State *L )
{
    lua_iovec_t *iov = lauxh_checkudata( L, 1, IOVEC_MT );
    lua_Integer idx = lauxh_checkinteger( L, 2 );

    if( idx < iov->used && idx >= 0 )
    {
        lauxh_pushref( L, iov->refs[idx] );
        lauxh_unref( L, iov->refs[idx] );
        iov->bytes -= iov->lens[idx];
        iov->used--;

        // swap index
        // used: 6
        //  del: i4: -> 0, 1, 2, 3, '4', 5
        //           -> 0, 1, 2, 3, [5]
        // used: 5
        //  del: i2: -> 0, 1, '2', 3, 5
        //           -> 0, 1, [5], 3
        // used: 4
        //  del: i0: -> '0', 1, 5, 3
        //           -> [3], 1, 5
        // used: 3
        //  del: i2: -> 3, 1, '5'
        //           -> 3, 1
        if( iov->used != idx ){
            lua_pushinteger( L, iov->used );
            // fill holes in array by last data
            iov->refs[idx] = iov->refs[iov->used];
            iov->lens[idx] = iov->lens[iov->used];
            iov->data[idx] = iov->data[iov->used];
            return 2;
        }
    }
    else {
        lua_pushnil( L );
    }

    return 1;
}


static inline int pushstr( lua_State *L, lua_iovec_t *iov, int idx )
{
    // copy string if actual length is not equal to allocated size
    if( iov->data[idx].iov_len - iov->lens[idx] ){
        lua_pushlstring( L, iov->data[idx].iov_base, iov->lens[idx] );
        return 1;
    }

    lauxh_pushref( L, iov->refs[idx] );
    return 0;
}


static int get_lua( lua_State *L )
{
    lua_iovec_t *iov = lauxh_checkudata( L, 1, IOVEC_MT );
    lua_Integer idx = lauxh_checkinteger( L, 2 );

    if( idx >= 0 && idx < iov->used ){
        pushstr( L, iov, idx );
    }
    else {
        lua_pushnil( L );
    }

    return 1;
}


static int set_lua( lua_State *L )
{
    lua_iovec_t *iov = lauxh_checkudata( L, 1, IOVEC_MT );
    size_t len = 0;
    const char *str = lauxh_checklstring( L, 2, &len );
    lua_Integer idx = lauxh_checkinteger( L, 3 );

    if( idx >= 0 && idx <= iov->used )
    {
        // release current string
        iov->bytes -= iov->lens[idx];
        lauxh_unref( L, iov->refs[idx] );

        // maintain string
        lua_settop( L, 2 );
        iov->refs[idx] = lauxh_ref( L );
        iov->lens[idx] = len;
        iov->data[idx] = (struct iovec){
            .iov_base = (void*)str,
            .iov_len = len
        };
        iov->bytes += len;
        lua_pushboolean( L, 1 );
    }
    // not found
    else {
        lua_pushboolean( L, 0 );
    }

    return 1;
}


static inline int addstr_lua( lua_State *L, lua_iovec_t *iov )
{
    size_t len = 0;
    const char *str = lauxh_checklstring( L, 2, &len );
    int used = iov->used + 1;

    lua_settop( L, 2 );
    if( used > IOV_MAX ){
        errno = ENOBUFS;
        lua_pushnil( L );
        lua_pushstring( L, strerror( errno ) );
        return 2;
    }
    else if( iov->nvec < used )
    {
        // increase vec
        void *new = realloc( (void*)iov->data, sizeof( struct iovec ) * used );

        if( !new ){
            lua_pushnil( L );
            lua_pushstring( L, strerror( errno ) );
            return 2;
        }
        iov->data = (struct iovec*)new;

        // increase refs
        new = realloc( (void*)iov->refs, sizeof( int ) * used );
        if( !new ){
            lua_pushnil( L );
            lua_pushstring( L, strerror( errno ) );
            return 2;
        }
        iov->refs = (int*)new;

        // increase lens
        new = realloc( (void*)iov->lens, sizeof( size_t ) * used );
        if( !new ){
            lua_pushnil( L );
            lua_pushstring( L, strerror( errno ) );
            return 2;
        }
        iov->lens = (size_t*)new;

        // update size of vector
        iov->nvec = used;
    }

    // maintain string
    iov->refs[iov->used] = lauxh_ref( L );
    iov->lens[iov->used] = len;
    iov->data[iov->used] = (struct iovec){
        .iov_base = (void*)str,
        .iov_len = len
    };
    lua_pushinteger( L, iov->used );

    iov->used = used;
    iov->bytes += len;

    return 1;
}


static int addn_lua( lua_State *L )
{
    lua_iovec_t *iov = lauxh_checkudata( L, 1, IOVEC_MT );
    lua_Integer n = lauxh_checkinteger( L, 2 );

    // check argument
    lauxh_argcheck(
        L, n > 0, 2, "1 or more integer expected, got less than 1"
    );

    lua_settop( L, 1 );
    // create buffer
    lauxh_pushbuffer( L, n );

    return addstr_lua( L, iov );
}


static int add_lua( lua_State *L )
{
    lua_iovec_t *iov = lauxh_checkudata( L, 1, IOVEC_MT );

    return addstr_lua( L, iov );
}


static int concat_lua( lua_State *L )
{
    lua_iovec_t *iov = lauxh_checkudata( L, 1, IOVEC_MT );

    lua_settop( L, 0 );
    if( iov->used > 0 )
    {
        int used = iov->used;
        int i = 0;

        // push actual used values
        while( i < used && pushstr( L, iov, i++ ) == 0 ){}
        lua_concat( L, i );
    }
    else {
        lua_pushstring( L, "" );
    }

    return 1;
}


static int consume_lua( lua_State *L )
{
    lua_iovec_t *iov = lauxh_checkudata( L, 1, IOVEC_MT );
    lua_Integer n = lauxh_checkinteger( L, 2 );

    // check argument
    lauxh_argcheck(
        L, n >= 0, 2, "unsigned integer expected, got signed integer"
    );

    if( (size_t)n >= iov->bytes )
    {
        int *refs = iov->refs;
        int used = iov->used;
        int i = 0;

        for(; i < used; i++ ){
            lauxh_unref( L, refs[i] );
        }
        iov->used = 0;
        iov->bytes = 0;
    }
    else if( n )
    {
        struct iovec *data = iov->data;
        int *refs = iov->refs;
        size_t *lens = iov->lens;
        int used = iov->used;
        int head = 0;
        size_t len = 0;

        iov->bytes -= n;

        while( n > 0 )
        {
            len = lens[head];
            // update the last data block
            if( len > (size_t)n ){
                char *ptr = data[head].iov_base;

                len -= n;
                memmove( ptr, ptr + n, len );
                ptr[len] = 0;
                lens[head] = len;
                break;
            }

            // remove refenrece
            n -= lens[head];
            lauxh_unref( L, refs[head] );
            head++;
        }

        iov->used = used - head;
        // update references
        if( head && head != used )
        {
            int i = 0;

            for(; head < used; head++ ){
                data[i] = data[head];
                refs[i] = refs[head];
                lens[i] = lens[head];
                i++;
            }
        }
    }

    lua_pushinteger( L, iov->bytes );
    return 1;
}


static int bytes_lua( lua_State *L )
{
    lua_iovec_t *iov = lauxh_checkudata( L, 1, IOVEC_MT );

    lua_pushinteger( L, iov->bytes );

    return 1;
}


static int len_lua( lua_State *L )
{
    lua_iovec_t *iov = lauxh_checkudata( L, 1, IOVEC_MT );

    lua_pushinteger( L, iov->used );

    return 1;
}


static int tostring_lua( lua_State *L )
{
    lua_pushfstring( L, IOVEC_MT ": %p", lua_touserdata( L, 1 ) );
    return 1;
}


static int gc_lua( lua_State *L )
{
    lua_iovec_t *iov = lauxh_checkudata( L, 1, IOVEC_MT );
    int *refs = iov->refs;
    int used = iov->used;
    int i = 0;

    free( iov->lens );
    free( iov->data );
    for(; i < used; i++ ){
        lauxh_unref( L, refs[i] );
    }
    free( refs );

    return 0;
}


static int new_lua( lua_State *L )
{
    int top = lua_gettop( L );
    lua_Integer nvec = lauxh_optinteger( L, 1, 0 );

    if( nvec > IOV_MAX ){
        errno = EOVERFLOW;
    }
    else
    {
        lua_iovec_t *iov = lua_newuserdata( L, sizeof( lua_iovec_t ) );

        if( nvec < 0 ){
            nvec = 0;
        }

        iov->data = malloc( sizeof( struct iovec ) * nvec );
        if( iov->data )
        {
            if( ( iov->refs = (int*)malloc( sizeof( int ) * nvec ) ) &&
                ( iov->lens = (size_t*)malloc( sizeof( size_t ) * nvec ) ) ){
                iov->used = 0;
                iov->nvec = nvec;
                iov->bytes = 0;
                lauxh_setmetatable( L, IOVEC_MT );
                return 1;
            }
            else if( iov->refs ){
                free( (void*)iov->refs );
            }
            free( (void*)iov->data );
        }

        lua_settop( L, top );
    }

    // got error
    lua_pushnil( L );
    lua_pushstring( L, strerror( errno ) );

    return 2;
}


LUALIB_API int luaopen_iovec( lua_State *L )
{
    // create metatable
    if( luaL_newmetatable( L, IOVEC_MT ) )
    {
        struct luaL_Reg mmethod[] = {
            { "__gc", gc_lua },
            { "__tostring", tostring_lua },
            { "__len", len_lua },
            { NULL, NULL }
        };
        struct luaL_Reg method[] = {
            { "bytes", bytes_lua },
            { "consume", consume_lua },
            { "concat", concat_lua },
            { "add", add_lua },
            { "addn", addn_lua },
            { "set", set_lua },
            { "get", get_lua },
            { "del", del_lua },
            { "writev", writev_lua },
            { NULL, NULL }
        };
        struct luaL_Reg *ptr = mmethod;

        // metamethods
        do {
            lauxh_pushfn2tbl( L, ptr->name, ptr->func );
            ptr++;
        } while( ptr->name );
        // methods
        lua_pushstring( L, "__index" );
        lua_newtable( L );
        ptr = method;
        do {
            lauxh_pushfn2tbl( L, ptr->name, ptr->func );
            ptr++;
        } while( ptr->name );
        lua_rawset( L, -3 );
    }
    lua_pop( L, 1 );

    // create module table
    lua_newtable( L );
    lauxh_pushfn2tbl( L, "new", new_lua );
    lauxh_pushnum2tbl( L, "IOV_MAX", IOV_MAX );

    return 1;
}

