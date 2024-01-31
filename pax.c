/*
 This is really ugly PAX-specific ZX emulation code.
 
 No sound yet, otherwise should be working. 
*/
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/soundcard.h>

#include <stdint.h>
#include <string.h>

#include "zxem.h"
#include "gui.h"

static const char default_rtc[] = "/dev/rtc0";

static int keypad_fd = -1; // keypad input device
static int printer_fd = -1; // printer output device
static int dsp_fd = -1;   // sound device, not finished yet
static int touchpad_fd = -1;
int realx = 0; 
int realy = 0; 
int touch_up = 0;
struct input_event ev0[64]; 
int cs = 0;		// Toggle for Caps Shift
int ss = 0;		// Toggle for Symbol Shift
int rtc_fd = -1;
unsigned short *fblines; // memory mapped framebuffer

void printscreen() {
  unsigned char prnbuf[2+48*192];
  int i;
  unsigned char dat[50*512];
  int x, y, a, b, a2, b2;
  int pad = 0;
  int pade = 50;
  
  prnbuf[0] = 0x04;
  prnbuf[1] = 0x00;
  memset(prnbuf + 2, 0, sizeof(prnbuf)-2);
 
  if (prtsz == 2) { 
  //ub880d
  /**/
         for (x = 0; x < 256; x++) {
            if (pad) {
                dat[x * 100 +  0] = 0x04;
                dat[x * 100 +  1] = 0x00;
                dat[x * 100 + pade] = 0x00;
                dat[x * 100 + pade +1] = 0x00;
                
            }
            for (y = 0; y < 192; y++) {
                
                a = (x >> 3) | ((y & 192) << 5) | ((y & 56) << 2) | ((y & 7) << 8);
                b = (128 >> (x & 7));
                
                a2 = (x * ((pad) ? 100 : 96)) + (49 - (y >> 2)) + (2 * pad);
                b2 = (3 << (2 * (y & 3)));
                
                if (memory[0x4000 + a ] & b ) {    // (scr[a] & b)
                    dat[a2] |= b2;
                    dat[a2 + ((pad) ? pade : 48)] |= b2;
                } else {
                    dat[a2] &= (~b2);
                    dat[a2 + ((pad) ? pade : 48)] &= (~b2);
                }
            }
        }
        write(printer_fd, dat, 512*((pad) ? pade : 48)) ; //!= 512*((pad) ? 50 : 48));
  }
        
// MHI puvodni tisk jednou teckou
  if (prtsz == 1) { 
  
  
  
  for (i=0;i<192;i++) {
    memcpy(&prnbuf[2+48*i + 0], &memory[0x4000 | (((i>>3)&7)<<5)| (((i>>0)&7)<<8) | (((i>>6)&3)<<11) ], 32); //was 6
  }
    
  write(printer_fd, prnbuf, 2+192*48);
  
  }  
}



/*
 Handle Linux keyboard events naturally (not for emulator)
*/
int
inkey()
{
  struct input_event ev0;
  int rd;
  rd = read (keypad_fd, &ev0, sizeof (struct input_event));
  if (rd < sizeof (struct input_event))
    return 0;

  if (ev0.type != 1)
    return -1;


  // some paxes return first event with weird value ... discard this one
  if (ev0.value != 0 && ev0.value != 1)
    return -1;

   printf("Key code: %d \n",ev0.code);

   if (ev0.value ==1 ) { 
  	return ev0.code;
   } else {
	return 0;
   }

/* Return codes:
  2 - key 1
  3 - key 2
  4 - key 3
  5 - key 4
  6 - key 5
  7 - key 6
  8 - key 7
  9 - key 8
 10 - key 9
 11 - key 0
 69 - Alpha
102 - Func 
 14 - Back (YELLOW)
 28 - Enter (GREEN)
223 - ESC (red)
*/
}

