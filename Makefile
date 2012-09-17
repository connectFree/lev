# TODO: shoube be versioning ...
VERSION=0.0.1
LUADIR=deps/luajit
LUAJIT_VERSION=$(shell git --git-dir ${LUADIR}/.git describe --tags)
YAJLDIR=deps/yajl
YAJL_VERSION=$(shell git --git-dir ${YAJLDIR}/.git describe --tags)
UVDIR=deps/uv
UV_VERSION=$(shell git --git-dir ${UVDIR}/.git describe --all --long | cut -f 3 -d -)
HTTPDIR=deps/http-parser
HTTP_VERSION=$(shell git --git-dir ${HTTPDIR}/.git describe --tags)
ZLIBDIR=deps/zlib
SSLDIR=deps/openssl
BUILDDIR=build
CRYPTODIR=deps/luacrypto

PREFIX?=/usr/local
BINDIR?=${DESTDIR}${PREFIX}/bin
INCDIR?=${DESTDIR}${PREFIX}/include/lev
LIBDIR?=${DESTDIR}${PREFIX}/lib/lev

OPENSSL_LIBS=$(shell pkg-config openssl --libs 2> /dev/null)
ifeq (${OPENSSL_LIBS},)
USE_SYSTEM_SSL?=0
else
USE_SYSTEM_SSL?=1
endif

OS_NAME=$(shell uname -s)
MH_NAME=$(shell uname -m)
ifeq (${OS_NAME},Darwin)
ifeq (${MH_NAME},x86_64)
LDFLAGS+=-framework CoreServices -pagezero_size 10000 -image_base 100000000
else
LDFLAGS+=-framework CoreServices
endif
else ifeq (${OS_NAME},Linux)
LDFLAGS+=-Wl,-E
endif
# LUAJIT CONFIGURATION #
#XCFLAGS=-g
#XCFLAGS+=-DLUAJIT_DISABLE_JIT
XCFLAGS+=-DLUAJIT_ENABLE_LUA52COMPAT
#XCFLAGS+=-DLUA_USE_APICHECK
export XCFLAGS
# verbose build
export Q=
MAKEFLAGS+=-e

LDFLAGS+=-L${BUILDDIR}
LIBS += -llev \
	${ZLIBDIR}/libz.a \
	${YAJLDIR}/yajl.a \
	${UVDIR}/uv.a \
	${LUADIR}/src/libluajit.a \
	-lm -ldl -lpthread
ifeq (${USE_SYSTEM_SSL},1)
CFLAGS+=-Wall -w
CPPFLAGS+=$(shell pkg-config --cflags openssl)
LIBS+=${OPENSSL_LIBS}
else
CPPFLAGS+=-I${SSLDIR}/openssl/include
LIBS+=${SSLDIR}/libopenssl.a
endif

ifeq (${OS_NAME},Linux)
LIBS+=-lrt
endif

CPPFLAGS += -DUSE_OPENSSL
CPPFLAGS += -DL_ENDIAN
CPPFLAGS += -DOPENSSL_THREADS
CPPFLAGS += -DPURIFY
CPPFLAGS += -D_REENTRANT
CPPFLAGS += -DOPENSSL_NO_ASM
CPPFLAGS += -DOPENSSL_NO_INLINE_ASM
CPPFLAGS += -DOPENSSL_NO_RC2
CPPFLAGS += -DOPENSSL_NO_RC5
CPPFLAGS += -DOPENSSL_NO_MD4
CPPFLAGS += -DOPENSSL_NO_HW
CPPFLAGS += -DOPENSSL_NO_GOST
CPPFLAGS += -DOPENSSL_NO_CAMELLIA
CPPFLAGS += -DOPENSSL_NO_CAPIENG
CPPFLAGS += -DOPENSSL_NO_CMS
CPPFLAGS += -DOPENSSL_NO_FIPS
CPPFLAGS += -DOPENSSL_NO_IDEA
CPPFLAGS += -DOPENSSL_NO_MDC2
CPPFLAGS += -DOPENSSL_NO_MD2
CPPFLAGS += -DOPENSSL_NO_SEED
CPPFLAGS += -DOPENSSL_NO_SOCK

ifeq (${MH_NAME},x86_64)
CPPFLAGS += -I${SSLDIR}/openssl-configs/x64
else
CPPFLAGS += -I${SSLDIR}/openssl-configs/ia32
endif

