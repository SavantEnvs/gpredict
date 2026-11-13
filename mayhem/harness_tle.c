/* Fuzzes the TLE reader (Get_Next_Tle_Set) and, for accepted sets, SGP4/SDP4 propagation.
   Input: name line, TLE line 1, TLE line 2 (newline separated), then 3 optional bytes for the observer.
   A blank or tagged name line and degenerate elements reach the library (known-findings/). Inputs shorter than two TLE element lines are rejected: the format fixes each
   line at 69 columns (Good_Elements() reads tle_set[69] as the start of line 2), so nothing shorter holds a
   set, and a longer input with a blank name reaches the same defect. */
#define TLE_PAIR_MIN 138
#include <glib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "gtk-sat-data.h"
#include "predict-tools.h"
#include "qth-data.h"
#include "sgpsdp/sgp4sdp4.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char            lines[3][80];
    size_t          pos = 0;
    int             i;
    sat_t           sat;
    qth_t           qth;
    static const double dt[] = { 0.0, 0.25, 1.0, -1.0, 30.0 };

    if (size < TLE_PAIR_MIN)
        return -1;

    memset(lines, 0, sizeof(lines));
    for (i = 0; i < 3; i++)
    {
        size_t          n = 0;

        while (pos < size && data[pos] != '\n')
        {
            if (n < 79)
                lines[i][n++] = (char)data[pos];
            pos++;
        }
        if (pos < size)
            pos++;
    }

    memset(&sat, 0, sizeof(sat));
    if (Get_Next_Tle_Set(lines, &sat.tle) != 1)
        return -1;

    qth_init(&qth);
    qth.lat = 55.7;
    qth.lon = 12.5;
    qth.alt = 10;
    if (size - pos >= 3)
    {
        qth.lat = ((int8_t)data[pos]) * 90.0 / 128.0;
        qth.lon = ((int8_t)data[pos + 1]) * 180.0 / 128.0;
        qth.alt = data[pos + 2] * 10;
    }

    sat.flags = 0;
    select_ephemeris(&sat);
    gtk_sat_data_init_sat(&sat, &qth);
    for (i = 0; i < (int)(sizeof(dt) / sizeof(dt[0])); i++)
        predict_calc(&sat, &qth, sat.jul_epoch + dt[i]);

    g_free(qth.qra);
    return 0;
}
