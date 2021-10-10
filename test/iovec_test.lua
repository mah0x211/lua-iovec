-- you must install 'mah0x11/nosigpipe' and 'mah0x211/pipe' modules
require('nosigpipe')
local pipe = require('pipe')
local iovec = require('iovec')


local function test_new()
    -- test that returns an instance of iovec
    local v = iovec.new()
    assert(v ~= nil)
    assert(string.match(tostring(v), '^iovec: '))
end


local function test_iovec_add()
    local v = iovec.new()

    -- test that add the string 'foo'
    local ok, idx = v:add('foo')
    assert(ok)
    assert(idx == 1)
    assert(#v == 1)
    assert(v:bytes() == 3)
    assert('foo' == v:get(idx))

    -- test that add the string 'bar'
    ok, idx = v:add('bar')
    assert(ok)
    assert(idx == 2)
    assert(#v == 2)
    assert(v:bytes() == 6)
    assert(v:get(idx) == 'bar')

    -- test that returns false if argument is empty-string
    ok, idx = v:add('')
    assert(not ok)
    assert(not idx)
    assert(#v == 2)
    assert(v:bytes() == 6)

    -- test that error occurs with non-string argument
    local ok, err = pcall(function()
        v:add(1)
    end)
    assert(not ok)
    assert(string.match(err, 'string expected, '))

    -- test that returns false if no buffer space available
    local nbyte = v:bytes()
    for i = 1, iovec.IOV_MAX do
        ok = v:add('a')
        if ok then
            nbyte = nbyte + 1
        else
            assert(#v == iovec.IOV_MAX)
            assert(v:bytes() == nbyte)
        end
    end
end


local function test_iovec_addn()
    local v = iovec.new()

    -- test that add 3-byte buffer string
    local ok, idx = v:addn(3)
    assert(ok)
    assert(idx == 1)
    assert(#v == 1)
    assert(v:bytes() == 3)
    assert(#v:get(idx) == 3)

    -- test that add 9-byte buffer string
    ok, idx = v:addn(9)
    assert(ok)
    assert(idx == 2)
    assert(#v == 2)
    assert(v:bytes() == 12)
    assert(#v:get(idx) == 9)

    -- test that error occurs with n<=0
    local ok, err = pcall(function()
        v:addn(0)
    end)
    assert(not ok)
    assert(string.match(err, 'got less than 1'))

    -- test that error occurs with non-number argument
    local ok, err = pcall(function()
        v:addn('foo')
    end)
    assert(not ok)
    assert(string.match(err, 'number expected, '))

    -- test that returns false if no buffer space available
    local nbyte = v:bytes()
    for i = 1, iovec.IOV_MAX do
        local ok = v:addn(1)
        if ok then
            nbyte = nbyte + 1
        else
            assert(#v == iovec.IOV_MAX)
            assert(v:bytes() == nbyte)
        end
    end
end


local function test_iovec_set()
    local v = iovec.new()

    for i, s in ipairs({'foo', 'bar', 'baz'}) do
        local ok, idx = v:add(s)
        assert(ok)
        assert(idx == i)
    end

    -- test that replace existing value with new value
    local ok = v:set("replace", 2)
    assert(ok)
    assert(#v == 3)
    assert(v:bytes() == 13)
    assert(v:get(2) == 'replace')

    -- test that returns false if index is out of range
    for _, i in ipairs({-1, 0, 4}) do
        local ok = v:set('replace', i)
        assert(not ok)
        assert(#v == 3)
        assert(v:bytes() == 13)
    end

    -- test that error occurs with non-string argument
    local ok, err = pcall(function()
        v:set(1, 1)
    end)
    assert(not ok)
    assert(string.match(err, 'string expected, '))
    assert(#v == 3)
    assert(v:bytes() == 13)
end


local function test_iovec_get()
    local v = iovec.new()

    for i, s in ipairs({'foo', 'bar', 'baz'}) do
        local ok, idx = v:add(s)
        assert(ok)
        assert(idx == i)

        -- test that returns a value
        assert(v:get(idx) == s)
    end

    -- test that returns nil if index is out of range
    for _, i in ipairs({-1, 0, 4}) do
        assert(not v:get(i))
    end

    -- test that error occurs with non-number argument
    local ok, err = pcall(function()
        v:get('foo')
    end)
    assert(not ok)
    assert(string.match(err, 'number expected, '))
end


local function test_iovec_del()
    local v = iovec.new()

    for i, s in ipairs({'foo', 'bar', 'baz'}) do
        local ok, idx = v:add(s)
        assert(ok)
        assert(idx == i)
    end

    -- test that returns nil if index is out of range
    for _, i in ipairs({-1, 0, 4}) do
        assert(not v:get(i))
    end

    -- test that returns the value of deleted index
    assert(v:del(2) == 'bar')
    assert(#v == 2)
    assert(v:bytes() == 6)

    assert(v:del(1) == 'foo')
    assert(#v == 1)
    assert(v:bytes() == 3)

    assert(v:del(1) == 'baz')
    assert(#v == 0)
    assert(v:bytes() == 0)

    -- test that error occurs with non-number argument
    local ok, err = pcall(function()
        v:del('foo')
    end)
    assert(not ok)
    assert(string.match(err, 'number expected, '))
end


local function test_iovec_concat()
    local v = iovec.new()

    -- test that returns empty-string
    assert(v:concat() == '')

    -- test that returns 'foobarbaz'
    for i, s in ipairs({'foo', 'bar', 'baz'}) do
        local ok, idx = v:add(s)
        assert(ok)
        assert(idx == i)
    end
    assert(v:concat() == 'foobarbaz')

    -- test that returns 'foob'
    assert(v:concat(0, 4) == 'foob')

    -- test that returns 'arbaz'
    assert(v:concat(4) == 'arbaz')

    -- test that returns 'rba'
    assert(v:concat(5, 3) == 'rba')
end


local function test_iovec_consume()
    local v = iovec.new()

    for i, s in ipairs({'foo', 'bar', 'baz'}) do
        local ok, idx = v:add(s)
        assert(ok)
        assert(idx == i)
    end

    -- test that consume 0-byte
    assert(v:consume(0) == 9)
    assert(v:consume(-1) == 9)

    -- test that consume 5-byte
    assert(v:consume(5) == 4)
    assert(#v == 2)
    assert(v:get(1) == 'r')
    assert(v:get(2) == 'baz')

    -- test that consume 100-byte
    assert(v:consume(100) == 0)
    assert(#v == 0)

    -- test that error occurs with non-number argument
    local ok, err = pcall(function()
        v:consume('foo')
    end)
    assert(not ok)
    assert(string.match(err, 'number expected, '))
end



local function test_iovec_writev()
    local v = iovec.new()

    for i, s in ipairs({'foo', 'bar', 'baz'}) do
        local ok, idx = v:add(s)
        assert(ok)
        assert(idx == i)
    end


    local r, w, err = pipe()
    assert(not err)

    -- test that writes all data to fd
    local n, err = v:writev(w:fd())
    assert(n == 9)
    assert(not err)
    local s, err = r:read()
    assert(s == 'foobarbaz')
    assert(not err)

    -- test that writes data after first 5-byte to fd
    n, err = v:writev(w:fd(), 5)
    assert(n == 4)
    assert(not err)
    s, err = r:read()
    assert(s == 'rbaz')
    assert(not err)

    -- test that writes data after first 5-byte limit 1-byte to fd
    n, err = v:writev(w:fd(), 5, 1)
    assert(n == 1)
    assert(not err)
    s, err = r:read()
    assert(s == 'r')
    assert(not err)

    -- test that writes data limit 4-byte to fd
    n, err = v:writev(w:fd(), nil, 4)
    assert(n == 4)
    assert(not err)
    s, err = r:read()
    assert(s == 'foob')
    assert(not err)

    -- test that returns again=true if the data written reaches to
    -- maximum buffer size
    v:consume(10)
    local ok = v:addn(4096)
    local total = 0
    assert(ok)
    assert(not w:nonblock(true))
    while true do
        local n, err, again = v:writev(w:fd())
        if again then
            assert(not err)
            break
        end
        assert(n == 4096)
        assert(not err)
        total = total + n
    end
    assert(w:nonblock(false))

    -- test that returns nil if closed by peer
    r:close()
    local res = {v:writev(w:fd())}
    assert(#res == 0)

    -- test that returns error
    w:close()
    n, err = v:writev(w:fd())
    assert(not n)
    assert(err)

    -- test that error occurs with non-number fd argument
    ok, err = pcall(function()
        v:writev('foo')
    end)
    assert(not ok)
    assert(string.match(err, 'number expected, '))

    -- test that error occurs with non-number offset argument
    ok, err = pcall(function()
        v:writev(123, 'foo')
    end)
    assert(not ok)
    assert(string.match(err, 'number expected, '))

    -- test that error occurs with non-number nbyte argument
    ok, err = pcall(function()
        v:writev(123, 0, 'foo')
    end)
    assert(not ok)
    assert(string.match(err, 'number expected, '))
end


local function test_iovec_readv()
    local v = iovec.new()

    for i, s in ipairs({'foo', 'bar', 'baz', 'qux'}) do
        local ok, idx = v:addn(#s)
        assert(ok)
        assert(idx == i)
    end

    local r, w, err = pipe()
    assert(not err)
    r:nonblock(true)

    -- test that reads data from fd into buffer
    local n, err = w:write('hello world!')
    assert(not err)
    n, err = v:readv(r:fd())
    assert(n == 12)
    assert(not err)
    assert(v:concat(0, n) == 'hello world!')

    -- test that reads data at offset of 7-byte
    n, err = w:write('hello world!')
    assert(not err)
    n, err = v:readv(r:fd(), 7)
    assert(n == v:bytes() - 7)
    assert(not err)
    assert(v:concat(7) == 'hello')
    r:read()

    -- test that reads data at offset of 3-byte limit 4-byte
    n, err = w:write('hello world!')
    assert(not err)
    n, err = v:readv(r:fd(), 3, 4)
    assert(n == 4)
    assert(not err)
    assert(v:concat(3, 4) == 'hell')
    r:read()

    -- test that returns nil if closed by peer
    w:close()
    local res = {v:readv(r:fd())}
    assert(#res == 0)

    -- test that error occurs with non-number fd argument
    local ok, err = pcall(function()
        v:readv('foo')
    end)
    assert(not ok)
    assert(string.match(err, 'number expected, '))

    -- test that error occurs with non-number offset argument
    ok, err = pcall(function()
        v:readv(123, 'foo')
    end)
    assert(not ok)
    assert(string.match(err, 'number expected, '))

    -- test that error occurs with non-number nbyte argument
    ok, err = pcall(function()
        v:readv(123, 0, 'foo')
    end)
    assert(not ok)
    assert(string.match(err, 'number expected, '))
end


test_new()
test_iovec_add()
test_iovec_addn()
test_iovec_set()
test_iovec_get()
test_iovec_del()
test_iovec_concat()
test_iovec_consume()
test_iovec_writev()
test_iovec_readv()
