/* Build: see README.md. Aborts under UBSan (index -1 out of bounds) in Get_Next_Tle_Set. */
#include <string.h>
#include "sgpsdp/sgp4sdp4.h"

int main(void)
{
    char            lines[3][80];
    tle_t           tle;

    memset(lines, 0, sizeof(lines));
    memset(lines[0], ' ', 3);   /* blank name line */
    strcpy(lines[1], "1 25544U 98067A   20194.88612269  .00002001  00000-0  44289-4 0  9991");
    strcpy(lines[2], "2 25544  51.6443 164.0000 0001500  95.0000 265.0000 15.49300000    05");
    return Get_Next_Tle_Set(lines, &tle) == 1 ? 0 : 1;
}
