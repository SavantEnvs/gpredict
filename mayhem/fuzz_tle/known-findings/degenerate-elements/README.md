# NaN or huge doubles cast to `int` in the SGP4/SDP4 helpers

- **Cause:** `FMod2p()` (`src/sgpsdp/sgp_math.c:171`) does `int i = x / twopi;`, and the TLE reader
  (`src/sgpsdp/sgp_in.c`, the revolution-number field) assigns `g_ascii_strtod()` to an `int`. Both overflow the
  `int` range when a TLE field is garbage (`nan`, `inf`, `1e74`) or when an absurd drag term (`bstar` around 0.66,
  `ndot` around 0.07) decays the orbit to NaN within days. Mean motion 0 divides by zero first.
- **Impact:** undefined behaviour (UBSan `float-cast-overflow`); on x86 the result is a garbage angle, never a
  memory error. `Good_Elements()` only checks the line checksum, so a corrupted but checksum-valid TLE reaches it.
- **Upstream fix:** reject non-finite or out-of-range elements in `Good_Elements()`/`Convert_Satellite_Data()`
  and write `FMod2p` with `fmod()`: `ret_val = fmod(x, twopi); if (ret_val < 0) ret_val += twopi;`.
- **Harness:** `fuzz_tle` and `fuzz_satfile` propagate every element set the reader accepts; nothing is range
  checked. `mayhem/build.sh` compiles only `sgp_math.c` and `sgp_in.c` with `-fno-sanitize=float-cast-overflow`
  (ASan and every other UBSan check stay halting): garbage fields turn NaN into a garbage angle on a large share of
  inputs, and the result is a wrong number, not memory corruption. Zero mean motion and the other
  divide-by-zero cases are not UBSan checks and still reach the harness.
- **Reproducer:**
  `clang -g -fsanitize=undefined -fno-sanitize-recover=all -I src -I /usr/include/glib-2.0 repro.c -lm`
  (from the repo root, run in `mayhem/fuzz_tle/known-findings/degenerate-elements`).
