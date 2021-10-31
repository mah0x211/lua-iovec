# lua-iovec

[![test](https://github.com/mah0x211/lua-iovec/actions/workflows/test.yml/badge.svg)](https://github.com/mah0x211/lua-iovec/actions/workflows/test.yml)

Vectored I/O module

**NOTE: Do not use this module. this module is under heavy development.**


## Dependencies

- lauxhlib: <https://github.com/mah0x211/lauxhlib>


## Installation

```bash
$ luarocks install iovec --from=http://mah0x211.github.io/rocks/
```

## Constants

- `iovec.IOV_MAX`: maximum size of an iovec.


## Functions

### iov = iovec.new()

create an instance of iovec.

**NOTE:** iovec can hold maximum [IOV_MAX](#constants) elements.

**Returns**

- `iov:iovec`: instance of [iovec](#iovec-instance-methods).


## iovec Instance Methods

`iovec` instance has following methods.


### nbyte = iov:bytes()

get a number of bytes used.

**Returns**

- `nbyte:integer`: number of bytes used.


### idx, err = iov:add( str )

add an element with specified string.

**Parameters**

- `str:string`: string.

**Returns**

- `idx:integer`: positive index number of added element. or the following negative index number;
  - `-1`: no buffer space available
  - `-2`: stack memory cannot be increased
  - `-3`: empty string cannot be added
- `err:string`: error string.


### idx, err = iov:addn( nbyte )

add an element that size of specified number of bytes.

**Parameters**

- `nbyte:integer`: number of bytes.

**Returns**

- `idx:integer`: positive index number of added element. or the following negative index number;
  - `-1`: no buffer space available
  - `-2`: stack memory cannot be increased
- `err:string`: error string


### ok, err = iov:set( str, idx )

replace a string of element at specified index.

**Parameters**

- `str:string`: string.
- `idx:integer`: index of element.

**Returns**

- `ok:boolean`: `true` on success, `false` on the specified index does not exist.
- `err:string`: error string if stack memory cannot be increased.


### str = iov:get( idx )

get a string of element at specified index.

**Parameters**
- `idx:integer`: index of element.

**Returns**

- `str:string`: string of element.


### str, err = iov:del( idx )

delete an element at specified index.

**Parameters**

- `idx:integer`: index of element.

**Returns**

- `str:string`: string of deleted element.


### str, err = iov:concat( [offset, [, nbyte]])

concatenate all data of elements in use into a string.

**Parameters**

- `offset:integer`: where to begin in the data.
- `nbyte:integer`: number of bytes concat.

**Returns**

- `str:string`: string.
- `err:string`: error string if stack memory cannot be increased.


### nbyte = iov:consume( nbyte )

delete the specified number of bytes of data.

**Parameters**

- `nbyte:integer`: number of bytes.

**Returns**

- `nbyte:integer`: number of bytes used.


### nbyte, err, again = iov:writev( fd [, offset, [, nbyte]] )

write iovec messages at once to fd.

**Parameters**

- `fd:integer`: file descriptor.
- `offset:integer`: start position of the data to be sent.
- `nbyte:integer`: number of bytes to send.

**Returns**

- `nbyte:integer`: the number of bytes sent.
- `err:string`: error string.
- `again:bool`: `true` if all data has not been sent.

**NOTE:** all return values will be nil if closed by peer.


### nbyte, err, again = iov:readv( fd [, offset, [, nbyte]] )

read the messages from fd into iovec.

**Parameters**

- `fd:integer`: file descriptor.
- `offset:integer`: insertion position of received data.
- `nbyte:integer`: maximum number of bytes to be received.

**Returns**

- `nbyte:integer`: the number of bytes received.
- `err:string`: error string.
- `again:bool`: `true` if all data has not been sent.

**NOTE:** all return values will be nil if closed by peer.
