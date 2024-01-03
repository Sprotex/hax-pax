/*
	Copyright 2001, 2002 Georges Menie (www.menie.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdint.h>         /* For uint32_t definition                        */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "gui.h"
#include "zxem.h"
#include "screens.h"
//#include "ssd1289.h"
//#include <plib.h>

unsigned int flsh_state=0;

// Definitions from pax.c
//extern void putpix(int x, int y, int color);
//extern void putpx(int x, int y, int color);

// Define Spectrum colors
//static const 

uint16_t colors [16] = {
    BLACK,HBLUE,HRED,HMAGENTA,HGREEN,HCYAN,HYELLOW,HWHITE,BLACK,BLUE,RED,MAGENTA,GREEN,CYAN,YELLOW,WHITE
}; 


// Draw one line
void DrawLine(unsigned char l, unsigned char s, unsigned int d) {
// l = line, s = screen
    unsigned int in,pa,x,y,i,j,k,p,m,q,addr;
    unsigned char ink,pap,c,a,fl;

    uint16_t img[2048];
    unsigned char txt[30];

    //for(i=0;i<2048;i++) {
    //    img[i] = colors[l & 0x07];
    //}

    x=START_X;
    y=START_Y + (l*8);

    j = l/8;        // Third
    i = l & 0x07;   // Line in third
    //addr = 0;

    for (p=32;p>0;p--) { // Byte
        //a = screens[6912*s + 6144 + j * 256 + i * 32 + (p-1)];            // Attr byte
	a = memory[16384 + 6912*s + 6144 + j * 256 + i * 32 + (p-1)];            // Attr byte

        ink = a & 0x07;
        pap = ( a >> 3 ) & 0x07;

        if ( (a & 0x40) == 0x40 ) {  // Bright1 - not for black (0)
            ink += 8;
            pap += 8;
        }

        in = colors[ink];
        pa = colors[pap];
		if ((a & 0x80) == 0x80) {
			fl = 1;
		} else {
			fl = 0;
		}
		fl = fl & flsh_state;

        for (k=0;k<8;k++) {      // Microline

        //c = screens[6912*s + j * 2048 + k * 256 + i * 32 + (p-1)];        // Pixel byte
	c = memory[16384 + 6912*s + j * 2048 + k * 256 + i * 32 + (p-1)];        // Pixel byte

            for (q=0;q<8;q++) {                                     // Pixel

                addr = q*8 + (32-p)*64 + k;

                if ( (c & 0x01) == 0x01 ) {
                    img[addr] = fl ? pa : in;
                } else {
                    img[addr] = fl ? in : pa;
                }
                c >>= 1;
            } // end for m

        }
    }
 
    if (d==1) {
		//SSD1289_drawAreaDMA(y,x,8,256,img);
	} else {
		//SSD1289_drawArea(y,x,8,256,img);
	}
}

// Display screen$ on the TFT line by line 
// Input: s - screen number, d - dma mode: 0/1
void Lscr2lcd(unsigned char s, unsigned int d) {

    unsigned char l;
    for(l=0;l<24;l++) {
        DrawLine(l,s,d);
    }
}




// Show Spectrum screen on the display
unsigned char scr2lcd(int w) {
    unsigned char a,ink,pap,pal;
    uint16_t in=0,pa=7;
    unsigned char c;
    unsigned char std=1;
    unsigned int x,y,i,j,k,l,m,pos;
    
    uint16_t palette64[64];
	unsigned int line[256];
    // if w is 0 it shows help screen, otherwise it shows screen in current memory[]
    
    x=START_X;
    y=START_Y;  

    for (j=0;j<3;j++) {              // Screen "third"
        for (i=0;i<8;i++) {          // Line in third
            for (k=0;k<8;k++) {      // Microline
                for (l=32;l>0;l--) { // Byte
                    if (w==0) {
			    c = screens[j * 2048 + k * 256 + i * 32 + (l-1)];        // Pixel byte
		    } else {
			    c = memory[16384 + j * 2048 + k * 256 + i * 32 + (l-1)];
		    }

                    if (w==0) {
			    a = screens[6144 + j * 256 + i * 32 + (l-1)];            // Attr byte
		    } else {
			    a = memory[22528 + j * 256 + i * 32 + (l-1)];            // Attr byte
		    }

                    if (std) {                                              // Handle attributes as ULA
                        ink = a & 0x07;
                        pap = ( a >> 3 ) & 0x07;
                        if ( (a & 0x40) == 0x40 ) {  // Bright1 - not for black (0)
                            ink += 8;
                            pap += 8;
                        }
						//in = colors[ink];
						//pa = colors[pap];
                    } else {                                                // Otherwise treat as ULA+
                        pal = a >> 6;                                       // Palette suffix
                        ink = a & 0x07;
                        pap = ( a >> 3 ) & 0x07;
                        //in = palette64[pal*16+ink];
                        //pa = palette64[pal*16+8+pap]; 
                    } // end if

                    for (m=0;m<8;m++) {                                     // Pixel

                        if ( (c & 0x01) == 0x01 ) {
				putpix(256-x,y,ink);
                        } else {
                            	putpix(256-x,y,pap);
                        } // end if
                        
                        
                        x++;
                        c >>= 1;
                    } // end for m
	
                }
                y++;
                x=START_X;
            }
        }
    }

    //return(0);
}
/*
void fb_box(int x, int y, int dx, int dy, int color) {

	int xx,yy;

	for(xx=x;xx<x+dx;xx++) {
		for(yy=y;yy<y+dy;y++) putpx(xx,yy,color);
	}
}

void DrawBorder(int color) {

	//uint16_t color;

	//color = colors[border];

	fb_box(0,0,319,23,color);	//top
	fb_box(0,239-24,319,24,color); //bottom
	fb_box(319-32,0,32,239,color); 	//right
	fb_box(0,0,31,239,color); 	//left
}

void HandleEvent(void) {
	int i;
	for(i=0;i<256;i++) {
		memory[22528+256+i] = manic_attr[i];
	}
}
*/

