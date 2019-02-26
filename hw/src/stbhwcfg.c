/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include "stbhwcfg.h"
#include <stdio.h>
#include <expat.h>
#include <string.h>
#include <limits.h>

#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwtun.h"

#define CFG_FILE_PATH "/vendor/etc/tvconfig/dtvkit/config.xml"

#define CFG_PARSER_BUF_SIZE 512

#ifdef CFG_DEBUG
   #define CFG_DBG(x,...)   STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define CFG_DBG(x,...)   ((void)0)
#endif

#define CFG_ERR(x,...)     STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )

stb_hardware_cfg aml_hw_cfg = {
.tuners = {
	{
	.ts_input_idx  = 2,
	.signal_types  = 0,
	.support_dvbt2 = 1,
	.support_dvbs2 = 1
	}
	},
.tuner_num    = 1,
.demux_num    = 3,
.recorder_num = 1,
.ci_slot_num  = 1,
.vdec_num     = 1,
.adec_num     = 2,
.demux        = 0
};

static void
elem_start_handler (void *userData, const XML_Char *name, const XML_Char **atts)
{
	stb_hardware_cfg  *cfg = &aml_hw_cfg;
	const XML_Char   **att, *an, *av;

	if (!strcmp(name, "tuner")) {
		stb_tuner_cfg *tun;

		if (cfg->tuner_num >= AML_MAX_TUNER_NUM)
			return;

		tun = &cfg->tuners[cfg->tuner_num ++];

		tun->ts_input_idx  = 0;
		tun->signal_types  = 0;
		tun->support_dvbt2 = 0;
		tun->support_dvbs2 = 0;

		att = atts;
		while (*att) {
			an = att[0];
			av = att[1];

			if (!strcmp(an, "ts_input")) {
				long int i;

				i = strtol(av, NULL, 0);
				if ((i != LONG_MIN) && (i != LONG_MAX))
					tun->ts_input_idx = i;
			} else if (!strcmp(an, "dvbt") && !strcmp(av, "yes")) {
				tun->signal_types  |= TUNE_SIGNAL_COFDM;
			} else if (!strcmp(an, "dvbt2") && !strcmp(av, "yes")) {
				tun->signal_types  |= TUNE_SIGNAL_COFDM;
				tun->support_dvbt2  = 1;
			} else if (!strcmp(an, "dvbs") && !strcmp(av, "yes")) {
				tun->signal_types  |= TUNE_SIGNAL_QPSK;
			} else if (!strcmp(an, "dvbs2") && !strcmp(av, "yes")) {
				tun->signal_types  |= TUNE_SIGNAL_QPSK;
				tun->support_dvbs2  = 1;
			} else if (!strcmp(an, "dvbc") && !strcmp(av, "yes")) {
				tun->signal_types  |= TUNE_SIGNAL_QAM;
			}

			att += 2;
		}

	} else if (!strcmp(name, "demux")) {
		cfg->demux_num ++;
	} else if (!strcmp(name, "recorder")) {
		cfg->recorder_num ++;
	} else if (!strcmp(name, "ci_slot")) {
		cfg->ci_slot_num ++;
	} else if (!strcmp(name, "vdec")) {
		cfg->vdec_num ++;
	} else if (!strcmp(name, "adec")) {
		cfg->adec_num ++;
	} else if (!strcmp(name, "av")) {
		att = atts;
		an = att[0];
		av = att[1];
		if (!strcmp(an, "demux")) {
			long int i;
			i = strtol(av, NULL, 0);
			if ((i != LONG_MIN) && (i != LONG_MAX))
				cfg->demux = i;

		}
	}
}

static void
elem_end_handler (void *userData, const XML_Char *name)
{
}

void STB_CfgInitialise(void)
{
	XML_Parser      parser;
	enum XML_Status status;
	FILE           *fp;
	int             i;

	fp = fopen(CFG_FILE_PATH, "rb");
	if (!fp) {
		CFG_ERR("cannot open \"%s\"", CFG_FILE_PATH);
		return;
	}

	parser = XML_ParserCreate(NULL);
	if (!parser) {
		CFG_ERR("XML_ParserCreate failed");
		fclose(fp);
		return;
	}

	XML_SetElementHandler(parser, elem_start_handler, elem_end_handler);

	aml_hw_cfg.tuner_num    = 0;
	aml_hw_cfg.demux_num    = 0;
	aml_hw_cfg.recorder_num = 0;
	aml_hw_cfg.ci_slot_num  = 0;
	aml_hw_cfg.vdec_num     = 0;
	aml_hw_cfg.adec_num     = 0;
	aml_hw_cfg.demux        = 0;

	while (1) {
		char    buf[CFG_PARSER_BUF_SIZE];
		size_t  len;
		int     is_end;

		len    = fread(buf, 1, CFG_PARSER_BUF_SIZE, fp);
		is_end = (len == 0) ? 1 : 0;

		status = XML_Parse(parser, buf, len, is_end);
		if (status == XML_STATUS_ERROR) {
			CFG_ERR("parse \"%s\" failed: %s", CFG_FILE_PATH,
					XML_ErrorString(XML_GetErrorCode(parser)));
			break;
		}

		if (is_end)
			break;
	}

	CFG_DBG("tuner_num:%d demux_num:%d ci_slot_num:%d recorder_num:%d vdec_num:%d adec_num:%d demux:%d",
			aml_hw_cfg.tuner_num,
			aml_hw_cfg.demux_num,
			aml_hw_cfg.ci_slot_num,
			aml_hw_cfg.recorder_num,
			aml_hw_cfg.vdec_num,
			aml_hw_cfg.adec_num,
			aml_hw_cfg.demux);

	for (i = 0; i < aml_hw_cfg.tuner_num; i ++) {
		stb_tuner_cfg *tun = &aml_hw_cfg.tuners[i];

		CFG_DBG("tuner%d ts_input:%d signal_types:%d dvbt2:%d dvbs2:%d",
				i,
				tun->ts_input_idx,
				tun->signal_types,
				tun->support_dvbt2,
				tun->support_dvbs2);
	}

	XML_ParserFree(parser);
	fclose(fp);
}

