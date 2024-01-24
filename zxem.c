/*
 This is really ugly ZX emulation code which can barely run manic miner.

 Please do not distribute outside Bytefest hackathon group until the code is fixed
 
THIS SOFTWARE IS PROVIDED BY an anoumous author AS IS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

*/

#include <stdio.h>
#include <linux/limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <linux/rtc.h>
#include <sys/time.h>

#include "z80emu.h"
#include "zxem.h"
#include "gui.h"
#include "pax.h"

unsigned char *memory;  // memory space including ROM, screenbuffer, etc.
unsigned char kbdlines[8];	// keyboard lines, as per http://www.breakintoprogram.co.uk/hardware/computers/zx-spectrum/keyboard

int emurun=0;		// Defines the status of emulation: 0=stopped, 1=running
int fntsel=0;		// Select font

int rotlcd=1;		// Rotate screen and shrink
int ba=0;		// Border active

long int lms;
struct rtc_time rtc_tm; 
struct timeval tp;

// Directory entries variables
DIR *d;
struct dirent *dir;
//char *filesList[n];

long dly;

int kbdjoy=0;		// Keyboard or joystick
int joyval=0;		// Default joy value
int intcnt=0;		// Interrupt counter
int flstate;		// Actual status of flash (0 or 1 alternates between INK and PAPER)

//static const char default_rtc[] = "/dev/rtc0";

Z80_STATE state;
/*
 Z80 in-port
*/
int
zxin (int port)
{
  int i, v;
  
  /*
    tehere is some kind of bug that prevents me from masking keystrokes
    if manic miner is reading multiple keylines at one time. this happens 
    at 0x8c5c, see https://skoolkid.github.io/manicminer/asm/8BDD.html
    so we force the last keyline with space to allow jumping
    
    this should be fixed for other games or if it becomes obvious that manic
    miner needs more keys than implemented now :)
  */
  if (port == 0x7efe)
    port = 0x7ffe;

  
  if (port == 0x00fe)
    port = 0xbffe;

  if ((port & 0xFF) == 0xFE)
    {
      v = 0x1f;
      for (i = 0; i < 8; i++)
	if ((port & (1 << (i + 8))) == 0)
	  {
	    v &= kbdlines[i];
#ifndef __arm__
          if (i != 7)
	    kbdlines[i] = 0xff;	//reset key
#endif
                return v;
	  }
      //return v;
    }

  // Handle joystick value
  if ((port & 0x001f) == 0x001f) {
	  return joyval;
  }
  // Debug information
  printf ("IN %X\n", port);
  return 0;
}

/*
 ZX OUT port
 
 this should be implemented for sound
*/
void
zxout (int port, int val)
{
  int x,y,color;
  static int pb=0;
  
  if ((port == 0xFE && ba == 1) && (rotlcd ==0)) {
	
  	color = val&0x07; 
	if (color !=pb ) {
	   for(y=0;y<25;y++)  { 
		for(x=0;x<=320;x++) {
			putpx(x,y,color);
			putpx(x,y+192+24,color);
		} // end X
	   } // end Y
	   //for(x=0;x<=320;x++) putpx(x,240,0);
	   for(x=0;x<33;x++) {
		for(y=24;y<192+24;y++) {
			putpx(x,y,color);
			putpx(x+256+32,y,color);
		} // end Y
	   } // end X
	} // end if

	pb=color;
  }
}

// Redraw block for attr change
//
void
redrawblock(int addr, int val) {

      int x,y,i,j,v;

      x = (addr - 0x5800) & 31;
      y = (addr - 0x5800)/32;

      // redraw 8x8 block
      for (j = 0; j < 8; j++) {
        v = memory[0x4000 + x + ( (((y*8+j)&7)<<8) | ((((y*8+j)>>3)&7)<<5) | ((((y*8+j)>>6)&3)<<11)  ) ];
      for (i = 0; i < 8; i++) { 
	//if( attr && 0x40 == 0x40 ) b=1; else b=0;
	//
	  if ( ((val & 0x80) == 0x80) && (flstate == 1) ) {
		  putpix ( (x * 8 + i), (y*8)+j, ((v << i) & 0x80) ? (val>>3&7) : (val&7));
	  } else {
		  putpix ( (x * 8 + i), (y*8)+j, ((v << i) & 0x80) ? (val&7) : ((val>>3)&7));
	  }
	  }
	 
	}
}