void ZXCls(void) 
{

	int x,y;

	for (x=0;x<256;x++) {
		for (y=0;y<192;y++) putpix(x,y,7);
	}
}

void ZXChar(char ch, int x, int y, int Font, int ink, int pap ) 
{

	//unsigned char znak[8] = { 0,16,16,16,16,0,16,0 } ;
	//unsigned char znak[8] = { 0,66,36,24,24,36,66,0 } ;

	unsigned char m,n,c;
 
	Font = 0;
	x+=8;
	for (n=0;n<8;n++) {
		//c = font[(ch-32)*8+n];	// c
		//c = znak[n];
		c = memory[(ch-32)*8+n+15616];	// c
		for (m=0;m<8;m++) {                                     // Pixel
			if ( (c & 0x01) == 0x01 ) {
				putpix(x,y,ink);
			} else {
				putpix(x,y,pap);
			} // end if
                        
			x--;
    		c >>= 1;
    		} // end for m
		y++;
		x+=8;
	} // end for n
	
}

void ZXPrint(char *S, int x, int y, int Font, int ink, int pap ) 
{

  int lenght,cnt;
  char buffer[40];
  lenght = strlen(S);		
  
  	for(cnt = 0; cnt <= (lenght - 1); cnt++)
  	{
    		buffer[cnt] = S[cnt];
  	}

	for(cnt = 0; cnt <= (lenght - 1); cnt++)
	{
		ZXChar(buffer[cnt],x,y,Font,ink,pap);
		x = x + 8;
	}
}

int zxputc(char c) {

    static int x=0;
    static int y=0;

    if (c < ' ' && c != '\r' && c != '\n' && c != '\t' && c != '\b') return 0;

    if (c >= ' ' && c < 128) {
          ZXChar(c,x,y,0,0,7);
          x+=8;
    }

    if (c == '\n' || c == '\r') {
        //New line
        x=0;
        y+=8;
//        if(y>239) y=0;
//        return;
    } 

    if (c=='\t') {
        x+=40;
    }

    if (c == '\b') {
        x-=8;
    }

    if (x<0) {
        x=311;
        y-=8;
        if (y<0) y=231;
    }

    if(x>319) {
        x=0;
        y+=8;
    }

    if(y>239) y=0;
    
    return 0;
}

/*
	putchar is the only external dependency for this file,
	if you have a working putchar, just remove the following
	define. If the function should be called something else,
	replace outbyte(c) by your own function call.
*/
//#define putchar(c) zxputc(c)

static void zxprintchar(char **str, int c)
{
	//extern int putchar(int c);
	//zxputc(c);
	if (str) {
		**str = c;
		++(*str);
	}
	else (void)zxputc(c);
}

#define PAD_RIGHT 1
#define PAD_ZERO 2

static int zxprints(char **out, const char *string, int width, int pad)
{
	register int pc = 0, padchar = ' ';

	if (width > 0) {
		register int len = 0;
		register const char *ptr;
		for (ptr = string; *ptr; ++ptr) ++len;
		if (len >= width) width = 0;
		else width -= len;
		if (pad & PAD_ZERO) padchar = '0';
	}
	if (!(pad & PAD_RIGHT)) {
		for ( ; width > 0; --width) {
			zxprintchar (out, padchar);
			++pc;
		}
	}
	for ( ; *string ; ++string) {
		zxprintchar (out, *string);
		++pc;
	}
	for ( ; width > 0; --width) {
		zxprintchar (out, padchar);
		++pc;
	}

	return pc;
}

/* the following should be enough for 32 bit int */
#define PRINT_BUF_LEN 12

