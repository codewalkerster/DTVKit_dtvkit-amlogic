/*******************************************************************************
 *  Copyright @ 2023
 *
 *  for DTV global log
 *
 *
 *******************************************************************************/

/**
 * @brief   Functions for DTV global log
 * @file    dtv_log.c
 * @date    2023-05-05
 */

/* compiler library header files */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

/* third party header files */
/* DVBCore header files*/

#include "techtype.h"
#include "dbgfuncs.h"
#include "dtv_log.h"
#include "stbpathcfg.h"

#define TAG         "dtvkit-amlogic:Logcfg"
#define BUFFSIZE    512
#define PROFILENAME "dtv_logfilter"

#define LOGCFG_LOGD(...)    DTV_LOG(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGCFG_LOGI(...)    DTV_LOG(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGCFG_LOGW(...)    DTV_LOG(ANDROID_LOG_WARN,  TAG, __VA_ARGS__)
#define LOGCFG_LOGE(...)    DTV_LOG(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#define LOGCFG_LOGF(...)    DTV_LOG(ANDROID_LOG_FATAL, TAG, __VA_ARGS__)


#ifdef RDK_COMPILE
typedef void (* sighandler_t)(int);
#endif


/*---local (static) variable declarations for this file-----------------------*/
static char dtv_log_buff[BUFFSIZE];

static void *sg_dtv_log_mutex = NULL;

static U8BIT dtv_logfilter_levelctrl = ANDROID_LOG_INFO;
static U32BIT dtv_loglogfilter_modulectrl = 0;

struct sigaction newact,oldact;

static const char* loglevel_string[9][3] = {{"unknown","UNKNOWN","ANDROID_LOG_UNKNOWN"},
                                            {"default","DEFAULT","ANDROID_LOG_DEFAULT"},
                                            {"verbose","VERBOSE","ANDROID_LOG_VERBOSE"},
                                            {"debug",  "DEBUG",  "ANDROID_LOG_DEBUG"},
                                            {"info",   "INFO",   "ANDROID_LOG_INFO"},
                                            {"warn",   "WARN",   "ANDROID_LOG_WARN"},
                                            {"error",  "ERROR",  "ANDROID_LOG_ERROR"},
                                            {"fatal",  "FATAL",  "ANDROID_LOG_FATAL"},
                                            {"silent", "SILENT", "ANDROID_LOG_SILENT"}};

static void DTV_ShowLogLevel(U8BIT loglevel);

static U8BIT DTV_GetLogFilterConfig(void);


static void LogFilterUpdate_handler(int para)
{
    U8BIT logfilter_level;
    DTV_SetLogFilterLevel(ANDROID_LOG_INFO);
    logfilter_level = DTV_GetLogFilterConfig();
    DTV_ShowLogLevel(logfilter_level);
    DTV_SetLogFilterLevel(logfilter_level);

    LOGCFG_LOGD("DEBUG Output Test");
    LOGCFG_LOGI("INFO Output Test");
    LOGCFG_LOGW("WARN Output Test");
    LOGCFG_LOGE("ERROR Output Test");
    LOGCFG_LOGF("FATAL Output Test");
}

U8BIT DTV_GetLogFilterConfig(void)
{
    static U8BIT logfilter_level = ANDROID_LOG_INFO;
    FILE* fp = NULL;
    char filecontent[32];
    char filepath[128];

    STB_GetFullPathForDtvKitDataFile(filepath,sizeof(filepath),PROFILENAME);
    LOGCFG_LOGI("dtv_logfilter filepath: %s", filepath);

    fp = fopen(filepath, "r");
    if (fp == NULL)
    {
        LOGCFG_LOGW("Can not open file: %s", filepath);
        return logfilter_level;
    }

    fgets(filecontent,sizeof(filecontent),fp);

    /* Allow to set level with num or keyword */
    if (strlen(filecontent) <= 4 && strtol((const char *)filecontent, NULL, 0) <= 0x0f)
    {
        logfilter_level = (U8BIT)strtol((const char *)filecontent, NULL, 0);
    }
    else
    {
        for (U8BIT i = 0; i < 9; i++)
        {
            if (NULL != strstr(filecontent,loglevel_string[i][0]) ||
                NULL != strstr(filecontent,loglevel_string[i][1]))
            {
                logfilter_level = i;
            }
        }
    }
    fclose(fp);
    LOGCFG_LOGI("logfilter_config:%u", logfilter_level);

    return logfilter_level;
}

