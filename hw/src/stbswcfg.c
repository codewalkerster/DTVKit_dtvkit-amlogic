/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */
#include <pthread.h>
#include <stdio.h>
#include <expat.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include "cJSON.h"
#include "techtype.h"
#include "dbgfuncs.h"
#include "app_cfg.h"
#include "dtv_log.h"
#define TAG  "STBSWCFG"

#define TV_JSONFILE "TVDatebase.json"
#define CONFIG_JSONFILE "config.json"

typedef struct
{
    char system_start_mode[12];
    char dvb_country_code[4];
    char isdb_country_code[4];
} TV_CONFIG;

#define SW_CFG_DEBUG 1
#ifdef SW_CFG_DEBUG
   #define CFG_DBG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define CFG_DBG(x,...)   ((void)0)
#endif

static char g_config_json_file[16] = {0};

static TV_CONFIG tv_config;

static pthread_rwlock_t g_config_lock;

static BOOLEAN g_config_file_parse_state = FALSE;

static BOOLEAN TvConfigSyncJsonDBFromCache(const char* filePath, TV_CONFIG* ptv_config)
{
    if (NULL == ptv_config || NULL == filePath)
    {
        return FALSE;
    }

    cJSON *root = cJSON_CreateObject();
    if (NULL != root)
    {
        cJSON_AddItemToObject(root, "system_start_mode", cJSON_CreateString(ptv_config->system_start_mode));
        cJSON_AddItemToObject(root, "dvb_country_code", cJSON_CreateString(ptv_config->dvb_country_code));
        cJSON_AddItemToObject(root, "isdb_country_code", cJSON_CreateString(ptv_config->isdb_country_code));
    }

    FILE* fp = fopen(filePath, "w+");
    char *cjValue = cJSON_Print(root);
    if (fp != NULL) {
        int db_len = strlen(cjValue);
        int wr_len = fwrite(cjValue, 1, db_len, fp);

        if (wr_len != db_len) {
            CFG_DBG("write error: db_len:wr_len=[%d:%d]\n", db_len, wr_len);
        }
        else {
            CFG_DBG("write success: db_len:wr_len=[%d:%d]\n", db_len, wr_len);
        }

        fclose(fp);
        return TRUE;
    }
    else {
        CFG_DBG("TvConfigSyncJsonDBFromCache:: open %s error", filePath);
    }

    return FALSE;
}


static BOOLEAN TvConfigSyncCacheFromJsonDB(TV_CONFIG* ptv_config,
                                   const char* pfile_Path)
{
    long len;

    if (NULL == ptv_config || NULL == pfile_Path)
    {
        return FALSE;
    }

    FILE* fp = fopen(pfile_Path, "r");
    if (fp != NULL) {
        fseek(fp, 0L, SEEK_END);
        len = ftell(fp);
        if (len < 0)
        {
            CFG_DBG("### %s ### %d####, ftell error\n", __FUNCTION__, __LINE__);
            fclose(fp);
            return FALSE;
        }
        char* data = (char*)malloc(len + 1);
        if (NULL == data)
        {
            fclose(fp);
            return FALSE;
        }
        rewind(fp);
        if (len != (long)fread(data, sizeof(char), len, fp))
        {
            fclose(fp);
            free(data);
            CFG_DBG("### %s ### %d#### fread fail\n", __FUNCTION__,__LINE__);
            return FALSE;
        }
        data[len] = '\0';

        cJSON *root = cJSON_Parse(data);
        if (NULL != root)
        {
            cJSON *item;
            item = cJSON_GetObjectItem(root, "system_start_mode");
            if (item->type == cJSON_String)
            {
                CFG_DBG("system_start_mode[%s]", item->valuestring);
                memcpy(ptv_config->system_start_mode, item->valuestring, strlen(item->valuestring));
            }

            item = cJSON_GetObjectItem(root, "dvb_country_code");
            if (item->type == cJSON_String)
            {
                CFG_DBG("dvb_country_code[%s]", item->valuestring);
                memcpy(ptv_config->dvb_country_code, item->valuestring, strlen(item->valuestring));
            }

            item = cJSON_GetObjectItem(root, "isdb_country_code");
            if (item->type == cJSON_String)
            {
                CFG_DBG("isdb_country_code[%s]", item->valuestring);
                memcpy(ptv_config->isdb_country_code, item->valuestring, strlen(item->valuestring));
            }
        }

        if (data)
            free(data);

        fclose(fp);
        return TRUE;
    }
    else {
        CFG_DBG("TvConfigSyncCacheFromJsonDB:: open %s error", pfile_Path);
    }

    return FALSE;
}


void STB_LoadSwConfigJsonDB()
{
    char strCfgPath[128];
    char strDataPath[128];

    pthread_rwlock_init(&g_config_lock, NULL);
    pthread_rwlock_wrlock(&g_config_lock);

    if (0 != strcmp(g_config_json_file, TV_JSONFILE))
    {
        ACFG_GetFullPathForDtvKitDataFile(strDataPath,128,TV_JSONFILE);
        if ((access(strDataPath, F_OK)) != 0)
        {
            ACFG_GetFullPathForDtvKitConfigFile(strCfgPath,128,CONFIG_JSONFILE);
            CFG_DBG("%s:: %s isn't found and restore it from %s", __FUNCTION__,strCfgPath, CONFIG_JSONFILE);
            g_config_file_parse_state = TvConfigSyncCacheFromJsonDB(&tv_config, strCfgPath);
            if (g_config_file_parse_state == TRUE)
            {
                TvConfigSyncJsonDBFromCache(strDataPath, &tv_config);
            }
        }
        else
        {
            g_config_file_parse_state = TvConfigSyncCacheFromJsonDB(&tv_config, strDataPath);
        }

        memcpy(g_config_json_file, TV_JSONFILE, strlen(TV_JSONFILE));
    }

    pthread_rwlock_unlock(&g_config_lock);

}

int STB_GetSystemStartingMode(char* system_starting_mode)
{
    if ((system_starting_mode == NULL) || (g_config_file_parse_state == FALSE))
    {
        return -1;
    }

    pthread_rwlock_rdlock(&g_config_lock);
    memcpy(system_starting_mode, tv_config.system_start_mode, strlen(tv_config.system_start_mode));
    pthread_rwlock_unlock(&g_config_lock);
    return 0;
}

int  STB_SetSystemStartingMode(char* system_starting_mode)
{
    char strDataPath[128];

    if ((system_starting_mode == NULL) || (g_config_file_parse_state == FALSE))
    {
        return -1;
    }

    pthread_rwlock_wrlock(&g_config_lock);
    memset(tv_config.system_start_mode, 0, sizeof(tv_config.system_start_mode));
    memcpy(tv_config.system_start_mode, system_starting_mode, strlen(system_starting_mode));
    ACFG_GetFullPathForDtvKitDataFile(strDataPath,128,g_config_json_file);
    TvConfigSyncJsonDBFromCache(strDataPath, &tv_config);

    pthread_rwlock_unlock(&g_config_lock);

    return 0;
}

int STB_GetDvbCountryCode(char *country_code)
{
    if ((country_code == NULL) || (g_config_file_parse_state == FALSE))
    {
        return -1;
    }

    memcpy(country_code, tv_config.dvb_country_code, strlen(tv_config.dvb_country_code));

    return 0;
}

int STB_GetIsdbCountryCode(char *country_code)
{
    if ((country_code == NULL) || (g_config_file_parse_state == FALSE))
    {
        return -1;
    }

    memcpy(country_code, tv_config.isdb_country_code, strlen(tv_config.isdb_country_code));

    return 0;
}


