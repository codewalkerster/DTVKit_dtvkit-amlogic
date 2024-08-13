/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief for tvin tvafe interfaces
 *
 * \author kieth.liu <kieth.liu@amlogic.com>
 * \date 2023-07-09: create the document
 ***************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include <math.h>
#include <errno.h>
#include <sys/epoll.h>
#include <stdlib.h>


#include <pthread.h>
#ifdef RDK_COMPILE
#include <sys/ioctl.h>
#endif

#include "techtype.h"
#include "atv_vlfend.h"
#include "atv_fend_internal.h"
#include "atv_vdin_tvafe.h"
#include "../src/systemcontrol.h"

#include "dtv_log.h"
#include "stbhwcfg.h"
#include "stbhwtun.h"
#include "stbhwresm.h"

#define TAG "TUNER"



/****************************************************************************
 * Macro definitions
 ***************************************************************************/


#define TVIN_IOC_MAGIC 'T'
#define TVIN_IOC_OPEN               _IOW(TVIN_IOC_MAGIC, 0x01, struct tvin_parm_s)
#define TVIN_IOC_START_DEC          _IOW(TVIN_IOC_MAGIC, 0x02, struct tvin_parm_s)
#define TVIN_IOC_STOP_DEC           _IO(TVIN_IOC_MAGIC, 0x03)
#define TVIN_IOC_CLOSE              _IO(TVIN_IOC_MAGIC, 0x04)
#define TVIN_IOC_G_SIG_INFO         _IOR(TVIN_IOC_MAGIC, 0x07, struct tvin_info_s)
#define TVIN_IOC_S_AFE_CVBS_STD     _IOW(TVIN_IOC_MAGIC, 0x1b, enum tvin_sig_fmt_e)
#define TVIN_IOC_S_AFE_SONWON       _IO(TVIN_IOC_MAGIC, 0x22)
#define TVIN_IOC_S_AFE_SONWOFF      _IO(TVIN_IOC_MAGIC, 0x23)
#define TVIN_IOC_SNOWON             _IO(TVIN_IOC_MAGIC, 0x47)
#define TVIN_IOC_SNOWOFF            _IO(TVIN_IOC_MAGIC, 0x48)

#define SYS_VFM_MAP_PATH            "/sys/class/vfm/map"

/****************************************************************************
 * Structure definitions
 ***************************************************************************/
enum tvin_port_e {
    TVIN_PORT_CVBS3 = 0x00001003,
};

enum tvin_sig_fmt_e {
    TVIN_SIG_FMT_NULL = 0,
    //Video Formats
    TVIN_SIG_FMT_CVBS_NTSC_M                        = 0x601,
    TVIN_SIG_FMT_CVBS_NTSC_443                      = 0x602,
    TVIN_SIG_FMT_CVBS_PAL_I                         = 0x603,
    TVIN_SIG_FMT_CVBS_PAL_M                         = 0x604,
    TVIN_SIG_FMT_CVBS_PAL_60                        = 0x605,
    TVIN_SIG_FMT_CVBS_PAL_CN                        = 0x606,
    TVIN_SIG_FMT_CVBS_SECAM                         = 0x607,
    TVIN_SIG_FMT_CVBS_NTSC_50                       = 0x608,
    TVIN_SIG_FMT_CVBS_MAX                           = 0x609,
    TVIN_SIG_FMT_CVBS_THRESHOLD                     = 0x800,
    TVIN_SIG_FMT_MAX,
};

enum tvin_trans_fmt {
    TVIN_TFMT_2D = 0,
};

enum tvin_aspect_ratio_e {
    TVIN_ASPECT_NULL = 0,
    TVIN_ASPECT_1x1,
    TVIN_ASPECT_4x3_FULL,
    TVIN_ASPECT_14x9_FULL,
    TVIN_ASPECT_14x9_LB_CENTER,
    TVIN_ASPECT_14x9_LB_TOP,
    TVIN_ASPECT_16x9_FULL,
    TVIN_ASPECT_16x9_LB_CENTER,
    TVIN_ASPECT_16x9_LB_TOP,
    TVIN_ASPECT_MAX,
};

