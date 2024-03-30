#include <stdlib.h> 
#include <stdio.h>


/* include the X library headers */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>



#include "zxem.h"


Display *dis;
int screen;
Window win;
GC gc;


#define SCRMUL  2


#define RGB(r,g,b)  ((r<<16) | (g<<8) | b)
static const int speccolors[8] = {RGB(0,0,0), RGB(0,0,0xff), RGB(0xff,0,0),  RGB(0xFF,0,0xFF), RGB(0,0xFF,0), RGB(0,0xff,0xff), RGB(0xFF,0xFF,0x00), RGB(0xff,0xff,0xff)};


unsigned long colors[16];
int x_ready;

void putpix(int x, int y, int color)
{
  XSetForeground (dis, gc, speccolors[color&7]);
  XFillRectangle (dis, win, gc, y*SCRMUL, x*SCRMUL, SCRMUL, SCRMUL);
}

void
handle_x ()
{
  int x, y, cnt;
  KeySym key;			/* a dealie-bob to handle KeyPress Events */
  char text[255];		/* a char buffer for KeyPress Events */

  XEvent e;


  /* event loop */
  while (XPending (dis))
    {
      XNextEvent (dis, &e);
      printf ("Ev %d\n", e.type);
      /* draw or redraw the window */
      if (e.type == Expose)
	{
	  x_ready = 1;
          
          /*
	  for (x = 0; x < 64; x++)
	    for (y = 0; y < 16; y++)
	      paintchar (dis, win, gc, x * 6 * 2, y * 12 * 2,
			 tvd_vram[x + y * 64]);
                         */
                         
	}
      /* exit on key press */

      if (e.type == KeyPress)
	{
	  cnt = XLookupString (&e.xkey, text, 255, &key, 0);

	  printf ("Press st 0x%X, code 0x%X key 0x%X (key_press=%d)\n",
		  e.xkey.state, e.xkey.keycode, key, -1);

	 /* if (e.xkey.state == 4 && key == 0xFF6B)
	    do_dump = 1;
	  if (key < 0x80 || key == 0xFF0D || key == 0xFF08)
	    {
	      key_code = key;
	      key_press = 1;
	    }*/
            
          if (key == ' ') kbdlines[7] = ~(1<<0);  
          if (key == 'm') kbdlines[7] = ~(1<<0);
            
          if (key == 'l') kbdlines[6] = ~(1<<1);  
          if (key == 'o') kbdlines[5] = ~(1<<1);  
          if (key == 'w') kbdlines[2] = ~(1<<1);  
          if (key == 0xff0d) kbdlines[6] = ~(1<<0); // enter  
	}
    }

}



void screen_init(void)
{

  Colormap cmap;
  XColor color;
  int i;


  x_ready = 0;

  dis = XOpenDisplay ((char *) 0);
  if (dis == NULL)
    {
      printf ("dis err\n");
      exit (1);
    }
  screen = DefaultScreen (dis);

  cmap = DefaultColormap (dis, screen);

  colors[0] = BlackPixel (dis, screen);
  colors[15] = WhitePixel (dis, screen);

  for (i = 1; i < 7; i++)
    {
      //color.flags= DoRed | DoGreen | DoBlue; 
      color.blue = speccolors[i]&0xff;
      color.green = (speccolors[i]>>8)&0xff;
      color.red = (speccolors[i]>>16)&0xff;

      if (XAllocColor (dis, cmap, &color))
	colors[i] = color.pixel;
    }

  win = XCreateSimpleWindow (dis, DefaultRootWindow (dis), 0, 0,
			     256*SCRMUL, 192*SCRMUL, 5, colors[0],
			     colors[15]);
  if (win == NULL)
    {
      printf ("win err\n");
      exit (1);
    }

  XSetStandardProperties (dis, win, "ZX", "qqq", None, NULL, 0, NULL);

  XSelectInput (dis, win, ExposureMask | KeyPressMask);

  gc = XCreateGC (dis, win, 0, 0);

  if (gc == NULL)
    {
      printf ("gc err\n");
      exit (1);
    }

  XSetBackground (dis, gc, colors[15]);
  XSetForeground (dis, gc, colors[0]);
  XClearWindow (dis, win);
  XMapRaised (dis, win);



  while (!x_ready)
    handle_x ();

}

