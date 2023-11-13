#define _GNU_SOURCE
#include <pthread.h>
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
#include "dtv_log.h"
#define TAG  "EMU_TUNER"

#ifdef DTVKIT_IN_VENDOR_PARTITION
#include <cutils/properties.h>
#endif

#include "stbhwos.h"
#include "stbhwdmx.h"
#include "stbhwmem.h"
#include "stbdpc.h"

#include "dmx.h"
#include "emu_internal.h"

#define TUNER_DEV_COUNT (2)
#define REGION_BUFFER_SIZE (164*188)

enum
{
    EMU_THREAD_STOPPING = -1,
    EMU_THREAD_STOP,
    EMU_THREAD_RUN,
};

static S_EMU_TUNER_DATA tuner_data[TUNER_DEV_COUNT];
static int  emu_tuner_init         = 0;
static int  emu_support_soft_tuner = 0;

static int OpenTsFile(char *name)
{
    int fd;
    int ret;

    fd = open(name, O_RDONLY);
    if (fd == -1)
    {
        DTV_LOGI(TAG, "cannot open \"%s\" (%s)\n", name, strerror(errno));
        return -1;
    }
    DTV_LOGI(TAG, "open %s success\n", name);

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

    if (infd < 0)
    {
        return 0;
    }

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
            DTV_LOGI(TAG, "read file failed [%d] ret=%d\n", errno, ret);
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

static void TunerLockEvent(unsigned char path, int lock)
{
    unsigned short event;
    if (lock)
    {
        event = HW_EV_TYPE_LOCKED;
    }
    else
    {
        event = HW_EV_TYPE_NOTLOCKED;
    }
    //DTV_LOGI(TAG, "TunerLockEvent:%d", lock);
    STB_OSSendEvent(FALSE, HW_EV_CLASS_TUNER, event, &path, sizeof(path));
}

static int TunerDataUpdate(S_EMU_TUNER_DATA *tuner)
{
    int ret;
    int lock_change = 0;
    S_EMU_CONFIG config;
    S_EMU_CONFIG *pConfig = &tuner->config;

    emu_support_soft_tuner = EmuCfgLoad();
    ret = EmuCfgGetConfig(pConfig->tunerid, pConfig->freq, pConfig->modulation, &config);
    if (ret == 0)
    {
        if (pConfig->lockstate != config.lockstate)
        {
            pConfig->lockstate = config.lockstate;
            lock_change = 1;
        }
        pConfig->bitrate   = config.bitrate;
        //DTV_LOGI(TAG, "TunerUpdate:%s %s", config.name, pConfig->name);
        if (strcmp(config.name, pConfig->name) || tuner->ifd < 0)
        {
            if (tuner->ifd >= 0)
            {
                CloseTsFile(tuner->ifd);
            }
            strcpy(pConfig->name, config.name);
            tuner->ifd = OpenTsFile(config.name);
        }
    }
    else
    {
        lock_change = 1;
        pConfig->lockstate = 0;
        if (tuner->ifd >= 0)
        {
            CloseTsFile(tuner->ifd);
            tuner->ifd = -1;
        }
    }

    if (lock_change)
    {
        TunerLockEvent(tuner->path, pConfig->lockstate);
    }

    return 0;
}

static int IsConfigUpdate(unsigned int difftime)
{
    int ret = 0;

#ifdef DTVKIT_IN_VENDOR_PARTITION
    static struct timeval begin_tm;
    struct timeval now_tm;

    gettimeofday(&now_tm, NULL);
    unsigned int diff = DiffTimeval(&begin_tm, &now_tm);
    if (diff > difftime)
    {
        int size;
        char buf[PROPERTY_VALUE_MAX];
        char *name = "vendor.tv.softtuner.config.update";

        memset(buf, 0, sizeof(buf));
        size = property_get(name, buf, "false");
        if (size > 0 && !strcmp(buf, "true"))
        {
            property_set(name, "false");
            ret = 1;
        }
        begin_tm = now_tm;
    }
#endif

    return ret;
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
            DTV_LOGI(TAG, "reset input:%d" , input);
            EmuDmxSetInput(dmx, DMX_MEMORY);
        }
        begin_tv = now_tv;
    }
}

