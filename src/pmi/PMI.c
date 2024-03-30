
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "img.h" // soubor s hexadecimálními daty obrázku klávesnice PMI

int _init()
{
    int fbfd = 0;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize = 0;
    char *fbp = 0;
    char *framebuffer;
    int x = 0, y = 0;
    long int location = 0;
    long int framebuffer_pos;

    // Open the file for reading and writing
    fbfd = open("/dev/fb", O_RDWR);
    if (fbfd == -1)
    {
        perror("Error: cannot open framebuffer device");
        exit(1);
    }
    printf("The framebuffer device was opened successfully.\n");

    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        perror("Error reading fixed information");
        exit(2);
    }

    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        perror("Error reading variable information");
        exit(3);
    }

    // printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    // Figure out the size of the screen in bytes
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    // Map the device to memory
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((int)fbp == -1)
    {
        perror("Error: failed to map framebuffer device to memory");
        exit(4);
    }

    // Figure out where in memory to put the picure
    for (y = 00; y < 240; y++)
        for (x = 00; x < 260; x++)
        {
            location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) +
                       (y + vinfo.yoffset) * finfo.line_length;
            {
                // assume 16bpp
                int b = 64 - (y - 100) / 16;                                                                                        // 100;
                int g = (x - 100) / 6;                                                                                              // A little green
                int r = x;                                                                                                          // 31-(y-100)/16;    // A lot of red
                int picturepos = ((260 - x) * 240 + y);                                                                             // 6 0 0
                unsigned short int t = img_data[picturepos * 3] << 6 | img_data[picturepos * 3 + 1] | img_data[picturepos * 3 + 2]; // r<<11 | g << 5 | b;
                *((unsigned short int *)(fbp + location)) = t;
            }
        }

    // prostor pro display je y:0-240 a x:260-320
    for (int seq = 0; seq < 9; seq++)
        for (int delkaseg = 0; delkaseg < 19; delkaseg++)
            for (int tloustkaseg = 0; tloustkaseg < 4; tloustkaseg++)
            {
                y = delkaseg;
                x = tloustkaseg + 311; // posun prvního segmentu od 0/0
                int r = 254;
                unsigned short int t = r << 11 | 0 | 0;                       // ostrá červená
                location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) + // horní řada segmentu 06-25:19pix x 311-315:4pix
                           ((seq * 27) + y + vinfo.yoffset) * finfo.line_length;
                *((unsigned short int *)(fbp + location)) = t;

                location = (x - 20 + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) + // střední řada segmentů
                           ((seq * 27) + y + vinfo.yoffset) * finfo.line_length;
                *((unsigned short int *)(fbp + location)) = t;

                location = (x - 40 + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) + // dolní řada segmentů
                           ((seq * 27) + y + vinfo.yoffset) * finfo.line_length;
                *((unsigned short int *)(fbp + location)) = t;

                x = delkaseg + 311;
                y = tloustkaseg;                                                   // posun prvního segmentu od 0/0
                location = (x - 40 + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) + // první dolní řada segmentů
                           ((seq * 27) + y + vinfo.yoffset) * finfo.line_length;
                *((unsigned short int *)(fbp + location)) = t;
                location = (x - 20 + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) + // první horní řada segmentů
                           ((seq * 27) + y + vinfo.yoffset) * finfo.line_length;
                *((unsigned short int *)(fbp + location)) = t;

                location = (x - 18 + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) + // druhá horní řada segmentů
                           ((seq * 27) + y + 18 + vinfo.yoffset) * finfo.line_length;
                *((unsigned short int *)(fbp + location)) = t;
                location = (x - 20 - 18 + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) + // druhá horní řada segmentů
                           ((seq * 27) + y + 18 + vinfo.yoffset) * finfo.line_length;
                *((unsigned short int *)(fbp + location)) = t;
            }

    munmap(fbp, screensize);
    sleep(10);

    // Uzavřít framebuffer

    close(fbfd);
    return 0;
}
