/* Build: see README.md. Aborts under ASan: double free in qth_data_read() when ALT is missing. */
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include "qth-data.h"

int main(void)
{
    char            path[] = "/tmp/gpredict-repro-XXXXXX.qth";
    int             fd = mkstemps(path, 4);
    FILE           *f = fdopen(fd, "w");
    qth_t          *qth = g_new0(qth_t, 1);

    fputs("[QTH]\nLOCATION=Test\nLAT=55.5\nLON=13.0\n", f);
    fclose(f);
    qth_init(qth);
    qth_data_read(path, qth);
    remove(path);
    return 0;
}
