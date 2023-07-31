/*******************************************************************************
 * Copyright (c) 2022 The Amlogic Incorporated
 *
 * This file is part of a DTVKit Software Component
 *
 *******************************************************************************/
/**
 * @brief   Set Top Box - System interface for events
 * @file    stbos_utils.c
 * @date    Apr 2022
 */

/*---includes for this file---------------------------------------------------*/
/* compiler library header files */
#include <stdio.h>
#include <cutils/properties.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dtv_log.h"
#define TAG  "STBOS_UTILS"

/* third party header files */
/* DVBCore header files*/
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwos.h"
#include "stb_utils.h"

/*!- Select-Deselect Local Debug Text Output */
/*#define  UTILS_DEBUG*/

#ifdef  UTILS_DEBUG
#define  UTL_DBG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define  UTL_DBG(x,...)
#endif

/**
 * @brief   get dynamic prop
   @param   name prop name
   @param   buf returned value
   @param   len length of buf
   @return  TRUE if got, FALSE otherwise
 */
BOOLEAN STB_DVRProp_Get(const char *name, char *buf, int len)
{
    char *value = NULL;
    if (name == NULL || buf == NULL) {
        UTL_DBG("error param, name or buf is NULL!");
        return FALSE;
    }

#if defined(__ANDROID__)
    int ret = property_get(name, buf, NULL);

    if (!ret) {
        UTL_DBG("error!");
        return FALSE;
    }
#else
    value = getenv(name);
    if(value == NULL) {
        UTL_DBG("error!");
        return FALSE;
    }
    memcpy(buf, value, len);
#endif

    return TRUE;
}

/**
 * @brief       set dynamic prop
   @param[in]   name prop name
   @param[in]   value set value
   @return      TRUE if got, FALSE otherwise
*/
BOOLEAN STB_DVRProp_Set(const char *name, const char *value)
{
    int ret = -1;

    if (name == NULL || value == NULL) {
        UTL_DBG("error param, name or value is NULL");
        return FALSE;
    }

#if defined(__ANDROID__)
    ret = property_set(name, value);
    if (!ret) {
        UTL_DBG("error!");
        return FALSE;
    }
#else
    ret = setenv(name, value, 1);
    if (!ret) {
        UTL_DBG("error!");
        return FALSE;
    }
#endif

    return TRUE;
}

/**
   @brief      Write a string cmd to a file
   @param[in]  name, File name
   @param[in]  cmd, String command
   @return     TRUE if got, FALSE otherwise
 */
BOOLEAN STB_File_Echo(const char *name, const char *cmd)
{
    int fd = -1, len = -1, ret = -1;

    if (name == NULL || cmd == NULL) {
        UTL_DBG("error param, name or value is NULL");
        return FALSE;
    }

    fd = open(name, O_WRONLY);
    if (fd == -1) {
        UTL_DBG("cannot open file \"%s\"", name);
        return FALSE;
    }

    len = strlen(cmd);
    ret = write(fd, cmd, len);
    if (ret != len) {
        UTL_DBG("write the file:\"%s\" failed, cmd:\"%s\"", name, cmd);
        close(fd);
        return FALSE;
    }

    close(fd);
    return TRUE;
}

/**
   @brief       read sysfs file
   @param[in]   name, File name
   @param[out]  buf, store sysfs node value
   @return      TRUE if got, FALSE otherwise
 */
BOOLEAN STB_File_Read(const char *name, char *buf, int len)
{
    int fp = -1, ret = -1;

    if (name == NULL || buf == NULL) {
        UTL_DBG("error param is NULL");
        return FALSE;
    } else if (len <= 0) {
        UTL_DBG("error param len %d ", len);
        return FALSE;
    }

    fp = open(name, O_RDONLY);
    if (fp == -1) {
        UTL_DBG("cannot open file \"%s\"", name);
        return FALSE;
    }

    ret = read(fp, buf, len);
    if (ret == -1) {
        UTL_DBG("read the file:\"%s\" failed", name);
        close(fp);
        return FALSE;
    }

    close(fp);
    return TRUE;
}