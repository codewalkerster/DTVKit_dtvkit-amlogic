#include <pthread.h>
#include <stdbool.h>

#include "techtype.h"
#include "dbgfuncs.h"
#include "emu_internal.h"
#include "wrapper_frontend.h"
#include "atv_vdin_tvafe.h"
#include "atv_vlfend.h"
#include "atv_fend_internal.h"
#include "stbhwos.h"
#include "stbhwav.h"


typedef struct _atv_tun_param_
{
    U32BIT audio_mode;
    U32BIT frequency;
    U32BIT std;
    U32BIT flag;
} ATV_TUN_PARAM;

typedef enum _atv_tun_sigtype_
{
    EN_ATV_TUN_SIGTYPE_AIR = 0,
    EN_ATV__TUN_SIGTYPE_CABLE
} ATV_TUN_SIGTYPE;

typedef struct _atv_scan_data_
{
    ATV_TUN_SIGTYPE atv_sigtype;
    U32BIT frequency;
    U32BIT freq_table_idx;
    U32BIT afc_range;
    S32BIT dev_no;
    U32BIT analog_standard;
    U32BIT analog_audio_mode;
} ATV_SCAN_DATA;

enum FrontendAtfAnalogStd
{
    ATF_UNDEFINED = 0,

    ATF_AUTO = 1 << 0,

    ATF_PAL = 1 << 1,

    ATF_PAL_M = 1 << 2,

    ATF_PAL_N = 1 << 3,

    ATF_PAL_60 = 1 << 4,

    ATF_NTSC = 1 << 5,

    ATF_NTSC_443 = 1 << 6,

    ATF_SECAM = 1 << 7
};

enum FrontendAtfAnalogAudioMode {
    ATF_AUDIO_MODE_UNDEFINED = 0,

    ATF_AUDIO_MODE_AUTO = 1 << 0,

    ATF_AUDIO_MODE_BG = 1 << 1,

    ATF_AUDIO_MODE_BG_A2 = 1 << 2,

    ATF_AUDIO_MODE_BG_NICAM = 1 << 3,

    ATF_AUDIO_MODE_I = 1 << 4,

    ATF_AUDIO_MODE_DK = 1 << 5,

    ATF_AUDIO_MODE_DK1_A2 = 1 << 6,

    ATF_AUDIO_MODE_DK2_A2 = 1 << 7,

    ATF_AUDIO_MODE_DK3_A2 = 1 << 8,

    ATF_AUDIO_MODE_DK_NICAM = 1 << 9,

    ATF_AUDIO_MODE_L = 1 << 10,

    ATF_AUDIO_MODE_M = 1 << 11,

    ATF_AUDIO_MODE_M_BTSC = 1 << 12,

    ATF_AUDIO_MODE_M_A2 = 1 << 13,

    ATF_AUDIO_MODE_M_EIAJ = 1 << 14,

    ATF_AUDIO_MODE_I_NICAM = 1 << 15,

    ATF_AUDIO_MODE_L_NICAM = 1 << 16,

    ATF_AUDIO_MODE_L_PRIME = 1 << 17
};


#define LOG_TAG "ATV_TUN_AFC"


static ATV_TUN_PARAM atv_tun_param = {0, 0, 0, 0};
static ATV_SCAN_DATA atv_scan_data = {0, 0, 0, 0, 0, 0, 0};
static AM_FEND_Callback_t call_back = NULL;
static void *g_atv_tun_queue = NULL;
static int g_dev_no = 0;

static void atv_tuner_EventCallback(BOOLEAN repeat, U16BIT event_class, U16BIT event_type, void *data, U32BIT data_size);
static void* atv_tun_thread(void *arg);
static U32BIT convert_atf_analog_std(U32BIT analog_std);
static U32BIT convert_atf_analog_audio_mode(U32BIT analog_audio_mode, U32BIT analog_std);

/*
void setAtvSearchstatus(int searched)
{
}

void initCurrentSignalInfo()
{
}

void setChannelLockd(int locked)
{
}

int getCurrentSignalInfo(int *fmt, int *transFmt, int *status, int *frameRate)
{
    return 0;
}

int stop_vdin_signal_detect()
{
    return 0;
}

int close_vdin_signal_detect()
{
    return 0;
}

int start_vdin_signal_detect(AM_VDIN_STATUS_Callback_t cb)
{
    return 0;
}

int set_tvafe(int videoStd, int audioStd, int vfmt)
{
    //Wrapper_TuneSetSearchMode(0, TRUE);
    return 0;
}

*/

