#!/usr/bin/env bash
#
# mayhem/build.sh — builds gpredict's library (sanitized, SanCov), the five fuzz harnesses plus their
# -standalone twins, and the normal-flags oracle binaries that mayhem/test.sh runs.
# Runs inside the commit image as `mayhem` in /mayhem; fully offline (apt -dev packages only).
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${RUST_DEBUG_FLAGS:=-C debuginfo=2 -C force-frame-pointers=yes -C llvm-args=-gdwarf-3}"
: "${GO_DEBUG_FLAGS:=-gcflags=all=-N -l}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS RUST_DEBUG_FLAGS GO_DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

# build-config.h is hand-written (mayhem/include) so no autotools run is needed.
GTK_CFLAGS="$(pkg-config --cflags gtk+-3.0 libcurl)"
GTK_LIBS="$(pkg-config --libs gtk+-3.0 libcurl)"
DEFS='-DHAVE_CONFIG_H -DPACKAGE_DATA_DIR="/usr/share/gpredict" -DPACKAGE_PIXMAPS_DIR="/usr/share/pixmaps/gpredict" -DPACKAGE_LOCALE_DIR="/usr/share/locale" -DGTK_DISABLE_SINGLE_INCLUDES -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGSEAL_ENABLE'
INCS="-I$SRC/mayhem/include -I$SRC -I$SRC/src"

SGP_SRCS="src/sgpsdp/sgp4sdp4.c src/sgpsdp/sgp_in.c src/sgpsdp/sgp_math.c src/sgpsdp/sgp_obs.c src/sgpsdp/sgp_time.c src/sgpsdp/solar.c"
LIB_SRCS="$(ls src/*.c | grep -v '^src/main\.c$') $SGP_SRCS src/nxjson/nxjson.c"

# 1) The project itself, sanitized + SanCov + DWARF 3, as one static library.
rm -rf mayhem/build && mkdir -p mayhem/build/obj
compile_one() {
    src="$1"
    obj="mayhem/build/obj/$(echo "${src%.c}" | tr / _).o"
    relax=""
    # FMod2p(), the TLE revnum parse, and dec2dms()/longlat2locator() cast a NaN/huge double to int;
    # only float-cast-overflow is relaxed, on these three files (known-findings/degenerate-elements and
    # fuzz_locator/known-findings/nonfinite-coordinates). ASan and the rest of UBSan stay halting.
    # fuzz_qth links a strict locator.c below, so a NaN cast reached through a .qth file is still reported.
    case "$src" in src/sgpsdp/sgp_math.c|src/sgpsdp/sgp_in.c|src/locator.c) relax="-fno-sanitize=float-cast-overflow" ;; esac
    # shellcheck disable=SC2086
    "$CC" $SANITIZER_FLAGS $relax $DEBUG_FLAGS -fsanitize=fuzzer-no-link -O1 -w \
        $GTK_CFLAGS $INCS $DEFS -c "$src" -o "$obj"
}
export -f compile_one
export CC SANITIZER_FLAGS DEBUG_FLAGS GTK_CFLAGS INCS DEFS
printf '%s\n' $LIB_SRCS | xargs -P "$MAYHEM_JOBS" -I{} bash -c 'compile_one {}'
rm -f mayhem/build/libgpredict.a
ar rcs mayhem/build/libgpredict.a mayhem/build/obj/*.o
# locator.c again with every UBSan check halting, linked ahead of the archive into fuzz_qth only.
# shellcheck disable=SC2086
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link -O1 -w $GTK_CFLAGS $INCS $DEFS \
    -c src/locator.c -o mayhem/build/locator_strict.o

# 2) Harnesses, each as a libFuzzer binary and as a standalone run-once reproducer.
# shellcheck disable=SC2086
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c mayhem/lsan_off.cc -o mayhem/build/lsan_off.o
for t in tle satfile qth nxjson locator; do
    extra=""
    [ "$t" = qth ] && extra="mayhem/build/locator_strict.o"
    # shellcheck disable=SC2086
    $CC $SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link -O1 -w $GTK_CFLAGS $INCS $DEFS \
        -c "mayhem/harness_$t.c" -I"$SRC/mayhem" -o "mayhem/build/harness_$t.o"
    # shellcheck disable=SC2086
    $CC $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "mayhem/build/harness_$t.o" mayhem/build/lsan_off.o $extra \
        mayhem/build/libgpredict.a $GTK_LIBS -lm -o "/mayhem/fuzz_$t"
    # shellcheck disable=SC2086
    $CC $SANITIZER_FLAGS $DEBUG_FLAGS $STANDALONE_FUZZ_MAIN "mayhem/build/harness_$t.o" mayhem/build/lsan_off.o $extra \
        mayhem/build/libgpredict.a $GTK_LIBS -lm -o "/mayhem/fuzz_$t-standalone"
done

# 3) Oracle binaries: upstream's SGP4/SDP4 unit tests plus a known-answer probe, normal flags.
rm -rf mayhem/build-tests && mkdir -p mayhem/build-tests
ORACLE_SRCS="src/locator.c $SGP_SRCS src/nxjson/nxjson.c"
# shellcheck disable=SC2086
for prog in src/sgpsdp/test-001.c src/sgpsdp/test-002.c mayhem/kat.c; do
    name="$(basename "${prog%.c}")"
    $CC -O2 $COVERAGE_FLAGS -w $GTK_CFLAGS $INCS $DEFS "$prog" $ORACLE_SRCS \
        $GTK_LIBS -lm $COVERAGE_FLAGS -o "mayhem/build-tests/$name"
done
