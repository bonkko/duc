bin_PROGRAMS := duc

duc_SOURCES := \
	src/libduc/buffer.c \
	src/libduc/buffer.h \
	src/libduc/db.c \
	src/libduc/db.h \
	src/libduc/db-tokyo.c \
	src/libduc/db-kyoto.c \
	src/libduc/db-leveldb.c \
	src/libduc/db-sqlite3.c \
	src/libduc/db-lmdb.c \
	src/libduc/dir.c \
	src/libduc/duc.c \
	src/libduc/duc.h \
	src/libduc/index.c \
	src/libduc/private.h \
	src/libduc/canonicalize.c \
	src/libduc/varint.c \
	src/libduc/varint.h \
	src/libduc/uthash.h \
	src/libduc/utlist.h \
	src/libduc/utstring.h

duc_SOURCES += \
	src/glad/glad.c \
	src/glad/KHR/khrplatform.h \
	src/glad/glad/glad.h

duc_SOURCES += \
	src/libduc-graph/graph.c \
	src/libduc-graph/graph-cairo.c \
	src/libduc-graph/graph-opengl.c \
	src/libduc-graph/graph-svg.c \
	src/libduc-graph/graph-html.c \
	src/libduc-graph/graph-private.h \
	src/libduc-graph/duc-graph.h

duc_SOURCES  += \
	src/duc/cmd-cgi.c \
	src/duc/cmd-graph.c \
	src/duc/cmd-gui.c \
	src/duc/cmd-guigl.c \
	src/duc/cmd.h \
	src/duc/cmd.h  \
	src/duc/cmd-index.c \
	src/duc/cmd-info.c \
	src/duc/cmd-ls.c \
	src/duc/cmd-ui.c \
	src/duc/cmd-xml.c \
	src/duc/ducrc.c \
	src/duc/ducrc.h \
	src/duc/main.c

AM_CFLAGS := @CAIRO_CFLAGS@ @PANGO_CFLAGS@ @PANGOCAIRO_CFLAGS@
AM_CFLAGS += @TC_CFLAGS@ @SQLITE3_CFLAGS@ @GLFW3_CFLAGS@ @LMDB_CFLAGS@ @KC_CFLAGS@
AM_CFLAGS += -Isrc/libduc -Isrc/libduc-graph -Isrc/glad

duc_LDADD := @CAIRO_LIBS@ @PANGO_LIBS@ @PANGOCAIRO_LIBS@
duc_LDADD += @TC_LIBS@ @SQLITE3_LIBS@ @GLFW3_LIBS@ @LMDB_LIBS@ @KC_LIBS@
check_PROGRAMS = tests/check_duc_ls_api
tests_check_duc_ls_api_SOURCES = tests/check_duc_ls_api.c
tests_check_duc_ls_api_CFLAGS = -lcheck -lm -lpthread -lrt $(AM_CFLAGS)
tests_check_duc_ls_api_LDADD = $(duc_LDADD) src/libduc/buffer.o src/libduc/db.o src/libduc/db-tokyo.o src/libduc/db-kyoto.o src/libduc/db-leveldb.o src/libduc/db-sqlite3.o src/libduc/db-lmdb.o src/libduc/dir.o src/libduc/duc.o src/libduc/index.o src/libduc/canonicalize.o src/libduc/varint.o src/glad/glad.o src/libduc-graph/graph.o src/libduc-graph/graph-cairo.o src/libduc-graph/graph-opengl.o src/libduc-graph/graph-svg.o src/libduc-graph/graph-html.o src/duc/cmd-cgi.o src/duc/cmd-graph.o src/duc/cmd-gui.o src/duc/cmd-guigl.o src/duc/cmd-index.o src/duc/cmd-info.o src/duc/cmd-ls.o src/duc/cmd-ui.o src/duc/cmd-xml.o src/duc/ducrc.o    


man1_MANS = \
	doc/duc.1

EXTRA_DIST = \
	doc/duc.1 \
	doc/duc.1.html \
	src/libduc-graph/font.c \
	doc/duc.md

install-exec-hook:
	-/sbin/ldconfig || :;