LUVLIBS=${BUILDDIR}/utils.o            \
				${BUILDDIR}/lev_mpack.o        \
        ${BUILDDIR}/lev_new_fs.o       \
        ${BUILDDIR}/lev_new_net.o      \
        ${BUILDDIR}/lev_new_tcp.o      \
        ${BUILDDIR}/lev_new_dns.o      \
        ${BUILDDIR}/lev_new_udp.o      \
        ${BUILDDIR}/lev_new_base.o     \
        ${BUILDDIR}/lev_new_core.o     \
        ${BUILDDIR}/lev_new_pipe.o     \
        ${BUILDDIR}/lev_new_timer.o    \
				${BUILDDIR}/lev_new_buffer.o   \
        ${BUILDDIR}/lev_new_process.o  \
        ${BUILDDIR}/lev_new_signal.o   \
        ${BUILDDIR}/lev_slab.o         \
        ${BUILDDIR}/luv_debug.o        \
        ${BUILDDIR}/lenv.o             \
        ${BUILDDIR}/lhttp_parser.o   

DEPS=${LUADIR}/src/libluajit.a \
     ${YAJLDIR}/yajl.a         \
     ${UVDIR}/uv.a             \
     ${ZLIBDIR}/libz.a         \
     ${HTTPDIR}/http_parser.o

ifeq (${USE_SYSTEM_SSL},0)
DEPS+=${SSLDIR}/libopenssl.a
endif

all: Makefile ${BUILDDIR}/lev

Makefile: .gitmodules
	git submodule sync

${LUADIR}/Makefile:
	git submodule update --init ${LUADIR}