enum tvin_color_fmt_e {
    RGB444 = 0,
    YUV422, // 1
    YUV444, // 2
    YUYV422,// 3
    YVYU422,// 4
    UYVY422,// 5
    VYUY422,// 6
    NV12,   // 7
    NV21,   // 8
    BGGR,   // 9  raw data
    RGGB,   // 10 raw data
    GBRG,   // 11 raw data
    GRBG,   // 12 raw data
    COLOR_FMT_MAX,
};

typedef struct tvin_info_s {
    enum tvin_trans_fmt trans_fmt;
    enum tvin_sig_fmt_e fmt;
    enum tvin_sig_status_e status;
    enum tvin_color_fmt_e cfmt;
    unsigned int fps;
    unsigned int is_dvi;
    unsigned int signal_type;
    unsigned int input_colorimetry;
    enum tvin_aspect_ratio_e aspect_ratio;
    __u8 amdolby_vision;
    __u8 low_latency;
}tvin_info_t;


struct tvin_parm_s
{
    int index;                      // index of frontend for vdin
    enum tvin_port_e port;          // must set port in IOCTL
    struct tvin_info_s info;
    unsigned int hist_pow;
    unsigned int luma_sum;
    unsigned int pixel_sum;
    unsigned short histgram[64];
    unsigned int flag;
    unsigned short dest_width;      //for vdin horizontal scale down
    unsigned short dest_height;     //for vdin vertical scale down
    BOOLEAN h_reverse;                 //for vdin horizontal reverse
    BOOLEAN v_reverse;                 //for vdin vertical reverse
    unsigned int reserved;
};

enum EPOLL_OP {
    ADD = EPOLL_CTL_ADD,
    MOD = EPOLL_CTL_MOD,
    DEL = EPOLL_CTL_DEL
};

enum atv_audo_std_e {
    CC_ATV_AUDIO_STD_START = 0,
    CC_ATV_AUDIO_STD_DK = 0,
    CC_ATV_AUDIO_STD_I,
    CC_ATV_AUDIO_STD_BG,
    CC_ATV_AUDIO_STD_M,
    CC_ATV_AUDIO_STD_L,
    CC_ATV_AUDIO_STD_LC,
    CC_ATV_AUDIO_STD_AUTO,
    CC_ATV_AUDIO_STD_END = CC_ATV_AUDIO_STD_AUTO,
    CC_ATV_AUDIO_STD_MUTE,
};

enum atv_video_std_e {
    CC_ATV_VIDEO_STD_START = 0,
    CC_ATV_VIDEO_STD_AUTO = 0,
    CC_ATV_VIDEO_STD_PAL,
    CC_ATV_VIDEO_STD_NTSC,
    CC_ATV_VIDEO_STD_SECAM,
    CC_ATV_VIDEO_STD_PAL_M,
    CC_ATV_VIDEO_STD_PAL_N,
    CC_ATV_VIDEO_STD_END = CC_ATV_VIDEO_STD_PAL_N,
};


/****************************************************************************
 * Static data
 ***************************************************************************/
static int fd_tvafe = -1;
static int fd_vdin = -1;
static int fd_epoll = -1;
static int enable_thread = 0;
pthread_mutex_t     lock;
pthread_t           thread;
pthread_cond_t      cond;

AM_VDIN_STATUS_Callback_t call_back = NULL;
static tvin_info_t m_cur_sig_info;

static int mSnowStatusEnable = 0;
static int mSearchStatus = 0;
static int mLocked = 0;
static int mSetPQmode = 0;
static int mSourcePlayed = 0;

static char *str_vstd[] =
{
    "PAL",
    "NTSC",
    "SECAM",
    "UNKNOWN",
};

static char *str_astd[] =
{
    "DK",
    "I",
    "BG",
    "M",
    "L",
    "LC",
    "UNKNOWN",
};

static char *str_cvbs[] =
{
    "PAL_STD",
    "PAL_M",
    "PAL_N",
    "PAL_60",
    "NTSC_M",
    "NTSC_443",
    "SECAM",
    "UNKNOWN",
};

static int mlistener;
static struct SysClientWrapper_t * pSysClientWrapper;
static void SysEventCallback(int color);
static BOOLEAN atv_resm_adc_requested = FALSE;

