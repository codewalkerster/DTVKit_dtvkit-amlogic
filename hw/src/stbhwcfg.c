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
#include "stbhwdmx.h"
#ifdef DTVKIT_IN_VENDOR_PARTITION
#include <cutils/properties.h>
#else
#ifdef USE_TSPLAYER
#include "dvr_types.h"
#include "dvr_utils.h"
#else
#include "am_types.h"
#endif
#endif


#define CFG_FILE_PATH "/odm/etc/tvconfig/dtvkit/config.xml"


#define CFG_PARSER_BUF_SIZE 512
//#define CFG_DEBUG 1
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
	.frontend_idx  = 0,
	.signal_types  = 0,
	.support_dvbt2 = 1,
	.support_dvbs2 = 1
	}
	},
.cam = {
	{
	.is_set_tsout  = 0,
	.tsout_source  = 0,
	.is_set_tssource = 0,
	.camPlug_tssource = 2,
	.camUnplug_tssource = 2,
	.is_changeTo_utf8 = 0,
	.encodec_source = {0},
	}
	},
.dmx_cap = {
		0
	},
.tuner_num    = 1,
.demux_num    = 3,
.recorder_num = 1,
.ci_slot_num  = 1,
.vdec_num     = 1,
.adec_num     = 2,
.demux        = 0,
.cam_num      = 0,
.pvr = {
    .encrypt = 0,
    },
.country_code = {'d', 'e', 'u'},
.network = {
    .net_id_max = 0xffff,
    .orig_net_id_max = 0xffff,
    .usr_def_orig_net_id = 0xffff,
    .net_id_change_update = 0,
    },
.sipsi = {
    .sdt_timeout = 15000,
    .pat_timeout = 800,
    .pmt_timeout = 3000,
    .nit_timeout = 10000,
    .cat_timeout = 2000,
    .bat_timeout = 12000,
    .tot_timeout = 32000,
    .tdt_timeout = 32000,
    .eit_timeout = 3000,
},
.service_without_sdt = 0,
};

