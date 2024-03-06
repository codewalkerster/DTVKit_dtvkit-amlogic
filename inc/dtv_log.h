/*******************************************************************************
 *  Copyright @ 2023
 *
 *  for DTV global log
 *
 *
 *******************************************************************************/

/**
 * @brief   Functions for DTV global log
 * @file    dtv_log.h
 * @date    2023-05-05
 */

#ifndef __DTV_LOG_H__
#define __DTV_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "techtype.h"

#ifdef _WIN32
#include "type_for_win32.h"
#else
//#include <utils/Log.h>
#include <android/log.h>
#endif

#define DTV_LOGD(module, ...)   DTV_LOG(ANDROID_LOG_DEBUG, module, __VA_ARGS__)
#define DTV_LOGI(module, ...)   DTV_LOG(ANDROID_LOG_INFO,  module, __VA_ARGS__)
#define DTV_LOGW(module, ...)   DTV_LOG(ANDROID_LOG_WARN,  module, __VA_ARGS__)
#define DTV_LOGE(module, ...)   DTV_LOG(ANDROID_LOG_ERROR, module, __VA_ARGS__)
#define DTV_LOGF(module, ...)   DTV_LOG(ANDROID_LOG_FATAL, module, __VA_ARGS__)

U8BIT DTV_GetLogFilterLevel(void);

U8BIT DTV_SetLogFilterLevel(U8BIT loglevel);

void DTV_LOG_Init(void);

void DTV_LOG(U32BIT loglevel, const char *module, const char *format, ... );

#ifdef __cplusplus
}
#endif

#endif // "#ifndef __DTV_LOG_H__"

