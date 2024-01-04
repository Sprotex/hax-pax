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

#include "z80emu.h"
#include "zxem.h"
#include "gui.h"
#include "pax.h"

unsigned char *memory;  // memory space including ROM, screenbuffer, etc.
unsigned char kbdlines[8];	// keyboard lines, as per http://www.breakintoprogram.co.uk/hardware/computers/zx-spectrum/keyboard

int emurun=0;		// Defines the status of emulation: 0=stopped, 1=running
int fntsel=0;		// Select font

// Directory entries variables
DIR *d;
struct dirent *dir;
//char *filesList[n];

long dly;

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
  //printf ("OUT %X %X\n", port, val);
  //if (port == 0xFE) fb_box(1,1,9,9,2);
  
  if (port == 0xFE) {
	
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

/*
 This handles writes to low memory to allow screen redraw. All changes are
 stored in global memory[] buffer
*/
void
z80lowmemwrite (int addr, int val)
{
  int x, y, i, j, attr, v, b;

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
  if (addr >= 0x5800 && addr < (0x5800 + ((192/8) * 32)))
    {
      x = (addr - 0x5800) & 31;
      y = (addr - 0x5800)/32;

      // redraw 8x8 block
      for (j = 0; j < 8; j++) {
        v = memory[0x4000 + x + ( (((y*8+j)&7)<<8) | ((((y*8+j)>>3)&7)<<5) | ((((y*8+j)>>6)&3)<<11)  ) ];
      for (i = 0; i < 8; i++) { 
	//if( attr && 0x40 == 0x40 ) b=1; else b=0;
	  putpix ( (x * 8 + i), (y*8)+j, ((v << i) & 0x80) ? (val&7) : ((val>>3)&7));
	}
      }
    }
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

/* Show selection menu */
void ShowMenu() {
	//ZXChar('"',0,0,0,0,7);
       zxout(254,7);
       ZXCls();
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
/* Cache directory content match the snap files and populate the list */
void Dir(void) {
    
    int n=0,i=0,m,cnt;
    char *ext;

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

    cnt = n;
    if (cnt>10) cnt=10;

    for(i=0;i<cnt;i++) {
	    sprintf(fnum, "%2d", i);
	    ZXPrint(fnum,0,80+i*8,fntsel,0,7);

	    strncpy (fname, fileList[i],strlen(fileList[i]));
	    ZXPrint(fname,24,80+i*8,fntsel,0,7);
	    for(m=0;m<30;m++) fname[m]='\0';
	    //fname[0]='\0';
    }

    i = inkey();
    while (i <= 0) i = inkey();
    if (i>1 && i<12) {
	   if (i==11) {
		  i=0;
	   } else {
		  i--;
	   }
	   //fname = fileList[i];
	   strncpy (fname, fileList[i],strlen(fileList[i]));
	   ZXPrint(fname,24,80+i*8,fntsel,4,0);
	   LoadSNA(fname);
	   ZXPrint("SNAP loaded, press 0 to start",24,40,fntsel,1,5);

    } 
   } // End of select

    closedir(d);
}

/* Load snap from given filename */
int LoadSNA(char *name) {

  unsigned char *snadata;
  int snasize;

  char *snap;
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
}

/* Main loop start */
int
main ()
{
  char *romfilename;
  //char *snafilename;
  int romsize;
  int fd;
  int cycles;
  int i;
  struct stat sb;
  //char *snap;
//  Z80_STATE state;

//  unsigned char *snadata;
// int snasize;

  romfilename = "zx48.rom";
//  snafilename = "manic.sna";
// snafilename = "horace.sna";
// snafilename = "ThroTheWall.sna";
// snafilename = "Uridium.sna";
//   snafilename = "diag.sna";
//   snafilename = "48z80ful.sna";

  memory = (unsigned char *) malloc (0x10000);
  memset (memory, 0, 0x10000);

  for (i = 0; i < 8; i++)
    kbdlines[i] = 0x1f;


  fd = open (romfilename, O_RDONLY);
  //fstat(fd,&sb);
  romsize = 16384;		//sb.st_size;
  if (read (fd, memory, romsize) != romsize || romsize == 0)
    {
      printf ("rom error\n");
      exit (1);
    }
  close (fd);

  emurun=0;
  fntsel=1;

  screen_init ();

  Z80Reset (&state);

  state.pc = 0x0000;

  ShowMenu();

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
	if ( (i == 6) || (i == 11) ) 
/* Emulate ZX Spectrum */
{	
  scr2lcd(1);

  cycles = 0;

  if (i == 6) { state.pc = 0x0000; }
  emurun=1;

  while (emurun==1)
    {
      cycles += Z80Emulate (&state, 200 /*cycles */ , NULL);
      int p;
      for (int o = 0; o < (1000); o++) p=o; //zpomlait
      /*
       This is really ugly hack to fire ZX refresh interrupt
       Changing this constant probably changes speed of the game. Who knows
       I don't know how the game should behave. 
       
       This also handles keyboard, but it should be done in
       an independent thread
       */
      if (cycles > 1024*10)
	{
	  handle_x (); // handle keyboard (keypad)
	  Z80Interrupt (&state, 0, NULL);
	  cycles = 0;
	}
    }

  //return 0;
  ZXCls();
  ShowMenu();
  i=0;
}
/* End of emulation option */

	if (i == 223) {
		i = 0;
		ZXPrint("Sure to exit? Green to confirm",0,184,fntsel,6,2);

		while (i <= 0) {
			i = inkey();
			if (i == 28) exit (1);
		}
		//return 0;
  	}
  }
}
