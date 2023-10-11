/*******************************************************************************
 *  Copyright @ 2023
 *
 *  for get file path
 *
 *
 *******************************************************************************/

/**
 * @brief   Functions for get file path
 * @file    stbos_filepath.c
 * @date    2023-10-10
 */

/* compiler library header files */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

/* third party header files */
/* DVBCore header files*/

#include "techtype.h"
#include "dtv_log.h"
#include "cJSON.h"
#include "stbpathcfg.h"

#define TAG         "dtvkit-amlogic:filepath"

/* Default path  Begin*/
#define MAX_PATHLEN     128
#ifndef DTVKIT_RDK_ANCHOR_FILE
#define DTVKIT_RDK_ANCHOR_FILE  "/etc/dtvkit_anchor.json"
#endif
#ifndef DTVKIT_ANDROID_ANCHOR_FILE
#define DTVKIT_ANDROID_ANCHOR_FILE  "vendor/etc/dtvkit/dtvkit_anchor.json"
#endif

#ifdef RDK_COMPILE
    #define DTVKIT_ANCHOR_FILE  DTVKIT_RDK_ANCHOR_FILE

    static char configpath[MAX_PATHLEN] = "/etc/";
    static char datapath[MAX_PATHLEN] = "/data/";

    #ifdef DTVKIT_IN_VENDOR_PARTITION
    static char dbpath[MAX_PATHLEN] = "/data/data/com.droidlogic.dtvkit.inputsource/";
    #else
    static char dbpath[MAX_PATHLEN] = "/data/";
    #endif

#else
    /* Android */
    #define DTVKIT_ANCHOR_FILE  DTVKIT_ANDROID_ANCHOR_FILE

    #ifdef _WIN32
    static char configpath[MAX_PATHLEN] = ".\\dtvjsondb\\";
    #else
        #if ANDROID_PLATFORM_SDK_VERSION >= 30
        static char configpath[MAX_PATHLEN] = "/mnt/vendor/odm_ext/etc/tvconfig/dtvkit/";
        #else
        static char configpath[MAX_PATHLEN] = "/mnt/vendor/odm_ext/etc/tvconfig/dtvkit/";
        #endif  /* ANDROID_PLATFORM_SDK_VERSION >= 30 */

    #endif  /* _WIN32 */

    #ifdef DTVKIT_IN_VENDOR_PARTITION
    static char datapath[MAX_PATHLEN] = "/data/vendor/dtvkit/";
    static char dbpath[MAX_PATHLEN] = "/data/vendor/dtvkit/";
    #else
    static char datapath[MAX_PATHLEN] = "/data/data/com.droidlogic.dtvkit.inputsource/";
    static char dbpath[MAX_PATHLEN] = "/data/data/com.droidlogic.dtvkit.inputsource/";
    #endif  /* DTVKIT_IN_VENDOR_PARTITION */

#endif  /* RDK_COMPILE */
/* Default path  End */

/*---local (static) variable declarations for this file-----------------------*/


BOOLEAN STB_StrCombiner(char *targetBuf, U16BIT BufLen, const char *str1, const char *str2)
{
    BOOLEAN bRet = FALSE;
    if ((NULL == targetBuf) || (NULL == str1) || (NULL == str2))
    {
        DTV_LOGE(TAG,"%s %d ERROR:NULL pointer!\n",__FUNCTION__,__LINE__);
        return FALSE;
    }

    if (BufLen > strlen(str1)+strlen(str2))
    {
        sprintf(targetBuf,"%s%s",str1,str2);
        bRet = TRUE;
    }
    else
    {
        DTV_LOGE(TAG,"%s %d =>ERROR: Buffsize(%d) not enough!\n",__FUNCTION__,__LINE__,BufLen);
    }
    return bRet;
}