U8BIT DTV_GetLogFilterLevel(void)
{
    return dtv_logfilter_levelctrl;
}

U8BIT DTV_SetLogFilterLevel(U8BIT loglevel)
{
    if (loglevel > 0xf)
    {
        loglevel = 0xf;
    }
    dtv_logfilter_levelctrl = loglevel & 0xf;
    DTV_LOG(loglevel,TAG,"Set LogFilter_Level:%u", dtv_logfilter_levelctrl);
    return dtv_logfilter_levelctrl;
}

void DTV_ShowLogLevel(U8BIT loglevel)
{
    if (loglevel < 9)
    {
        LOGCFG_LOGI("loglevel(%u)-%s", loglevel, loglevel_string[loglevel][2]);
    }
    else
    {
        LOGCFG_LOGI("Invalid loglevel(%u)",loglevel);
    }
}

void DTV_LOG_Init(void)
{
    U8BIT logfilter_level = 0x0;
    newact.sa_handler = (sighandler_t)LogFilterUpdate_handler;
    sigemptyset(&newact.sa_mask);
    newact.sa_flags = 0;
    sigaction(SIGUSR1,&newact,&oldact);

    logfilter_level = DTV_GetLogFilterConfig();
    DTV_ShowLogLevel(logfilter_level);
    DTV_SetLogFilterLevel(logfilter_level);
}


#ifdef _WIN32
#include <windows.h>

#include <pthread.h>

#include "stbhwos.h"

static char msg[2*BUFFSIZE];

void DTV_LOG(U32BIT loglevel, const char* module, const char* format, ...)
{
    va_list vparams;
    char exformat[BUFFSIZE];
    pthread_t tid = pthread_self();

    ASSERT(module != NULL);
    ASSERT(format != NULL);

    if (NULL == sg_dtv_log_mutex)
    {
        sg_dtv_log_mutex = STB_OSCreateMutex();
    }

    STB_OSMutexLock(sg_dtv_log_mutex);

    if (loglevel >= DTV_GetLogFilterLevel())
    {
        sprintf(exformat,"<%s> %s", module, format);
        va_start(vparams, format);
        vsnprintf(dtv_log_buff, sizeof(dtv_log_buff), exformat, vparams);
        va_end(vparams);

        SYSTEMTIME localSysTime;
        GetLocalTime(&localSysTime);
        snprintf(msg, sizeof(msg), "[%2d-%2d-%2d:%3d] <tid:%u>\tDTV_LOG: %s",
                        localSysTime.wHour, localSysTime.wMinute,
                        localSysTime.wSecond, localSysTime.wMilliseconds,
                        pthread_getw32threadid_np(tid), dtv_log_buff);
        printf("%s\n", msg);
    }
    STB_OSMutexUnlock(sg_dtv_log_mutex);
}

#else

void DTV_LOG(U32BIT loglevel, const char *module, const char *format, ...)
{
    va_list vparams;
    char exformat[BUFFSIZE];
    ASSERT(module != NULL);
    ASSERT(format != NULL);

    if (loglevel >= DTV_GetLogFilterLevel())
    {
        if (loglevel < ANDROID_LOG_INFO)
        {
            /* For Custom, The lowest log level that can be output is ANDROID LOG INFO. */
            loglevel = ANDROID_LOG_INFO;
        }
        snprintf(exformat, sizeof(exformat), "<%s> %s", module, format);

        va_start(vparams, format);
        vsnprintf(dtv_log_buff, sizeof(dtv_log_buff), exformat, vparams);
        va_end(vparams);

        __android_log_print(loglevel, "DTV_LOG", "%s", dtv_log_buff);
    }
}


#endif