static int zxprinti(char **out, int i, int b, int sg, int width, int pad, int letbase)
{
	char print_buf[PRINT_BUF_LEN];
	register char *s;
	register int t, neg = 0, pc = 0;
	register unsigned int u = i;

	if (i == 0) {
		print_buf[0] = '0';
		print_buf[1] = '\0';
		return zxprints (out, print_buf, width, pad);
	}

	if (sg && b == 10 && i < 0) {
		neg = 1;
		u = -i;
	}

	s = print_buf + PRINT_BUF_LEN-1;
	*s = '\0';

	while (u) {
		t = u % b;
		if( t >= 10 )
			t += letbase - '0' - 10;
		*--s = t + '0';
		u /= b;
	}

	if (neg) {
		if( width && (pad & PAD_ZERO) ) {
			zxprintchar (out, '-');
			++pc;
			--width;
		}
		else {
			*--s = '-';
		}
	}

	return pc + zxprints (out, s, width, pad);
}

static int zxprint(char **out, int *varg)
{
	register int width, pad;
	register int pc = 0;
	register char *format = (char *)(*varg++);
	char scr[2];

	for (; *format != 0; ++format) {
		if (*format == '%') {
			++format;
			width = pad = 0;
			if (*format == '\0') break;
			if (*format == '%') goto out;
			if (*format == '-') {
				++format;
				pad = PAD_RIGHT;
			}
			while (*format == '0') {
				++format;
				pad |= PAD_ZERO;
			}
			for ( ; *format >= '0' && *format <= '9'; ++format) {
				width *= 10;
				width += *format - '0';
			}
			if( *format == 's' ) {
				register char *s = *((char **)varg++);
				pc += zxprints (out, s?s:"(null)", width, pad);
				continue;
			}
			if( *format == 'd' ) {
				pc += zxprinti (out, *varg++, 10, 1, width, pad, 'a');
				continue;
			}
			if( *format == 'x' ) {
				pc += zxprinti (out, *varg++, 16, 0, width, pad, 'a');
				continue;
			}
			if( *format == 'X' ) {
				pc += zxprinti (out, *varg++, 16, 0, width, pad, 'A');
				continue;
			}
			if( *format == 'u' ) {
				pc += zxprinti (out, *varg++, 10, 0, width, pad, 'a');
				continue;
			}
			if( *format == 'c' ) {
				/* char are converted to int then pushed on the stack */
				scr[0] = *varg++;
				scr[1] = '\0';
				pc += zxprints (out, scr, width, pad);
				continue;
			}
		}
		else {
		out:
			zxprintchar (out, *format);
			++pc;
		}
	}
	if (out) **out = '\0';
	return pc;
}

/* assuming sizeof(void *) == sizeof(int) */

int zxprintf(const char *format, ...)
{
	register int *varg = (int *)(&format);
	return zxprint(0, varg);
}

/*
int sprintf(char *out, const char *format, ...)
{
	register int *varg = (int *)(&format);
	return print(&out, varg);
}
*/
#ifdef TEST_PRINTF
int main(void)
{
	char *ptr = "Hello world!";
	char *np = 0;
	int i = 5;
	unsigned int bs = sizeof(int)*8;
	int mi;
	char buf[80];

	mi = (1 << (bs-1)) + 1;
	printf("%s\n", ptr);
	printf("printf test\n");
	printf("%s is null pointer\n", np);
	printf("%d = 5\n", i);
	printf("%d = - max int\n", mi);
	printf("char %c = 'a'\n", 'a');
	printf("hex %x = ff\n", 0xff);
	printf("hex %02x = 00\n", 0);
	printf("signed %d = unsigned %u = hex %x\n", -3, -3, -3);
	printf("%d %s(s)%", 0, "message");
	printf("\n");
	printf("%d %s(s) with %%\n", 0, "message");
	sprintf(buf, "justif: \"%-10s\"\n", "left"); printf("%s", buf);
	sprintf(buf, "justif: \"%10s\"\n", "right"); printf("%s", buf);
	sprintf(buf, " 3: %04d zero padded\n", 3); printf("%s", buf);
	sprintf(buf, " 3: %-4d left justif.\n", 3); printf("%s", buf);
	sprintf(buf, " 3: %4d right justif.\n", 3); printf("%s", buf);
	sprintf(buf, "-3: %04d zero padded\n", -3); printf("%s", buf);
	sprintf(buf, "-3: %-4d left justif.\n", -3); printf("%s", buf);
	sprintf(buf, "-3: %4d right justif.\n", -3); printf("%s", buf);

	return 0;
}

/*
 * if you compile this file with
 *   gcc -Wall $(YOUR_C_OPTIONS) -DTEST_PRINTF -c printf.c
 * you will get a normal warning:
 *   printf.c:214: warning: spurious trailing `%' in format
 * this line is testing an invalid % at the end of the format string.
 *
 * this should display (on 32bit int machine) :
 *
 * Hello world!
 * printf test
 * (null) is null pointer
 * 5 = 5
 * -2147483647 = - max int
 * char a = 'a'
 * hex ff = ff
 * hex 00 = 00
 * signed -3 = unsigned 4294967293 = hex fffffffd
 * 0 message(s)
 * 0 message(s) with %
 * justif: "left      "
 * justif: "     right"
 *  3: 0003 zero padded
 *  3: 3    left justif.
 *  3:    3 right justif.
 * -3: -003 zero padded
 * -3: -3   left justif.
 * -3:   -3 right justif.
 */

#endif