/****************************************************************************
 * Static functions
 ***************************************************************************/
int open_vdin_port_tvafe()
{
    int ret = 0;
    struct tvin_parm_s vdinParam;

    if (fd_tvafe < 0) {
        fd_tvafe = open("/dev/tvafe0", O_RDWR);
        if (fd_tvafe < 0)
        {
            DTV_LOGE(TAG, "!!! Open tvafe module, error (%s).\n", strerror(errno));
            //return -1;
        }
        DTV_LOGE(TAG, "!!! Waring: Tvafe device is also required for ATV.\n");
    }


    if (fd_vdin < 0) {
        fd_vdin = open("/dev/vdin0", O_RDWR);
        if (fd_vdin < 0)
        {
            DTV_LOGE(TAG, "!!! Open vdin module, error (%s).\n", strerror(errno));
            return -1;
        }

        vdinParam.port = TVIN_PORT_CVBS3;
        vdinParam.index = 0;

        ret = ioctl(fd_vdin, TVIN_IOC_STOP_DEC);
        if (ret < 0)
        {
        DTV_LOGE(TAG, "!!! ioctl TVIN_IOC_STOP_DEC, error (%s).\n", strerror(errno));
        }

        ret = ioctl(fd_vdin, TVIN_IOC_OPEN, &vdinParam);
        if (ret < 0)
        {
            DTV_LOGE(TAG, "!!! ioctl TVIN_IOC_OPEN, error (%s).\n", strerror(errno));
            return ret;
        }
    } else {
            vdinParam.port = TVIN_PORT_CVBS3;
            vdinParam.index = 0;
            ret = ioctl(fd_vdin, TVIN_IOC_OPEN, &vdinParam);
            if (ret < 0)
            {
                DTV_LOGE(TAG, "!!! ioctl TVIN_IOC_OPEN, error (%s).\n", strerror(errno));
                return ret;
            }
    }

    if (!mlistener) {
    #ifndef RDK_COMPILE
        pSysClientWrapper = SC_getInstance();
        SC_setSysClientCallback(SysEventCallback);
    #endif
        mlistener = 1;
    }
    return ret;
}

int close_vdin_port_tvafe()
{
    int ret = 0;
    if (mlistener) {
        mlistener = 0;
    #ifndef RDK_COMPILE
        SC_releaseInstance(&pSysClientWrapper);
    #endif
    }
    if (fd_vdin > 0)
    {
        ret = ioctl(fd_vdin, TVIN_IOC_STOP_DEC);
        if (ret < 0)
        {
            DTV_LOGE(TAG, "!!! ioctl TVIN_IOC_STOP_DEC, error (%s).\n", strerror(errno));
        }

        ret = ioctl(fd_vdin, TVIN_IOC_CLOSE);
        if (ret < 0)
        {
        DTV_LOGE(TAG, "!!! ioctl TVIN_IOC_CLOSE, error (%s).\n", strerror(errno));
        }
        close(fd_vdin);
        fd_vdin = -1;
    }

    if (fd_tvafe > 0)
    {
        close(fd_tvafe);
        fd_tvafe = -1;
    }

    return 0;
}

int vdin_get_signal_info ( struct tvin_info_s *Info )
{
    int rt = ioctl(fd_vdin, TVIN_IOC_G_SIG_INFO, Info);
    if ( rt < 0 ) {
        DTV_LOGE(TAG, "Vdin get signal info, error(%s), ret = %d.\n", strerror ( errno ), rt );
    }
    return rt;
}

int stop_vdin_dec()
{
    return ioctl(fd_vdin, TVIN_IOC_STOP_DEC);
}

int close_vdin_port()
{
    return ioctl(fd_vdin, TVIN_IOC_CLOSE);
}

int start_vdin_dec(struct tvin_info_s signal_info)
{
    int ret = 0;
    struct tvin_parm_s vdinParam;

    vdinParam.port = TVIN_PORT_CVBS3;
    vdinParam.index = 0;
    vdinParam.info = signal_info;

    ret = ioctl(fd_vdin, TVIN_IOC_STOP_DEC);
    if (ret < 0)
    {
        DTV_LOGE(TAG, "!!! ioctl TVIN_IOC_STOP_DEC, error (%s).\n", strerror(errno));
    }

    ret = ioctl(fd_vdin, TVIN_IOC_START_DEC, &vdinParam);
    if (ret < 0)
    {
        DTV_LOGE(TAG, "!!! ioctl TVIN_IOC_START_DEC, error (%s).\n", strerror(errno));
    }

    return ret;
}