/*
 This handles writes to low memory to allow screen redraw. All changes are
 stored in global memory[] buffer
*/
void
z80lowmemwrite (int addr, int val)
{
  int x, y, i, attr;

  // http://www.breakintoprogram.co.uk/hardware/computers/zx-spectrum/screen-memory-layout

  // handle pixels
  if (addr >= 0x4000 && addr < (0x4000 + (192 * 32)))
    {
      x = (addr - 0x4000) & 31;
      y = (addr - 0x4000);
      y = ((y >> 8) & 7) | (((y >> 5) & 7) << 3) | ((y >> 11) & 3) << 6;
      
      attr = memory[0x5800 + x + 32*(y/8)];

      for (i = 0; i < 8; i++) {
	//if( attr && 0x40 == 0x40 ) b=1; else b=0;
	putpix ( (x * 8 + i), y, ((val << i) & 0x80) ?  (attr&7) : ((attr>>3)&7));
	}
    }
  else
  // handle color attributes
  if (addr >= 0x5800 && addr < (0x5800 + ((192/8) * 32))) redrawblock(addr,val);
    
  else
    {
     // this should not happen :)
      printf ("Unhandled lowmemwrite addr=%X val= %X\n", addr, val);
    }
}

#define Z80_SPH          13
#define Z80_SPL          12

#ifdef __arm__
#define main _init
#endif

int GetTime() {
	int retval;
	/* Read the RTC time/date */
        retval = ioctl(rtc_fd, RTC_RD_TIME, &rtc_tm);
        if (retval == -1) {
	                perror("RTC_RD_TIME ioctl");
	                exit(errno);
        }
	return retval;
}

/* Show selection menu */
void ShowMenu() {
	//ZXChar('"',0,0,0,0,7);
       //zxout(254,7);
       Clear();
       ZXPrint("ZXEM Menu",0,0,fntsel,6,2);
       ZXPrint("1. Keyboard help",0,8,fntsel,0,7);
       ZXPrint("2. Save SNApshot",0,16,fntsel,0,7);
       ZXPrint("3. Load SNApshot",0,24,fntsel,0,7);
       ZXPrint("4. Load ROM & Reset",0,32,fntsel,0,7);
       ZXPrint("5. Reset",0,40,fntsel,0,7);
       ZXPrint("7. Open TAPe",0,48,fntsel,0,7);
       ZXPrint("8. Poke",0,56,fntsel,0,7);
       ZXPrint("9. Options",0,64,fntsel,0,7);
       ZXPrint("0. Exit menu",0,72,fntsel,0,7);

       //ZXPrint("Zdravi te PAX! Nativne...",0,100,0,0,7);
       //zxprintf("%s","Pozdravuje PAX, nie emulator!");
}

/* Show options */
void ShowOpts() {
       ZXPrint("ZXEM Options",0,80,fntsel,6,2);
       ZXPrint("1. Font width: ",0,88,fntsel,0,7);
       ZXPrint("2. Screen orientation: ",0,96,fntsel,0,7);
       ZXPrint("3. Border emulation: ",0,104,fntsel,0,7);
       ZXPrint("0. Exit options",0,112,fntsel,0,7);
}

