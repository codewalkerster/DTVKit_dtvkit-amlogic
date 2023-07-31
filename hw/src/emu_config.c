#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include "dtv_log.h"
#define TAG  "EMU_CONFIG"

#include <expat.h>

#include "stbhwtun.h"
#include "emu_internal.h"

#define EMU_TUNER_CONFIG_V1 "/data/vendor/dtvkit/aml_emu_tuner.ini"
#define EMU_TUNER_CONFIG_V2 "/data/vendor/dtvkit/aml_emu_tuner.xml"

#define STREAM_BIT_RATE (20*1024*1024) // 20 Megabits per second
#define MAX_FREQ_NUM    (50)

enum
{
    EMU_CFG_V1 = 1,
    EMU_CFG_V2 = 2,
};

static S_EMU_CONFIG emu_freq_config[MAX_FREQ_NUM];
static int emu_freq_config_num;
static int emu_tuner_config_ver;

static long CalcBitrate(long rate)
{
    if (rate <= 1 || rate >= 50)
    {
        rate = STREAM_BIT_RATE;
    }
    else
    {
        rate = rate * 1024 * 1024;
    }
    return rate;
}

static long GetStreamBitrate(FILE* fd)
{
    char buf[64];
    long rate = 0;

    memset(buf, 0, sizeof(buf));
    if (fgets(buf, sizeof(buf), fd) != NULL)
    {
       rate = atoi(buf);
    }

    DTV_LOGI(TAG, "\nbitrate:%sM\n",buf);
    return CalcBitrate(rate);
}

static int LoadConfigV1(FILE* fp)
{
    emu_freq_config[0].tunerid   = 0;
    emu_freq_config[0].lockstate = 1;
    emu_freq_config[0].bitrate   = GetStreamBitrate(fp);
    emu_freq_config_num = 1;
    return 0;
}


static void ElemStartHandler (void *userData, const XML_Char *name, const XML_Char **atts)
{
    S_EMU_CONFIG *pConfig;
    const XML_Char   **att, *an, *av;

    if (!strcmp(name, "freq"))
    {
        if (emu_freq_config_num >= MAX_FREQ_NUM)
        {
            return;
        }

        pConfig = &emu_freq_config[emu_freq_config_num++];
        memset(pConfig, 0, sizeof(S_EMU_CONFIG));
        pConfig->lockstate = 1;
        pConfig->bitrate   = STREAM_BIT_RATE;

        att = atts;
        while (*att)
        {
            an = att[0];
            av = att[1];

            if (!strcmp(an, "tunerid"))
            {
                int i;
                i = strtol(av, NULL, 0);
                pConfig->tunerid = i;
            }
            else if (!strcmp(an, "modulation"))
            {
                if (!strcmp(av, "DVBS"))
                {
                    pConfig->modulation = TUNE_SIGNAL_QPSK;
                }
                else if (!strcmp(av, "DVBT"))
                {
                    pConfig->modulation = TUNE_SIGNAL_COFDM;
                }
                else if (!strcmp(av, "DVBC"))
                {
                    pConfig->modulation = TUNE_SIGNAL_QAM;
                }
                else if (!strcmp(av, "ISDBT"))
                {
                    pConfig->modulation = TUNE_SIGNAL_ISDBT;
                }
                else
                {
                    pConfig->modulation = 0;
                }
            }
            else if (!strcmp(an, "frequency"))
            {
                unsigned int i;
                i = strtol(av, NULL, 0);
                pConfig->freq = i;
            }
            else if (!strcmp(an, "lockstate"))
            {
                if (!strcmp(av, "true"))
                {
                    pConfig->lockstate = 1;
                }
                else
                {
                    pConfig->lockstate = 0;
                }
            }
            else if (!strcmp(an, "filename"))
            {
                strncpy(pConfig->name, av, sizeof(pConfig->name));
            }
            else if (!strcmp(an, "bitrate"))
            {
                pConfig->bitrate = CalcBitrate(strtol(av, NULL, 0));
            }
            att += 2;
        }
        //DTV_LOGI(TAG, "read config:%d %d %d %d %s %d" , pConfig->tunerid,
        //            pConfig->modulation, pConfig->lockstate,
        //            pConfig->freq, pConfig->name,pConfig->bitrate);
    }
}

static void ElemEndHandler (void *userData, const XML_Char *name)
{
}

static int LoadConfigV2(FILE* fp)
{
    XML_Parser      parser;
    enum XML_Status status;
    int             i;

    parser = XML_ParserCreate(NULL);
    if (!parser) {
        DTV_LOGI(TAG, "XML_ParserCreate failed");
        return -1;
    }

    XML_SetElementHandler(parser, ElemStartHandler, ElemEndHandler);
    while (1)
    {
        char    buf[512];
        size_t  len;
        int     is_end;

        len    = fread(buf, 1, sizeof(buf), fp);
        is_end = (len == 0) ? 1 : 0;

        status = XML_Parse(parser, buf, len, is_end);
        if (status == XML_STATUS_ERROR)
        {
            break;
        }

        if (is_end)
            break;
    }

    return 0;
}

/*
1, support emu tuner
*/
int EmuCfgLoad()
{
    FILE* fd;
    int soft_tuner       = 0;

    emu_tuner_config_ver = 0;
    emu_freq_config_num  = 0;

    fd = fopen(EMU_TUNER_CONFIG_V2, "rb");
    if (fd != NULL)
    {
        DTV_LOGI(TAG, "LoadConfigV2");
        LoadConfigV2(fd);
        emu_tuner_config_ver = EMU_CFG_V2;
        soft_tuner = 1;
        fclose(fd);
    }
    else
    {
        fd = fopen(EMU_TUNER_CONFIG_V1, "r");
        if (fd != NULL)
        {
            DTV_LOGI(TAG, "LoadConfigV1");
            LoadConfigV1(fd);
            emu_tuner_config_ver = EMU_CFG_V1;
            soft_tuner = 1;
            fclose(fd);
        }
    }

    return soft_tuner;
}


int EmuCfgGetConfig(unsigned int tunerid, unsigned int freq, unsigned int modulation, S_EMU_CONFIG *config)
{
    int ret = 0;

    if (config == NULL)
    {
        return -1;
    }

    if (emu_tuner_config_ver == EMU_CFG_V1)
    {
        config->tunerid    = tunerid;
        config->freq       = freq;
        config->modulation = modulation;
        config->bitrate    = emu_freq_config[0].bitrate;
        config->lockstate  = emu_freq_config[0].lockstate;
        snprintf(config->name, sizeof(config->name), "/data/vendor/dtvkit/%d.ts", freq);
    }
    else if (emu_tuner_config_ver == EMU_CFG_V2)
    {
        int i;

        for (i=0; i<emu_freq_config_num; i++)
        {
            if (tunerid    == emu_freq_config[i].tunerid &&
                freq       == emu_freq_config[i].freq    &&
                modulation == emu_freq_config[i].modulation)
            {
                memcpy(config, &emu_freq_config[i], sizeof(S_EMU_CONFIG));
                break;
            }
        }

        if (i == emu_freq_config_num)
        {
            ret = -1;
        }
    }

    return ret;
}


