/* Build: see README.md. Aborts under UBSan (null pointer offset): gtk_sat_data_read_sat() on a .sat file with no TLE1 key. */
#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gtk-sat-data.h"

int main(void)
{
    char            root[] = "/tmp/gpredict-repro-XXXXXX";
    char            path[256];
    FILE           *f;
    sat_t          *sat = g_new0(sat_t, 1);

    if (mkdtemp(root) == NULL)
        return 2;
    snprintf(path, sizeof(path), "%s/Gpredict", root);
    g_mkdir(path, 0700);
    snprintf(path, sizeof(path), "%s/Gpredict/satdata", root);
    g_mkdir(path, 0700);
    setenv("XDG_CONFIG_HOME", root, 1);
    snprintf(path, sizeof(path), "%s/Gpredict/satdata/25544.sat", root);
    f = fopen(path, "w");
    fputs("[Satellite]\nVERSION=1.1\nNAME=ISS\nNICKNAME=ISS\nTLE2=2 25544  51.6443 164.0000 0001500\n", f);
    fclose(f);
    return gtk_sat_data_read_sat(25544, sat);
}