/* Load snap from given filename */
int LoadSNA(char *name) {

  unsigned char *snadata;
  int snasize;

  unsigned char *snap;
  int fd;

  snasize = 0;
  snadata = 0;

  if (name)
    {
      fd = open (name, O_RDONLY);
      //fstat(fd,&sb);
      snasize = 49179;		//sb.st_size;
      snadata = (unsigned char *) malloc (snasize);
      if (snasize != 49179)
	{
	  printf ("Warning: sna size not 49179 ! \n");
	  //exit (1);
	}
      if (read (fd, snadata, snasize) != snasize)
	printf ("sna error\n");
      close (fd);
    }

  if (snadata)
    {
      memcpy (&memory[0x4000], &snadata[0x1b], 0xC000);
      state.pc = 0x0072; // 0x0072;

      snap = snadata;
      
      // xxx may be wrong!!

      state.i = *(snap++);
      ((unsigned char *) state.alternates)[Z80_L] = *(snap++); 
      ((unsigned char *) state.alternates)[Z80_H] = *(snap++); 
      ((unsigned char *) state.alternates)[Z80_E] = *(snap++); 
      ((unsigned char *) state.alternates)[Z80_D] = *(snap++); 
      ((unsigned char *) state.alternates)[Z80_C] = *(snap++); 
      ((unsigned char *) state.alternates)[Z80_B] = *(snap++); 
      ((unsigned char *) state.alternates)[Z80_F] = *(snap++); 
      ((unsigned char *) state.alternates)[Z80_A] = *(snap++); 
     
      state.registers.byte[Z80_L] = *(snap++); 
      state.registers.byte[Z80_H] = *(snap++); 
      state.registers.byte[Z80_E] = *(snap++); 
      state.registers.byte[Z80_D] = *(snap++); 
      state.registers.byte[Z80_C] = *(snap++); 
      state.registers.byte[Z80_B] = *(snap++); 
      
      state.registers.byte[Z80_IYL] = *(snap++); 
      state.registers.byte[Z80_IYH] = *(snap++); 
      
      state.registers.byte[Z80_IXL] = *(snap++); 
      state.registers.byte[Z80_IXH] = *(snap++); 
      
      if ((*(snap++) & 4) != 0)  // was 0
	{
	  state.iff1 = 1;
	  state.iff2 = 1;
	}
      else
	{
	  state.iff1 = 0;
	  state.iff2 = 0;
	}
      state.r = *(snap++);
      state.registers.byte[Z80_F] = *(snap++); 
      state.registers.byte[Z80_A] = *(snap++); 
      
      state.registers.byte[Z80_SPL] = *(snap++);  //was L
      state.registers.byte[Z80_SPH] = *(snap++);   // was H
      state.im =  *(snap++);
      //output(254, *(snap++) & 7);

      state.pc = memory[state.registers.word[Z80_SP]];
      state.registers.word[Z80_SP]++; 
      state.pc += 256*memory[state.registers.word[Z80_SP]];
      state.registers.word[Z80_SP]++; 
    }

  return 0;
}

