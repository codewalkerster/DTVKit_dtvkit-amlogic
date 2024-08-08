/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
/**\file
 * \brief for ATV frontend interface
 *
 * \author nengwen.chen <nengwen.chen@amlogic.com>
 * \date 2018-04-16: create the document
 * \modified by Robin
 * \date 2023-06-13: porting for aml dtv turnkey solution
 ***************************************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include <math.h>
#include <errno.h>

#include <pthread.h>
#ifdef RDK_COMPILE
#include <sys/ioctl.h>
#include "techtype.h"
#endif

#include "atv_vlfend.h"
#include "atv_fend_internal.h"
#include "atv_vlfend_test.h"

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

enum tvin_sig_status_e {
    TVIN_SIG_STATUS_NULL = 0, // processing status from init to the finding of the 1st confirmed status
    TVIN_SIG_STATUS_NOSIG,    // no signal - physically no signal
    TVIN_SIG_STATUS_UNSTABLE, // unstable - physically bad signal
    TVIN_SIG_STATUS_NOTSUP,   // not supported - physically good signal & not supported
    TVIN_SIG_STATUS_STABLE,   // stable - physically good signal & supported
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




/****************************************************************************
 * Static data
 ***************************************************************************/
static int fd_tvafe = -1;
static int fd_vdin = -1;

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
static int vlfend_format_frequency(int freq);
int open_vdin_tvafe();

static int vlfend_format_frequency(int freq)
{
    int tmp_val = 0;

    tmp_val = (freq % 1000000) / 10000;

    if (tmp_val >= 0 && tmp_val <= 30)
    {
        tmp_val = 25;
    }
    else if (tmp_val >= 70 && tmp_val <= 80)
    {
        tmp_val = 75;
    }

    freq = (freq / 1000000) * 1000000 + tmp_val * 10000;

    return freq;
}

int open_vdin_tvafe()
{
    int ret = 0;
    struct tvin_parm_s vdinParam;

#if 1
    fd_tvafe = open("/dev/tvafe0", O_RDWR);

    if (fd_tvafe < 0)
    {
        DTV_LOGE(TAG, "!!! Open tvafe module, error (%s).\n", strerror(errno));
        return -1;
    }

#else
    DTV_LOGE(TAG, "!!! Waring: Tvafe device is also required for ATV.\n");
#endif

#if 1
    fd_vdin = open("/dev/vdin0", O_RDWR);
    if (fd_vdin < 0)
    {
        DTV_LOGE(TAG, "!!! Open vdin module, error (%s).\n", strerror(errno));
        close(fd_tvafe);
        fd_tvafe = -1;
        return -1;
    }

/*

    vdinParam.port = TVIN_PORT_CVBS3;
    vdinParam.index = 0;

    ret = ioctl(fd_vdin, TVIN_IOC_STOP_DEC);
    if (ret)
    {
        DTV_LOGE(TAG, "!!! ioctl TVIN_IOC_STOP_DEC, error (%s).\n", strerror(errno));
        //close(fd_tvafe);
        //close(fd_vdin);
        //return ret;
    }

    ret = ioctl(fd_vdin, TVIN_IOC_OPEN, &vdinParam);
    if (ret)
    {
        DTV_LOGE(TAG, "!!! ioctl TVIN_IOC_OPEN, error (%s).\n", strerror(errno));
        //close(fd_tvafe);
        //close(fd_vdin);
        return ret;
    }
*/

#else
    DTV_LOGE(TAG, "!!! Waring: Vdin device is also required for ATV.\n");
#endif

    return ret;
}

int stop_vdin()
{
    int ret = 0;

    ret = ioctl(fd_vdin, TVIN_IOC_STOP_DEC);
    if (ret)
    {
        DTV_LOGE(TAG, "!!! ioctl TVIN_IOC_STOP_DEC, error (%s).\n", strerror(errno));
        //close(fd_tvafe);
        //close(fd_vdin);
        //return ret;
    }

    ret = ioctl(fd_vdin, TVIN_IOC_CLOSE);
    if (ret)
    {
       DTV_LOGE(TAG, "!!! ioctl TVIN_IOC_CLOSE, error (%s).\n", strerror(errno));
    }

    return ret;
}

int start_vdin()
{
    int ret = 0;
    struct tvin_parm_s vdinParam;

    vdinParam.port = TVIN_PORT_CVBS3;
    vdinParam.index = 0;


    ret = ioctl(fd_vdin, TVIN_IOC_STOP_DEC);
    if (ret)
    {
        DTV_LOGE(TAG, "!!! ioctl TVIN_IOC_STOP_DEC, error (%s).\n", strerror(errno));
        //close(fd_tvafe);
        //close(fd_vdin);
        //return ret;
    }

    ret = ioctl(fd_vdin, TVIN_IOC_OPEN, &vdinParam);
    if (ret)
    {
        DTV_LOGE(TAG, "!!! ioctl TVIN_IOC_OPEN, error (%s).\n", strerror(errno));
        //close(fd_tvafe);
        //close(fd_vdin);
    }

    return ret;
}


//------------------Export Function--------------
int test_atv_vlfend_open()
{
    int ret = 0;

    AM_FEND_OpenPara_t para;
    para.mode = FE_ANALOG;
    AM_ErrorCode_t err_code = AM_VLFEND_Open(0, &para);

    DTV_LOGI(TAG, "[%s] AM_VLFEND_Open:%d", __FUNCTION__, err_code);

    open_vdin_tvafe();
    start_vdin();

    return ret;
}