int vdin_signal_handle()
{
    //struct tvin_info_s Info;
    BOOLEAN tvin_db_reg;
    int ret = vdin_get_signal_info ( &m_cur_sig_info );
    if (ret < 0) {
        m_cur_sig_info.status = TVIN_SIG_STATUS_NULL;
        return ret;
    }
    DTV_LOGE(TAG, "trans_fmt is %d,fmt is %d, status is %d\n", m_cur_sig_info.trans_fmt, m_cur_sig_info.fmt, m_cur_sig_info.status);
    DTV_LOGI(TAG, "mSearchStatus: %d\n", mSearchStatus);

    if (m_cur_sig_info.status == TVIN_SIG_STATUS_STABLE ) {
    #ifndef RDK_COMPILE
        SC_setATVVideoColor(1, 0, 5);
    #endif
        if (mSetPQmode) {
        #ifndef RDK_COMPILE
            SC_setDisplayMode(SC_getDisplayMode(), 0);
        #endif
            mSetPQmode = 0;
        }
    #ifndef RDK_COMPILE
        SC_SetCurrentSourceInfo(0, m_cur_sig_info.fmt, m_cur_sig_info.trans_fmt);
    #endif
        set_atv_snow_status(0);
        ret = start_vdin_dec(m_cur_sig_info);
        DTV_LOGI(TAG, "mLocked: %d\n", mLocked);
        if (!mLocked && !mSearchStatus) {
        #ifndef RDK_COMPILE
            SC_setATVVideoColor(1, 0, 6);
        #endif
        }
        if (ret == 0) {
            tvin_db_reg = STB_Get_Tvin_Db_Reg_Enabled();
            DTV_LOGI(TAG, "tvin_db_reg: %d\n", tvin_db_reg);
            if (tvin_db_reg) {
            #ifndef RDK_COMPILE
                SC_SetCVD2Values();
            #endif
            }
        }
        if (call_back) {
            call_back(m_cur_sig_info.status);
        }
    } else if (m_cur_sig_info.status == TVIN_SIG_STATUS_UNSTABLE ) {
        if (!mSearchStatus) {
        #ifndef RDK_COMPILE
            SC_setATVVideoColor(1, 0, 5);
        #endif
            ret = stop_vdin_dec();
        }
    } else if (m_cur_sig_info.status == TVIN_SIG_STATUS_NOTSUP ) {
        if (!mSearchStatus) {
        #ifndef RDK_COMPILE
            SC_setATVVideoColor(1, 0, 5);
        #endif
            ret = stop_vdin_dec();
        }
    } else if (m_cur_sig_info.status == TVIN_SIG_STATUS_NOSIG ) {
    #ifndef RDK_COMPILE
        SC_setATVVideoColor(1, 0, 5);
        if (5 != SC_getDisplayMode()) {//5:VPP_DISPLAY_MODE_FULL
            SC_setDisplayMode(5, 0);//no sig need full screen
            mSetPQmode = 1;
        }
    #endif
        set_atv_snow_status(1);
        ret = start_vdin_dec(m_cur_sig_info);
    #ifndef RDK_COMPILE
        if (SC_getScreenColorSetting() != VIDEO_LAYER_COLOR_BLUE || mSearchStatus) {
            SC_setATVVideoColor(1, 0, 6);
        } else {
            SC_setATVVideoColor(0, 0, 5);
        }
    #endif
        if (call_back) {
            call_back(m_cur_sig_info.status);
        }
    } else {
        initCurrentSignalInfo();
    }

    return ret;
}


struct epoll_event backEvents[2];

int Epoll_isvalid()
{
    if (fd_epoll < 0)
        return 0;
    else
        return 1;
}

int Epoll_add(int fd, struct epoll_event *event)
{
    if (Epoll_isvalid())
    {
        return epoll_ctl(fd_epoll, ADD, fd, event);
    }
    return -1;
}

