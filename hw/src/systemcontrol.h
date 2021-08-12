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

int SC_setVideoColor(int color) ;
int SC_getScreenColorSetting() ;

#ifdef __cplusplus
}
#endif



#endif
