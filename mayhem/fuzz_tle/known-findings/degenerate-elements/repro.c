/* Build: see README.md. UBSan float-cast-overflow abort in FMod2p(), compiled from upstream source. */
#include <math.h>
#include <stdio.h>
#include "sgpsdp/sgp_math.c"

int main(void)
{
    volatile double nan_value = NAN;

    printf("%f\n", FMod2p(nan_value));
    return 0;
}