static void
elem_start_handler (void *userData, const XML_Char *name, const XML_Char **atts)
{
	stb_hardware_cfg  *cfg = &aml_hw_cfg;
	const XML_Char   **att, *an, *av;
	//CFG_DBG("name [%s] ", name);
	if (!strcmp(name, "tuner")) {
		stb_tuner_cfg *tun;

		if (cfg->tuner_num >= AML_MAX_TUNER_NUM)
			return;

		tun = &cfg->tuners[cfg->tuner_num ++];

		tun->ts_input_idx  = 0;
		tun->frontend_idx  = 0;
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
			} else if (!strcmp(an, "frontend")) {
				long int i;
				i = strtol(av, NULL, 0);
				if ((i != LONG_MIN) && (i != LONG_MAX)) {
					STB_SPDebugWrite("find cfg, frontend:%d", i);
					tun->frontend_idx = i;
				}
			}else if (!strcmp(an, "dvbt") && !strcmp(av, "yes")) {
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

		att = atts;
		while (*att) {
			an = att[0];
			av = att[1];
			if (!strcmp(an, "rec") && !strcmp(av, "yes")) {
				cfg->dmx_cap[cfg->demux_num]  |= DMX_CAPS_RECORDING;
			} else if (!strcmp(an, "live") && !strcmp(av, "yes")) {
				cfg->dmx_cap[cfg->demux_num]  |= DMX_CAPS_LIVE;
			} else if (!strcmp(an, "pip") && !strcmp(av, "yes")) {
				cfg->dmx_cap[cfg->demux_num]  |= DMX_CAPS_PIP;
			} else if (!strcmp(an, "playback") && !strcmp(av, "yes")) {
				cfg->dmx_cap[cfg->demux_num]  |= DMX_CAPS_PLAYBACK;
			} else if (!strcmp(an, "si") && !strcmp(av, "yes")) {
				cfg->dmx_cap[cfg->demux_num]  |= DMX_CAPS_MONITOR_SI;
			}
			att += 2;
		}
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
	}else if (!strcmp(name, "ci_source")) {
		stb_cam_cfg *cam;

		if (cfg->cam_num >= AML_MAX_CAM_NUM)
			return;

		cam = &cfg->cam[cfg->cam_num ++];

		cam->is_set_tsout  = 0;
		cam->tsout_source  = 0;
		cam->is_set_tssource = 0;
		cam->camPlug_tssource = 0;
		cam->camUnplug_tssource = 0;

		att = atts;
		while (*att) {
			an = att[0];
			av = att[1];
			//CFG_DBG("an [%s] av[%s]", an, av);
			if (!strcmp(an, "is_set_tsout")) {
				cam->is_set_tsout = atoi(av);
				//CFG_DBG("cam->is_set_tsout[%d]", cam->is_set_tsout);
			} else if (!strcmp(an, "tsout_source")) {
				cam->tsout_source = atoi(av);
				//CFG_DBG("cam->tsout_source[%d]", cam->tsout_source);
			} else if (!strcmp(an, "is_set_tssource")) {
				cam->is_set_tssource = atoi(av);
				//CFG_DBG("cam->is_set_tssource[%d]", cam->is_set_tssource);
			} else if (!strcmp(an, "camPlug_tssource")) {
				cam->camPlug_tssource = atoi(av);
				//CFG_DBG("cam->camPlug_tssource[%d]", cam->camPlug_tssource);
			} else if (!strcmp(an, "camUnplug_tssource")) {
				cam->camUnplug_tssource = atoi(av);
				//CFG_DBG("cam->camUnplug_tssource[%d]", cam->camUnplug_tssource);
			} else if (!strcmp(an, "is_changeTo_utf8")) {
				cam->is_changeTo_utf8 = atoi(av);
				//CFG_DBG("cam->camUnplug_tssource[%d]", cam->camUnplug_tssource);
			} else if (!strcmp(an, "encodec_source")) {
				memcpy(cam->encodec_source, av, strlen(av));
				CFG_DBG("cam->encodec_source[%s]", cam->encodec_source);
			}
			att += 2;
		}

	} else if (!strcmp(name, "pvr")) {
		att = atts;
		an = att[0];
		av = att[1];
		if (!strcmp(an, "encrypt")) {
			long int i;
			i = strtol(av, NULL, 0);
			if ((i != LONG_MIN) && (i != LONG_MAX))
				cfg->pvr.encrypt = i;
		}
	}else if (!strcmp(name, "country")) {
        att = atts;
		an = att[0];
		av = att[1];
		if (!strcmp(an, "code") && strlen(av) == 3) {
			STB_SPDebugWrite("cfg country_code:%c%c%c", av[0], av[1], av[2]);
			memcpy(cfg->country_code, av, strlen(av));
		}
    }else if (!strcmp(name, "network")) {
        long int i;
        cfg->network.net_id_max = 0xffff;
        cfg->network.orig_net_id_max = 0xffff;
        cfg->network.usr_def_orig_net_id = 0xffff;
        cfg->network.net_id_change_update = 0;
        att = atts;
        while (*att) {
            an = att[0];
            av = att[1];
            if (!strcmp(an, "net_id_max")) {
                i = strtol(av, NULL, 0);
                if ((i != LONG_MIN) && (i != LONG_MAX))
                    cfg->network.net_id_max = i;
            }else if (!strcmp(an, "orig_net_id_max")) {
                i = strtol(av, NULL, 0);
                if ((i != LONG_MIN) && (i != LONG_MAX))
                    cfg->network.orig_net_id_max = i;
            }else if (!strcmp(an, "usr_def_orig_net_id")) {
                i = strtol(av, NULL, 0);
                if ((i != LONG_MIN) && (i != LONG_MAX))
                    cfg->network.usr_def_orig_net_id = i;
            }else if (!strcmp(an, "net_id_change_update")) {
                i = strtol(av, NULL, 0);
                if ((i != LONG_MIN) && (i != LONG_MAX))
                    cfg->network.net_id_change_update = i;
            }
            att += 2;
        }
    }else if (!strcmp(name, "sipsi")) {
        long int i;
        cfg->sipsi.sdt_timeout = 15000;
        cfg->sipsi.pat_timeout = 800;
        cfg->sipsi.pmt_timeout = 3000;
        cfg->sipsi.nit_timeout = 10000;
        cfg->sipsi.cat_timeout = 2000;
        cfg->sipsi.bat_timeout = 12000;
        cfg->sipsi.tot_timeout = 32000;
        cfg->sipsi.tdt_timeout = 32000;
        cfg->sipsi.eit_timeout = 3000;
        att = atts;
        while (*att) {
            an = att[0];
            av = att[1];
            if (!strcmp(an, "sdt_timeout")) {
                i = strtol(av, NULL, 0);
                if ((i != LONG_MIN) && (i != LONG_MAX))
                    cfg->sipsi.sdt_timeout = i;
            }else if (!strcmp(an, "pat_timeout")) {
                i = strtol(av, NULL, 0);
                if ((i != LONG_MIN) && (i != LONG_MAX))
                    cfg->sipsi.pat_timeout = i;
            }else if (!strcmp(an, "pmt_timeout")) {
                i = strtol(av, NULL, 0);
                if ((i != LONG_MIN) && (i != LONG_MAX))
                     cfg->sipsi.pmt_timeout = i;
            }else if (!strcmp(an, "nit_timeout")) {
                i = strtol(av, NULL, 0);
                if ((i != LONG_MIN) && (i != LONG_MAX))
                    cfg->sipsi.nit_timeout = i;
            }else if (!strcmp(an, "cat_timeout")) {
                i = strtol(av, NULL, 0);
                if ((i != LONG_MIN) && (i != LONG_MAX))
                    cfg->sipsi.cat_timeout = i;
            }else if (!strcmp(an, "bat_timeout")) {
                i = strtol(av, NULL, 0);
                if ((i != LONG_MIN) && (i != LONG_MAX))
                     cfg->sipsi.bat_timeout = i;
            }else if (!strcmp(an, "tot_timeout")) {
                i = strtol(av, NULL, 0);
                if ((i != LONG_MIN) && (i != LONG_MAX))
                    cfg->sipsi.tot_timeout = i;
            }else if (!strcmp(an, "tdt_timeout")) {
                i = strtol(av, NULL, 0);
                if ((i != LONG_MIN) && (i != LONG_MAX))
                    cfg->sipsi.tdt_timeout = i;
            }else if (!strcmp(an, "eit_timeout")) {
                i = strtol(av, NULL, 0);
                if ((i != LONG_MIN) && (i != LONG_MAX))
                    cfg->sipsi.eit_timeout = i;
            }
            att += 2;
        }
    }else if (!strcmp(name, "service_list")) {
        att = atts;
        while (*att) {
            an = att[0];
            av = att[1];
            if (!strcmp(an, "service_without_sdt") && !strcmp(av, "yes")) {
                cfg->service_without_sdt = 1;
            }
            att += 2;
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

		CFG_DBG("tuner%d ts_input:%d frontend:%d signal_types:%d dvbt2:%d dvbs2:%d",
				i,
				tun->ts_input_idx,
				tun->frontend_idx,
				tun->signal_types,
				tun->support_dvbt2,
				tun->support_dvbs2);
	}

	XML_ParserFree(parser);
	fclose(fp);
}
/**
 * @brief   get is need change chara encode from source to utf8
 * @param   isChange is need change encode
 * @param   encodec_source chara encode source, for example gdk gb2312 and so on
 */
int STB_Get_IsChangeUtf8(int *isChange, char *encodec_source)
{
	//get is need change code and encodec source from cfg struct
	if (isChange == NULL || encodec_source == NULL) {
		return -1;
	}
	*isChange = aml_hw_cfg.cam[0].is_changeTo_utf8;
	memcpy(encodec_source, aml_hw_cfg.cam[0].encodec_source, strlen(aml_hw_cfg.cam[0].encodec_source));
	return 0;
}

/**
 * @brief   get country code from cfg
 * @param   country code
 */
int STB_Get_Country_Code(char *country_code)
{
	if (country_code == NULL) {
		return -1;
	}
	memcpy(country_code, aml_hw_cfg.country_code, strlen(aml_hw_cfg.country_code));
	return 0;
}

/**
 * @brief   get max value that orig_net_id_max and net_id_max from cfg
 * @param   net_id_max
 * @param   orig_net_id_max
 */
void STB_Get_Max_Network_Id(int *net_id_max, int *orig_net_id_max)
{
    if (net_id_max != NULL && orig_net_id_max != NULL) {
        *net_id_max = aml_hw_cfg.network.net_id_max;
        *orig_net_id_max = aml_hw_cfg.network.orig_net_id_max;
    }
}

/**
 * @brief   get user define value about orig_net_id from cfg
 * @return  usr_def_orig_net_id
 */
int STB_Get_Usr_Orig_Net_Id()
{
   return aml_hw_cfg.network.usr_def_orig_net_id;
}

/**
 * @brief   get support that network id changed whether auto update channels
 * @return  net_id_change_update
 */
int STB_NetId_Change_Update_Ch()
{
   return aml_hw_cfg.network.net_id_change_update == 1 ? 1 : 0;
}

/**
 * @brief   get si/psi timeout from cfg
 * @param   si/psi type
 * @return  si/psi timeout about this type
 */
int STB_Get_SI_PSI_Timeout(E_SI_PSI_TYPE sipsi_type)
{
   int si_psi_timeout = 0;

   switch (sipsi_type)
   {
      case SIPSI_PAT:
      {
          si_psi_timeout = aml_hw_cfg.sipsi.pat_timeout;
          break;
      }
      case SIPSI_PMT:
      {
          si_psi_timeout = aml_hw_cfg.sipsi.pmt_timeout;
          break;
      }
      case SIPSI_SDT:
      {
          si_psi_timeout = aml_hw_cfg.sipsi.sdt_timeout;
          break;
      }
      case SIPSI_NIT:
      {
          si_psi_timeout = aml_hw_cfg.sipsi.nit_timeout;
          break;
      }
      case SIPSI_CAT:
      {
          si_psi_timeout = aml_hw_cfg.sipsi.cat_timeout;
          break;
      }
      case SIPSI_BAT:
      {
          si_psi_timeout = aml_hw_cfg.sipsi.bat_timeout;
          break;
      }
      case SIPSI_TOT:
      {
          si_psi_timeout = aml_hw_cfg.sipsi.tot_timeout;
          break;
      }
      case SIPSI_TDT:
      {
          si_psi_timeout = aml_hw_cfg.sipsi.tdt_timeout;
          break;
      }
      case SIPSI_EIT:
      {
          si_psi_timeout = aml_hw_cfg.sipsi.eit_timeout;
          break;
      }
      default:
      {
          break;
      }
   }
   return si_psi_timeout;
}

/**
 * @brief   get config of whether need to add service without sdt to service list
 * @return  1 if support, 0 otherwise
 */
int STB_Get_Service_WithoutSDT()
{
   return aml_hw_cfg.service_without_sdt == 1 ? 1 : 0;
}

/**
 * @brief   get dynamic prop
   @param   name prop name
   @param   buf returned value
   @param   len length of buf
   @return  TRUE if got, FALSE otherwise
 */
BOOLEAN STB_Get_Prop(const char *name, char *buf, int len)
{
#ifdef DTVKIT_IN_VENDOR_PARTITION
    return (property_get(name, buf, NULL) > 0) ? TRUE : FALSE;
#else
#ifdef USE_TSPLAYER
      return (DVR_SUCCESS == dvr_prop_read(name, buf, len)) ? TRUE : FALSE;
#else
      return (AM_SUCCESS == AM_PropRead(name, buf, len)) ? TRUE : FALSE;
#endif
#endif

}

/**
 * @brief   set dynamic prop
   @param   name prop name
   @param   value set value
 */
void STB_Set_Prop(const char *name, const char *value)
{
#ifdef DTVKIT_IN_VENDOR_PARTITION
    property_set(name, value);
#else
#ifdef USE_TSPLAYER
    dvr_prop_echo(name, value);
#else
    AM_PropEcho(name, value);
#endif
#endif
}