/*
 Handle Linux input events. this maps keystrokes to ZX spectrum keylines
*/
void
event()
{
  struct input_event ev0;
  int rd;
  rd = read (keypad_fd, &ev0, sizeof (struct input_event));
  if (rd < sizeof (struct input_event))
    return;
    
  if (ev0.type != 1)
    return;


  // some paxes return first event with weird value ... discard this one
  if (ev0.value != 0 && ev0.value != 1)
    return;
    
  // xxx there are probably many missing keys for manic miner 
  /* VHB 
       line4 - 09876
       line5 - POIUY
       line6 - enLKJH
       line7 -  ssMNB
       line3 - 12345
       line2 - QWERT
       line1 - ASDFG
       line0 - csZXCV      
  */
  switch (ev0.code)
    {
    case 223: // X / ESC key
      emurun=0; //exit (1);  // END of emulator
      break;
      
    case 2:  //1
      if (ev0.value == 1) {
	if (kbdjoy == 0) kbdlines[3] = ~(1 << 0);/* 1 */
        if ( (kbdjoy == 1) && (rotlcd == 0) ) joyval |= 0x02;
	} else {
	if (kbdjoy == 0) kbdlines[3] = 0xff;
        if ( (kbdjoy == 1) && (rotlcd == 0) ) joyval &= ~0x02;
	}
      break;
      printf ("Key 1\n"); // copied from a test app so as you know what # means what key
      break;
    case 3:   //2
      if (ev0.value == 1)
	kbdlines[3] = ~(1 << 1);/* 2 */
      else
	kbdlines[3] = 0xff;
      break;
    case 4:    //3
       if (ev0.value == 1)
	kbdlines[3] = ~(1 << 2);/* 3 */
      else
	kbdlines[3] = 0xff;
      break;
    case 5: // "4"
       if (ev0.value == 1) {
	if (kbdjoy == 0) kbdlines[3] = ~(1 << 3);/* 4 */
        if ( (kbdjoy == 1) && (rotlcd == 0) ) joyval |= 0x04;
      	} else {
	if (kbdjoy == 0) kbdlines[3] = 0xff;
        if ( (kbdjoy == 1) && (rotlcd == 0) ) joyval &= ~0x04;
	}
      break;
    case 6: // "5"
       if (ev0.value == 1) {
	if (kbdjoy == 0) kbdlines[3] = ~(1 << 4);/* 5 */
        if (kbdjoy == 1) joyval |= 0x08;
       } else {
	if (kbdjoy == 0) kbdlines[3] = 0xff;
        if (kbdjoy == 1) joyval &= ~0x08;
       }
      break;
    case 7: // "6"
       if (ev0.value == 1)
	kbdlines[4] = ~(1 << 4);/* 6 */
      else
	kbdlines[4] = 0xff;
      break;
    case 8: // 7
      if (ev0.value == 1) {
	if (kbdjoy == 0) kbdlines[4] = ~(1 << 3);/* 7 */
        if ( (kbdjoy == 1) && (rotlcd == 1) ) joyval |= 0x02;
	if ( (kbdjoy == 1) && (rotlcd == 0) ) joyval |= 0x01;
      } else {
	if (kbdjoy == 0) kbdlines[4] = 0xff;
        if ( (kbdjoy == 1) && (rotlcd == 1) ) joyval &= ~0x02;
	if ( (kbdjoy == 1) && (rotlcd == 0) ) joyval &= ~0x01;
      }
      break;
    case 9: // 8
      if (ev0.value == 1) {

	if (kbdjoy == 0) kbdlines[4] = ~(1 << 2);	/* 8 */
        if ( (kbdjoy == 1) && (rotlcd == 1) ) joyval |= 0x04;
      } else {
	if (kbdjoy == 0) kbdlines[4] = 0xff;
        if ( (kbdjoy == 1) && (rotlcd == 1) )  joyval &= ~0x04;
      }
      break;
     case 10: // 9
      if (ev0.value == 1) {
	if (kbdjoy == 0) kbdlines[4] = ~(1 << 1);	/* 9 */
	if ( (kbdjoy == 1) && (rotlcd == 1) ) joyval |= 0x01;
      } else {
	if (kbdjoy == 0) kbdlines[4] = 0xff;
	if ( (kbdjoy == 1) && (rotlcd == 1) )  joyval &= ~0x01;
      }
      break;
     case 11: // 0
      if (ev0.value == 1)
	kbdlines[4] = ~(1 << 0);	/* 0 */
      else
	kbdlines[4] = 0xff;
      break;
    case 102: // func
      //emurun=0;
      //break;
      if (ev0.value == 1)
//      kbdlines[6] = ~(1 << 2);	// * K .. LIST 
      kbdlines[2] = ~(1 << 3);		// * R .. RUN 
//	kbdlines[7] = ~(1 << 0);	// * SPACE 
      else
	kbdlines[2] = 0xff;
      break;
    case 14:   // yellow <
      if (ev0.value == 1) {
	      if (prtsz > 0)  {
		      printscreen();
	      } else {
		      kbdlines[7] = ~(1 << 0);
		      kbdlines[0] = ~(1 << 0);
	      }
	      
      } else {
	      kbdlines[7] = 0xff;
	      kbdlines[0] = 0xff;
      }
      break;
    case 28: // enter 
      if (ev0.value == 1) {
	if (kbdjoy == 0) kbdlines[6] = ~(1 << 0);	/* enter */
	if (kbdjoy == 1) joyval |= 0x10;
      } else {
	if (kbdjoy == 0) kbdlines[6] = 0xff;
	if (kbdjoy == 1) joyval &= ~0x10;
      }
      break;
    case 69:   // alpha
      if (ev0.value == 1) {
	if (kbdjoy == 0) kbdlines[5] = ~(1 << 4);	/* y */
	if (kbdjoy == 1) kbdlines[6] = ~(1 << 0);	/* Enter */
      } else {
	if (kbdjoy == 0) kbdlines[5] = 0xff;
	if (kbdjoy == 1) kbdlines[6] = 0xff;
      }
      break;
    default:
        printf ("Key XX\n"); // copied from a test app so as you know what # means what key
      break;
    }                 
}

