#!/usr/bin/env bash
#
# mayhem/test.sh — runs the pre-built oracle (built by mayhem/build.sh with normal flags):
#   * upstream's SGP4 (test-001) and SDP4 (test-002) unit tests: every printed position/velocity
#     component must match the published reference values within 0.01 (km, km/s);
#   * mayhem/kat.c, a known-answer probe of the locator, TLE reader, SGP4 and nxjson code.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}


fail=0; pass=0
ok()  { pass=$((pass + 1)); }
bad() { fail=$((fail + 1)); echo "FAIL: $*" >&2; }

for t in 001 002; do
    bin="$SRC/mayhem/build-tests/test-$t"
    [ -x "$bin" ] || { echo "missing $bin (build.sh should have built it)" >&2; bad "test-$t missing"; continue; }
    out="$(cd "$SRC/src/sgpsdp" && "$bin" 2>&1)"
    rc=$?
    [ "$rc" -eq 0 ] && ok || bad "test-$t exited $rc"
    want_deep=0; [ "$t" = 002 ] && want_deep=64
    if printf '%s\n' "$out" | grep -q "^DEEP_SPACE_EPHEM: $want_deep "; then ok; else bad "test-$t DEEP_SPACE_EPHEM != $want_deep"; fi
    # 5 steps x 6 components = 30 reference comparisons
    res="$(printf '%s\n' "$out" | awk '
        /^STEP [0-9]+ / { sub(/^STEP [0-9]+ +t: *[-0-9.]+ +/, "") }
        /^ *(X|Y|Z|VX|VY|VZ): / {
            n++; d = $2 - $3; if (d < 0) d = -d
            if (d <= 0.01) good++; else print "delta " d " on: " $0 > "/dev/stderr"
        }
        END { printf "%d %d\n", n + 0, good + 0 }')"
    if [ "$res" = "30 30" ]; then pass=$((pass + 30)); else bad "test-$t reference comparisons: '$res' (want '30 30')"; fi
done

kat="$SRC/mayhem/build-tests/kat"
if [ -x "$kat" ]; then
    got="$("$kat" 2>&1)"; rc=$?
    want='locator2longlat JO65 = 13.0000 55.5000
locator2longlat JO65ab = 12.0417 55.0625
locator2longlat bad rc = 0
longlat2locator 12.5 55.7 = JO65GQ
longlat2locator -122.4 37.8 = CM87
qrb Copenhagen-SF = 8777.0 km 325.2 deg
dms2dec 12 30 0 = 12.500000
dec2dms -96.8333 = 96 49 59.88 1
Get_Next_Tle_Set rc = 1
tle catnr = 88888 inclination = 72.8435 xno = 16.05824518
sgp4 t=0 pos = 2328.9707 -5995.2208 1719.9707
bad checksum rc = -2
json length = 1
json description = Mode V/u
json norad = 25544
json uplink_low = 145990000
json alive = 1
json ratio = 1.50
json tags = 2 b
json missing type = 0'
    [ "$rc" -eq 0 ] && ok || bad "kat exited $rc"
    while IFS= read -r line; do
        if printf '%s\n' "$got" | grep -qxF -- "$line"; then ok; else bad "kat missing line: $line"; fi
    done <<< "$want"
else
    echo "missing $kat (build.sh should have built it)" >&2; bad "kat missing"
fi

emit_ctrf "gpredict-kat" "$pass" "$fail" 0
