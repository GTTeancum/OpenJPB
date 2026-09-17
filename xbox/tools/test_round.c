#include <math.h>
#include <stdio.h>
extern long jpb_XboxRoundLong(float value);
int main(void)
{
    static const struct { float input; long output; } cases[] = {
        {0,0}, {0.49f,0}, {-0.49f,0}, {0.5f,1}, {-0.5f,-1},
        {1.5f,2}, {-1.5f,-2}, {2047.5f,2048}, {-2047.5f,-2048},
        {8388609.0f,8388609}, {-8388609.0f,-8388609}
    };
    long (*volatile round_function)(float) = jpb_XboxRoundLong;
    for (unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        if (round_function(cases[i].input)!=cases[i].output) {
            fprintf(stderr,"rounding case %u failed\n",i); return 1;
        }
    }
    return 0;
}