#define SNDBUF_SIZE 512  // to be adjusted xxx fixme ikon
unsigned char sndbuf[SNDBUF_SIZE];
int sndstate;

//Synthesize Output Loop
void dsp_sound_synth(void) {

  int nwritten;

  memset(sndbuf, sndstate ? 0x3f : 0x00, SNDBUF_SIZE);

  //Send to dsp
  nwritten = write(dsp_fd, sndbuf, SNDBUF_SIZE );
  printf("SND dbg %d \n",nwritten);
}

/*
 This handles keyboard. Originally this was for X11
*/
void
handle_x()
{
  fd_set rfds;
  struct timeval tv;
  int retval;
  
  //dsp_sound_synth();

  /* Watch stdin (fd 0) to see when it has input. */
  FD_ZERO (&rfds);
  FD_SET (keypad_fd, &rfds);
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  retval = select (1, &rfds, NULL, NULL, &tv);

  if (retval == -1)
    perror ("select()");

  else if (retval)
    event();

 
  event();

  return;
}

/*
Touchscreen handling routine 
*/


void touch_event() {

    int rd, i, c, r;

	 rd = read(touchpad_fd, ev0, sizeof(struct input_event) * 64);

  	if (rd < sizeof (struct input_event))
    		return;

         //fprintf(stderr,"sizeof(struct input_event) = %d\n", sizeof(struct input_event));

	 for (i = 0; i < rd / sizeof(struct input_event); i++) {
         	  fprintf(stderr,"event(%d): type: %d; code: %3d; value: %3d; realx: %3d; realy: %3d\n", i, ev0[i].type, ev0[i].code, ev0[i].value, realx, realy);
	 	  if (ev0[i].type == 3 && ev0[i].code == ABS_X) { 
			  realx = ev0[i].value;
		  } else if (ev0[i].type == 3 && ev0[i].code == ABS_Y) {
			  realy = ev0[i].value;
		  }

		  /* Determine if touch is pressed or released */
		  if (ev0[i].type == 1 && ev0[i].code == 330) {
			  if (ev0[i].value == 1) {
				  touch_up = 0;		// Pressed
			  } else {
			  	  touch_up = 1;		// Released
			  }
		  }
	}
	// Left half of kbd	
	for(c=0;c<5;c++) {

	   for(r=0;r<4;r++) {
	   /* 1 - 5 */
	      if ( (realx < 128-r*32) && (realx > 96-r*32) && (realy < 24+24*c) && (realy > 24*c) ) {
		 if (c == 0 && r == 3) {
			 // Skip Caps Shift
		 } else { 
	         if (touch_up == 0) {
	      	    // Press
	     	    if (r==3 && cs==1) {
			    kbdlines[3-r] = ~(1 << c) & ~(1 << 0);/* 1 */
		    } else {
			    kbdlines[3-r] = ~(1 << c);/* 1 */
		    }
	          } else {
	      	    // Release
		    if (r==3 && cs==1) {
			    kbdlines[3-r] = ~(1 << 0);/* CS */
		    } else {
	       	    	    kbdlines[3-r] = 0xff;
	          }
		 }
	      }
	     }
	   }
	}
	// Right half of kbd	
	for(c=0;c<5;c++) {

	   for(r=0;r<4;r++) {
	   /* 6 - 0 */
	      if ( (realx < 128-r*32) && (realx > 96-r*32) && (realy < 240-24*c) && (realy > 216-(24*c)) ) {
		 if (c == 1 && r == 3) {
			 // Skip Symbol Shift
		 } else {
	         if (touch_up == 0) {
	      	    // Press
		    if (r==3 && ss==1) {
	     	    	kbdlines[4+r] = ~(1 << c) & ~(1 << 1);/* 0 */
		    } else {
	     	    	kbdlines[4+r] = ~(1 << c);/* 0 */
		    }
	          } else {
	      	    // Release
		    if (r==3 && ss==1) {
			kbdlines[4+r] = ~(1 << 1);
		    } else {
	       	    	kbdlines[4+r] = 0xff;
		    }
	          }
		 }
	      }
	   }
	}
	/* Handle Caps shift */
	if ( (realx < 32) && (realx > 0) && (realy < 24) && (realy > 0) ) {

		if (cs == 0) {
	  	    if (touch_up == 0) {
	     	    	kbdlines[0] = ~(1 << 0);/* CS  */
		    	cs++;
		    	ZXChr('c',240-12,320-18,1,2,6);
		    }
		} else {
		    if (touch_up == 0) {
			kbdlines[0] = 0xff;
                        cs--;
		        ZXChr('c',240-12,320-18,1,0,7);
		     }
		}
	}
	/* Handle Symbol shift */
	if ( (realx < 32) && (realx > 0) && (realy < 240-24) && (realy > 240-48) ) {

		if (ss == 0) {
	  	    if (touch_up == 0) {
	     	    	kbdlines[7] = ~(1 << 1);/* SS  */
		    	ss++;
		    	ZXChr('s',36,320-18,1,2,6);
		    }
		} else {
		    if (touch_up == 0) {
			kbdlines[7] = 0xff;
                        ss--;
		        ZXChr('s',36,320-18,1,0,7);
		     }
		}
	}

}

