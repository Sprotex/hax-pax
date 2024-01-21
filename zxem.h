
extern unsigned char *memory;

extern unsigned char kbdlines[8];

extern int emurun;
extern int rotlcd;
extern int joyval;
extern int kbdjoy;
extern int flstate;
extern int rtc_fd;

int zxin(int port);
void zxout(int port, int val);

void z80lowmemwrite(int addr, int val);

void screen_init(void);

void putpix(int x, int y, int color);
void putpx(int x, int y, int color);

void redrawblock(int addr, int val);

