#include <stdio.h>
#include <stdint.h>
#include "font.c"

int main(void)
{
    int widthInBytes= font_image.width*font_image.bytes_per_pixel;
    printf("static const unsigned char PROGMEM font[%d]= { \n", font_image.width);
    for(int i= 0; i<font_image.width; i++)
    {
        const char *p= font_image.pixel_data + i*font_image.bytes_per_pixel;
        uint8_t rowbits= 0;
        printf("\t0b");
        for(int bit= 0; bit<8; bit++)
        {
            if(p[bit*widthInBytes]) 
                putchar('1');
            else
                putchar('0');
        }
        puts(",");
    }
    puts("};\n");
    return 0;
}