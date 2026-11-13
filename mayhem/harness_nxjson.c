/* Fuzzes the bundled nxjson parser the way trsp-update.c uses it (parse, then walk by key/index). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "nxjson/nxjson.h"

static void walk(const nx_json *js, int depth)
{
    const nx_json  *c;

    if (js == NULL || depth > 64)
        return;
    (void)nx_json_get(js, "name");
    (void)nx_json_get(js, "norad_cat_id");
    (void)nx_json_item(js, 0);
    (void)nx_json_item(js, js->length);
    for (c = js->child; c != NULL; c = c->next)
        walk(c, depth + 1);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char           *buf;
    const nx_json  *js;

    buf = malloc(size + 1);
    if (buf == NULL)
        return -1;
    memcpy(buf, data, size);
    buf[size] = '\0';
    js = nx_json_parse(buf, 0);
    if (js == NULL)
    {
        free(buf);
        return -1;
    }
    walk(js, 0);
    nx_json_free(js);
    free(buf);

    buf = malloc(size + 1);
    if (buf == NULL)
        return 0;
    memcpy(buf, data, size);
    buf[size] = '\0';
    js = nx_json_parse_utf8(buf);
    if (js != NULL)
    {
        walk(js, 0);
        nx_json_free(js);
    }
    free(buf);
    return 0;
}
