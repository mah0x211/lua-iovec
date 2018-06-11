# lua-iovec

Vectored I/O module

**NOTE: Do not use this module.  this module is under heavy development.**


## Dependencies

- luarocks-fetch-gitrec: <https://github.com/siffiejoe/luarocks-fetch-gitrec>
- lauxhlib: <https://github.com/mah0x211/lauxhlib>


## Installation

```bash
$ luarocks install iovec --from=http://mah0x211.github.io/rocks/
```

## Constants

- `iovec.IOV_MAX`: maximum size of an iovec.


## Functions

### iov, err = iovec.new( [nvec] )

create an instance of iovec.

**NOTE:** iovec can hold maximum [IOV_MAX](#constants) elements.

- **Parameters**
    - `nvec:number`: number of vector.
- **Returns**
    - `iov:iovec`: instance of [iovec](#iovec-instance-methods).
    - `err:string`: error string.


## iovec Instance Methods

`iovec` instance has following methods.

### bytes = iov:bytes()

get a number of bytes used.

- **Returns**
    - `bytes:number`: number of bytes used.


### bytes = iov:consume( bytes )

delete the specified number of bytes of data.

- **Parameters**
    - `bytes:number`: number of bytes.
- **Returns**
    - `bytes:number`: number of bytes used.


### str = iov:concat()

concatenate all data of elements in use into a string.

- **Returns**
    - `str:string`: string.


### idx, err = iov:add( str )

add an element with specified string.

- **Parameters**
    - `str:string`: string.
- **Returns**
    - `idx:number`: index number of added element.
    - `err:string`: error string.


### idx, err = iov:addn( bytes )

add an element that size of specified number of bytes.

- **Parameters**
    - `bytes:number`: number of bytes.
- **Returns**
    - `idx:number`: index number of added element.
    - `err:string`: error string.


### str = iov:get( idx )

get a string of element at specified index.

- **Parameters**
    - `idx:number`: index of element.
- **Returns**
    - `str:string`: string of element.


### ok = iov:set( str, idx )

replace a string of element at specified index.

- **Parameters**
    - `str:string`: string.
    - `idx:number`: index of element.
- **Returns**
    - `ok:boolean`: if the specified index does not exist, it returns `false`.


### str, midx = iov:del( idx )

delete an element at specified index.

- **Parameters**
    - `idx:number`: index of element.
- **Returns**
    - `str:string`: string of deleted element.
    - `midx:number`: index of moved element.



### len, err, again = iov:writev( fd [, offset] )

write iovec messages at once to fd.

- **Parameters**
    - `fd:number`: file descriptor.
    - `offset:numbger`: offset at which the output operation is to be performed.
- **Returns**
    - `len:number`: the number of bytes sent.
    - `err:string`: error string.
    - `again:bool`: true if all data has not been sent.

**NOTE:** all return values will be nil if closed by peer.

