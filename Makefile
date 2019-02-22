##
## @file
## @brief SQLDBAL Developer Makefile.
## @author James Humphrey (mail@somnisoft.com)
## @version 0.99
##
## This Makefile used internally to build and test the sqldbal library.
## Do not use this Makefile for building the library into your application.
## Instead, include the src/sqldbal.h and src/sqldbal.c directly into your
## project and add those files as part of your own build system.
##
## This software has been placed into the public domain using CC0.
##
.PHONY: all clean doc install release test test_unit
.SUFFIXES:

BDIR = build
INSTALL_PREFIX = /usr/local

SILENT = @

CWARN += -Waggregate-return
CWARN += -Wall
CWARN += -Wbad-function-cast
CWARN += -Wcast-align
CWARN += -Wcast-qual
CWARN += -Wdeclaration-after-statement
CWARN += -Wdisabled-optimization
CWARN += -Wdouble-promotion
CWARN += -Werror
CWARN += -Wextra
CWARN += -Wfatal-errors
CWARN += -Wfloat-equal
CWARN += -Wformat=2
CWARN += -Wframe-larger-than=5000
CWARN += -Winit-self
CWARN += -Winline
CWARN += -Winvalid-pch
CWARN += -Wlarger-than=10000
CWARN += -Wlong-long
CWARN += -Wmissing-declarations
CWARN += -Wmissing-include-dirs
CWARN += -Wmissing-prototypes
CWARN += -Wnested-externs
CWARN += -Wold-style-definition
CWARN += -Wpacked
CWARN += -Wpedantic
CWARN += -pedantic-errors
CWARN += -Wredundant-decls
CWARN += -Wshadow
CWARN += -Wstack-protector
CWARN += -Wstrict-aliasing
CWARN += -Wstrict-prototypes
CWARN += -Wswitch-default
CWARN += -Wundef
CWARN += -Wuninitialized
CWARN += -Wunknown-pragmas
CWARN += -Wunused-parameter
CWARN += -Wvla
CWARN += -Wwrite-strings

CWARN += -Wno-long-long
CWARN += -Wno-covered-switch-default
CWARN += -Wno-switch-enum

CWARN.gcc += $(CWARN)
CWARN.gcc += -Wjump-misses-init
CWARN.gcc += -Wlogical-op
CWARN.gcc += -Wnormalized=nfkc
CWARN.gcc += -Wstack-usage=5000
CWARN.gcc += -Wsync-nand
CWARN.gcc += -Wtrampolines
CWARN.gcc += -Wunsafe-loop-optimizations
CWARN.gcc += -Wunsuffixed-float-constants
CWARN.gcc += -Wvector-operation-performance

CWARN.clang += $(CWARN)
CWARN.clang += -Weverything
CWARN.clang += -fcomment-block-commands=retval

CDEF_POSIX = -D_POSIX_C_SOURCE=200809

CFLAGS += $(CWARN)
CFLAGS += -fstack-protector-all
CFLAGS += -fstrict-overflow
CFLAGS += -std=c99
CFLAGS += -MD
CFLAGS += -DSQLDBAL_MARIADB
CFLAGS += -DSQLDBAL_POSTGRESQL
CFLAGS += -DSQLDBAL_SQLITE
CFLAGS += -Isrc
CFLAGS += -I/usr/include/mysql
CFLAGS += -isystem /usr/include/mysql
CFLAGS += -I/usr/include/postgresql

CFLAGS.debug   += $(CFLAGS)
CFLAGS.debug   += $(CWARN.gcc)
CFLAGS.debug   += -g3
CFLAGS.debug   += -DSQLDBAL_TEST
CFLAGS.debug   += -fprofile-arcs -ftest-coverage

CFLAGS.clang   += $(CFLAGS)
CFLAGS.clang   += $(CWARN.clang)
CFLAGS.clang   += -DSQLDBAL_TEST
CFLAGS.clang   += -fsanitize=undefined

CFLAGS.release += $(CFLAGS)
CFLAGS.debug   += $(CWARN.gcc)
CFLAGS.release += -O3

CFLAGS.test += -Wno-padded

LFLAGS += -L/usr/lib/x86_64-linux-gnu

SCAN_BUILD = $(SILENT) scan-build -maxloop 100          \
                                  -o $(BDIR)/scan-build \
                                  --status-bugs         \
             clang $(CFLAGS) -c -o $(BDIR)/debug/scan-build-sqldbal.o src/sqldbal.c > /dev/null

VFLAGS += -q
VFLAGS += --error-exitcode=1
VFLAGS += --gen-suppressions=yes
VFLAGS += --num-callers=40