int test_atv_vlfend_lockfreq(u32 freq, u32 afc_range, u32 flag, u32 std)
{
    int ret = 0;

    struct dvb_frontend_parameters frontend_para;

    frontend_para.frequency             = freq;
    frontend_para.u.analog.audmode      = 0;
    frontend_para.u.analog.soundsys     = 0xFF;
    frontend_para.u.analog.std          = std;
    frontend_para.u.analog.flag         = flag;
    frontend_para.u.analog.afc_range    = afc_range;

    AM_ErrorCode_t setmod_ret = AM_VLFEND_SetMode(0, FE_ANALOG);
    AM_ErrorCode_t err_code = AM_VLFEND_SetPara(0, &frontend_para);


    DTV_LOGI(TAG, "[%s] lockfreq:%u, afc_range:%u, flag:%u, std:0x%x", __FUNCTION__,
                  frontend_para.frequency,
                  frontend_para.u.analog.afc_range,
                  frontend_para.u.analog.flag,
                  (U32BIT)frontend_para.u.analog.std);

    DTV_LOGI(TAG, "[%s] AM_VLFEND_SetMode:%u", __FUNCTION__, setmod_ret);
    DTV_LOGI(TAG, "[%s] AM_VLFEND_SetPara:%u", __FUNCTION__, err_code);

    return ret;
}

int test_atv_vlfend_getstatus()
{
    int ret = 0;

    fe_status_t st = 0;

    AM_ErrorCode_t err_code = AM_VLFEND_GetStatus(0, &st);

    DTV_LOGI(TAG, "[%s] AM_VLFEND_GetStatus:%u", __FUNCTION__, err_code);

    if (st & FE_HAS_LOCK)
        DTV_LOGI(TAG, "[%s] Status(0x%x): Locked", __FUNCTION__, st);
    else
        DTV_LOGI(TAG, "[%s] Status(0x%x): UnLocked", __FUNCTION__, st);

    return ret;
}


int test_atv_vlfend_getpara()
{
    int ret = 0;

    struct dvb_frontend_parameters get_para;

    memset(&get_para, 0, sizeof(struct dvb_frontend_parameters));

    AM_ErrorCode_t err_code  = AM_VLFEND_GetPara(0, &get_para);
    unsigned int freq = 0, video_std = 0, audio_std = 0, cvbs = 0;
    unsigned int std = 0, afc = 0;

    DTV_LOGI(TAG, "[%s] AM_VLFEND_GetPara:%d", __FUNCTION__, err_code);

    std = get_para.u.analog.std;

    DTV_LOGI(TAG, "------ channel info ------\n");
    DTV_LOGI(TAG, "### freq:%d Hz, format_freq: %d Hz",
          get_para.frequency,
          vlfend_format_frequency(get_para.frequency));
    DTV_LOGI(TAG, "### standard : 0x%x.\n", std);

    if ((std & V4L2_COLOR_STD_PAL) == V4L2_COLOR_STD_PAL)
    {
        video_std = 0;
    }
    else if ((std & V4L2_COLOR_STD_NTSC) == V4L2_COLOR_STD_NTSC)
    {
        video_std = 1;
    }
    else if ((std & V4L2_COLOR_STD_SECAM) == V4L2_COLOR_STD_SECAM)
    {
        video_std = 2;
    }
    else
    {
        video_std = 3;
        DTV_LOGI(TAG, "!!! color std ERROR !!!.\n");
    }

    DTV_LOGI(TAG, "### color std: %s.\n", str_vstd[video_std]);

    if (((std & V4L2_STD_PAL_DK) == V4L2_STD_PAL_DK)
            || ((std & V4L2_STD_SECAM_DK) == V4L2_STD_SECAM_DK))
    {
        audio_std = 0;
    }
    else if ((std & V4L2_STD_PAL_I) == V4L2_STD_PAL_I)
    {
        audio_std = 1;
    }
    else if ((std & V4L2_STD_PAL_BG) == V4L2_STD_PAL_BG)
    {
        audio_std = 2;
    }
    else if (((std & V4L2_STD_PAL_M) == V4L2_STD_PAL_M)
             || ((std & V4L2_STD_NTSC_M) == V4L2_STD_NTSC_M))
    {
        audio_std = 3;
    }
    else if ((std & V4L2_STD_SECAM_L) == V4L2_STD_SECAM_L)
    {
        audio_std = 4;
    }
    else if ((std & V4L2_STD_SECAM_LC) == V4L2_STD_SECAM_LC)
    {
        audio_std = 5;
    }
    else
    {
        audio_std = 6;
        DTV_LOGI(TAG, "!!! audio std ERROR !!!\n");
    }

    DTV_LOGI(TAG, "### audio std: %s.\n", str_astd[audio_std]);

    if ((std & V4L2_STD_PAL_I) == V4L2_STD_PAL_I)
    {
        cvbs = 0;
    }
    else if ((std & V4L2_STD_PAL_M) == V4L2_STD_PAL_M)
    {
        cvbs = 1;
    }
    else if ((std & V4L2_STD_PAL_Nc) == V4L2_STD_PAL_Nc)
    {
        cvbs = 2;
    }
    else if ((std & V4L2_STD_PAL_60) == V4L2_STD_PAL_60)
    {
        cvbs = 3;
    }
    else if ((std & V4L2_STD_NTSC_M) == V4L2_STD_NTSC_M)
    {
        cvbs = 4;
    }
    else if ((std & V4L2_STD_NTSC_443) == V4L2_STD_NTSC_443)
    {
        cvbs = 5;
    }
    else if ((std & V4L2_STD_SECAM_L) == V4L2_STD_SECAM_L)
    {
        cvbs = 6;
    }
    else
    {
        cvbs = 7;
        DTV_LOGI(TAG, "# cvbs std ERROR !!!\n");
    }

    DTV_LOGI(TAG, "### cvbs std : %s.\n", str_cvbs[cvbs]);

    return ret;
}




