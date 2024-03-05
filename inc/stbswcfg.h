/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _STBSWCFG_H_
#define _STBSWCFG_H_

#ifdef __cplusplus
extern "C" {
#endif

void STB_LoadSwConfigJsonDB();
int STB_GetSystemStartingMode(char* system_starting_mode);
int STB_SetSystemStartingMode(char* system_starting_mode);
int STB_GetDvbCountryCode(char *country_code);
int STB_GetIsdbCountryCode(char *country_code);

#ifdef __cplusplus
}
#endif



#endif
