#ifndef _SYSTEMCONTROl_
#define _SYSTEMCONTROl_




#ifdef __cplusplus
extern "C" {
#endif


enum {
    VIDEO_LAYER_COLOR_BLACK,
    VIDEO_LAYER_COLOR_BLUE,
    VIDEO_LAYER_COLOR_MAX,
};
#ifndef RDK_COMPILE

typedef void (*EventCallback)(int color);
EventCallback mEventCallback;
struct SysClientWrapper_t;

int SC_setVideoColor(int window, int color) ;
int SC_getScreenColorSetting() ;
int SC_getStaticFrameEnable() ;
int SC_setATVVideoColor(int forceColor, int setColor, int freq) ;
int SC_WriteSysfs(const char *path, const char *value);
int SC_ReadSysfs(const char *path, char *value);
int SC_getDisplayMode();
int SC_setDisplayMode(int mode, int issave);
int SC_SetCurrentSourceInfo(int source_input, int sig_fmt, int trans_fmt);
int SC_SetCVD2Values();

struct SysClientWrapper_t *SC_getInstance(void);
int SC_setSysClientCallback(EventCallback Callback) ;
void SC_releaseInstance(struct SysClientWrapper_t **ppInstance);
#endif

#ifdef __cplusplus
}
#endif



#endif
