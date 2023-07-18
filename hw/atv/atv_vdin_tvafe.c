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


#include <pthread.h>

#include "atv_vlfend.h"
#include "atv_fend_internal.h"
#include "atv_vdin_tvafe.h"

#include "dtv_log.h"

#define TAG "TUNER"


typedef unsigned char bool;

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

struct tvin_info_s {
    enum tvin_trans_fmt trans_fmt;
    enum tvin_sig_fmt_e fmt;
    enum tvin_sig_status_e status;
    enum tvin_color_fmt_e cfmt;
    unsigned int fps;
    unsigned int is_dvi;
    unsigned int signal_type;
    unsigned int input_colorimetry;
    enum tvin_aspect_ratio_e aspect_ratio;
    __u8 dolby_vision;
    __u8 low_latency;
};


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
    bool h_reverse;                 //for vdin horizontal reverse
    bool v_reverse;                 //for vdin vertical reverse
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
    CC_ATV_VIDEO_STD_END = CC_ATV_VIDEO_STD_SECAM,
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


/****************************************************************************
 * Static functions
 ***************************************************************************/
int open_vdin_port_tvafe()
{
    int ret = 0;
    struct tvin_parm_s vdinParam;

    fd_tvafe = open("/dev/tvafe0", O_RDWR);

    if (fd_tvafe < 0)
    {
        DTV_LOGE(TAG, "!!! Open tvafe module, error (%s).\n", strerror(errno));
        return -1;
    }
    DTV_LOGE(TAG, "!!! Waring: Tvafe device is also required for ATV.\n");

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

    return ret;
}

int close_vdin_port_tvafe()
{
    int ret = 0;
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
    struct tvin_info_s Info;
    int ret = vdin_get_signal_info ( &Info );
        if (ret < 0) {
            return ret;
        }

    DTV_LOGE(TAG, "status is %d\n", Info.status);
    if (Info.status == TVIN_SIG_STATUS_STABLE ) {
        ret = start_vdin_dec(Info);
        if (call_back) {
            call_back(Info.status);
        }
    } else if (Info.status == TVIN_SIG_STATUS_UNSTABLE ) {
        ret = stop_vdin_dec();
        if (call_back) {
            call_back(Info.status);
        }
    } else if (Info.status == TVIN_SIG_STATUS_NOTSUP ) {

    } else if (Info.status == TVIN_SIG_STATUS_NOSIG ) {
        ret = stop_vdin_dec();
        if (call_back) {
            call_back(Info.status);
        }
    } else {
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

int Epoll_create()
{
    if (!Epoll_isvalid())
    {
        fd_epoll = epoll_create(2);
    }
    return fd_epoll;
}

int Epoll_add(int fd, struct epoll_event *event)
{
    if (Epoll_isvalid())
    {
        return epoll_ctl(fd_epoll, ADD, fd, event);
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
                    vdin_signal_handle();
                }
            }
        }
    }//exit
    return 0;
}

int start_vdin_signal_detect(AM_VDIN_STATUS_Callback_t cb)
{
    struct epoll_event m_event;
    Epoll_create();
    open_vdin_port_tvafe();
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
            return -1;
        }
    }
    call_back = cb;
    return 0;
}

int stop_vdin_signal_detect()
{
    call_back = NULL;
    stop_vdin_dec();
    if (Epoll_isvalid()) {
        enable_thread = 0;
        pthread_join(thread, NULL);
        close(fd_epoll);
        fd_epoll = -1;
        pthread_mutex_destroy(&lock);
        pthread_cond_destroy(&cond);
    }

    if (fd_vdin >0) {
        close(fd_vdin);
        fd_vdin = -1;
    }
    if (fd_tvafe) {
        close(fd_tvafe);
        fd_tvafe = -1;
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
            if ((std & V4L2_STD_PAL_M) || (std & V4L2_STD_NTSC_M)) {
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
    len = write(fd, "add  tvpath  vdin0 amlvideo2.0 deinterlace videoqueue.0 > /sys/class/vfm/map",
                 strlen("add  tvpath  vdin0 amlvideo2.0 deinterlace videoqueue.0 > /sys/class/vfm/map"));
    close(fd);
    return len;

}