VFLAGS_MEMCHECK += --tool=memcheck
VFLAGS_MEMCHECK += --expensive-definedness-checks=yes
VFLAGS_MEMCHECK += --track-origins=yes
VFLAGS_MEMCHECK += --leak-check=full
VFLAGS_MEMCHECK += --leak-resolution=high
VALGRIND_MEMCHECK = $(SILENT) valgrind $(VFLAGS) $(VFLAGS_MEMCHECK)

CC       = gcc
CC.clang = clang

AR.c.debug             = $(SILENT) $(AR) -c -r $@ $^
AR.c.release           = $(SILENT) $(AR) -c -r $@ $^
COMPILE.c.debug        = $(SILENT) $(CC) $(CFLAGS.debug) -c -o $@ $<
COMPILE.c.release      = $(SILENT) $(CC) $(CFLAGS.release) -c -o $@ $<
COMPILE.c.clang        = $(SILENT) $(CC.clang) $(CFLAGS.clang) -c -o $@ $<
LINK.c.debug           = $(SILENT) $(CC) $(LFLAGS) $(CFLAGS.debug) -o $@ $^
LINK.c.release         = $(SILENT) $(CC) $(LFLAGS) $(CFLAGS.release) -o $@ $^
LINK.c.clang           = $(SILENT) $(CC.clang) $(LFLAGS) $(CFLAGS.clang) -o $@ $^
MKDIR                  = $(SILENT) mkdir -p $@
CP                     = $(SILENT) cp $< $@

all: $(BDIR)/debug/libsqldbal.a        \
     $(BDIR)/release/libsqldbal.a      \
     $(BDIR)/debug/test                \
     $(BDIR)/debug/clang_test          \
     $(BDIR)/release/example           \
     $(BDIR)/release/test_only_mariadb \
     $(BDIR)/release/test_only_pq      \
     $(BDIR)/release/test_only_sqlite  \
     $(BDIR)/doc/html/index.html

clean:
	$(SILENT) rm -rf $(BDIR)

doc $(BDIR)/doc/html/index.html: src/sqldbal.h             \
                                 src/sqldbal.c             \
                                 test/seams.h              \
                                 test/seams.c              \
                                 test/test.h               \
                                 test/test.c               \
                                 doc.cfg | $(BDIR)/doc
	$(SILENT) doxygen doc.cfg

install: all
	cp src/sqldbal.h $(INSTALL_PREFIX)/include/sqldbal.h
	cp $(BDIR)/release/libsqldbal.a $(INSTALL_PREFIX)/lib/libsqldbal.a

test_unit: all
	$(VALGRIND_MEMCHECK) $(BDIR)/debug/test -u

test: all
	$(SCAN_BUILD)
	$(VALGRIND_MEMCHECK) $(BDIR)/debug/test
	$(VALGRIND_MEMCHECK) $(BDIR)/debug/clang_test
	$(VALGRIND_MEMCHECK) $(BDIR)/release/example
	$(VALGRIND_MEMCHECK) $(BDIR)/release/test_only_mariadb
	$(VALGRIND_MEMCHECK) $(BDIR)/release/test_only_pq
	$(VALGRIND_MEMCHECK) $(BDIR)/release/test_only_sqlite

-include $(shell find $(BDIR)/ -name "*.d" 2> /dev/null)

$(BDIR)/doc:
	$(MKDIR)

$(BDIR)/release:
	$(MKDIR)

$(BDIR)/debug:
	$(MKDIR)

$(BDIR):
	$(MKDIR)

$(BDIR)/debug/libsqldbal.a: $(BDIR)/debug/sqldbal.o
	$(AR.c.debug)

$(BDIR)/release/libsqldbal.a : $(BDIR)/release/sqldbal.o
	$(AR.c.release)

$(BDIR)/debug/sqldbal.o: src/sqldbal.c | $(BDIR)/debug
	$(COMPILE.c.debug)

$(BDIR)/release/sqldbal.o: src/sqldbal.c | $(BDIR)/release
	$(COMPILE.c.release)

$(BDIR)/debug/test: $(BDIR)/debug/seams.o   \
                    $(BDIR)/debug/sqldbal.o \
                    $(BDIR)/debug/test.o    \
                    $(BDIR)/debug/config.o
	$(LINK.c.debug) -lsqlite3 -lpq -lmariadbclient -lgcov -lpthread -ldl

$(BDIR)/debug/test.o: test/test.c | $(BDIR)/debug
	$(COMPILE.c.debug) $(CDEF_POSIX)

