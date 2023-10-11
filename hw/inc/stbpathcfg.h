/*******************************************************************************
 *  Copyright @ 2023
 *
 *  for file path config
 *
 *
 *******************************************************************************/

/**
 * @brief   Functions for file path config
 * @file    stbpathcfg.h
 * @date    2023-10-10
 */

#ifndef _STBFILECFG_H_
#define _STBFILECFG_H_

/* DVBCore header files*/

#include "techtype.h"

#define PATH_MAX_LENGTH (128)

BOOLEAN STB_InitFilePathForDtvKit(void);
BOOLEAN STB_GetFullPathForDtvKitConfigFile(char *targetBuf, U16BIT BufLen, const char *filename);
BOOLEAN STB_GetFullPathForDtvKitDataFile(char *targetBuf, U16BIT BufLen, const char *filename);
BOOLEAN STB_GetFullPathForDtvKitDBFile(char *targetBuf, U16BIT BufLen, const char *filename);
BOOLEAN STB_StrCombiner(char *targetBuf, U16BIT BufLen, const char *str1, const char *str2);

#endif

