
#include <utils/threads.h>
#include <SystemControlClient.h>
#include <android/log.h>
#include "dtv_log.h"
#ifdef __cplusplus
extern "C" {
#endif

// Ocean Blue Software header files
#include "techtype.h"
#include "dbgfuncs.h"
#include "stb_utils.h"
#include "linuxdvbdmx_wrapper.h"
#include "systemcontrol.h"
#include "ap_cfg.h"


#ifdef __cplusplus
}
#endif


using namespace android;
/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define LOG_TAG "tvserver"
#define SCDBG(x,...) DTV_LOG(ANDROID_LOG_INFO, LOG_TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )

static Mutex amLock;
static sp<SystemControlClient> sysctrlClient = nullptr;
/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

#ifndef RDK_COMPILE
#ifdef __cplusplus
extern "C" {
#endif

    typedef void (*SysClientCallbackFunc)(int color);

    class SysClientWrapper: public SysCtrlListener {
    public:
        SysClientWrapper() {
            SCDBG("%s: SysClientWrapper \n", __FUNCTION__);
            mpSysClient = SystemControlClient::getInstance();
            mListener = this;
            mpSysClient->setListener(mListener);
        };
        ~SysClientWrapper() {
            SCDBG("%s: ~SysClientWrapper \n", __FUNCTION__);
            if (mpSysClient != nullptr) {
                mListener = nullptr;
                mpSysClient = nullptr;
            }
        };

        virtual void notify(int event);
        virtual void notifyFBCUpgrade(int state, int param);
        virtual void onSetDisplayMode(int mode);
        virtual void onHdrInfoChange(int newHdrInfo);
        virtual void onAudioEvent(int param1, int param2, int param3, int param4);
        virtual void onDensityChange(int param1,int param2, int param3);
        virtual void onScreenColorChange(int newColor);

        int RegisterCallback(SysClientCallbackFunc callbcakfunc) {
            if (callbcakfunc == nullptr) {
                SCDBG("%s: eventCallbackFunc is NULL.\n", __FUNCTION__);
                return -1;
            } else {
                mSysCallbackFunc = callbcakfunc;
                return 0;
            }
        }

        SystemControlClient *mpSysClient;
        sp<SysCtrlListener> mListener;
        SysClientCallbackFunc mSysCallbackFunc;
    };

    void SysClientWrapper::notify(int event) {};
    void SysClientWrapper::notifyFBCUpgrade(int state, int param) {};
    void SysClientWrapper::onSetDisplayMode(int mode) {};
    void SysClientWrapper::onHdrInfoChange(int newHdrInfo) {};
    void SysClientWrapper::onAudioEvent(int param1, int param2, int param3, int param4) {};
    void SysClientWrapper::onDensityChange(int param1,int param2, int param3) {};
    void SysClientWrapper::onScreenColorChange(int newColor) {
        SCDBG("%s:newcolor is = %d", __FUNCTION__, newColor);
        mSysCallbackFunc(newColor);
    }

    static void HandleSysCallBackEvent(int color) {
        mEventCallback(color);
    }
//=============================wrapper api ===================================



    struct SysClientWrapper_t {
    #if ANDROID_PLATFORM_SDK_VERSION >= 30
        SysClientWrapper sysClientCallbackWrapper;
    #endif
    };

    struct SysClientWrapper_t *SC_getInstance(void)
    {
        SCDBG("%s:GetInstance.\n", __FUNCTION__);
        struct SysClientWrapper_t *pSysClientWrapper = new struct SysClientWrapper_t;
    #if ANDROID_PLATFORM_SDK_VERSION >= 30
        pSysClientWrapper->sysClientCallbackWrapper.RegisterCallback(HandleSysCallBackEvent);
    #endif
        return pSysClientWrapper;
    }

    void SC_releaseInstance(struct SysClientWrapper_t **ppInstance)
    {
        SCDBG("%s:releaseInstance\n", __FUNCTION__);
        delete *ppInstance;
        *ppInstance = 0;
    }

    int SC_setSysClientCallback(EventCallback Callback)
    {
    #if ANDROID_PLATFORM_SDK_VERSION >= 30
        if (Callback == nullptr) {
            SCDBG("%s: Callback is NULL.\n", __FUNCTION__);
        } else {
            SCDBG("%s: setSysClientCallback", __FUNCTION__);
            mEventCallback = Callback;
        }
    #endif
        return 0;
    }

#ifdef __cplusplus
}
#endif
#endif

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

extern "C"  int SC_setVideoColor(int window, int color)
{
#if ANDROID_PLATFORM_SDK_VERSION >= 30
    int s32Ret = -1;
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
/*
   window�� 0: reserved;   1: main_window;    2: sub_window.

       Color:    0: Black;  1: Blue.

frequency:  4: only show once,will recovery when receive new frame.

                    5: MUTE: always show the solid color frame, until receive disable cmd or surface disconnect

                    6: UNMUTE: disable color frame.
*/

            if (color == VIDEO_LAYER_COLOR_MAX)
            {
                SCDBG("@@@@@@@@@@@@@ UNMUTE");
                if (STB_IsNewHW())
                {
                    if (4 == SC_getDisplayMode())
                    {
                        s32Ret = sws->setVideoScreenColorByVT(window,0,6);
                        SCDBG("@@@@@@@@@@@@@ no need UNMUTE6[%d]",window);
                    }
                    //no need
                }
                else
                {
                    s32Ret = sws->setVideoScreenColor(color);
                }
            }
            else
            {
                if (color)
                {
                    SCDBG("@@@@@@@@@@@@@ MUTE blue [%d]", color);
                }
                else
                {
                    SCDBG("@@@@@@@@@@@@@ MUTE black [%d]", color);
                }

                if (STB_IsNewHW())
                {
                    s32Ret = sws->setVideoScreenColorByVT(window,color,5);
                }
                else
                {
                    s32Ret = sws->setVideoScreenColor(color);
                }
            }
            return s32Ret;
        }
