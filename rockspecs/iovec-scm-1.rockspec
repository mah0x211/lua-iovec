package = "iovec"
version = "scm-1"
source = {
    url = "gitrec://github.com/mah0x211/lua-iovec.git"
}
description = {
    summary = "vectored I/O module",
    homepage = "https://github.com/mah0x211/lua-iovec",
    license = "MIT/X11",
    maintainer = "Masatoshi Teruya"
}
dependencies = {
    "lua >= 5.1",
    "luarocks-fetch-gitrec >= 0.2"
}
build = {
    type = "builtin",
    modules = {
        iovec = {
            incdirs = { "deps/lauxhlib" },
            sources = { "src/iovec.c" }
        },
    }
}
