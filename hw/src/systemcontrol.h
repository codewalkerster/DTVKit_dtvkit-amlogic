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
int SC_setVideoColor(int color) ;
int SC_getScreenColorSetting() ;
int SC_getStaticFrameEnable() ;
#endif

#ifdef __cplusplus
}
#endif



#endif
