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

#include "z80emu.h"
#include "zxem.h"


unsigned char *memory;  // memory space including ROM, screenbuffer, etc.
unsigned char kbdlines[8];	// keyboard lines, as per http://www.breakintoprogram.co.uk/hardware/computers/zx-spectrum/keyboard

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
  printf ("OUT %X %X\n", port, val);
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



int
main ()
{
  char *romfilename;
  char *snafilename;
  int romsize;
  int fd;
  int cycles;
  int i;
  struct stat sb;
  char *snap;
  Z80_STATE state;

  unsigned char *snadata;
  int snasize;

  romfilename = "zx48.rom";
  snafilename = "manic.sna";
// snafilename = "horace.sna";
// snafilename = "ThroTheWall.sna";
// snafilename = "Uridium.sna";
//   snafilename = "diag.sna";
//   snafilename = "48z80ful.sna";

  snasize = 0;
  snadata = 0;
  if (snafilename)
    {
      fd = open (snafilename, O_RDONLY);
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


  screen_init ();

  Z80Reset (&state);

  state.pc = 0x0000;

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
    

/*
        unsigned short  alternates[4];

        int             i, r, pc, iff1, iff2, im; 
        */

      // simulate enter
      // kbdlines[6] = ~(1 << 0);	// enter        
    }

  cycles = 0;

  while (1)
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

  return 0;
}