static void* EmuTunerThread(void* arg)
{
    S_EMU_TUNER_DATA *tuner = (S_EMU_TUNER_DATA *)arg;
    int infd = tuner->ifd;
    int fd   = tuner->ofd;
    int send, ret;

    struct timeval start_tv;
    struct timeval now_tv;
    long diff_time;
    long BURST_US = (1000000 / (tuner->config.bitrate / (REGION_BUFFER_SIZE * 8)));

    char *buf = (char *)STB_MEMGetSysRAM(REGION_BUFFER_SIZE);
    if (!buf)
    {
        EMU_DBG("emu thread malloc failed\n");
        return NULL;
    }

    DTV_LOGI(TAG, "emu thread start\n");
    usleep(300*1000); //wait for av init
    EmuDmxSetInput(tuner->dmx, DMX_MEMORY);
    gettimeofday(&start_tv, NULL);
    TunerLockEvent(tuner->path, tuner->config.lockstate); //maybe it is false

    while (tuner->running == EMU_THREAD_RUN)
    {
        ResetDmxInput(tuner->dmx, 1000);
        if (IsConfigUpdate(2000))
        {
            TunerDataUpdate(tuner);
            infd = tuner->ifd;
            BURST_US = (1000000 / (tuner->config.bitrate / (REGION_BUFFER_SIZE * 8)));
        }

        ret = ReadTsFile(infd, buf, REGION_BUFFER_SIZE);
        if (ret > 0)
        {
            send = EmuDmxInjectData(fd, buf, ret, 200);
            if (send != ret)
            {
                DTV_LOGI(TAG, "dmx inject error:%d %d", send, ret);
                ResetDmxInput(tuner->dmx, 0);
                send = EmuDmxInjectData(fd, buf, ret, 200);
            }
        }
        else if(ret < 0)
        {
            DTV_LOGI(TAG, "ReadTsFile failed\n");
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

    DTV_LOGI(TAG, "emu thread end\n");
    STB_MEMFreeSysRAM(buf);
    tuner->running = EMU_THREAD_STOPPING;
    return NULL;
}


int EmuTunerInit()
{
    if (emu_tuner_init)
    {
        return 0;
    }

    emu_tuner_init = 1;
    emu_support_soft_tuner = EmuCfgLoad();

    EmuDmxInit();
    return 0;
}

int EmuTunerStart(unsigned char path, unsigned int freq, unsigned int modulation)
{
    int fd;
    unsigned char dmx_no;
    S_EMU_CONFIG config;

    EmuTunerInit();
    if (!emu_support_soft_tuner || path >= TUNER_DEV_COUNT)
    {
        return 0;
    }
    EmuTunerStop(path);

    DTV_LOGI(TAG, "EmuTunerStart:support %d path %d " , emu_support_soft_tuner, path);
    memset(&tuner_data[path].config, 0, sizeof(S_EMU_CONFIG));
    if (EmuCfgGetConfig(path, freq, modulation, &tuner_data[path].config) < 0)
    {
        DTV_LOGI(TAG, "no freq config");
        return 0;
    }

    fd = OpenTsFile(tuner_data[path].config.name);
    if (fd < 0)
    {
        DTV_LOGI(TAG, "open ts failed:%s" , tuner_data[path].config.name);
        return 0;
    }
    tuner_data[path].ifd = fd;

    dmx_no = STB_DPGetPathDemux(path);
    fd = EmuDmxOpen(path);
    if (fd < 0)
    {
        DTV_LOGI(TAG, "open demux failed");
        CloseTsFile(tuner_data[path].ifd);
        return 0;
    }

    tuner_data[path].path    = path;
    tuner_data[path].ofd     = fd;
    tuner_data[path].dmx     = dmx_no;
    tuner_data[path].running = EMU_THREAD_RUN;

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

    DTV_LOGI(TAG, "EmuTunerStop:%d", tuner_data[path].running);

    if (tuner_data[path].running == EMU_THREAD_STOP)
    {
        return 0;
    }

    if (tuner_data[path].running == EMU_THREAD_RUN)
    {
        tuner_data[path].running = EMU_THREAD_STOP;
        do
        {
            usleep(20*1000);
        }while((tuner_data[path].running != EMU_THREAD_STOPPING));
        tuner_data[path].running = EMU_THREAD_STOP;
    }

    CloseTsFile(tuner_data[path].ifd);
    EmuDmxClose(tuner_data[path].ofd);
    DTV_LOGI(TAG, "EmuTunerStop end");

    return 0;
}

int EmuTunerGetState(unsigned char path)
{
    if (!emu_support_soft_tuner || path >= TUNER_DEV_COUNT)
    {
        return 0;
    }

    if (tuner_data[path].running == EMU_THREAD_RUN)
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
    if (EmuTunerGetState(path) && tuner_data[path].config.lockstate)
    {
        return 66;
    }

    return 0;
}

int EmuTunerGetSignalQuality(unsigned char path)
{
    if (EmuTunerGetState(path) && tuner_data[path].config.lockstate)
    {
        return 88;
    }

    return 0;
}


