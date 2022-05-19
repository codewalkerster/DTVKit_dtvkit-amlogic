#ifndef _EMU_INTERNAL_H
#define _EMU_INTERNAL_H

#define EMU_DBG(x,...)          STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
//#define EMU_DBG(x,...)          printf("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )

enum
{
    DMX_X2,
    DMX_X4,
};

typedef struct
{
    int running;
    int ifd;     //file fd
    int ofd;     //dvr fd
    int dmx;
    pthread_t thread;
}S_EMU_TUNER_DATA;


int EmuTunerInit();
int EmuTunerStart(unsigned char path, unsigned int freq);
int EmuTunerStop(unsigned char path);
int EmuTunerGetState(unsigned char path);
int EmuTunerReset(unsigned char path);
int EmuTunerGetSignalStrength(unsigned char path);
int EmuTunerGetSignalQuality(unsigned char path);


int EmuDmxInit();
int EmuDmxOpen(int dmx_no, int search);
int EmuDmxClose(int handle);
int EmuDmxInjectData(int handle, unsigned char *buf, int size, int timeout);
int EmuDmxSetInput(int dmx_no, int input);
int EmuDmxGetInput(int dmx_no);

#endif
