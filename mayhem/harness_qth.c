/* Fuzzes the .qth ground station file reader (qth_data_read). */
#include <glib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "qth-data.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    const char     *tmp = getenv("TMPDIR");
    char            path[4096];
    int             fd;
    size_t          off = 0;
    qth_t          *qth;
    gint            ok;

    if (tmp == NULL || *tmp == '\0')
        tmp = "/tmp";
    snprintf(path, sizeof(path), "%s/fuzz-XXXXXX.qth", tmp);
    fd = mkstemps(path, 4);
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

    qth = g_new0(qth_t, 1);
    qth_init(qth);
    ok = qth_data_read(path, qth);
    unlink(path);
    qth_data_free(qth);
    return ok ? 0 : -1;
}