BOOLEAN STB_InitFilePathForDtvKit()
{
    BOOLEAN res = FALSE;
    FILE *fp= NULL;
    char *anchorpath = NULL;
    U8BIT i = 0;
    U16BIT fileSize = 0;
    U16BIT size = 0;
    char *temp_path = NULL;
    char cmdstr[100];
    char *jsonStr = NULL;
    cJSON *root = NULL;
    cJSON *item = NULL;
    struct stat statbuf;
    char* rwlimitpath[] = {"/data/"};

    /* 1 - Open file and get contents */
    fp = fopen(DTVKIT_ANCHOR_FILE, "r");
    if (fp == NULL)
    {
        DTV_LOGE(TAG,"%s,%d, Can not open %s !",__FUNCTION__,__LINE__, DTVKIT_ANCHOR_FILE);
        return res;
    }

    stat(DTVKIT_ANCHOR_FILE, &statbuf);
    fileSize = statbuf.st_size;

    jsonStr = (char *)cJSON_malloc(sizeof(char) * fileSize + 1);
    memset(jsonStr, 0, fileSize + 1);
    size = fread(jsonStr, sizeof(char), fileSize, fp);
    if (size == 0)
    {
        DTV_LOGE(TAG,"%s,%d, Nothing in file!",__FUNCTION__,__LINE__);
        cJSON_free(jsonStr);
        fclose(fp);
        return FALSE;
    }
    DTV_LOGI(TAG,"%s,%d,dtvkit_anchor.json: %s",__FUNCTION__,__LINE__, jsonStr);
    fclose(fp);

    /* 2 - Convert string to json object */
    root = cJSON_Parse(jsonStr);
    if (!root)
    {
        const char *err = cJSON_GetErrorPtr();
        DTV_LOGE(TAG,"%s,%d, Error before: [%s]",__FUNCTION__,__LINE__, err);
        cJSON_free((void *)err);
        cJSON_free(jsonStr);
        return FALSE;
    }
    cJSON_free(jsonStr);

    /* 3 - Parse json object and init path */

    //init datapath(DvbSDatebase.json,LnbDatabase.json,LocationDatabase.json,TVDatebase.json)
    item = cJSON_GetObjectItem(root, "datapath");
    temp_path = cJSON_GetStringValue(item);

    if (0 != access(temp_path, F_OK))   //create path if it's permitted
    {
        DTV_LOGE(TAG,"%s,%d, datapath:%s not exist",__FUNCTION__,__LINE__,temp_path);
        for (i = 0; i < sizeof(rwlimitpath)/sizeof(char*); i++)
        {
            if (NULL != strstr(temp_path,rwlimitpath[i]))
            {
                sprintf(cmdstr, "mkdir -p -m 766 %s", temp_path);
                system(cmdstr);
                DTV_LOGI(TAG,"%s,%d, try to create:[%s]",__FUNCTION__,__LINE__, temp_path);
                break;
            }
        }
    }
    if (0 == access(temp_path, F_OK))
    {
        strcpy(datapath,temp_path);
    }

    //init dbpath(dtvkit.sqlite3,dtvkit-isdb.sqlite3)
    item = cJSON_GetObjectItem(root, "dbpath");
    temp_path = cJSON_GetStringValue(item);

    if (0 != access(temp_path, F_OK))   //create path if it's permitted
    {
        DTV_LOGE(TAG,"%s,%d, dbpath:%s not exist",__FUNCTION__,__LINE__, temp_path);
        for (i = 0; i < sizeof(rwlimitpath)/sizeof(char*); i++)
        {
            if (NULL != strstr(temp_path,rwlimitpath[i]))
            {
                sprintf(cmdstr, "mkdir -p -m 766 %s", temp_path);
                system(cmdstr);
                DTV_LOGE(TAG,"%s,%d, try to create:[%s]",__FUNCTION__,__LINE__, temp_path);
                break;
            }
        }
    }
    if (0 == access(temp_path, F_OK))
    {
        strcpy(dbpath,temp_path);
    }

    //init configpath(dvbcountry.json, dvbtscancfg.json ...)
    item = cJSON_GetObjectItem(root, "configpath");
    temp_path = cJSON_GetStringValue(item);

    if (0 == access(temp_path, F_OK))
    {
        strcpy(configpath,temp_path);
    }

    //End init
    cJSON_free(root);

    DTV_LOGE(TAG,"%s, datapath: [%s]",__FUNCTION__, datapath);
    DTV_LOGE(TAG,"%s, dbpath: [%s]",__FUNCTION__, dbpath);
    DTV_LOGE(TAG,"%s, configpath: [%s]",__FUNCTION__, configpath);
    return TRUE;
}



BOOLEAN STB_GetFullPathForDtvKitConfigFile(char *targetBuf, U16BIT BufLen, const char *filename)
{
    BOOLEAN bRet = FALSE;

    bRet = STB_StrCombiner(targetBuf, BufLen, configpath, filename);

    return bRet;
}

BOOLEAN STB_GetFullPathForDtvKitDataFile(char *targetBuf, U16BIT BufLen, const char *filename)
{
    BOOLEAN bRet = FALSE;

    bRet = STB_StrCombiner(targetBuf, BufLen, datapath, filename);

    return bRet;
}

BOOLEAN STB_GetFullPathForDtvKitDBFile(char *targetBuf, U16BIT BufLen, const char *filename)
{
    BOOLEAN bRet = FALSE;

    bRet = STB_StrCombiner(targetBuf, BufLen, dbpath, filename);

    return bRet;
}


