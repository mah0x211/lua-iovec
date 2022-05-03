TARGET=$(PACKAGE).$(LIB_EXTENSION)
SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)
INSTALL?=install

ifdef IOVEC_COVERAGE
COVFLAGS=--coverage
endif


.PHONY: all install

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(WARNINGS) $(COVFLAGS) $(CPPFLAGS) -o $@ -c $<

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS) $(PLATFORM_LDFLAGS) $(COVFLAGS)

install:
	$(INSTALL) $(TARGET) $(LIBDIR)
	$(INSTALL) src/lua_iovec.h $(CONFDIR)
	rm -f $(LUA_INCDIR)/lua_iovec.h
	ln -s $(CONFDIR)/lua_iovec.h $(LUA_INCDIR)
	rm -f $(OBJ)
	rm -f $(TARGET)
