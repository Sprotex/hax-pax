
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>


static int dsp_fd = -1;   // sound device, not finished yet
static int touchpad_fd = -1,
         realx = 0, 
	 realy = 0 ; 
  
struct input_event ev0[64]; 




static int handle_event0() {
	 int button = 0, 
	 i, 
	 rd; 
	 rd = read(touchpad_fd, ev0, sizeof(struct input_event) * 64);

	 for (i = 0; i < rd / sizeof(struct input_event); i++) {
	 	 printf("", ev0[i].type, ev0[i].code, ev0[i].value);
	 	  if (ev0[i].type == 3 && ev0[i].code == ABS_X)                                   realx = ev0[i].value;
	           else if (ev0[i].type == 3 && ev0[i].code == ABS_Y)  		             realy = ev0[i].value;
           } return 1;
	  }


int _init(void) { 
          perror("Testovani touchapdu... konec je levy dolni roh u USB konektoru\n"); 
	int done = 1; 
	printf("sizeof(struct input_event) = %d\n", sizeof(struct input_event));
	touchpad_fd = open("/dev/tp", O_RDWR);  
	if ( touchpad_fd < 0 ) {
         perror("Nelze otevřít zařízení");
	 return -1; 
	}
	 while ( done ) { 
	  perror("begin handel_event0...\n"); 
	  done = handle_event0(); 
	  printf("----- dalsi event\n");
          printf("Souradnice .. X: %3d; Y: %3d\n", realx, realy);
          perror("\n");
	  perror("end handel_event0...\n");
	  if ( realx < 20 && realy < 20 ) {    // levy dolni roh u USB konektoru
         perror("koncime");
         close(touchpad_fd); 
	  touchpad_fd = -1; 
 	  return 2; 
	  	} 
	   } 
	  if ( touchpad_fd > 0 ) { 
	  close(touchpad_fd); 
	  touchpad_fd = -1; 
	  } 
	  return 0; 
	  }


