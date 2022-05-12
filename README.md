# lua-iovec

[![test](https://github.com/mah0x211/lua-iovec/actions/workflows/test.yml/badge.svg)](https://github.com/mah0x211/lua-iovec/actions/workflows/test.yml)
[![codecov](https://codecov.io/gh/mah0x211/lua-iovec/branch/master/graph/badge.svg)](https://codecov.io/gh/mah0x211/lua-iovec)

Vectored I/O module

**NOTE: Do not use this module. this module is under heavy development.**


## Installation

```bash
$ luarocks install iovec
```

this module install the `lua_iovec.h` to `CONFDIR` and creates a symbolic link in `LUA_INCDIR`.


## Constants

- `iovec.IOV_MAX`: maximum size of an iovec.


## Error Handling

the following functions return the `error` object created by https://github.com/mah0x211/lua-errno module.


## iov = iovec.new()

create an instance of iovec.

**NOTE:** iovec can hold maximum [IOV_MAX](#constants) elements.

**Returns**

- `iov:iovec`: instance of [iovec](#iovec-instance-methods).


## nbyte = iov:bytes()

get a number of bytes used.

**Returns**

- `nbyte:integer`: number of bytes used.


## idx, err = iov:add( str )

add an element with specified string.

**Parameters**

- `str:string`: string.

**Returns**

- `idx:integer`: positive index number of added element. or the following negative index number;
  - `-1`: no buffer space available
  - `-2`: stack memory cannot be increased
  - `-3`: empty string cannot be added
- `err:error`: error object.


## idx, err = iov:addn( nbyte )

add an element that size of specified number of bytes.

**Parameters**

- `nbyte:integer`: number of bytes.

**Returns**

- `idx:integer`: positive index number of added element. or the following negative index number;
  - `-1`: no buffer space available
  - `-2`: stack memory cannot be increased
- `err:error`: error object.


## ok, err = iov:set( str, idx )

replace a string of element at specified index.

**Parameters**

- `str:string`: string.
- `idx:integer`: index of element.

**Returns**

- `ok:boolean`: `true` on success, `false` on the specified index does not exist.
- `err:object`: error object if stack memory cannot be increased.


## str = iov:get( idx )

get a string of element at specified index.

**Parameters**
- `idx:integer`: index of element.

**Returns**

- `str:string`: string of element.


## str = iov:del( idx )

delete an element at specified index.

**Parameters**

- `idx:integer`: index of element.

**Returns**

- `str:string`: string of deleted element.


## str, err = iov:concat( [offset, [, nbyte]])

concatenate all data of elements in use into a string.

**Parameters**

- `offset:integer`: where to begin in the data.
- `nbyte:integer`: number of bytes concat.

**Returns**

- `str:string`: string.
- `err:error`: error object if stack memory cannot be increased.


## nbyte = iov:consume( nbyte )

delete the specified number of bytes of data.

**Parameters**

- `nbyte:integer`: number of bytes.

**Returns**

- `nbyte:integer`: number of bytes used.


## n, err = iov:writev( fd [, offset, [, nbyte]] )

write iovec messages at once to fd.

**Parameters**

- `fd:integer`: file descriptor.
- `offset:integer`: start position of the data to be written.
- `nbyte:integer`: number of bytes to write.

**Returns**

- `n:integer`: the number of bytes written.
- `err:error`: error object.


## n, err = iov:readv( fd [, offset, [, nbyte]] )

read the messages from fd into iovec.

**Parameters**

- `fd:integer`: file descriptor.
- `offset:integer`: insertion position of read data.
- `nbyte:integer`: maximum number of bytes to be read.

**Returns**

- `n:integer`: the number of bytes read.
- `err:error`: error object.

**NOTE:** all return values will be `nil` if `readv` is returned `0`.


## Use from C module

the `iovec` module installs `lua_iovec.h` in the lua include directory.

the following API can be used to create an iovec object.


### void lua_iovec_loadlib( lua_State *L, int level )

load the lua-iovec module.  

**NOTE:** you must call this API at least once before using the following API.


### int lua_iovec_new( lua_State *L )

create a new `iovec` object on the stack that equivalent to `iovec.new()` function.


### size_t lua_iovec_setv( lua_iovec_t *iov, struct iovec *vec, int *nvec, size_t offset, size_t nbyte )

sets the buffers that held by `*iov` to `*vec` and returns the number of bytes set.

- `*nvec` must be set to length of `*vec`, and if it is `NULL` or `0`, it will return `0` without changing anything.
- `offset` specifies the starting position of the buffer. if it is greater than or equal to the number of bytes held by `*iov`, it returns `0` without changing anything.
- `nbyte` specifies the number of bytes to be set for `*vec`. if it is `0`, it set all data.