int test_atv_vlfend_open()
{
    return 0;
}

int test_atv_vlfend_getstatus()
{
    return 0;
}

int test_atv_vlfend_getpara()
{
    return 0;
}


static U32BIT convert_analog_std_to_atf(U32BIT analog_std)
{
    U32BIT video_std = ATF_AUTO;

    if (analog_std == (V4L2_COLOR_STD_PAL|V4L2_STD_PAL_M))
    {
        video_std = ATF_PAL_M;
    }
    else if (analog_std == (V4L2_COLOR_STD_PAL|V4L2_STD_PAL_Nc))
    {
        video_std = ATF_PAL_N;
    }
    else if (analog_std == (V4L2_COLOR_STD_PAL|V4L2_STD_PAL_I))
    {
        video_std = ATF_PAL;  //?
    }
    else if (analog_std == (V4L2_COLOR_STD_NTSC|V4L2_STD_NTSC_M))
    {
         video_std = ATF_NTSC;
    }
    else if (analog_std == V4L2_COLOR_STD_SECAM)
    {
        video_std = ATF_SECAM;
    }
    else if (analog_std == 0)
    {
        video_std = ATF_AUTO;
    }

    return video_std;
}

static U32BIT convert_analog_audio_mode_to_atf(U32BIT analog_audio_mode)
{
    U32BIT audio_mode;

    if (analog_audio_mode == V4L2_STD_SECAM_DK || analog_audio_mode == V4L2_STD_PAL_DK)
    {
        audio_mode = ATF_AUDIO_MODE_DK;
    }
    else if (analog_audio_mode == V4L2_STD_PAL_I)
    {
        audio_mode = ATF_AUDIO_MODE_I;
    }
    else if (analog_audio_mode == V4L2_STD_SECAM_B || analog_audio_mode == V4L2_STD_SECAM_G || analog_audio_mode == V4L2_STD_PAL_BG)
    {
        audio_mode = ATF_AUDIO_MODE_BG;
    }
    else if (analog_audio_mode == V4L2_STD_NTSC_M || analog_audio_mode == V4L2_STD_PAL_M)
    {
        audio_mode = ATF_AUDIO_MODE_M;
    }
    else if (analog_audio_mode == V4L2_STD_SECAM_L)
    {
        audio_mode = ATF_AUDIO_MODE_L;
    }
    else if (analog_audio_mode == 0)
    {
        audio_mode = ATF_AUDIO_MODE_AUTO;
    }
    else
    {
        audio_mode = ATF_AUDIO_MODE_DK;
    }

    return audio_mode;
}



void start_search(int dev_no, BOOLEAN searched)
{
    Wrapper_TuneSetSearchMode(dev_no, searched);
    if (searched == false)
    {
        atv_tun_param.std = convert_analog_std_to_atf(atv_tun_param.std);
        atv_tun_param.audio_mode = convert_analog_audio_mode_to_atf(atv_tun_param.audio_mode);
    }

    Wrapper_TuneStartTuner(dev_no, atv_tun_param.frequency, 0,
                           (EW_STB_TUNE_FEC)0, (EW_STB_TUNE_TMODE)255,
                           (EW_STB_TUNE_TBWIDTH)0, (EW_STB_TUNE_CMODE)255,
                           atv_tun_param.flag, atv_tun_param.audio_mode, atv_tun_param.std);

}

int AM_VLFEND_FormatFrequency(int freq)
{
    return freq;
}

AM_ErrorCode_t AM_VLFEND_SetMode(int dev_no, int mode)
{
    return AM_SUCCESS;
}

AM_ErrorCode_t AM_VLFEND_SetSlowSearchMode(int dev_no, int enable)
{
    return AM_SUCCESS;
}

AM_ErrorCode_t AM_VLFEND_SetPara(int dev_no, const struct dvb_frontend_parameters *para)
{
    if (NULL == para || dev_no < 0)
    {
        return AM_FAILURE;
    }

    atv_tun_param.frequency = para->frequency;
    atv_tun_param.flag = para->u.analog.flag;
    atv_tun_param.std = para->u.analog.std;
    atv_tun_param.audio_mode = para->u.analog.audmode;

    return AM_SUCCESS;
}