$(BDIR)/debug/seams.o: test/seams.c | $(BDIR)/debug
	$(COMPILE.c.debug) -Wno-cast-qual

$(BDIR)/debug/config.o: test/config.c | $(BDIR)/debug
	$(COMPILE.c.debug) $(CDEF_POSIX)

$(BDIR)/debug/clang_test: $(BDIR)/debug/clang_seams.o   \
                          $(BDIR)/debug/clang_sqldbal.o \
                          $(BDIR)/debug/clang_test.o    \
                          $(BDIR)/debug/clang_config.o
	$(LINK.c.clang) -lsqlite3 -lpq -lmariadbclient -lgcov -lubsan

$(BDIR)/debug/clang_seams.o: test/seams.c | $(BDIR)
	$(COMPILE.c.clang) -Wno-format-nonliteral -Wno-cast-qual

$(BDIR)/debug/clang_sqldbal.o: src/sqldbal.c | $(BDIR)
	$(COMPILE.c.clang)

$(BDIR)/debug/clang_test.o: test/test.c | $(BDIR)
	$(COMPILE.c.clang) $(CFLAGS.test) $(CDEF_POSIX)

$(BDIR)/debug/clang_config.o: test/config.c | $(BDIR)
	$(COMPILE.c.clang) $(CDEF_POSIX)

$(BDIR)/release/example: $(BDIR)/release/example.o \
                         $(BDIR)/release/sqldbal.o
	$(LINK.c.release) -lmariadbclient -lpq -lsqlite3

$(BDIR)/release/example.o: test/example.c | $(BDIR)
	$(COMPILE.c.release)

$(BDIR)/release/test_only_mariadb: $(BDIR)/release/test_only_mariadb_sqldbal.o \
                                   $(BDIR)/release/test_only_mariadb.o         \
                                   $(BDIR)/release/config.o
	$(LINK.c.release) -lmariadbclient

$(BDIR)/release/test_only_mariadb_sqldbal.o: src/sqldbal.c | $(BDIR)
	$(COMPILE.c.release) -USQLDBAL_POSTGRESQL -USQLDBAL_SQLITE

$(BDIR)/release/test_only_mariadb.o: test/test_only_mariadb.c | $(BDIR)
	$(COMPILE.c.release)

$(BDIR)/release/test_only_pq: $(BDIR)/release/test_only_pq_sqldbal.o \
                              $(BDIR)/release/test_only_pq.o         \
                              $(BDIR)/release/config.o
	$(LINK.c.release) -lpq

$(BDIR)/release/test_only_pq_sqldbal.o: src/sqldbal.c | $(BDIR)
	$(COMPILE.c.release) -USQLDBAL_MARIADB -USQLDBAL_SQLITE

$(BDIR)/release/test_only_pq.o: test/test_only_pq.c | $(BDIR)
	$(COMPILE.c.release)

$(BDIR)/release/test_only_sqlite: $(BDIR)/release/test_only_sqlite_sqldbal.o \
                                  $(BDIR)/release/test_only_sqlite.o         \
                                  $(BDIR)/release/config.o
	$(LINK.c.release) -lsqlite3

$(BDIR)/release/test_only_sqlite_sqldbal.o: src/sqldbal.c | $(BDIR)
	$(COMPILE.c.release) -USQLDBAL_MARIADB -USQLDBAL_POSTGRESQL

$(BDIR)/release/test_only_sqlite.o: test/test_only_sqlite.c | $(BDIR)
	$(COMPILE.c.release)

$(BDIR)/release/config.o: test/config.c | $(BDIR)
	$(COMPILE.c.release) $(CDEF_POSIX)

$(BDIR)/media:
	$(MKDIR)

release: $(BDIR)/sqldbal.tar.gz \
         $(BDIR)/sqldbal.zip
$(BDIR)/sqldbal.tar.gz: $(BDIR)/sqldbal/sqldbal.c \
                        $(BDIR)/sqldbal/sqldbal.h
	$(SILENT) tar -C $(BDIR) -c -z -v -f $@ sqldbal
$(BDIR)/sqldbal.zip: $(BDIR)/sqldbal/sqldbal.c \
                     $(BDIR)/sqldbal/sqldbal.h
	$(SILENT) cd $(BDIR) && zip -r -T -v sqldbal.zip sqldbal

$(BDIR)/sqldbal/sqldbal.c: src/sqldbal.c | $(BDIR)/sqldbal
	$(CP)

$(BDIR)/sqldbal/sqldbal.h: src/sqldbal.h | $(BDIR)/sqldbal
	$(CP)

$(BDIR)/sqldbal:
	$(MKDIR)

