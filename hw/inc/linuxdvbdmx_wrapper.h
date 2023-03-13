/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *
 * @brief   linux dvb demux wrapper
 * @file    linuxdvbdmx_wrapper.h
 *
 * \author chuanzhi wang <chaunzhi.wang@amlogic.com>
 * \date 2020-07-16: create the document
 ***************************************************************************/

#ifndef _AM_DMX_H
#define _AM_DMX_H
#include "techtype.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**\brief Input source of the demux*/
typedef enum
{
	AML_DMX_SRC_TS0,                    /**< TS input port 0*/
	AML_DMX_SRC_TS1,                    /**< TS input port 1*/
	AML_DMX_SRC_TS2,                    /**< TS input port 2*/
	AML_DMX_SRC_TS3,                    /**< TS input port 3*/
	AML_DMX_SRC_HIU,                    /**< HIU input (memory)*/
	AML_DMX_SRC_HIU1
} AML_DMX_Source_t;

typedef void (*AML_DMX_DataCb) (int dev_no, int fd, const uint8_t *data, int len, void *user_data);

BOOLEAN DMX_Open(int dev_no);
BOOLEAN DMX_Close(int dev_no);
BOOLEAN DMX_AllocateFilter(int dev_no, int *fhandle);
BOOLEAN DMX_SetSecFilter(int dev_no, int fhandle, const struct dmx_sct_filter_params *params);
BOOLEAN DMX_SetPesFilter(int dev_no, int fhandle, const struct dmx_pes_filter_params *params);
BOOLEAN DMX_SetBufferSize(int dev_no, int fhandle, int size);
BOOLEAN DMX_FreeFilter(int dev_no, int fhandle);
BOOLEAN DMX_StartFilter(int dev_no, int fhandle);
BOOLEAN DMX_StopFilter(int dev_no, int fhandle);
BOOLEAN DMX_SetCallback(int dev_no, int fhandle, AML_DMX_DataCb cb, void *user_data);
BOOLEAN DMX_SetSource(int dev_no, AML_DMX_Source_t src);
BOOLEAN DMX_FileEcho(const char *name, const char *cmd);
BOOLEAN DMX_IsNewHW(void);

#ifdef __cplusplus
}
#endif
#endif