AM_ErrorCode_t AM_VLFEND_GetPara(int dev_no, struct dvb_frontend_parameters *para)
{
    if (NULL == para || dev_no < 0)
    {
        return AM_FAILURE;
    }

    para->frequency = atv_scan_data.frequency;
    para->u.analog.std = convert_atf_analog_std(atv_scan_data.analog_standard);
    para->u.analog.audmode = convert_atf_analog_audio_mode(atv_scan_data.analog_audio_mode, atv_scan_data.analog_standard);

    return AM_SUCCESS;
}

AM_ErrorCode_t AM_VLFEND_ActiveThread(int dev_no, AM_Bool_t active)
{
    Wrapper_SendEvent callback = atv_tuner_EventCallback;

    Wrapper_TuneSetSearchMode(dev_no, TRUE);
    Wrapper_TuneSetSignalType(dev_no, WRAPPER_TUNE_SIGNAL_ANALOG);
    Wrapper_RegisterCallback(callback);
    return AM_SUCCESS;
}

AM_ErrorCode_t AM_VLFEND_SetCallback(int dev_no, AM_FEND_Callback_t cb, void *user_data, int user_data_len)
{
    if (NULL == user_data)
    {
        return AM_FAILURE;
    }

    call_back = cb;

    memcpy(&atv_scan_data, user_data, user_data_len);

    return AM_SUCCESS;
}

AM_ErrorCode_t AM_VLFEND_UpdateCallbackData(int dev_no, void *user_data, int user_data_len)
{
    if (NULL == user_data)
    {
        return AM_FAILURE;
    }

    //Wrapper_TuneSetSearchMode(dev_no, TRUE);

    memcpy(&atv_scan_data, user_data, user_data_len);

    atv_tun_param.frequency = atv_scan_data.frequency;

    return AM_SUCCESS;
}


AM_ErrorCode_t AM_VLFEND_Open(int dev_no, const AM_FEND_OpenPara_t *para)
{
    static bool opened = false;

    g_dev_no = dev_no;

    if (opened == false)
    {
        opened = true;

        g_atv_tun_queue = STB_OSCreateQueue(sizeof(U16BIT), 10);
        if (NULL == g_atv_tun_queue)
        {
            DTV_LOGE(LOG_TAG, "[%s] Create Queue Err!", __FUNCTION__);

            return AM_FAILURE;
        }

        if (STB_OSCreateTask(atv_tun_thread, (void *)NULL, VLFEND_TASK_STACK_SIZE, VLFEND_TASK_PRIORITY, "atvTun") == NULL)
        {
            DTV_LOGE(LOG_TAG, "[%s] Create Task Err!", __FUNCTION__);

            return AM_FAILURE;
        }
    }

    return AM_SUCCESS;
}

AM_ErrorCode_t AM_VLFEND_Close(int dev_no)
{
    return AM_SUCCESS;
}

AM_ErrorCode_t AM_VLFEND_SetSoundOutputMode(int dev_no, int mode)
{
    return AM_SUCCESS;
}

AM_ErrorCode_t AM_VLFEND_GetSoundSystem(int dev_no, int *sys)
{
    return AM_SUCCESS;
}

AM_ErrorCode_t AM_VLFEND_EnableAFC(int dev_no, int enable)
{
    return AM_SUCCESS;
}

AM_ErrorCode_t AM_VLFEND_AFCState(int dev_no, int *state)
{
    return AM_SUCCESS;
}

static void atv_tuner_EventCallback(BOOLEAN repeat, U16BIT event_class, U16BIT event_type, void *data, U32BIT data_size)
{
    int ret;

    ret = STB_OSWriteQueue(g_atv_tun_queue, (void *)&event_type, sizeof(U16BIT), TIMEOUT_NEVER);
    if (!ret)
    {
        DTV_LOGI(LOG_TAG,"%s Send Err", __FUNCTION__);
    }
    else
    {
        DTV_LOGI(LOG_TAG,"%s Send Succ", __FUNCTION__);
    }

}