/* Cache directory content match the snap files and populate the list */
void Dir(void) {
    
    int n=0,i=0,x,m,cnt;
    char *ext;
    static int cur=0;

    d = opendir(".");
    if (d) {
      while ((dir = readdir(d)) != NULL) {
        //zxprintf("%s\n", dir->d_name);
    	ext = strchr(dir->d_name, '.');
    	if ( ext && (!strcmp(ext, ".sna")) ) {
			printf("%s\n", dir->d_name);
			n++;
	}

      }
      rewinddir(d);

      char *fileList[n];

      while ((dir = readdir(d)) != NULL) {
	ext = strchr(dir->d_name, '.');

    	if ( ext && (!strcmp(ext, ".sna")) ) {
		fileList[i] = (char*) malloc (strlen(dir->d_name)+1);
	        strncpy (fileList[i],dir->d_name, strlen(dir->d_name) );	 
		i++;
	}
      }
    //for(i=0; i<=n; i++)
    //	      printf("%s\n", filesList[i]);
    
    ZXCls();
    ZXPrint("Select SNA to load:",0,0,fntsel,6,2);
    ZXPrint("<< FUNC, ALPHA >>, [0-9] Select",0,16,fntsel,0,7);
    
    char fname[30];
    char fnum[3];

    cnt = (1+cur/10)*10;

    for(m=0;m<30;m++) fname[m]='\0';
    /* Show one page of files if list is longer than 10 entries */
    if (cnt>10) cnt=10;
    for(x=0;x<cnt;x++) {
	    sprintf(fnum, "%2d", x);
	    ZXPrint(fnum,0,80+x*8,fntsel,0,7);

	    strncpy (fname, fileList[x+cur],strlen(fileList[x+cur]));
	    ZXPrint(fname,24,80+x*8,fntsel,0,7);
	    for(m=0;m<30;m++) fname[m]='\0';
    }

    sprintf(fname,"Entry: %d-%d of %d",cur+1,cur+10,n);
    ZXPrint(fname,0,192-8,fntsel,5,1);
    for(m=0;m<30;m++) fname[m]='\0';

    i = inkey();
    while (1) {
	    i = inkey();
      
/*
    if (cnt>10) cnt=10;
    for(i=0;i<cnt;i++) {
	    sprintf(fnum, "%2d", i);
	    ZXPrint(fnum,0,80+i*8,fntsel,0,7);

	    strncpy (fname, fileList[i+cur],strlen(fileList[i+cur]));
	    ZXPrint(fname,24,80+i*8,fntsel,0,7);
	    for(m=0;m<30;m++) fname[m]='\0';
    }*/
    if (i>1 && i<12) {
	   if (i==11) {
		  i=0;
	   } else {
		  i--;
	   }

	   if (i >= (n-cur) ) break;
	   //fname = fileList[i];
	   strncpy (fname, fileList[i+cur],strlen(fileList[i+cur]));
	   ZXPrint(fname,24,80+i*8,fntsel,4,0);
	   LoadSNA(fname);
	   ZXPrint("SNAP loaded, press 0 to start",24,40,fntsel,1,5);
	   break;

    }
    if (i == FUNC) {
	    if (cur >= 10) cur-=10;

    if (n-cur >= 10 ) cnt=10;

    ZXCls();
    ZXPrint("Select SNA to load:",0,0,fntsel,6,2);
    ZXPrint("<< FUNC, ALPHA >>, [0-9] Select",0,16,fntsel,0,7);
    if (cnt>10) cnt=10;
    for(x=0;x<cnt;x++) {
	    sprintf(fnum, "%2d", x);
	    ZXPrint(fnum,0,80+x*8,fntsel,0,7);

	    strncpy (fname, fileList[x+cur],strlen(fileList[x+cur]));
	    ZXPrint(fname,24,80+x*8,fntsel,0,7);
	    for(m=0;m<30;m++) fname[m]='\0';
    }
    sprintf(fname,"Entry: %d-%d of %d",cur+1,cur+10,n);
    ZXPrint(fname,0,192-8,fntsel,5,1);
    for(m=0;m<30;m++) fname[m]='\0';
    }

    if (i == ALPHA) {
	    if ((cur/10) < (n/10)) cur+=10;
    
    if ((cur/10) == (n/10)) cnt=n%10;

    ZXCls();
    ZXPrint("Select SNA to load:",0,0,fntsel,6,2);
    ZXPrint("<< FUNC, ALPHA >>, [0-9] Select",0,16,fntsel,0,7);
    for(x=0;x<cnt;x++) {
	    sprintf(fnum, "%2d", x);
	    ZXPrint(fnum,0,80+x*8,fntsel,0,7);

	    strncpy (fname, fileList[x+cur],strlen(fileList[x+cur]));
	    ZXPrint(fname,24,80+x*8,fntsel,0,7);
	    for(m=0;m<30;m++) fname[m]='\0';
    }
    sprintf(fname,"Entry: %d-%d of %d",cur+1,(cur+10 > n) ? n : cur+10,n);
    ZXPrint(fname,0,192-8,fntsel,5,1);
    for(m=0;m<30;m++) fname[m]='\0';
    }
    } // End of while
   } // End of select

    closedir(d);
}

/* Load default rom file */
int LoadROM(void) {

  int romsize;
  char *romfilename;
  romfilename = "zx48.rom";

  int fd;
  fd = open (romfilename, O_RDONLY);
  //fstat(fd,&sb);
  romsize = 16384;		//sb.st_size;
  if (read (fd, memory, romsize) != romsize || romsize == 0)
    {
      printf ("rom error\n");
      exit (1);
    }
  close (fd);

  return 0;
}

