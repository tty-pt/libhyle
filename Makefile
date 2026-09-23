FOLDER := hyle
all := libhyle hyle_test

share := assets/hyle.css
share-dir := hyle

LDLIBS-libhyle := -lstoma -lcorm
libhyle-obj-y := src/ctx.o src/value.o src/query.o src/view.o src/field.o src/blueprint.o src/purify.o src/registry.o src/url.o
LDLIBS-hyle_test := -lhyle -lstoma -lcorm

include ../mk/include.mk

# POSIX <regex.h> is not available on Windows; use PCRE2's POSIX wrapper.
# Since PCRE2 10.43 the wrapper lives in libpcre2-posix (see pcre2posix.h macros).
# Defined at the SYS level (not per-target): portable.mk folds LDLIBS-${SYS} into the
# global LDLIBS, which the generic rules emit AFTER every per-target library. Required
# for correct left-to-right static archive resolution: purify.o (in libhyle.a) references
# pcre2_reg*, so -lpcre2 must come after -lhyle/-lstoma/-lcorm.
LDLIBS-Windows := -lpcre2-posix -lpcre2-8

# Install pkgconfig descriptor
${DESTDIR}${PREFIX}/lib/pkgconfig/hyle.pc: hyle.pc
	install -d ${DESTDIR}${PREFIX}/lib/pkgconfig
	install -m 644 hyle.pc $@

install: ${DESTDIR}${PREFIX}/lib/pkgconfig/hyle.pc

# Also build a static archive for Rust FFI (avoids naming conflict with Rust cdylib)
lib/libhyle.a: ${libhyle-obj-y} lib
	ar rcs $@ ${libhyle-obj-y}

all: lib/libhyle.a # zig-bindings/libhyle_zig.a

# Zig bindings (native static lib + WASM module)
ZIG ?= zig
zig-bindings/libhyle_zig.a: zig-bindings/src/*.zig lib/libhyle.a
	cd zig-bindings && $(ZIG) build && cp zig-out/lib/libhyle_zig.a ../$@

zig-bindings/hyle_wasm.wasm: zig-bindings/src/*.zig
	cd zig-bindings && $(ZIG) build && cp zig-out/bin/hyle_wasm.wasm ../$@

zig-test:
	cd zig-bindings && $(ZIG) build test

test: all
	LD_LIBRARY_PATH=./lib:../libcorm/lib:../libstoma/lib ./bin/hyle_test${EXE}
	$(MAKE) zig-test
