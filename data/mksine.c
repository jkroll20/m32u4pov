#define _GNU_SOURCE // to get M_PI with -std=c99.
#include <stdio.h>
#include <stdint.h>
#include <math.h>

int main(void)
{
    // make a sine table of first quadrant. sine values are unsigned 00..FF
    int sin_width= 0x100;
    printf("static const unsigned char /* PROGMEM */ sintab[0x100]= { \n");
    for(int i= 0; i<sin_width; i++)
    {
        double s= sin(i*(M_PI/2)/sin_width);
        printf("\t0x%02X, /* %.02f */\n", (int)((s*0.5+0.5)*0x100), s);
    }
    puts("};\n");
    return 0;
}