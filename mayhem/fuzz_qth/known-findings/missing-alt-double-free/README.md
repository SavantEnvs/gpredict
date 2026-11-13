# Double free in `qth_data_read` when `ALT` is missing or not an integer

- **Cause:** `src/qth-data.c:183` frees `buff` in the altitude error branch, but `buff` still points to the
  longitude string that was already `g_free`d after `g_ascii_strtod()` at line 168.
- **Impact:** heap double free (`attempting double-free`) when loading a `.qth` file with a valid `LON` and no
  valid integer `ALT`; any user's hand-edited file can trigger it.
- **Upstream fix:** delete the `if (buff != NULL) g_free(buff);` in the altitude error branch (or set
  `buff = NULL` after each free).
- **Harness:** `fuzz_qth` passes every input to `qth_data_read()`; nothing is filtered, so this defect is
  reported by the target. The seeds all carry a valid `ALT`, so replaying them never trips it.
- **Reproducer:** `repro.c`, linked against `mayhem/build/libgpredict.a`.

## Related: `LAT`/`LON` of `nan` or `inf`

`qth_data_read()` (`src/qth-data.c:231`) passes the values it read to `longlat2locator()` before
`qth_validate()` zeroes a non-finite one, which casts NaN to `int`. `fuzz_qth` links a `locator.c` with every UBSan
check halting, so it reports this too. Details and the build-flag choice are in
`fuzz_locator/known-findings/nonfinite-coordinates`.

## Coverage: `fuzz_qth` is near its ceiling

Only `qth-data.c` is SanCov-instrumented on this path; GKeyFile parsing in glib is not, so Mayhem's edge count
stays low however well the reader is exercised. After 90 s of local fuzzing from the shipped seeds, `llvm-cov`
reports 94% of regions in `qth_data_read` (82 regions, 5 missed; 93% of lines) and 94% in `qth_validate`, with
every key reached (`DESCRIPTION`, `LOCATION`, `LAT`, `LON`, `ALT`, `WX`, `QTH_TYPE`), the failed-load path and
the out-of-range wrap branches. What is missed: the `g_free(buff)` calls in the `LAT`/`LON` error branches
(dead, `g_key_file_get_string` returns NULL on error), the `longlat2locator` failure branch (`pair_count` is a
constant 2), one wrap branch of `qth_validate`, and the double-free line itself, which aborts the process before a
coverage profile is written. `GPSDSERVER`/`GPSDPORT` are compiled out (`HAS_LIBGPS` is not defined in
`mayhem/include/build-config.h`; the image has no libgps).
