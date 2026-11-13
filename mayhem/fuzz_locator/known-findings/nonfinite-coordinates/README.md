# NaN or infinite coordinates cast to `int` in `locator.c`

- **Cause:** `dec2dms()` and `longlat2locator()` (`src/locator.c`, around lines 259 and 435) convert
  `floor(fabs(dec))` and `ordinate / square_size` to `int` without checking they are finite.
- **Reachable from:** `fuzz_locator` (the coordinates are raw doubles), and from a user's `.qth` file:
  `qth_data_read()` (`src/qth-data.c:231`) calls `longlat2locator()` with the `LAT`/`LON` it just read, before
  `qth_validate()` zeroes a non-finite value, so `LAT=nan` or `LON=inf` reaches it. `fuzz_qth` can find it that way.
- **Impact:** undefined behaviour (UBSan `float-cast-overflow`), a garbage locator string; no memory error.
- **Upstream fix:** return early (or clamp) when `!isfinite(value)` at the top of both functions.
- **Harness:** neither harness filters these inputs. `fuzz_locator` links `src/locator.c` built with
  `-fno-sanitize=float-cast-overflow` (so do `sgp_math.c` and `sgp_in.c`): raw double coordinates are NaN/Inf often
  enough that, with the check on, about 1% of executions abort (about 1500 crashes a minute) and hide everything
  else. `fuzz_qth` links a second copy of `locator.c` with every UBSan check halting
  (`mayhem/build/locator_strict.o`), because there the value comes from a user's file; it reports the cast when
  a `.qth` has `LAT=nan` or `LON=inf` (the dictionary has `nan` and `inf`; no seed does, so replaying the seeds
  does not crash). ASan and every other UBSan check stay halting everywhere.
- **Reproducer:** `repro.c`; from the repo root, in this directory's parent chain:
  `clang -g -fsanitize=undefined -fno-sanitize-recover=all -I mayhem/include -I src $(pkg-config --cflags gtk+-3.0)
  -DHAVE_CONFIG_H mayhem/fuzz_locator/known-findings/nonfinite-coordinates/repro.c src/locator.c -lm`
  (compiled directly, so the relaxed library build does not hide it).

## Coverage: `fuzz_locator` is near its ceiling

`locator.c` is nine small functions. After 90 s of local fuzzing from the shipped seeds, `llvm-cov` reports
92% of regions (189, 15 missed), 94% of lines and 84% of branches in the file, and 100% for `dms2dec`,
`dmmm2dec`, `distance_long_path` and `azimuth_long_path`. Per function: `locator2longlat` 95%,
`longlat2locator` 93%, `qrb` 91%, `dec2dms` 90%, `dec2dmmm` 79%. What is missed is the NULL-pointer guards (`dec2dms`, `dec2dmmm`, `locator2longlat`, `longlat2locator`, `qrb`)
that a harness passing real pointers cannot reach, and the `qrb` clamp for a latitude of exactly +/-90 (seed
`poles.bin` starts the fuzzer there). Mayhem's low absolute edge count comes from the library's size, not from a
harness that never reached the parser.
