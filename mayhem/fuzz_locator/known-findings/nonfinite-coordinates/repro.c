/* Build: see README.md. UBSan float-cast-overflow abort in dec2dms() with a NaN coordinate. */
#include <glib.h>
#include <math.h>
#include "locator.h"

int main(void)
{
    volatile double nan_value = NAN;
    char            loc[16];
    double          lon = nan_value, lat = 10.0;
    int             d, m, sw;
    double          s;

    dec2dms(lon, &d, &m, &s, &sw);
    longlat2locator(lon, lat, loc, 3);
    return 0;
}
