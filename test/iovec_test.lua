-- you must install 'mah0x11/nosigpipe' and 'mah0x211/pipe' modules
require('nosigpipe')
local pipe = require('pipe')
local iovec = require('iovec')
local testcase = require('testcase')
local assert = require('assertex')

function testcase.new()
    -- test that returns an instance of iovec
    local v = iovec.new()
    assert(v ~= nil)
    assert.match(tostring(v), '^iovec: ', false)
end

function testcase.iovec_add()
    local v = iovec.new()

    -- test that add the string 'foo'
    local ok, idx = v:add('foo')
    assert.is_true(ok)
    assert.equal(idx, 1)
    assert.equal(#v, 1)
    assert.equal(v:bytes(), 3)
    assert.equal(v:get(idx), 'foo')

    -- test that add the string 'bar'
    ok, idx = v:add('bar')
    assert.is_true(ok)
    assert.equal(idx, 2)
    assert.equal(#v, 2)
    assert.equal(v:bytes(), 6)
    assert.equal(v:get(idx), 'bar')

    -- test that returns false if argument is empty-string
    ok, idx = v:add('')
    assert.is_false(ok)
    assert.is_nil(idx)
    assert.equal(#v, 2)
    assert.equal(v:bytes(), 6)

    -- test that error occurs with non-string argument
    local err = assert.throws(function()
        v:add(1)
    end)
    assert.match(err, 'string expected, ')

    -- test that returns false if no buffer space available
    local nbyte = v:bytes()
    for _ = 1, iovec.IOV_MAX do
        ok = v:add('a')
        if ok then
            nbyte = nbyte + 1
        else
            assert.equal(#v, iovec.IOV_MAX)
            assert.equal(v:bytes(), nbyte)
        end
    end
end

function testcase.iovec_addn()
    local v = iovec.new()

    -- test that add 3-byte buffer string
    local ok, idx = v:addn(3)
    assert.is_true(ok)
    assert.equal(idx, 1)
    assert.equal(#v, 1)
    assert.equal(v:bytes(), 3)
    assert.equal(#v:get(idx), 3)

    -- test that add 9-byte buffer string
    ok, idx = v:addn(9)
    assert.is_true(ok)
    assert.equal(idx, 2)
    assert.equal(#v, 2)
    assert.equal(v:bytes(), 12)
    assert.equal(#v:get(idx), 9)

    -- test that error occurs with n<=0
    local err = assert.throws(function()
        v:addn(0)
    end)
    assert.match(err, 'got less than 1')

    -- test that error occurs with non-number argument
    err = assert.throws(function()
        v:addn('foo')
    end)
    assert.match(err, 'number expected, ')

    -- test that returns false if no buffer space available
    local nbyte = v:bytes()
    for _ = 1, iovec.IOV_MAX do
        ok = v:addn(1)
        if ok then
            nbyte = nbyte + 1
        else
            assert.equal(#v, iovec.IOV_MAX)
            assert.equal(v:bytes(), nbyte)
        end
    end
end

function testcase.iovec_set()
    local v = iovec.new()
    for _, s in ipairs({
        'foo',
        'bar',
        'baz',
    }) do
        local ok, _, err = v:add(s)
        assert(ok, err)
    end

    -- test that replace existing value with new value
    local ok = v:set("replace", 2)
    assert.is_true(ok)
    assert.equal(#v, 3)
    assert.equal(v:bytes(), 13)
    assert.equal(v:get(2), 'replace')

    -- test that returns false if index is out of range
    for _, i in ipairs({
        -1,
        0,
        4,
    }) do
        ok = v:set('replace', i)
        assert.is_false(ok)
        assert.equal(#v, 3)
        assert.equal(v:bytes(), 13)
    end

    -- test that error occurs with non-string argument
    local err = assert.throws(function()
        v:set(1, 1)
    end)
    assert.match(err, 'string expected, ')
    assert.equal(#v, 3)
    assert.equal(v:bytes(), 13)
end

function testcase.iovec_get()
    local v = iovec.new()
    for _, s in ipairs({
        'foo',
        'bar',
        'baz',
    }) do
        local ok, idx, err = v:add(s)
        assert(ok, err)

        -- test that returns a value
        assert.equal(v:get(idx), s)
    end

    -- test that returns nil if index is out of range
    for _, i in ipairs({
        -1,
        0,
        4,
    }) do
        assert.is_nil(v:get(i))
    end

    -- test that error occurs with non-number argument
    local err = assert.throws(function()
        v:get('foo')
    end)
    assert.match(err, 'number expected, ')
end

function testcase.iovec_del()
    local v = iovec.new()
    for _, s in ipairs({
        'foo',
        'bar',
        'baz',
    }) do
        local ok, _, err = v:add(s)
        assert(ok, err)
    end

    -- test that returns nil if index is out of range
    for _, i in ipairs({
        -1,
        0,
        4,
    }) do
        assert.is_nil(v:get(i))
    end

    -- test that returns the value of deleted index
    assert.equal(v:del(2), 'bar')
    assert.equal(#v, 2)
    assert.equal(v:bytes(), 6)

    assert.equal(v:del(1), 'foo')
    assert.equal(#v, 1)
    assert.equal(v:bytes(), 3)

    assert.equal(v:del(1), 'baz')
    assert.equal(#v, 0)
    assert.equal(v:bytes(), 0)

    -- test that error occurs with non-number argument
    local err = assert.throws(function()
        v:del('foo')
    end)
    assert.match(err, 'number expected, ')
end

function testcase.iovec_concat()
    local v = iovec.new()

    -- test that returns empty-string
    assert.equal(v:concat(), '')

    -- test that returns 'foobarbaz'
    for _, s in ipairs({
        'foo',
        'bar',
        'baz',
    }) do
        local ok, _, err = v:add(s)
        assert(ok, err)
    end
    assert.equal(v:concat(), 'foobarbaz')

    -- test that returns 'foob'
    assert.equal(v:concat(0, 4), 'foob')

    -- test that returns 'arbaz'
    assert.equal(v:concat(4), 'arbaz')

    -- test that returns 'rba'
    assert.equal(v:concat(5, 3), 'rba')
end

function testcase.iovec_consume()
    local v = iovec.new()
    for _, s in ipairs({
        'foo',
        'bar',
        'baz',
    }) do
        local ok, _, err = v:add(s)
        assert(ok, err)
    end

    -- test that consume 0-byte
    assert.equal(v:consume(0), 9)
    assert.equal(v:consume(-1), 9)

    -- test that consume 5-byte
    assert.equal(v:consume(5), 4)
    assert.equal(#v, 2)
    assert.equal(v:get(1), 'r')
    assert.equal(v:get(2), 'baz')

    -- test that consume 100-byte
    assert.equal(v:consume(100), 0)
    assert.equal(#v, 0)

    -- test that error occurs with non-number argument
    local err = assert.throws(function()
        v:consume('foo')
    end)
    assert.match(err, 'number expected, ')
end

function testcase.iovec_writev()
    local v = iovec.new()
    for _, s in ipairs({
        'foo',
        'bar',
        'baz',
    }) do
        local ok, _, err = v:add(s)
        assert(ok, err)
    end

    local r, w, err = pipe()
    assert(not err, err)

    -- luacheck: ignore err
    -- test that writes all data to fd
    local n, err = v:writev(w:fd())
    assert.equal(n, 9)
    assert(not err, err)
    local s, err = r:read()
    assert.equal(s, 'foobarbaz')
    assert(not err, err)

    -- test that writes data after first 5-byte to fd
    n, err = v:writev(w:fd(), 5)
    assert.equal(n, 4)
    assert(not err, err)
    s, err = r:read()
    assert.equal(s, 'rbaz')
    assert(not err, err)

    -- test that writes data after first 5-byte limit 1-byte to fd
    n, err = v:writev(w:fd(), 5, 1)
    assert.equal(n, 1)
    assert(not err, err)
    s, err = r:read()
    assert.equal(s, 'r')
    assert(not err, err)

    -- test that writes data limit 4-byte to fd
    n, err = v:writev(w:fd(), nil, 4)
    assert.equal(n, 4)
    assert(not err, err)
    s, err = r:read()
    assert.equal(s, 'foob')
    assert(not err, err)

    -- test that returns again=true if the data written reaches to
    -- maximum buffer size
    v:consume(10)
    v:addn(4096)
    w:nonblock(true)
    local total = 0
    while true do
        -- luacheck: ignore n
        local n, err, again = v:writev(w:fd())
        if again then
            assert(not err, err)
            break
        end
        assert.equal(n, 4096)
        assert(not err, err)
        total = total + n
    end
    assert(w:nonblock(false))

    -- test that returns nil if closed by peer
    r:close()
    local res = {
        v:writev(w:fd()),
    }
    assert.equal(#res, 0)

    -- test that returns error
    w:close()
    n, err = v:writev(w:fd())
    assert.is_nil(n)
    assert(err, 'writev() did not returns err')

    -- test that error occurs with non-number fd argument
    err = assert.throws(function()
        v:writev('foo')
    end)
    assert.match(err, 'number expected, ')

    -- test that error occurs with non-number offset argument
    err = assert.throws(function()
        v:writev(123, 'foo')
    end)
    assert.match(err, 'number expected, ')

    -- test that error occurs with non-number nbyte argument
    err = assert.throws(function()
        v:writev(123, 0, 'foo')
    end)
    assert.match(err, 'number expected, ')
end

function testcase.iovec_readv()
    local v = iovec.new()
    for _, s in ipairs({
        'foo',
        'bar',
        'baz',
        'qux',
    }) do
        local ok, _, err = v:addn(#s)
        assert(ok, err)
    end

    local r, w, err = pipe()
    assert(not err, err)
    r:nonblock(true)

    -- luacheck: ignore n
    -- luacheck: ignore err
    -- test that reads data from fd into buffer
    local n = assert(w:write('hello world!'))
    assert.equal(v:readv(r:fd()), 12)
    assert.equal(v:concat(0, n), 'hello world!')

    -- test that reads data at offset of 7-byte
    assert(w:write('hello world!'))
    assert.equal(v:readv(r:fd(), 7), v:bytes() - 7)
    assert.equal(v:concat(7), 'hello')
    r:read()

    -- test that reads data at offset of 3-byte limit 4-byte
    assert(w:write('hello world!'))
    assert.equal(assert(v:readv(r:fd(), 3, 4)), 4)
    assert(not err)
    assert(v:concat(3, 4) == 'hell')
    r:read()

    -- test that returns nil if closed by peer
    w:close()
    assert.empty({
        v:readv(r:fd()),
    })

    -- test that error occurs with non-number fd argument
    err = assert.throws(function()
        v:readv('foo')
    end)
    assert.match(err, 'number expected, ')

    -- test that error occurs with non-number offset argument
    err = assert.throws(function()
        v:readv(123, 'foo')
    end)
    assert.match(err, 'number expected, ')

    -- test that error occurs with non-number nbyte argument
    err = assert.throws(function()
        v:readv(123, 0, 'foo')
    end)
    assert.match(err, 'number expected, ')
end