int Epoll_delete(int fd)
{
    if (Epoll_isvalid())
    {
        return epoll_ctl(fd_epoll, DEL, fd, NULL);
    }
    return -1;
}

int Epoll_wait()
{
    if (Epoll_isvalid())
    {
        return epoll_wait(fd_epoll, backEvents, 2, 5);
    }
    return -1;
}

void* signal_detect_thread(void *arg)
{
    while (enable_thread)
    {
        int num = Epoll_wait();
        for (int i = 0; i < num; ++i) {
            int fd = backEvents[i].data.fd;
            /**
             * EPOLLIN event
             */
            if (backEvents[i].events & EPOLLIN) {
                if ( fd == fd_vdin ) {
                    if (mSourcePlayed) {
                        vdin_signal_handle();
                   } else {
                        DTV_LOGI(TAG, "%s: mSource has stoped\n", __FUNCTION__);
                   }

                }
            }
        }
    }//exit
    return 0;
}

int Epoll_create()
{
    struct epoll_event m_event;

    if (!Epoll_isvalid())
    {
        fd_epoll = epoll_create(2);

        if (fd_vdin >0) {
            m_event.data.fd = fd_vdin;
            m_event.events = EPOLLIN | EPOLLET;
            Epoll_add(fd_vdin, &m_event);

            pthread_mutex_init(&lock, NULL);
            pthread_cond_init(&cond, NULL);
            enable_thread = 1;
            if (pthread_create(&thread, NULL, signal_detect_thread, NULL)) {
                pthread_mutex_destroy(&lock);
                pthread_cond_destroy(&cond);
            }
        }
    }
    return fd_epoll;
}

int start_vdin_signal_detect(AM_VDIN_STATUS_Callback_t cb)
{
    if (STB_TuneIsTvPlatform() && !atv_resm_adc_requested && STB_Resman_Support())
    {
        if (!STB_Resman_Request(RESMAN_APP_DVBKIT, RESMAN_ID_ADC_PLL, 2000))
        {
            DTV_LOGI(TAG, "STB_Resman_Request RESMAN_ID_ADC_PLL failed!!!");

            return FALSE;
        }

        atv_resm_adc_requested = TRUE;

        DTV_LOGI(TAG, "STB_Resman_Request RESMAN_ID_ADC_PLL OK.");
    }

    open_vdin_port_tvafe();
#ifndef RDK_COMPILE
    SC_disableTsync();
#endif
    if (Epoll_isvalid()) {
        enable_thread = 1;
    } else {
        Epoll_create();
    }
    call_back = cb;
    mSourcePlayed = 1;
    return 0;
}

int close_vdin_signal_detect()
{
    call_back = NULL;
    mSourcePlayed = 0;
#ifndef RDK_COMPILE
    SC_setATVVideoColor(1, 0, 5);
#endif
    if (fd_vdin >0) {
        stop_vdin_dec();
        close_vdin_port();
    }
    mLocked = 0;

    if (STB_TuneIsTvPlatform() && atv_resm_adc_requested && STB_Resman_Support())
    {
        STB_Resman_FreeRes(RESMAN_ID_ADC_PLL);

        atv_resm_adc_requested = FALSE;

        DTV_LOGI(TAG, "STB_Resman_FreeRes RESMAN_ID_ADC_PLL OK.");
    }

    return 0;
}

int stop_vdin_signal_detect()
{
    if (fd_vdin >0) {
        stop_vdin_dec();
    }
    return 0;
}