static void* atv_tun_thread(void *arg)
{
    U16BIT atv_lock_state;
    struct dvb_frontend_event evt;

    while (1)
    {
        if (STB_OSReadQueue(g_atv_tun_queue, &atv_lock_state, sizeof(U16BIT), TIMEOUT_NEVER))
        {
            switch (atv_lock_state)
            {
                case WRPPER_HW_EV_TYPE_LOCKED:
                    evt.status = FE_HAS_LOCK;
                    int count = 0;
                    while (count < 100)
                    {
                        BOOLEAN analog_standard_flag;
                        U32BIT analog_standard;
                        BOOLEAN analog_audio_mode_flag;
                        U32BIT analog_audio_mode;

                        analog_standard_flag = Wrapper_TuneGetActualAnalogStandard(g_dev_no, &atv_scan_data.analog_standard);
                        analog_audio_mode_flag = Wrapper_TuneGetActualAnalogAudioMode(g_dev_no, &atv_scan_data.analog_audio_mode);

                        if ((TRUE == analog_standard_flag) && (TRUE == analog_audio_mode_flag))
                        {
                            atv_scan_data.frequency = Wrapper_TuneGetActualAnalogFreq(g_dev_no);
                            break;
                        }

                        STB_OSTaskDelay(100);

                        count ++;
                    }
                    break;
                case WRPPER_HW_EV_TYPE_NOTLOCKED:
                    evt.status = FE_TIMEDOUT;
                    break;
            }

            call_back(g_dev_no, &evt, &atv_scan_data);
        }
    }

    DTV_LOGI(LOG_TAG,"%s o o  task exit....", __FUNCTION__);
    return NULL;
}

static U32BIT convert_atf_analog_std(U32BIT analog_std)
{
    U32BIT video_std = V4L2_COLOR_STD_PAL;

    if (analog_std == ATF_PAL_M)
    {
        video_std = V4L2_COLOR_STD_PAL|V4L2_STD_PAL_M;
    }
    else if (analog_std == ATF_PAL_N)
    {
        video_std = V4L2_COLOR_STD_PAL|V4L2_STD_PAL_Nc;
    }
    else if (analog_std == ATF_PAL)
    {
        video_std = V4L2_COLOR_STD_PAL|V4L2_STD_PAL_I;  //?
    }
    else if (analog_std == ATF_NTSC)
    {
        video_std = V4L2_COLOR_STD_NTSC|V4L2_STD_NTSC_M;
    }
    else if (analog_std == ATF_SECAM)
    {
        video_std = V4L2_COLOR_STD_SECAM;
    }
    else if (analog_std == ATF_AUTO)
    {
        video_std = 0;
    }

    return video_std;
}

static U32BIT convert_atf_analog_audio_mode(U32BIT analog_audio_mode, U32BIT analog_std)
{
    U32BIT audio_mode;

    if (analog_audio_mode == ATF_AUDIO_MODE_DK)
    {
        if (analog_std == ATF_SECAM)
        {
            audio_mode = V4L2_STD_SECAM_DK;
        }
        else
        {
            audio_mode = V4L2_STD_PAL_DK;
        }
    }
    else if (analog_audio_mode == ATF_AUDIO_MODE_I)
    {
        audio_mode = V4L2_STD_PAL_I;
    }
    else if (analog_audio_mode == ATF_AUDIO_MODE_BG)
    {
        if (analog_std == ATF_SECAM)
        {
            audio_mode = V4L2_STD_SECAM_B;
        }
        else if (analog_std == ATF_SECAM)
        {
            audio_mode = V4L2_STD_SECAM_G;
        }
        else
        {
            audio_mode = V4L2_STD_PAL_BG;
        }
    }
    else if (analog_audio_mode == ATF_AUDIO_MODE_M)
    {
        if (analog_std == ATF_NTSC)
        {
            audio_mode = V4L2_STD_NTSC_M;
        }
        else
        {
            audio_mode = V4L2_STD_PAL_M;
        }
    }
    else if (analog_audio_mode == ATF_AUDIO_MODE_L)
    {
        audio_mode = V4L2_STD_SECAM_L;
    }
    /*else if (analog_audio_mode == ATF_AUDIO_MODE_IC)
    {
        audio_mode = V4L2_STD_SECAM_LC;
    }*/
    else if (analog_audio_mode == ATF_AUDIO_MODE_AUTO)
    {
        audio_mode = 0;
    }
    else
    {
        if (analog_std == ATF_SECAM)
        {
            audio_mode = V4L2_STD_SECAM_DK;
        }
        else
        {
            audio_mode = V4L2_STD_PAL_DK;
        }
    }

    return audio_mode;
}


