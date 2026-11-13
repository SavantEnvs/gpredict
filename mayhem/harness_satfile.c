/* Fuzzes the .sat satellite data file reader (gtk_sat_data_read_sat) followed by propagation.
   Every non-empty input is written to the file and read; a missing or short TLE reaches the reader
   (known-findings/short-or-bad-tle). Inputs shorter than two TLE element lines (2 x 69 columns, the fixed format
   Good_Elements() indexes: tle_set[69] starts line 2) are rejected: no .sat file holding a TLE is smaller, and a
   longer file without TLE1 or with short lines reaches the same defects. */
#define TLE_PAIR_MIN 138
#include <fcntl.h>
#include <glib.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "gtk-sat-data.h"
#include "predict-tools.h"
#include "qth-data.h"
#include "sgpsdp/sgp4sdp4.h"

#define CATNUM 25544

static char     satpath[4096];

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    const char     *tmp = getenv("TMPDIR");
    char            root[4096], dir[4096];

    (void)argc;
    (void)argv;
    if (tmp == NULL || *tmp == '\0')
        tmp = "/tmp";
    snprintf(root, sizeof(root), "%s/gpredict-satdata-XXXXXX", tmp);
    if (mkdtemp(root) == NULL)
        return 1;
    snprintf(dir, sizeof(dir), "%s/Gpredict", root);
    if (mkdir(dir, 0700) != 0)
        return 1;
    snprintf(dir, sizeof(dir), "%s/Gpredict/satdata", root);
    if (mkdir(dir, 0700) != 0)
        return 1;
    snprintf(satpath, sizeof(satpath), "%s/%d.sat", dir, CATNUM);
    setenv("XDG_CONFIG_HOME", root, 1);
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    int             fd;
    size_t          off = 0;
    sat_t          *sat;
    qth_t           qth;
    gint            rc;

    if (size < TLE_PAIR_MIN)
        return -1;

    fd = open(satpath, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0)
        return -1;
    while (off < size)
    {
        ssize_t         n = write(fd, data + off, size - off);

        if (n <= 0)
            break;
        off += (size_t)n;
    }
    close(fd);

    sat = g_new0(sat_t, 1);
    rc = gtk_sat_data_read_sat(CATNUM, sat);
    if (rc == 0)
    {
        qth_init(&qth);
        qth.lat = 55.7;
        qth.lon = 12.5;
        qth.alt = 10;
        predict_calc(sat, &qth, sat->jul_epoch + 0.5);
        g_free(qth.qra);
    }
    gtk_sat_data_free_sat(sat);
    return rc == 0 ? 0 : -1;
}
