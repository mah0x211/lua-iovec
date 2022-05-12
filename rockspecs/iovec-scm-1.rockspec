package = "iovec"
version = "scm-1"
source = {
    url = "git+https://github.com/mah0x211/lua-iovec.git"
}
description = {
    summary = "vectored I/O module",
    homepage = "https://github.com/mah0x211/lua-iovec",
    license = "MIT/X11",
    maintainer = "Masatoshi Fukunaga"
}
dependencies = {
    "lua >= 5.1",
    "errno >= 0.2.1",
    "lauxhlib >= 0.3.1",
}
build = {
    type = "make",
    build_variables = {
        PACKAGE         = "iovec",
        CFLAGS          = "$(CFLAGS)",
        WARNINGS        = "-Wall -Wno-trigraphs -Wmissing-field-initializers -Wreturn-type -Wmissing-braces -Wparentheses -Wno-switch -Wunused-function -Wunused-label -Wunused-parameter -Wunused-variable -Wunused-value -Wuninitialized -Wunknown-pragmas -Wshadow -Wsign-compare",
        CPPFLAGS        = "-I$(LUA_INCDIR)",
        LDFLAGS         = "$(LIBFLAG)",
        LIB_EXTENSION   = "$(LIB_EXTENSION)",
        IOVEC_COVERAGE  = "$(IOVEC_COVERAGE)",
    },
    install_variables = {
        PACKAGE         = "iovec",
        LIB_EXTENSION   = "$(LIB_EXTENSION)",
        CONFDIR         = '$(CONFDIR)',
        LIBDIR          = "$(LIBDIR)",
        LUA_INCDIR      = '$(LUA_INCDIR)',
    }
}