/* Main loop start */
int
main ()
{
  //char *romfilename;
  //char *snafilename;
  //int romsize;
  //int fd;
  int cycles;
  int i;
  //struct stat sb;
  unsigned char c;
  int curr_sec, prev_sec;
 
  char datetime[80];

  //char *snap;
//  Z80_STATE state;

//  unsigned char *snadata;
// int snasize;

//  romfilename = "zx48.rom";
//  snafilename = "manic.sna";
// snafilename = "horace.sna";
// snafilename = "ThroTheWall.sna";
// snafilename = "Uridium.sna";
//   snafilename = "diag.sna";
//   snafilename = "48z80ful.sna";

  memory = (unsigned char *) malloc (0x10000);
  memset (memory, 0, 0x10000);

  // Init keyboard and emulate no key press initially
  for (i = 0; i < 8; i++)
    kbdlines[i] = 0x1f;

  emurun=0;
  fntsel=1;

  screen_init ();

  flstate = 0;

  LoadROM();

  Z80Reset (&state);

  state.pc = 0x0000;

  ShowMenu();

  GetTime();
  prev_sec = rtc_tm.tm_sec;

  ZXPrint("System date/time:",0,192-16,fntsel,1,7);
  sprintf(datetime, "%2d-%2d-%4d %02d:%02d:%02d",
	rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,
        rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);

  ZXPrint(datetime,0,192-8,fntsel,1,7);

  while (1) {
     	i = inkey();
        if (i == 2) {
		zxout(254,0);
		scr2lcd(0);
		for(dly=0;dly<20000000;dly++) { asm("nop"); }
		zxout(254,7);
		ZXCls();
		ShowMenu();
	}
	if ( i == 4 ) Dir();
	if ( i == 10 ) {
		ShowOpts();
		c = ( rotlcd == 0 ) ? '0' : '1';
		ZXChar(c,184,96,fntsel,6,2);

		while (1) {
			i = inkey();
			if ( i == 3 ) {
				rotlcd = (rotlcd == 0) ? 1 : 0;
				if (rotlcd == 0) ba=1;
				ShowMenu();
				break;
			}

			if ( i == 11 ) {
				ShowMenu();
				break;
			}
		}
	}
	if ( i == FUNC ) {

		if (kbdjoy != 0) {
			ZXChr('K',232,312,1,1,5);
			ZXChr('B',224,312,1,1,5);
			ZXChr('D',216,312,1,1,5);
			kbdjoy=0;
		} else {
			ZXChr('J',232,312,1,2,6);
			ZXChr('O',224,312,1,2,6);
			ZXChr('Y',216,312,1,2,6);
			kbdjoy=1;
		}

	}
	if ( (i == 6) || (i == 11) ) 
/* Emulate ZX Spectrum */
{
 LoadROM();	// Load ROM again in case it was accidentialy overwritten

 if (rotlcd ==0) { 
  	scr2lcd(1);
  } else {
	tkbd2lcd(0);
	scr2lcd(1);
  }

  cycles = 0;

  if (i == 6) { state.pc = 0x0000; }
  emurun=1;

  while (emurun==1)
    {
      gettimeofday(&tp, NULL);
      long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
      cycles += Z80Emulate (&state, 200 /*cycles */ , NULL);
      
      for (int o = 0; o < (1000); o++) asm("nop"); //zpomlait
      /*
       This is really ugly hack to fire ZX refresh interrupt
       Changing this constant probably changes speed of the game. Who knows
       I don't know how the game should behave. 
       
       This also handles keyboard, but it should be done in
       an independent thread
       */
        if  ((ms -lms) > 20 )//(cycles > 1024*10)
	{
	  handle_x(); // handle keyboard (keypad)
	  //handle_event();
	  Z80Interrupt (&state, 0, NULL);
	  cycles = 0;
	  intcnt++;
	  lms = ms;
	}

      	if (intcnt == 25) {
		flstate=1;
		doflash();

	}
	if (intcnt == 50) {
		flstate=0;
		doflash();
		intcnt=0;
	}
    }

  ShowMenu();
  i=0;
}
/* End of emulation option */

	if (i == ESC) {
		i = 0;
		ZXPrint("Sure to exit? Green to confirm",0,184,fntsel,4,0);

		while (i <= 0) {
			i = inkey();
			if (i == ENTER ) exit (1);
		}
		//return 0;
  	}

	// BLOCKING! - ak sa nasledujuci riadok odkomentuje, bude sa vyzadovat 
	// kliknutie na touchscreen predtym ako sa otestuju kody klavesnice
	//handle_event();
	sprintf(datetime,"X= %d, Y= %d     ",realx,realy);
	ZXPrint(datetime,0,192-24,fntsel,0,7);

	GetTime();
	curr_sec = rtc_tm.tm_sec;
	if (curr_sec > prev_sec) {

		ZXPrint("System date/time:",0,192-16,fntsel,1,7);
		sprintf(datetime, "%2d-%2d-%4d %02d:%02d:%02d",
				rtc_tm.tm_mday, rtc_tm.tm_mon + 1, rtc_tm.tm_year + 1900,
			        rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec);
		ZXPrint(datetime,0,192-8,fntsel,1,7);
		if (curr_sec == 59) {
			prev_sec = -1;
		} else {
			prev_sec = curr_sec;
		}


	}
  }
}
