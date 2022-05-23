#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "stbhwdmx.h"

#include "dmx.h"
#include "emu_internal.h"

#define TUNER_DEV_COUNT (2)
#define REGION_BUFFER_SIZE (164*188)
#define STREAM_BIT_RATE (20*1024*1024) // 20 Megabits per second

#define EMU_TUNER_CONFIG "/data/vendor/dtvkit/aml_emu_tuner.ini"


static S_EMU_TUNER_DATA tuner_data[TUNER_DEV_COUNT];
static int  emu_tuner_init         = 0;
static int  emu_support_soft_tuner = 0;
static long emu_ts_bitrate;


static int OpenTsFile(unsigned int freq)
{
    char name[128];
    int fd;
    int ret;

    snprintf(name, sizeof(name), "/data/vendor/dtvkit/%d.ts", freq);
    fd = open(name, O_RDONLY);
    if (fd == -1)
    {
        EMU_DBG("cannot open \"%s\" (%s)\n", name, strerror(errno));
        return -1;
    }
    EMU_DBG("open %s success\n", name);

    return fd;
}

static int CloseTsFile(int fd)
{
    if (fd >= 0)
    {
        close(fd);
    }
    return 0;
}

static int ReadTsFile(int infd, char *buf, int size)
{
    int ret;
    ret = read(infd, buf, size);
    if(ret <= 0)
    {
        if (((ret == 0) || (errno == EAGAIN)))
        {
            lseek(infd, 0, SEEK_SET);
            ret = 0;
        }
        else
        {
            EMU_DBG("read file failed [%d] ret=%d\n", errno, ret);
            ret = -1;
        }
    }
    else if(ret < REGION_BUFFER_SIZE)
    {
        lseek(infd, 0, SEEK_SET);
    }
    return ret;
}

long DiffTimeval(const struct timeval *start_tv, const struct timeval *now_tv)
{
    long diff_time;

    if (now_tv->tv_usec < start_tv->tv_usec)
    {
        diff_time = (now_tv->tv_sec - 1 - start_tv->tv_sec) * 1000 + (now_tv->tv_usec + 1*1000*1000 - start_tv->tv_usec) / 1000;
    }
    else
    {
        diff_time = (now_tv->tv_sec - start_tv->tv_sec) * 1000 + (now_tv->tv_usec - start_tv->tv_usec) / 1000;
    }

    return diff_time;
}

static long GetStreamBitrate()
{
    char buf[64];
    long rate = 0;

    FILE* fd = fopen(EMU_TUNER_CONFIG, "r");
    if (fd == NULL)
    {
        return STREAM_BIT_RATE;
    }

    memset(buf, 0, sizeof(buf));
    if (fgets(buf, sizeof(buf), fd) != NULL)
    {
       rate = atoi(buf);
    }

    EMU_DBG("\nbitrate:%sM\n",buf);
    if (rate <= 1 || rate >= 50)
    {
        rate = STREAM_BIT_RATE;
    }
    else
    {
        rate = rate * 1024 * 1024;
    }
    EMU_DBG("bitrate:%ld\n",rate);

    fclose(fd);
    return rate;
}

static void ResetDmxInput(int dmx, unsigned int difftime)
{
    static struct timeval begin_tv;
    struct timeval now_tv;

    gettimeofday(&now_tv, NULL);
    unsigned int diff = DiffTimeval(&begin_tv, &now_tv);
    if (diff > difftime)
    {
        int input = EmuDmxGetInput(dmx);
        if (input != DMX_MEMORY)
        {
            EMU_DBG("reset input:%d" , input);
            EmuDmxSetInput(dmx, DMX_MEMORY);
        }
        begin_tv = now_tv;
    }
}