void handle_event() {

    fd_set rfds;
    struct timeval tv;
    int retval;


   /* Watch stdin (fd 0) to see when it has input. */
    FD_ZERO(&rfds);
    FD_SET(touchpad_fd, &rfds);

   /* Wait up to five seconds. */
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    retval = select(1, &rfds, NULL, NULL, &tv);
    /* Don't rely on the value of tv now! */
  	if (retval == -1)
    		perror ("select()");

	else if (retval)
		touch_event();
	 

	touch_event();
}

// 565 LCD
#define RGB(r,g,b)  (((r&0x1F)<<11) | ((g&0x2F)<<5) | ((b&0x1F))) 
/* black, blue, red, magenta, green, cyan, yellow and white */
//const unsigned short speccolors[16] = {RGB(0,0,0), RGB(0,0,0xE0), RGB(0xE0,0,0),  RGB(0xc0,0,0xc0), RGB(0,0xc0,0), RGB(0,0xc0,0xc0), RGB(0xc0,0xc0,0x00), RGB(0xc0,0xc0,0xc0), RGB(0xff,0xff,0), RGB(0,0,0xff), RGB(0xff,0,0),  RGB(0xff,0,0xff), RGB(0,0xFF,0), RGB(0,0xff,0xff), RGB(0xFF,0xFF,0x00), RGB(0xff,0xff,0xff)};
const unsigned short speccolors[8] = {RGB(0,0,0), RGB(0,0,0xff), RGB(0xff,0,0),  RGB(0xff,0,0xff), RGB(0,0xFF,0), RGB(0,0xff,0xff), RGB(0xFF,0xFF,0x00), RGB(0xff,0xff,0xff)};

#define LCD_HEIGHT 320
#define LCD_WIDTH 240

