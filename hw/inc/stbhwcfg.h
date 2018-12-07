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

typedef struct {
	stb_tuner_cfg tuners[AML_MAX_TUNER_NUM];
	int           tuner_num;
	int           demux_num;
	int           recorder_num;
	int           ci_slot_num;
	int           vdec_num;
	int           adec_num;
} stb_hardware_cfg;

extern stb_hardware_cfg aml_hw_cfg;

extern void STB_CfgInitialise(void);

#endif