#endif
#if 0

            if (color == VIDEO_LAYER_COLOR_MAX)
            {
                SCDBG("@@@@@@@@@@@@@ UNMUTE");
                s32Ret = sws->setVideoScreenColor(color);
            }
            else
            {
                if (color)
                {
                    SCDBG("@@@@@@@@@@@@@ MUTE blue [%d]", color);
                }
                else
                {
                    SCDBG("@@@@@@@@@@@@@ MUTE black [%d]", color);
                }
                s32Ret = sws->setVideoScreenColor(color);
            }
            return s32Ret;
        }
#endif

#if (ANDROID_PLATFORM_SDK_VERSION <= 28)
    /*used by custom only*/
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

extern "C"  int SC_getStaticFrameEnable()
{
    int s32Ret = -1;
#if ANDROID_PLATFORM_SDK_VERSION >= 30
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        s32Ret = sws->getStaticFrameEnable();
        SCDBG("[%s]: %d s32Ret %d!\n", __FUNCTION__, __LINE__, s32Ret);
    }
#endif

#if (ANDROID_PLATFORM_SDK_VERSION <= 28)
    s32Ret = 0;//Default black screen
#endif

    return s32Ret;
}

extern "C" int SC_setATVVideoColor(int forceColor, int setColor, int freq)
{
#if ANDROID_PLATFORM_SDK_VERSION >= 30
    int s32Ret = 0;
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        int color = sws->getScreenColorForSignalChange();
        if (forceColor) {
            SCDBG(" force set color %d !\n", setColor);
            color = setColor;
        }
        SCDBG(" color: %d freq: %d!\n", color, freq);
        if (STB_IsNewHW())
        {
            switch (freq) {
                case 4://VIDEO_LAYER_COLOR_SHOW_ONCE
                    sws->setVideoScreenColorByVT(0,color,4);
                    break;
                case 5://VIDEO_LAYER_COLOR_SHOW_ALWAYES
                    sws->setVideoScreenColorByVT(0,color,5);
                    break;
                case 6://VIDEO_LAYER_COLOR_SHOW_DISABLE
                    sws->setVideoScreenColorByVT(0,color,6);
                    break;
            }
        }
        else
        {
            if (freq == 6)
            {
                sws->setVideoScreenColor(VIDEO_LAYER_COLOR_MAX);
            }
            else
            {
                sws->setVideoScreenColor(color);
            }

        }
        SC_WriteSysfs("/sys/class/video/test_screen", "0x108080");
    }
#endif

    return s32Ret;
}

extern "C" int SC_disableTsync() {
    int ret = 0;
#if ANDROID_PLATFORM_SDK_VERSION >= 30
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        SC_WriteSysfs("/sys/class/tsync/enable", "0");
        SC_WriteSysfs("/sys/class/tsync/mode", "0");
    }
#endif
    return ret;
}

extern "C" int SC_getDisplayMode() {
    int ret = 0;
#if ANDROID_PLATFORM_SDK_VERSION >= 30
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        ret = sws->getDisplayMode(11);
        SCDBG("mode: %d\n",ret);
    }
#endif
    return ret;
}

/* issave: Whether to save to the database */
extern "C" int SC_setDisplayMode(int mode, int issave) {
    int ret = 0;
#if ANDROID_PLATFORM_SDK_VERSION >= 30
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        SCDBG("mode: %d\n", mode);
        ret = sws->setDisplayMode(11, mode, issave);
    }
#endif
    return 0;
}

extern "C" int SC_WriteSysfs(const char *path, const char *value) {
#if ANDROID_PLATFORM_SDK_VERSION >= 30
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        SCDBG(" %s : %s\n",path ,value);
        sws->writeSysfs(path, value);
    }
#endif
    return 0;
}

extern "C" int SC_ReadSysfs(const char *path, char *value) {
#if ANDROID_PLATFORM_SDK_VERSION >= 30
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        std::string v;
        if (sws->readSysfs(path, v)) {
            strcpy(value, v.c_str());
            return 0;
        }
    }
#endif
    return -1;
}

int SC_SetCurrentSourceInfo(int source_input, int sig_fmt, int trans_fmt)
{
    int s32Ret;
    SCDBG("%s: Start LoadPQ source info", __FUNCTION__);
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        s32Ret = sws->setCurrentSourceInfo(source_input, sig_fmt, trans_fmt);
        SCDBG("%s: End LoadPQ source info", __FUNCTION__);
        return s32Ret;
    }
    return -1;
}

int SC_SetCVD2Values()
{
    const sp<SystemControlClient> &sws = getSystemControlService();
    if (sws != nullptr) {
        return sws->setCVD2Values();
    }
    return -1;
}

#endif

/*****************************************************************************
*                    End Of File
*****************************************************************************/