${LUADIR}/src/libluajit.a: ${LUADIR}/Makefile
	touch -c ${LUADIR}/src/*.h
	$(MAKE) -C ${LUADIR}

${YAJLDIR}/CMakeLists.txt:
	git submodule update --init ${YAJLDIR}

${YAJLDIR}/Makefile: deps/Makefile.yajl ${YAJLDIR}/CMakeLists.txt
	cp deps/Makefile.yajl ${YAJLDIR}/Makefile

${YAJLDIR}/yajl.a: ${YAJLDIR}/Makefile
	rm -rf ${YAJLDIR}/src/yajl
	cp -r ${YAJLDIR}/src/api ${YAJLDIR}/src/yajl
	$(MAKE) -C ${YAJLDIR}

${UVDIR}/Makefile:
	git submodule update --init ${UVDIR}

${UVDIR}/uv.a: ${UVDIR}/Makefile
	$(MAKE) -C ${UVDIR} uv.a

${HTTPDIR}/Makefile:
	git submodule update --init ${HTTPDIR}

${HTTPDIR}/http_parser.o: ${HTTPDIR}/Makefile
	$(MAKE) -C ${HTTPDIR} http_parser.o

${ZLIBDIR}/zlib.gyp:
	git submodule update --init ${ZLIBDIR}

${ZLIBDIR}/libz.a: ${ZLIBDIR}/zlib.gyp
	cd ${ZLIBDIR} && ${CC} -c *.c && \
	$(AR) rvs libz.a *.o

${SSLDIR}/Makefile.openssl:
	git submodule update --init ${SSLDIR}

${SSLDIR}/libopenssl.a: ${SSLDIR}/Makefile.openssl
	$(MAKE) -C ${SSLDIR} -f Makefile.openssl

${BUILDDIR}/%.o: src/%.c ${DEPS}
	mkdir -p ${BUILDDIR}
	$(CC) ${CPPFLAGS} ${CFLAGS} --std=c99 -D_GNU_SOURCE -g -Wall -Werror -c $< -o $@ \
		-I${HTTPDIR} -I${UVDIR}/include -I${LUADIR}/src -I${YAJLDIR}/src/api \
		-I${YAJLDIR}/src -I${ZLIBDIR} -I${CRYPTODIR}/src \
		-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 \
		-DUSE_SYSTEM_SSL=${USE_SYSTEM_SSL} \
		-DHTTP_VERSION=\"${HTTP_VERSION}\" \
		-DUV_VERSION=\"${UV_VERSION}\" \
		-DYAJL_VERSIONISH=\"${YAJL_VERSION}\" \
		-DLEV_VERSION=\"${VERSION}\" \
		-DLUAJIT_VERSION=\"${LUAJIT_VERSION}\"

${BUILDDIR}/%.pre: src/%.c ${DEPS}
	mkdir -p ${BUILDDIR}
	$(CC) ${CPPFLAGS} ${CFLAGS} --std=c99 -D_GNU_SOURCE -g -Wall -Werror -E $< \
		-I${HTTPDIR} -I${UVDIR}/include -I${LUADIR}/src -I${YAJLDIR}/src/api \
		-I${YAJLDIR}/src -I${ZLIBDIR} -I${CRYPTODIR}/src \
		-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 \
		-DUSE_SYSTEM_SSL=${USE_SYSTEM_SSL} \
		-DHTTP_VERSION=\"${HTTP_VERSION}\" \
		-DUV_VERSION=\"${UV_VERSION}\" \
		-DYAJL_VERSIONISH=\"${YAJL_VERSION}\" \
		-DLEV_VERSION=\"${VERSION}\" \
		-DLUAJIT_VERSION=\"${LUAJIT_VERSION}\" \
    > $@

${BUILDDIR}/liblev.a: ${CRYPTODIR}/Makefile ${LUVLIBS} ${DEPS}
	$(AR) rvs ${BUILDDIR}/liblev.a ${LUVLIBS} ${DEPS}

${CRYPTODIR}/Makefile:
	git submodule update --init ${CRYPTODIR}

${CRYPTODIR}/src/lcrypto.o: ${CRYPTODIR}/Makefile
	${CC} ${CPPFLAGS} -c -o ${CRYPTODIR}/src/lcrypto.o -I${CRYPTODIR}/src/ \
		 -I${LUADIR}/src/ ${CRYPTODIR}/src/lcrypto.c

${BUILDDIR}/lev: ${BUILDDIR}/liblev.a ${BUILDDIR}/lev_main.o ${CRYPTODIR}/src/lcrypto.o
	$(CC) ${CPPFLAGS} ${CFLAGS} ${LDFLAGS} -g -o ${BUILDDIR}/lev ${BUILDDIR}/lev_main.o ${BUILDDIR}/liblev.a \
		${CRYPTODIR}/src/lcrypto.o ${LIBS}

clean:
	${MAKE} -C ${LUADIR} clean
	${MAKE} -C ${SSLDIR} -f Makefile.openssl clean
	${MAKE} -C ${HTTPDIR} clean
	${MAKE} -C ${YAJLDIR} clean
	${MAKE} -C ${UVDIR} distclean
	${MAKE} -C examples/native clean
	-rm ${ZLIBDIR}/*.o
	-rm ${CRYPTODIR}/src/lcrypto.o
	rm -rf build bundle

install: all
	mkdir -p ${BINDIR}
	install ${BUILDDIR}/lev ${BINDIR}/lev
	mkdir -p ${LIBDIR}
	cp lib/lev/*.lua ${LIBDIR}
	mkdir -p ${INCDIR}/luajit
	cp ${LUADIR}/src/lua.h ${INCDIR}/luajit/
	cp ${LUADIR}/src/lauxlib.h ${INCDIR}/luajit/
	cp ${LUADIR}/src/luaconf.h ${INCDIR}/luajit/
	cp ${LUADIR}/src/luajit.h ${INCDIR}/luajit/
	cp ${LUADIR}/src/lualib.h ${INCDIR}/luajit/
	mkdir -p ${INCDIR}/http_parser
	cp ${HTTPDIR}/http_parser.h ${INCDIR}/http_parser/
	mkdir -p ${INCDIR}/uv
	cp -r ${UVDIR}/include/* ${INCDIR}/uv/
	cp src/*.h ${INCDIR}/

uninstall:
	test -f ${BINDIR}/lev && rm -f ${BINDIR}/lev
	test -d ${LIBDIR} && rm -rf ${LIBDIR}
	test -d ${INCDIR} && rm -rf ${INCDIR}

bundle: bundle/lev

bundle/lev: build/lev ${BUILDDIR}/liblev.a
	build/lev tools/bundler.lua
	$(CC) --std=c99 -D_GNU_SOURCE -g -Wall -Werror -DBUNDLE -c src/lev_exports.c -o bundle/lev_exports.o -I${HTTPDIR} -I${UVDIR}/include -I${LUADIR}/src -I${YAJLDIR}/src/api -I${YAJLDIR}/src -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DHTTP_VERSION=\"${HTTP_VERSION}\" -DUV_VERSION=\"${UV_VERSION}\" -DYAJL_VERSIONISH=\"${YAJL_VERSION}\" -DLUVIT_VERSION=\"${VERSION}\" -DLUAJIT_VERSION=\"${LUAJIT_VERSION}\"
	$(CC) --std=c99 -D_GNU_SOURCE -g -Wall -Werror -DBUNDLE -c src/lev_main.c -o bundle/lev_main.o -I${HTTPDIR} -I${UVDIR}/include -I${LUADIR}/src -I${YAJLDIR}/src/api -I${YAJLDIR}/src -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -DHTTP_VERSION=\"${HTTP_VERSION}\" -DUV_VERSION=\"${UV_VERSION}\" -DYAJL_VERSIONISH=\"${YAJL_VERSION}\" -DLUVIT_VERSION=\"${VERSION}\" -DLUAJIT_VERSION=\"${LUAJIT_VERSION}\"
	$(CC) ${LDFLAGS} -g -o bundle/lev ${BUILDDIR}/liblev.a `ls bundle/*.o` ${LIBS} ${CRYPTODIR}/src/lcrypto.o

# Test section

test: test-lua test-install test-uninstall

test-lua: ${BUILDDIR}/lev
	./build/lev ./tools/checkit tests/*.lua

ifeq ($(MAKECMDGOALS),test)
DESTDIR=test_install
endif

test-install: install
	test -f ${BINDIR}/lev
	test -d ${INCDIR}
	test -d ${LIBDIR}

test-uninstall: uninstall
	test ! -f ${BINDIR}/lev
	test ! -d ${INCDIR}
	test ! -d ${LIBDIR}

api: api.markdown

api.markdown: $(wildcard lib/*.lua)
	find lib -name "*.lua" | grep -v "lev.lua" | sort | xargs -l lev tools/doc-parser.lua > $@

DIST_DIR?=${HOME}/lev/dist
DIST_NAME=lev-${VERSION}
DIST_FOLDER=${DIST_DIR}/${VERSION}/${DIST_NAME}
DIST_FILE=${DIST_FOLDER}.tar.gz
dist_build:
	sed -e 's/^VERSION=.*/VERSION=${VERSION}/' \
            -e 's/^LUAJIT_VERSION=.*/LUAJIT_VERSION=${LUAJIT_VERSION}/' \
            -e 's/^UV_VERSION=.*/UV_VERSION=${UV_VERSION}/' \
            -e 's/^HTTP_VERSION=.*/HTTP_VERSION=${HTTP_VERSION}/' \
            -e 's/^YAJL_VERSION=.*/YAJL_VERSION=${YAJL_VERSION}/' < Makefile > Makefile.dist
	sed -e 's/LEV_VERSION=".*/LEV_VERSION=\"${VERSION}\"'\'',/' \
            -e 's/LUAJIT_VERSION=".*/LUAJIT_VERSION=\"${LUAJIT_VERSION}\"'\'',/' \
            -e 's/UV_VERSION=".*/UV_VERSION=\"${UV_VERSION}\"'\'',/' \
            -e 's/HTTP_VERSION=".*/HTTP_VERSION=\"${HTTP_VERSION}\"'\'',/' \
            -e 's/YAJL_VERSIONISH=".*/YAJL_VERSIONISH=\"${YAJL_VERSION}\"'\'',/' < lev.gyp > lev.gyp.dist

tarball: dist_build
	rm -rf ${DIST_FOLDER} ${DIST_FILE}
	mkdir -p ${DIST_DIR}
	git clone . ${DIST_FOLDER}
	cp deps/gitmodules.local ${DIST_FOLDER}/.gitmodules
	cd ${DIST_FOLDER} ; git submodule update --init
	find ${DIST_FOLDER} -name ".git*" | xargs rm -r
	mv Makefile.dist ${DIST_FOLDER}/Makefile
	mv lev.gyp.dist ${DIST_FOLDER}/lev.gyp
	tar -czf ${DIST_FILE} -C ${DIST_DIR}/${VERSION} ${DIST_NAME}
	rm -rf ${DIST_FOLDER}

.PHONY: test install uninstall all api.markdown bundle tarball

