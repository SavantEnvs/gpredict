/* Fuzzes the Maidenhead locator and great-circle helpers in locator.c.
   Layout: inputs shorter than NUM_BYTES are a locator string, fed straight to locator2longlat();
   longer inputs carry 4 doubles and 4 bytes (pair count, degrees, minutes, hemisphere) up front (coordinates, any value including
   NaN/Inf, see known-findings/nonfinite-coordinates) and the rest is the locator string. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "locator.h"

#define NUM_BYTES (4 * sizeof(double) + 4)
#define LOC_PAIRS 6 /* locator.c handles 1 to 6 lon/lat pairs (MAX_LOCATOR_PAIRS) */

static void convert_locator(const uint8_t *s, size_t n)
{
    double          lon, lat;
    char            loc[16];
    char           *str = malloc(n + 1);

    if (str == NULL)
        return;
    memcpy(str, s, n);
    str[n] = '\0';
    if (locator2longlat(&lon, &lat, str) == RIG_OK)
        (void)longlat2locator(lon, lat, loc, (int)(strlen(str) / 2));
    free(str);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    double          v[4];
    double          dist, az;
    char            loc[16];
    int             deg, min, sw, pairs;
    double          sec, mins;

    if (size == 0)
        return -1;
    if (size < NUM_BYTES)
    {
        convert_locator(data, size);
        return 0;
    }

    memcpy(v, data, sizeof(v));
    pairs = data[sizeof(v)];
    convert_locator(data + NUM_BYTES, size - NUM_BYTES);

    (void)longlat2locator(v[0], v[1], loc, pairs);
    (void)longlat2locator(v[2], v[3], loc, 1 + pairs % LOC_PAIRS);
    if (qrb(v[0], v[1], v[2], v[3], &dist, &az) == RIG_OK)
    {
        (void)distance_long_path(dist);
        (void)azimuth_long_path(az);
    }
    (void)dec2dms(v[0], &deg, &min, &sec, &sw);
    (void)dec2dmmm(v[1], &deg, &mins, &sw);
    (void)dms2dec((int8_t)data[sizeof(v) + 1], (int8_t)data[sizeof(v) + 2], v[2], data[sizeof(v) + 3] & 1);
    (void)dmmm2dec((int8_t)data[sizeof(v) + 1], v[3], data[sizeof(v) + 3] & 1);
    return 0;
}