unsigned long enumToStdAndColor(int videoStd, int audioStd)
{
    unsigned long tmpTunerStd = 0;
    if (videoStd == CC_ATV_VIDEO_STD_PAL) {
        tmpTunerStd |= V4L2_COLOR_STD_PAL;
        if (audioStd == CC_ATV_AUDIO_STD_DK) {
            tmpTunerStd |= V4L2_STD_PAL_DK;
        } else if (audioStd == CC_ATV_AUDIO_STD_I) {
            tmpTunerStd |= V4L2_STD_PAL_I;
        } else if (audioStd == CC_ATV_AUDIO_STD_BG) {
            tmpTunerStd |= V4L2_STD_PAL_BG;
        } else if (audioStd == CC_ATV_AUDIO_STD_M) {
            tmpTunerStd |= V4L2_STD_PAL_M;
        }
    } else if (videoStd == CC_ATV_VIDEO_STD_NTSC) {
        tmpTunerStd |= V4L2_COLOR_STD_NTSC;
        if (audioStd == CC_ATV_AUDIO_STD_DK) {
            tmpTunerStd |= V4L2_STD_PAL_DK;
        } else if (audioStd == CC_ATV_AUDIO_STD_I) {
            tmpTunerStd |= V4L2_STD_PAL_I;
        } else if (audioStd == CC_ATV_AUDIO_STD_BG) {
            tmpTunerStd |= V4L2_STD_PAL_BG;
        } else if (audioStd == CC_ATV_AUDIO_STD_M) {
            tmpTunerStd |= V4L2_STD_NTSC_M;
        }
    } else if (videoStd == CC_ATV_VIDEO_STD_SECAM) {
        tmpTunerStd |= V4L2_COLOR_STD_SECAM;
        if (audioStd == CC_ATV_AUDIO_STD_DK) {
            tmpTunerStd |= V4L2_STD_SECAM_DK;
        } else if (audioStd == CC_ATV_AUDIO_STD_I) {
            tmpTunerStd |= V4L2_STD_PAL_I;
        } else if (audioStd == CC_ATV_AUDIO_STD_BG) {
            tmpTunerStd |= (V4L2_STD_SECAM_B | V4L2_STD_SECAM_G);
        } else if (audioStd == CC_ATV_AUDIO_STD_M) {
            tmpTunerStd |= V4L2_STD_NTSC_M;
        } else if (audioStd == CC_ATV_AUDIO_STD_L) {
            tmpTunerStd |= V4L2_STD_SECAM_L;
        } else if (audioStd == CC_ATV_AUDIO_STD_LC) {
            tmpTunerStd |= V4L2_STD_SECAM_LC;
        }
    } else if (videoStd == CC_ATV_VIDEO_STD_PAL_M) {
        tmpTunerStd |= V4L2_STD_PAL_M;
        if (audioStd == CC_ATV_AUDIO_STD_DK) {
            tmpTunerStd |= V4L2_STD_PAL_DK;
        } else if (audioStd == CC_ATV_AUDIO_STD_I) {
            tmpTunerStd |= V4L2_STD_PAL_I;
        } else if (audioStd == CC_ATV_AUDIO_STD_BG) {
            tmpTunerStd |= V4L2_STD_PAL_BG;
        }
    } else if (videoStd == CC_ATV_VIDEO_STD_PAL_N) {
        tmpTunerStd |= V4L2_STD_PAL_Nc;
        if (audioStd == CC_ATV_AUDIO_STD_DK) {
            tmpTunerStd |= V4L2_STD_PAL_DK;
        } else if (audioStd == CC_ATV_AUDIO_STD_I) {
            tmpTunerStd |= V4L2_STD_PAL_I;
        } else if (audioStd == CC_ATV_AUDIO_STD_BG) {
            tmpTunerStd |= V4L2_STD_PAL_BG;
        }
    }
    return tmpTunerStd;
}

