lib_LTLIBRARIES = libinterflop_cancellation.la
libinterflop_cancellation_la_SOURCES = \
    interflop_cancellation.c \
    @INTERFLOP_STDLIB_PATH@/include/interflop-stdlib/iostream/logger.c \
    @INTERFLOP_STDLIB_PATH@/include/interflop-stdlib/common/options.c
libinterflop_cancellation_la_CFLAGS = -DBACKEND_HEADER="interflop_cancellation" -I@INTERFLOP_STDLIB_PATH@/include/
if WALL_CFLAGS
libinterflop_cancellation_la_CFLAGS += -Wall -Wextra
endif
libinterflop_cancellation_la_LDFLAGS = -lm
libinterflop_cancellation_la_LIBADD = @INTERFLOP_STDLIB_PATH@/lib/libinterflop_rng.la
if BUILD_INTERFLOP_STDLIB
libinterflop_cancellation_la_LIBADD += @INTERFLOP_STDLIB_PATH@/lib/libinterflop_stdlib.la
endif
library_includedir =$(includedir)/
