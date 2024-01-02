
extern unsigned char *memory;

extern unsigned char kbdlines[8];

int zxin(int port);
void zxout(int port, int val);

void z80lowmemwrite(int addr, int val);

void screen_init(void);

void putpix(int x, int y, int color);
void putpx(int x, int y, int color);