int stdEnumToCvbsFmt (int vfmt, unsigned long std)
{
    enum tvin_sig_fmt_e cvbs_fmt = TVIN_SIG_FMT_NULL;
    if ((vfmt & 0xff000000) == V4L2_COLOR_STD_NTSC) {
        switch (vfmt & 0x00ffffff) {
        case V4L2_STD_NTSC_M:
        case V4L2_STD_NTSC_443:
        case V4L2_STD_PAL_I:
            if ((std & V4L2_STD_PAL_M) || (std & V4L2_STD_NTSC_M)) {
                cvbs_fmt = TVIN_SIG_FMT_CVBS_NTSC_M;
            } else if ((std & V4L2_STD_PAL_DK) || (std & V4L2_STD_PAL_BG)
                    || (std & V4L2_STD_PAL_I)) {
                if ((vfmt & 0x00ffffff) == V4L2_STD_NTSC_443) {
                    cvbs_fmt = TVIN_SIG_FMT_CVBS_NTSC_443;
                } else {
                    cvbs_fmt = TVIN_SIG_FMT_CVBS_NTSC_M;
                }
            } else {
                cvbs_fmt = TVIN_SIG_FMT_CVBS_NTSC_M;
            }
            break;
        default:
            cvbs_fmt = TVIN_SIG_FMT_CVBS_NTSC_M;
            break;
        }
    } else if ((vfmt & 0xff000000) == V4L2_COLOR_STD_PAL) {
        switch (vfmt & 0x00ffffff) {
        case V4L2_STD_PAL_DK:
        case V4L2_STD_PAL_BG:
        case V4L2_STD_PAL_I:
        case V4L2_STD_PAL_M:
        case V4L2_STD_NTSC_M:
            if ((std & V4L2_STD_PAL_M) || (std & V4L2_STD_NTSC_M)) {
                cvbs_fmt = TVIN_SIG_FMT_CVBS_PAL_M;
            } else {
                cvbs_fmt = TVIN_SIG_FMT_CVBS_PAL_I;
            }
            break;
        case V4L2_STD_PAL_60:
            if ((std & V4L2_STD_PAL_M) || (std & V4L2_STD_NTSC_M)) {
                cvbs_fmt = TVIN_SIG_FMT_CVBS_PAL_60;
            } else {
                cvbs_fmt = TVIN_SIG_FMT_CVBS_PAL_I;
            }
            break;
        case V4L2_STD_PAL_Nc:
            if ((std & V4L2_STD_PAL_M) || (std & V4L2_STD_NTSC_M) || (std & V4L2_STD_PAL_Nc)) {
                cvbs_fmt = TVIN_SIG_FMT_CVBS_PAL_CN;
            } else {
                cvbs_fmt = TVIN_SIG_FMT_CVBS_PAL_I;
            }
            break;
        default:
            cvbs_fmt = TVIN_SIG_FMT_CVBS_PAL_I;
            break;
        }
    } else if ((vfmt & 0xff000000) == V4L2_COLOR_STD_SECAM) {
        switch (vfmt & 0x00ffffff) {
        case V4L2_STD_SECAM_L:
        case V4L2_STD_SECAM_LC:
            cvbs_fmt = TVIN_SIG_FMT_CVBS_SECAM;
            break;
        default:
            cvbs_fmt = TVIN_SIG_FMT_CVBS_SECAM;
            break;
        }
    }

    return cvbs_fmt;
}

int set_tvafe(int videoStd, int audioStd, int vfmt)
{
    int ret = -1;
    unsigned long stdAndColor = enumToStdAndColor (videoStd, audioStd);

    //set CVBS
    enum tvin_sig_fmt_e fmt =(enum tvin_sig_fmt_e)stdEnumToCvbsFmt (vfmt, stdAndColor);
    return ioctl(fd_tvafe, TVIN_IOC_S_AFE_CVBS_STD, &fmt);
}

int set_atv_path()
{
    int fd;
    int len;

    if ((fd = open(SYS_VFM_MAP_PATH, O_RDWR)) < 0) {
        DTV_LOGE(TAG,"open %s error(%s)", SYS_VFM_MAP_PATH, strerror (errno));
        return -1;
    }

    len = write(fd, "rm tvpath > /sys/class/vfm/map", strlen("rm tvpath > /sys/class/vfm/map"));
    if (len < 0)
    {
        DTV_LOGE(TAG,"write %s error(%s)", SYS_VFM_MAP_PATH, strerror (errno));
    }

    len = write(fd, "add  tvpath  vdin0 amlvideo2.0 deinterlace videoqueue.0 > /sys/class/vfm/map",
                 strlen("add  tvpath  vdin0 amlvideo2.0 deinterlace videoqueue.0 > /sys/class/vfm/map"));
    if (len < 0)
    {
        DTV_LOGE(TAG,"write %s error(%s)", SYS_VFM_MAP_PATH, strerror (errno));
    }

    close(fd);
    return len;

}