static void* EmuTunerThread(void* arg) {
    S_EMU_TUNER_DATA *tuner = (S_EMU_TUNER_DATA *)arg;
    int infd = tuner->ifd;
    int fd   = tuner->ofd;
    char buf[REGION_BUFFER_SIZE];
    int send, ret;

    struct timeval start_tv;
    struct timeval now_tv;
    long diff_time = 0;
    long bytes = 0;
    long BURST_US = (1000000 / (emu_ts_bitrate / (REGION_BUFFER_SIZE * 8)));

    EMU_DBG("emu thread start\n");
    usleep(300*1000); //wait for av init
    EmuDmxSetInput(tuner->dmx, DMX_MEMORY);
    gettimeofday(&start_tv, NULL);

    while (tuner->running)
    {
        ResetDmxInput(tuner->dmx, 1000);
        ret = ReadTsFile(infd, buf, REGION_BUFFER_SIZE);
        if (ret > 0)
        {
            bytes += ret;
            send = EmuDmxInjectData(fd, buf, ret, 200);
            if (send != ret)
            {
                EMU_DBG("dmx inject error:%d %d", send, ret);
                ResetDmxInput(tuner->dmx, 0);
                send = EmuDmxInjectData(fd, buf, ret, 200);
            }
        }
        else if(ret < 0)
        {
            EMU_DBG("ReadTsFile failed\n");
            break;
        }

        gettimeofday(&now_tv, NULL);
        diff_time = DiffTimeval(&start_tv, &now_tv);
        diff_time = BURST_US - (long)(diff_time * 1000);
        if (diff_time > 0)
        {
            usleep(diff_time);
        }
        gettimeofday(&start_tv, NULL);
    }

    EMU_DBG("emu thread end\n");

    tuner->running = -1;
    return NULL;
}


int EmuTunerInit()
{
    if (emu_tuner_init)
    {
        return 0;
    }

    emu_tuner_init = 1;
    int fd = open(EMU_TUNER_CONFIG, O_RDONLY);
    if (fd == -1)
    {
        emu_support_soft_tuner = 0;
        return 0;
    }
    close(fd);
    emu_support_soft_tuner = 1;

    EmuDmxInit();
    return 0;
}


int EmuTunerStart(unsigned char path, unsigned int freq)
{
    int fd;
    unsigned char dmx_no, search;

    EmuTunerInit();
    if (!emu_support_soft_tuner || path >= TUNER_DEV_COUNT)
    {
        return 0;
    }
    EmuTunerStop(path);

    search = STB_DPGetSearchMode(path);
    dmx_no = STB_DPGetPathDemux(path);
    EMU_DBG("EmuTunerStart:support %d path %d dmx %d search:%d" , emu_support_soft_tuner, path, dmx_no, search);

    fd = OpenTsFile(freq);
    if (fd < 0)
    {
        EMU_DBG("open ts failed");
        return 0;
    }
    tuner_data[path].ifd = fd;

    fd = EmuDmxOpen(dmx_no, search);
    if (fd < 0)
    {
        EMU_DBG("open demux failed");
        CloseTsFile(tuner_data[path].ifd);
        return 0;
    }

    emu_ts_bitrate = GetStreamBitrate();
    tuner_data[path].ofd = fd;
    tuner_data[path].dmx = dmx_no;
    tuner_data[path].running = 1;

    pthread_create(&tuner_data[path].thread, NULL, EmuTunerThread, (void*)(long)&tuner_data[path]);
    pthread_setname_np(tuner_data[path].thread, "emu_tuner_thread");
    return 1;
}


int EmuTunerStop(unsigned char path)
{
    if (!emu_support_soft_tuner || path >= TUNER_DEV_COUNT)
    {
        return 0;
    }

    EMU_DBG("EmuTunerStop:%d", tuner_data[path].running);

    if (tuner_data[path].running == 0)
    {
        return 0;
    }

    if (tuner_data[path].running == 1)
    {
        tuner_data[path].running = 0;

        do
        {
            usleep(20*1000);
        }while((tuner_data[path].running != -1));
    }

    CloseTsFile(tuner_data[path].ifd);
    EmuDmxClose(tuner_data[path].ofd);
    EMU_DBG("EmuTunerStop end");

    return 0;
}

int EmuTunerGetState(unsigned char path)
{
    if (!emu_support_soft_tuner || path >= TUNER_DEV_COUNT)
    {
        return 0;
    }

    if (tuner_data[path].running == 1)
        return 1;

    return 0;
}


int EmuTunerReset(unsigned char path)
{
    if (!emu_support_soft_tuner || path >= TUNER_DEV_COUNT)
    {
        return 0;
    }

    EmuDmxSetInput(tuner_data[path].dmx , DMX_MEMORY);
    return 0;
}


int EmuTunerGetSignalStrength(unsigned char path)
{
    if (EmuTunerGetState(path))
    {
        return 66;
    }

    return 0;
}

int EmuTunerGetSignalQuality(unsigned char path)
{
    if (EmuTunerGetState(path))
    {
        return 88;
    }

    return 0;
}


