int inkey();
void handle_x();
void handle_event();
void dsp_sound_synth();
void dev_init(void);
void kbd_init(void);
void synth2(void);
void synth(void);

extern int realx,realy;
//extern int rtc_fd;


// Key codes definition
#define ALPHA 69
#define FUNC 102
#define BACK  14
#define ENTER 28
#define ESC  223
