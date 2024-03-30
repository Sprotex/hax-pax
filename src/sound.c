#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <linux/soundcard.h>
#include <stdint.h>
#include <string.h>

//Device Parameters
const int SampleRate = 48000,
          Channels   = 1,//mono
          SampleBits = 8;

//Set Device Parameter with error checking
#define ioset(field, argument)\
{\
    int arg = argument;\
    if(ioctl(fd, field, &arg)<0)\
        perror(#field "ioctl failed");\
    else if(arg!=argument)\
        perror(#argument "was not set in ioctl");\
    else\
        printf(#argument "\t:= %d\n", arg);\
}

int _init() {
    //Open Sound Device
    int fd = open("/dev/snd/dsp", O_WRONLY);
    if(fd < 0) {
        perror("open of /dev/dsp failed");
        return 1;
    }

    //Configure Sound Device
    ioset(SOUND_PCM_WRITE_BITS,     SampleBits);
    ioset(SOUND_PCM_WRITE_CHANNELS, Channels);
    ioset(SOUND_PCM_WRITE_RATE,     SampleRate);

    //Sound Buffer
    uint8_t buf[SampleRate];
    const unsigned Blocks    = 1000,
                   BlockSize = SampleRate/Blocks;

    //Synthesize Output Loop
    char state = 0;
    {
        //1s Square Wave Syntheis
        for(unsigned i=0; i<Blocks; ++i) {
            state = !state;
            memset(buf+i*BlockSize, state ? 0x1f : 0x00, BlockSize);
        }

        //Send to Soundcard
        if(write(fd, buf, sizeof(buf)) != SampleRate)
            perror("wrote wrong number of bytes");
    }
}

