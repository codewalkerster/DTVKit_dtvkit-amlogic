
#include <utils/threads.h>
#include <SystemControlClient.h>
#include <android/log.h>
#ifdef __cplusplus
extern "C" {
#endif

// Ocean Blue Software header files
#include "techtype.h"
#include "dbgfuncs.h"
#include "systemcontrol.h"

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

        if(color == VIDEO_LAYER_COLOR_MAX)
            SCDBG("@@@@@@@@@@@@@ UNMUTE");
        else
            SCDBG("@@@@@@@@@@@@@ MUTE [%d]", color);

        s32Ret = sws->setVideoScreenColor(color);
        return s32Ret;
    }
#endif

#if (ANDROID_PLATFORM_SDK_VERSION <= 28)
    if (color == VIDEO_LAYER_COLOR_MAX)
    {
        SCDBG("@@@@@@@@@@@@@ UNMUTE");
    }
    else
    {
        SCDBG("@@@@@@@@@@@@@ MUTE [%d]", color);
        if (0 != system("echo 2 > /sys/class/video/disable_video"))
        {
            SCDBG("[%s]: %d disable_video error!\n", __FUNCTION__, __LINE__);
        }
    }
#endif

    return -1;
}

extern "C"  int SC_getScreenColorSetting()
{
#if ANDROID_PLATFORM_SDK_VERSION >= 30
    int s32Ret = -1;
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        s32Ret = sws->getScreenColorForSignalChange();
        //s32Ret = VIDEO_LAYER_COLOR_BLUE;
        return s32Ret;
    }
#endif
    return -1;
}

#endif

/*****************************************************************************
*                    End Of File
*****************************************************************************/
