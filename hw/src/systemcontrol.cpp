
#include <utils/threads.h>
#include <SystemControlClient.h>
#include <android/log.h>
#ifdef __cplusplus
extern "C" {
#endif

// Ocean Blue Software header files
#include "techtype.h"
#include "dbgfuncs.h"
#include "linuxdvbdmx_wrapper.h"
#include "systemcontrol.h"
#include "ap_cfg.h"


#ifdef __cplusplus
}
#endif

#define SCDBG(x,...)	STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )

using namespace android;
/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define LOG_TAG "tvserver"

static Mutex amLock;
static sp<SystemControlClient> sysctrlClient = nullptr;
/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
static const sp<SystemControlClient> &getSystemControlService()
{
    Mutex::Autolock _l(amLock);
    if (sysctrlClient == nullptr) {
        sysctrlClient = SystemControlClient::getInstance();
    }
    //ALOGE_IF(sysctrlClient == nullptr, "no System Control Service!?")

    return sysctrlClient;
}

#ifndef RDK_COMPILE

extern "C"  int SC_setVideoColor(int color)
{
#if ANDROID_PLATFORM_SDK_VERSION >= 30
    int s32Ret = -1;
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
/*
   window£º 0: reserved;   1: main_window;    2: sub_window.

       Color:    0: Black;  1: Blue.

frequency:  4: only show once,will recovery when receive new frame.

                    5: always show the solid color frame, until receive disable cmd or surface disconnect

                    6. disable color frame.
*/
        if (color == VIDEO_LAYER_COLOR_MAX)
        {
            SCDBG("@@@@@@@@@@@@@ UNMUTE");
            if (DMX_IsNewHW() && ACFG_GetCustomBlueScreenCfg() != 1)
            {
                //no need
            }
            else
            {
                s32Ret = sws->setVideoScreenColor(color);
            }
        }
        else
        {
            SCDBG("@@@@@@@@@@@@@ MUTE [%d]", color);
            if (DMX_IsNewHW() && ACFG_GetCustomBlueScreenCfg() != 1)
            {
                s32Ret = sws->setVideoScreenColorByVT(1,color,4);
            }
            else
            {
                s32Ret = sws->setVideoScreenColor(color);
            }
        }
        return s32Ret;
    }
#endif

#if (ANDROID_PLATFORM_SDK_VERSION <= 28)
    /*used by shine only*/
    if (color == VIDEO_LAYER_COLOR_MAX)
    {
        SCDBG("@@@@@@@@@@@@@ UNMUTE");
        if (0 != system("echo 0 > /sys/class/video/disable_video"))
        {
            SCDBG("[%s]: %d disable_video error!\n", __FUNCTION__, __LINE__);
        }
    }
    else
    {
        SCDBG("@@@@@@@@@@@@@ MUTE [%d]", color);
        if (0 != system("echo 1 > /sys/class/video/disable_video"))
        {
            SCDBG("[%s]: %d disable_video error!\n", __FUNCTION__, __LINE__);
        }
    }
#endif
    return -1;
}

extern "C"  int SC_getScreenColorSetting()
{
    int s32Ret = -1;
#if ANDROID_PLATFORM_SDK_VERSION >= 30
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        s32Ret = sws->getScreenColorForSignalChange();
        //s32Ret = VIDEO_LAYER_COLOR_BLUE;
    }
#endif

#if (ANDROID_PLATFORM_SDK_VERSION <= 28)
    s32Ret = VIDEO_LAYER_COLOR_BLACK;//Default black screen
#endif

    return s32Ret;
}

#endif

/*****************************************************************************
*                    End Of File
*****************************************************************************/
