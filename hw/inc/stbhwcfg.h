/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _STBHWCFG_H_
#define _STBHWCFG_H_

typedef struct {
	int ts_input_idx;
	int signal_types;
	int support_dvbt2;
	int support_dvbs2;
} stb_tuner_cfg;

#define AML_MAX_TUNER_NUM 3
#define AML_MAX_CAM_NUM 2
#define AML_MAX_DMX_NUM 16

typedef struct
{
	int           is_set_tsout;
	int           tsout_source;
	int           is_set_tssource;
	int           camPlug_tssource;
	int           camUnplug_tssource;
	int           is_changeTo_utf8;
	char          encodec_source[16];
}stb_cam_cfg;

typedef struct
{
	int           encrypt;
} stb_pvr_cfg;


typedef struct {
	stb_tuner_cfg tuners[AML_MAX_TUNER_NUM];
	stb_cam_cfg   cam[AML_MAX_CAM_NUM];
	int dmx_cap[AML_MAX_DMX_NUM];
	int           tuner_num;
	int           demux_num;
	int           recorder_num;
	int           ci_slot_num;
	int           vdec_num;
	int           adec_num;
	int           demux;
	int           cam_num;
	stb_pvr_cfg   pvr;
	char          country_code[3];
} stb_hardware_cfg;

extern stb_hardware_cfg aml_hw_cfg;

extern void STB_CfgInitialise(void);
extern int STB_Get_IsChangeUtf8(int *isChange, char *encodec_source);

/**
 * @brief   get country code from cfg
 * @param   country code
 */
int STB_Get_Country_Code(char *country_code);

#endif