/*
 Output pixel at specified ZX spectrum xy coordinate
*/
void
putpix (int x, int y, int color )
{
  int lcdx, lcdy;

 // sanity checks  
  if (y < 0 || y>= 192) 
    return ;
    
  if (x >= 256)
    return ;

  if (x >= 256)
    return ;

  if (color > 16)
    return ;    
  
  /* ikon
  lcdx = x+32;
  lcdy = y+24;
  */
  lcdx = 320-(x+32);
  lcdy = 240-(y+24);

  if (rotlcd == 0) {

  	lcdx = 320-(x+32);
  	lcdy = 240-(y+24);
  } else {
    	// swap orientation and remove some pixels to fit PAX LCD resolution
    	lcdx = 319-y;
	lcdy = x-(x/16);
  }
	  fblines[lcdx + lcdy * LCD_HEIGHT] = speccolors[color];
} 

/*
 Output pixel out of limited coordinates of ZX paper - to enable border drawing etc.
*/
void
putpx (int x, int y, int color )
{
    int lcdx, lcdy;

    if (y < 0 || y > 240)
	return ;

    if (x > 320)
	return ;

    if (x > 320)
	return ;
    
    if (color > 8)
	return ;

    // Recalculate coordinates for current orientation
    lcdx=320-x;
    lcdy=240-y;

    fblines[lcdx + lcdy * LCD_HEIGHT] = speccolors[color];
}

//Set Device Parameter with error checking
#define ioset(field, argument)\
{\
    int arg = argument;\
    if(ioctl(dsp_fd, field, &arg)<0)\
        perror(#field "ioctl failed");\
    else if(arg!=argument)\
        perror(#argument "was not set in ioctl");\
    else\
        printf(#argument "\t:= %d\n", arg);\
}

/*
 This initializes all PAX related stuff.
 
 Originally there was X11 screen init
*/
void screen_init (void)
{
  int fd;
  int i, j;
  
  const char *rtc = default_rtc;

  // xxx add checks ! 

  keypad_fd = open ("/dev/keypad", O_RDWR);
  i = fcntl(keypad_fd, F_GETFL,0);
  fcntl(keypad_fd, F_SETFL, i| O_NONBLOCK);
  
  printer_fd = open ("/dev/printer", O_RDWR);

  dsp_fd = open("/dev/snd/dsp", O_WRONLY);
  i = fcntl(dsp_fd, F_GETFL,0);
  fcntl(dsp_fd, F_SETFL, i| O_NONBLOCK);


  rtc_fd = open(rtc, O_RDONLY);
        if (rtc_fd ==  -1) {
                perror(rtc);
                //exit(errno);
        }
 
  touchpad_fd = open("/dev/tp", O_RDWR);  
  i = fcntl(touchpad_fd, F_GETFL,0);
  fcntl(dsp_fd, F_SETFL, i| O_NONBLOCK);

	if ( touchpad_fd < 0 ) {
         perror("Unable to open touchscreen");
	 //return -1; 
	}

  // sound test, does not work, to be analyzed later
  //
  //Device Parameters
  const int SampleRate = 48000,
          Channels   = 1,//mono
          SampleBits = 8;

  //Configure Sound Device
  ioset(SOUND_PCM_WRITE_BITS,     SampleBits);
  ioset(SOUND_PCM_WRITE_CHANNELS, Channels);
  ioset(SOUND_PCM_WRITE_RATE,     SampleRate);

  //Sound Buffer
  uint8_t buf[SampleRate];
  unsigned Blocks    = 1000,
                   BlockSize = SampleRate/Blocks;

  //Synthesize Output Loop
  void synth(void) {
  char state = 0;
  {
        //1s Square Wave Syntheis
        for(unsigned i=0; i<Blocks; ++i) {
            state = !state;
            memset(buf+i*BlockSize, state ? 0x1f : 0x00, BlockSize);
      }

        //Send to Soundcard
        if(write(dsp_fd, buf, sizeof(buf)) != SampleRate)
            perror("wrote wrong number of bytes");
  }
}

  synth();
  Blocks = 500,
        BlockSize = SampleRate/Blocks;
  synth();
  // End sound check

  // sound test, does not work, to be analyzed later
  // write(dsp_fd,xxx,0x1000);
  //dsp_sound_synth();

  fd = open ("/dev/fb", O_RDWR);
  
  // fill space above
  fblines = mmap (0, 320 * 240 * 2, 3, 1, fd, 0);
  for (i = 0; i < 240; i++)
    for (j = 0; j < 320; j++)
      {
	fblines[j + i * 320] = RGB(0,0xff,0xff); //i * j;
      }
}
