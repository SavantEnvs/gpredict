# `gtk_sat_data_read_sat` trusts the `TLE1`/`TLE2` keys of a `.sat` file

- **Cause:** `src/gtk-sat-data.c`/`src/sgpsdp/sgp_in.c` (`gtk_sat_data_read_sat`) builds the TLE with
  `g_strconcat(TLE1, TLE2, NULL)`. With `TLE1` missing the result is NULL and `Good_Elements(NULL)` dereferences
  it. With short lines `Good_Elements`/`Convert_Satellite_Data` read at fixed offsets (columns up to 138) past the
  end of the string. After a failed `Good_Elements` the function carries on with zeroed or stale elements.
- **Impact:** NULL dereference or heap over-read when gpredict loads a damaged `~/.config/Gpredict/satdata/*.sat`.
- **Upstream fix:** return an error from `gtk_sat_data_read_sat` when either key is missing or
  `strlen(TLE1) + strlen(TLE2) < 138`, and when `Good_Elements` fails.
- **Harness:** `fuzz_satfile` writes every input to the `.sat` path, calls `gtk_sat_data_read_sat` and propagates when
  it returns 0; the only rejected inputs are those under 138 bytes (the fixed 2 x 69-column TLE pair), so the NULL dereference and the over-read are reported by the target (a file
  without `TLE1` already triggers the first).
- **Reproducer:** `repro.c`, linked against `mayhem/build/libgpredict.a` (see the `mayhem/build.sh` link line).
