
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv) {
    int fdi, fdo, pad;
    unsigned char scr[6912];
    unsigned char dat[50*512];
    int x, y, a, b, a2, b2;
    
    pad = 0;
    
    if (argc > 1) {
        if (!strcmp(argv[1], "-p")) {
            pad = 1;
        } else {
            printf("Usage: %s [-p]\n", argv[0]);
            return 1;
        }
    }
    
    fdi = 0;
    fdo = 1;
    
    if (read(fdi, scr, sizeof(scr)) != sizeof(scr)) {
        printf("Cannot read input\n");
    } else {
        for (x = 0; x < 256; x++) {
            if (pad) {
                dat[x * 100 +  0] = 0x04;
                dat[x * 100 +  1] = 0x00;
                dat[x * 100 + 50] = 0x04;
                dat[x * 100 + 51] = 0x00;
            }
            for (y = 0; y < 192; y++) {
                
                a = (x >> 3) | ((y & 192) << 5) | ((y & 56) << 2) | ((y & 7) << 8);
                b = (128 >> (x & 7));
                
                a2 = (x * ((pad) ? 100 : 96)) + (47 - (y >> 2)) + (2 * pad);
                b2 = (3 << (2 * (y & 3)));
                
                if (scr[a] & b) {
                    dat[a2] |= b2;
                    dat[a2 + ((pad) ? 50 : 48)] |= b2;
                } else {
                    dat[a2] &= (~b2);
                    dat[a2 + ((pad) ? 50 : 48)] &= (~b2);
                }
            }
        }
        if (write(fdo, dat, 512*((pad) ? 50 : 48)) != 512*((pad) ? 50 : 48))
            printf("Cannot write output\n");;
    }
    
    return 0;
}