int getCurrentSignalInfo(int *fmt, int *transFmt, int *status, int *frameRate)
{

    int ConstRate[5] = {24, 25, 30, 50, 60};
    float ConstRateDiffHz[5] = {0.5, 0.5, 0.5, 2, 2};
    int fps = m_cur_sig_info.fps;
    for (int i = 0; i < 5; i++) {
        if (abs(ConstRate[i] - fps) < ConstRateDiffHz[i])
            fps = ConstRate[i];
    }
    *fmt = m_cur_sig_info.fmt;
    *transFmt = m_cur_sig_info.trans_fmt;
    *status = m_cur_sig_info.status;
    if (mLocked) {
        if (m_cur_sig_info.status == TVIN_SIG_STATUS_UNSTABLE || m_cur_sig_info.status == TVIN_SIG_STATUS_NOTSUP
            || m_cur_sig_info.status == TVIN_SIG_STATUS_STABLE) {
            *status = TVIN_SIG_STATUS_BLOCKED;
        }
    }
    *frameRate = fps;
    DTV_LOGE(TAG, "trans_fmt is %d,fmt is %d, status is %d, frameRate is %d\n", *transFmt, *fmt, *status, *frameRate);

    return 0;
}
void initCurrentSignalInfo()
{
    m_cur_sig_info.fps = 0;
    m_cur_sig_info.is_dvi = 0;
    m_cur_sig_info.trans_fmt      = TVIN_TFMT_2D;
    m_cur_sig_info.fmt            = TVIN_SIG_FMT_NULL;
    m_cur_sig_info.status         = TVIN_SIG_STATUS_NULL;
    m_cur_sig_info.cfmt           = COLOR_FMT_MAX;
    m_cur_sig_info.aspect_ratio   = TVIN_ASPECT_NULL;
    m_cur_sig_info.amdolby_vision = 0;
    m_cur_sig_info.low_latency    = 0;
    m_cur_sig_info.signal_type    = 0;
    m_cur_sig_info.input_colorimetry = 0;
}

int set_atv_snow_status(int enable)
{
    int ret = 0;
    DTV_LOGI(TAG, "%s: enable is %d\n", __FUNCTION__, enable);

    if ( enable ) {
        ioctl(fd_tvafe, TVIN_IOC_S_AFE_SONWON);
        ioctl(fd_vdin, TVIN_IOC_SNOWON);

        mSnowStatusEnable = 1;
    } else {
        ioctl(fd_tvafe, TVIN_IOC_S_AFE_SONWOFF );
        ioctl(fd_vdin, TVIN_IOC_SNOWOFF );

        mSnowStatusEnable = 0;
    }

    return ret;
}


void setAtvSearchstatus(int searched)
{
    DTV_LOGI(TAG, "%s:searched: %d\n", __FUNCTION__, searched);
    mSearchStatus = searched;
}

extern void setChannelLockd(int locked)
{
    DTV_LOGI(TAG, "%s: set locked: %d\n", __FUNCTION__, locked);
    mLocked = locked;

    if (m_cur_sig_info.status == TVIN_SIG_STATUS_STABLE) {
    #ifndef RDK_COMPILE
        if (mLocked) {
            SC_setATVVideoColor(0, 0, 5);
        } else {
            SC_setATVVideoColor(0, 0, 6);
        }
    #endif
    }
}

static void SysEventCallback(int color)
{
    if (m_cur_sig_info.status == TVIN_SIG_STATUS_NOSIG ) {
        DTV_LOGI(TAG, "%s:TVIN_SIG_STATUS_NOSIG, mSnowStatusEnable = %d, mSearchStatus=%d\n", __FUNCTION__, mSnowStatusEnable, mSearchStatus);
        if (color && !mSearchStatus) {
        #ifndef RDK_COMPILE
            SC_setATVVideoColor(1, 1, 5);
        #endif
            if (mSnowStatusEnable) {
                set_atv_snow_status(0);
            }
        } else {
        #ifndef RDK_COMPILE
            SC_setATVVideoColor(0, 0, 6);
        #endif
            if (!mSnowStatusEnable) {
                set_atv_snow_status(1);
            }
        }
    } else {
        DTV_LOGI(TAG, "%s:TVIN_SIG_STATUS_STABLE, need't operation\n", __FUNCTION__);
    }
}

