/* Known-answer probe for gpredict's locator, TLE reader, SGP4 and nxjson code (built with normal flags). */
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include "locator.h"
#include "nxjson/nxjson.h"
#include "sgpsdp/sgp4sdp4.h"

static char     tle_json[] =
    "[{\"description\":\"Mode V/u\",\"norad_cat_id\":25544,\"uplink_low\":145990000,"
    "\"alive\":true,\"ratio\":1.5,\"tags\":[\"a\",\"b\"]}]";

int main(void)
{
    double          lon, lat, dist, az;
    char            loc[16];
    char            lines[3][80];
    sat_t           sat;
    const nx_json  *js, *o;
    int             deg, min, sw;
    double          sec;

    if (locator2longlat(&lon, &lat, "JO65") != RIG_OK)
        return 2;
    printf("locator2longlat JO65 = %.4f %.4f\n", lon, lat);
    if (locator2longlat(&lon, &lat, "JO65ab") != RIG_OK)
        return 2;
    printf("locator2longlat JO65ab = %.4f %.4f\n", lon, lat);
    printf("locator2longlat bad rc = %d\n", locator2longlat(&lon, &lat, "ZZ99") == RIG_OK);

    if (longlat2locator(12.5, 55.7, loc, 3) != RIG_OK)
        return 2;
    printf("longlat2locator 12.5 55.7 = %s\n", loc);
    if (longlat2locator(-122.4, 37.8, loc, 2) != RIG_OK)
        return 2;
    printf("longlat2locator -122.4 37.8 = %s\n", loc);

    if (qrb(12.5, 55.7, -122.4, 37.8, &dist, &az) != RIG_OK)
        return 2;
    printf("qrb Copenhagen-SF = %.1f km %.1f deg\n", dist, az);
    printf("dms2dec 12 30 0 = %.6f\n", dms2dec(12, 30, 0.0, 0));
    dec2dms(-96.8333, &deg, &min, &sec, &sw);
    printf("dec2dms -96.8333 = %d %d %.2f %d\n", deg, min, sec, sw);

    memset(lines, 0, sizeof(lines));
    memset(&sat, 0, sizeof(sat));
    strcpy(lines[0], "TEST SAT SGP 001");
    strcpy(lines[1], "1 88888U          80275.98708465  .00073094  13844-3  66816-4 0     9");
    strcpy(lines[2], "2 88888  72.8435 115.9689 0086731  52.6988 110.5714 16.05824518   103");
    printf("Get_Next_Tle_Set rc = %d\n", Get_Next_Tle_Set(lines, &sat.tle));
    printf("tle catnr = %d inclination = %.4f xno = %.8f\n", sat.tle.catnr, sat.tle.xincl, sat.tle.xno);
    select_ephemeris(&sat);
    SGP4(&sat, 0.0);
    Convert_Sat_State(&sat.pos, &sat.vel);
    printf("sgp4 t=0 pos = %.4f %.4f %.4f\n", sat.pos.x, sat.pos.y, sat.pos.z);
    lines[1][68] = '0';
    printf("bad checksum rc = %d\n", Get_Next_Tle_Set(lines, &sat.tle));

    js = nx_json_parse(tle_json, 0);
    if (js == NULL)
        return 2;
    o = nx_json_item(js, 0);
    printf("json length = %d\n", js->length);
    printf("json description = %s\n", nx_json_get(o, "description")->text_value);
    printf("json norad = %lld\n", nx_json_get(o, "norad_cat_id")->int_value);
    printf("json uplink_low = %lld\n", nx_json_get(o, "uplink_low")->int_value);
    printf("json alive = %lld\n", nx_json_get(o, "alive")->int_value);
    printf("json ratio = %.2f\n", nx_json_get(o, "ratio")->dbl_value);
    printf("json tags = %d %s\n", nx_json_get(o, "tags")->length,
           nx_json_item(nx_json_get(o, "tags"), 1)->text_value);
    printf("json missing type = %d\n", (int)nx_json_get(o, "nope")->type);
    nx_json_free(js);
    return 0;
}
