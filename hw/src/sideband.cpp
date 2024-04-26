#if (30 == ANDROID_PLATFORM_SDK_VERSION || 33 <= ANDROID_PLATFORM_SDK_VERSION)
#include "DisplayAdapter.h"
using meson::DisplayAdapter;
using std::unique_ptr;
#endif
#include "sideband.h"
#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif


/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define LOG_TAG "sideband"
#define DBG(x,...) DTV_LOG(ANDROID_LOG_INFO, LOG_TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )


void STB_SetDisableSidebandStream(int enable)
{
#if (30 == ANDROID_PLATFORM_SDK_VERSION || 33 <= ANDROID_PLATFORM_SDK_VERSION)   //only for T5D android R and T
    unique_ptr<DisplayAdapter> adapter = meson::DisplayAdapterCreateRemote();
    if (enable)
    {
         adapter ->disableSidebandStream(true);
    }
    else
    {
         adapter ->disableSidebandStream(false);
    }
#endif
    return;
}

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
    }
#endif

/*****************************************************************************
*                    End Of File
*****************************************************************************/
