# Blank or tagged name line in `Get_Next_Tle_Set`

- **Cause:** `src/sgpsdp/sgp_in.c` (`Get_Next_Tle_Set`) strips trailing spaces from the name with
  `idx = strlen(name) - 1; while (name[idx] == ' ') idx--`, which runs below index 0 when the name line is
  empty or all spaces (`line[0][-1]`). Separately, a name that starts with a status tag such as `[+]`
  writes `tle->sat_name[idx - 1]` with `idx == 0`.
- **Impact:** one-byte stack/heap underflow read and write while loading a TLE file whose first line is
  blank or starts with `[+]`, `[-]`, `[P]`, `[B]`, `[S]` or `[X]`.
- **Upstream fix:** guard the strip loop with `idx > 0 &&` (`while (idx > 0 && name[idx] == ' ')`) and the tag
  branch with `idx > 0`.
- **Harness:** `fuzz_tle` feeds the first input line to `Get_Next_Tle_Set` as the name line, unmodified, so the
  fuzzer reports this defect (any input of 138 bytes or more with an empty first line triggers it).
- **Reproducer:** `repro.c`, linked against `mayhem/build/libgpredict.a` (built by `mayhem/build.sh`).
