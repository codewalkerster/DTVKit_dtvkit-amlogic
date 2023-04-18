/***************************************************************************
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#include <stdio.h>
#include <expat.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwtun.h"
#include "stbhwdmx.h"
#include "app_cfg.h"
#include "cert_log.h"
#include "stbheap.h"
#include "stb_utils.h"
#include "stbhwcfg.h"
#include "stbhwmem.h"

#ifdef DTVKIT_IN_VENDOR_PARTITION
#include <cutils/properties.h>
#else
#endif

#ifdef RDK_COMPILE
#define DTVKIT_CONFIG_XML_FILE "/etc/config.xml"
#else
#define DTVKIT_CONFIG_XML_FILE "config.xml"
#endif

#define CFG_PARSER_BUF_SIZE 512
#define CFG_DEBUG 1
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
	.is_ciplus_mode = 0,
	.tsout_source  = 0,
	.is_set_tssource = 0,
	.camPlug_tssource = 2,
	.camUnplug_tssource = 2,
	.is_changeTo_utf8 = 0,
	.encodec_source = {0},
	.dev_id = -1,
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
.oui = 0x15a,
.pvr = {
    .encrypt = 0,
    .rec_ringbuf_size = 0,
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
.demo_cap = {
    .srate_auto = 1,
    .srate_auto_value = 0,
    },
.service_unsupport_type = 0,
.capture_adc = {
    .analog_enabled = FALSE,
    .dvbs_enabled = FALSE,
    .dvbt_enabled = FALSE,
    .dvbc_enabled = FALSE,
    .isdbt_enabled = FALSE,
    .file_path = {0},
},
.mem_level_num = 0,
.dmc_mem={
    {
        .level  = 0,
        .size  = 0
    }
},
.unsupport_descriptor_tag_num = 0,
.unsupport_descriptor_tag_list=NULL
};

stb_custom_config aml_custom_config = {
    .auto_time_enable = FALSE,
    .fos_ad_enable = FALSE,
    .shine_dmx_enable = FALSE,
    .ms12_ac4_enable = TRUE,
    .deu_sort_lcn_after_last = FALSE,
    .deu_use_invisible_flag = TRUE,
    .disable_automatic_update = FALSE,
};

static void DVR_Get_Prop(const char *name, char *buf, int len);
static void DVR_Set_Prop(const char *name, const char *value);

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
        while (*att) {
            an = att[0];
            av = att[1];
            if (!strcmp(an, "demux")) {
                long int i;
                i = strtol(av, NULL, 0);
                if ((i != LONG_MIN) && (i != LONG_MAX))
                cfg->demux = i;

            } else if (!strcmp(an, "support_4k") && !strcmp(av, "no")) {
                cfg->service_unsupport_type |= E_STB_CFG_SERVICE_UPSOPPORT_TYPE_4K;
            }
             else if (!strcmp(an, "support_hdr_hlg") && !strcmp(av, "no")) {
                cfg->service_unsupport_type |= E_STB_CFG_SERVICE_UPSOPPORT_TYPE_HDR_HLG;
            }
            att += 2;
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
        cam->dev_id = -1;

        att = atts;
        while (*att) {
            an = att[0];
            av = att[1];
            CFG_DBG("an [%s] av[%s]", an, av);
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
            } else if (!strcmp(an, "camUnPlug_tssource")) {
                cam->camUnplug_tssource = atoi(av);
                //CFG_DBG("cam->camUnplug_tssource[%d]", cam->camUnplug_tssource);
            } else if (!strcmp(an, "is_changeTo_utf8")) {
                cam->is_changeTo_utf8 = atoi(av);
                //CFG_DBG("cam->is_changeTo_utf8[%d]", cam->is_changeTo_utf8);
            } else if (!strcmp(an, "encodec_source")) {
                if (strlen(av) <= sizeof(cam->encodec_source))
                {
                    memcpy(cam->encodec_source, av, strlen(av));
                }
                else
                {
                    CFG_DBG("str av is too long");
                }
                CFG_DBG("cam->encodec_source[%s]", cam->encodec_source);
            } else if (!strcmp(an, "use_ciplus_mode")){
                cam->is_ciplus_mode = atoi(av);
                STB_SPDebugWrite("cam->is_ciplus_mode %d", cam->is_ciplus_mode);
            } else if (!strcmp(an, "dev_id")){
                cam->dev_id = atoi(av);
                STB_SPDebugWrite("cam->dev_id %d", cam->dev_id);
            } else if (!strcmp(an, "host_mode")){
                if (!strcmp(av, "user_mode"))
                {
                    cam->host_mode = 2;
                }
                CFG_DBG("cam->host_mode(%s)  %d",av, cam->host_mode);
            }
            att += 2;
        }

    } else if (!strcmp(name, "pvr")) {
        long int i;
        att = atts;
        while (*att) {
            an = att[0];
            av = att[1];
            if (!strcmp(an, "encrypt")) {
                i = strtol(av, NULL, 0);
                if ((i != LONG_MIN) && (i != LONG_MAX))
                    cfg->pvr.encrypt = i;
            } else if (!strcmp(an, "rec_ringbuf_size")) {
                i = strtol(av, NULL, 0);
                if ((i != LONG_MIN) && (i != LONG_MAX))
                    cfg->pvr.rec_ringbuf_size = i;
            }
            att += 2;
        }
    } else if (!strcmp(name, "oad")) {
            att = atts;
            an = att[0];
            av = att[1];
            if (!strcmp(an, "oui")) {
                int i;
                i = strtol(av, NULL, 0);
                cfg->oui = i;

            }
    } else if (!strcmp(name, "dmc_mem")) {
        long int i;
        att = atts;
        while (*att) {
            an = att[0];
            av = att[1];
            if (!strcmp(an, "level")) {
                i = strtol(av, NULL, 0);
                if ((i != LONG_MIN) && (i != LONG_MAX))
                    cfg->dmc_mem[cfg->mem_level_num].level = i;
            }else if (!strcmp(an, "size")) {
                i = strtol(av, NULL, 0);
                if ((i != LONG_MIN) && (i != LONG_MAX))
                    cfg->dmc_mem[cfg->mem_level_num].size = i;
            }
            att += 2;
        }
        cfg->mem_level_num++;
    }
    else if (!strcmp(name, "country")) {
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
    } else if (!strcmp(name, "android_property")) {
        BOOLEAN has_name = FALSE;
        BOOLEAN has_value = FALSE;
        const char* pname = NULL;
        const char* pvalue = NULL;
        ENTRY e, *ep;
        int hret;

        att = atts;
        while (*att) {
            an = att[0];
            av = att[1];
            //CFG_DBG("an [%s] av[%s]", an, av);
            if (strcmp(an, "name")==0) {
                pname = av;
                has_name=TRUE;
            } else if (strcmp(an, "value")==0) {
                pvalue = av;
                has_value=TRUE;
            }
            att += 2;
        }
        if ( has_name && has_value ) {
            e.key=strdup(pname);
            e.data=strdup(pvalue);
            if ( e.key!=NULL && e.data!=NULL )
            {
                hret = hsearch_r(e,ENTER,&ep,&(cfg->prop_htab));
                if ( hret == 0 ) {
                    CFG_ERR("Hash table, failed to add an entry to hash table %s:%s",pname,pvalue);
                } else {
                    CFG_DBG("Hash table, key:%s, value:%s", pname,pvalue);
                }
            } else {
                if (e.key!=NULL) {
                    STB_MEMFreeSysRAM(e.key);
                }
                if (e.data!=NULL) {
                    STB_MEMFreeSysRAM(e.data);
                }
                CFG_ERR("Hash table, failed to duplicate strings %s,%s due to insufficient memory"
                        ,pname,pvalue);
            }
        }
    }
    else if (!strcmp(name, "demo_cap"))
    {
        att = atts;
        while (*att) {
            an = att[0];
            av = att[1];
            if (!strcmp(an, "symbol_rate_auto") && !strcmp(av, "no")) {
                cfg->demo_cap.srate_auto = 0;
            }
            att += 2;
        }
    }
    else if (!strcmp(name, "capture_adc"))
    {
        att = atts;
        while (*att) {
            an = att[0];
            av = att[1];
            if (!strcmp(an, "analog") && !strcmp(av, "yes")) {
                cfg->capture_adc.analog_enabled = TRUE;
            }
            if (!strcmp(an, "dvbs") && !strcmp(av, "yes")) {
                cfg->capture_adc.dvbs_enabled = TRUE;
            }
            else if (!strcmp(an, "dvbt") && !strcmp(av, "yes")) {
                cfg->capture_adc.dvbt_enabled = TRUE;
            }
            else if (!strcmp(an, "dvbc") && !strcmp(av, "yes")) {
                cfg->capture_adc.dvbc_enabled = TRUE;
            }
            else if (!strcmp(an, "isdbt") && !strcmp(av, "yes")) {
                cfg->capture_adc.isdbt_enabled = TRUE;
            }
            else if (!strcmp(an, "file_path")) {
                size_t len = strlen(av);
                if (len > 0 && len < sizeof(cfg->capture_adc.file_path)) {
                    memcpy(cfg->capture_adc.file_path, av, len);
                    cfg->capture_adc.file_path[len] = '\0';
                }
            }
            att += 2;
        }
        CFG_DBG("capture_adc is set to analog[%s],dvbs[%s],dvbt[%s],dvbc[%s],isdbt[%s],path:%s",
                cfg->capture_adc.analog_enabled ? "Yes" : "No",
                cfg->capture_adc.dvbs_enabled ? "Yes" : "No",
                cfg->capture_adc.dvbt_enabled ? "Yes" : "No",
                cfg->capture_adc.dvbc_enabled ? "Yes" : "No",
                cfg->capture_adc.isdbt_enabled ? "Yes" : "No",
                strlen(cfg->capture_adc.file_path) > 0 ? cfg->capture_adc.file_path : "(empty)");
    }
    else if (!strcmp(name, "epg_config")) {
        cfg->epg_cfg.is_not_match_orignetid = 0;
        cfg->epg_cfg.is_not_match_tsid = 0;
        cfg->epg_cfg.barker_channel_enabled = 0;
        cfg->epg_cfg.eit_search_enabled = 0;
        att = atts;
        while (*att) {
            an = att[0];
            av = att[1];
            CFG_DBG("an [%s] av[%s]", an, av);
            if (!strcmp(an, "not_match_orig_net_id")) {
               cfg->epg_cfg.is_not_match_orignetid = atoi(av);
               CFG_DBG("cfg->epg_cfg.is_not_match_orignetid[%d]", cfg->epg_cfg.is_not_match_orignetid);
            } else if (!strcmp(an, "not_match_ts_id")) {
               cfg->epg_cfg.is_not_match_tsid = atoi(av);
               CFG_DBG("cfg->epg_cfg.is_not_match_tsid[%d]", cfg->epg_cfg.is_not_match_tsid);
            } else if (!strcmp(an, "barker_channel_enabled")) {
               cfg->epg_cfg.barker_channel_enabled = atoi(av);
               CFG_DBG("cfg->epg_cfg.barker_channel_enabled[%d]", cfg->epg_cfg.barker_channel_enabled);
            } else if (!strcmp(an, "eit_search_enabled")) {
               cfg->epg_cfg.eit_search_enabled = (!strcmp(av, "yes")) ? 1 : 0;
               CFG_DBG("cfg->epg_cfg.eit_search_enabled[%d]", cfg->epg_cfg.eit_search_enabled);
            }
            att += 2;
        }
    }
    else if (!strcmp(name, "time_function"))
    {
        att = atts;
        while (*att) {
            an = att[0];
            av = att[1];
            if (!strcmp(an, "auto_time_enable") && !strcmp(av, "yes")) {
                aml_custom_config.auto_time_enable = TRUE;
            }
            att += 2;
        }
        CFG_DBG("time_function, auto_time_enable is set to %d", aml_custom_config.auto_time_enable);
    }
    else if (!strcmp(name, "audio_function"))
    {
        att = atts;
        while (*att) {
            an = att[0];
            av = att[1];
            if (!strcmp(an, "shine_ad_enable") && !strcmp(av, "yes")) {
                aml_custom_config.fos_ad_enable = TRUE;
            }
            att += 2;
        }
        CFG_DBG("audio_function, fos_ad_enable is set to %d", aml_custom_config.fos_ad_enable);
    }
    else if (!strcmp(name, "demux_function"))
    {
        att = atts;
        while (*att) {
            an = att[0];
            av = att[1];
            if (!strcmp(an, "shine_dmx_enable") && !strcmp(av, "yes")) {
                aml_custom_config.shine_dmx_enable = TRUE;
            }
            att += 2;
        }
        CFG_DBG("demux_function, shine_dmx_enable is set to %d", aml_custom_config.shine_dmx_enable);
    }
    else if (!strcmp(name, "ms12_function"))
    {
        att = atts;
        while (*att) {
            an = att[0];
            av = att[1];
            if (!strcmp(an, "ms12_ac4_enable") && !strcmp(av, "yes")) {
                aml_custom_config.ms12_ac4_enable = TRUE;
            }
            else if (!strcmp(an, "ms12_ac4_enable") && !strcmp(av, "no")) {
                aml_custom_config.ms12_ac4_enable = FALSE;
            }
            att += 2;
        }
        CFG_DBG("ms12_function, ms12_ac4_enable is set to %d", aml_custom_config.ms12_ac4_enable);
    }
    else if (!strcmp(name, "lcn_function"))
    {
        att = atts;
        while (*att) {
            an = att[0];
            av = att[1];
            if (!strcmp(an, "deu_sort_lcn_after_last") && !strcmp(av, "yes")) {
                aml_custom_config.deu_sort_lcn_after_last = TRUE;
            }
            else if (!strcmp(an, "deu_use_invisible_flag") && !strcmp(av, "no")) {
                aml_custom_config.deu_use_invisible_flag = FALSE;
            }
            att += 2;
        }
        CFG_DBG("lcn_function, deu_sort_lcn_after_last is set to %d", aml_custom_config.deu_sort_lcn_after_last);
        CFG_DBG("lcn_function, deu_use_invisible_flag is set to %d", aml_custom_config.deu_use_invisible_flag);
    }
    else if (!strcmp(name,"monitor_function"))
    {
        att = atts;
        while (*att) {
            an = att[0];
            av = att[1];
            if (!strcmp(an, "disable_automatic_update") && !strcmp(av, "yes")) {
                aml_custom_config.disable_automatic_update = TRUE;
            }
            att += 2;
        }
        CFG_DBG("monitor_function, disable_automatic_update is set to %d", aml_custom_config.disable_automatic_update);
    }
    else if (!strcmp(name,"unsupport_descriptor_tag"))
    {
        att = atts;
        while (*att) {
            cfg->unsupport_descriptor_tag_num++;
            att += 2;
        }
        if (cfg->unsupport_descriptor_tag_num > 0)
        {
            cfg->unsupport_descriptor_tag_list=(U8BIT*)STB_MEMGetSysRAM(cfg->unsupport_descriptor_tag_num);
            if (NULL != cfg->unsupport_descriptor_tag_list)
            {
                int index = 0;
                att = atts;
                while (*att) {
                    an = att[0];
                    av = att[1];
                    cfg->unsupport_descriptor_tag_list[index] = (U8BIT)strtol(av, NULL, 0);
                    //CFG_DBG("cfg->unsupport_descriptor_tag_list[%d]=%x", index, cfg->unsupport_descriptor_tag_list[index]);
                    att += 2;
                    index++;
                }
            }
        }
        CFG_DBG("unsupport_descriptor_tag_num %d", cfg->unsupport_descriptor_tag_num);
    }
}

static void
elem_end_handler (void *userData, const XML_Char *name)
{
}

static BOOLEAN getDtvKitConfigXmlFile(char *strPathBuf,U16BIT u16PathBufLen)
{
    BOOLEAN bRet = FALSE;

    if(NULL == strPathBuf)
    {
        STB_SPDebugWrite("%s %d ERROR!!!", __FUNCTION__, __LINE__);
        return bRet;
    }

#ifdef RDK_COMPILE
    if(u16PathBufLen > strlen(DTVKIT_CONFIG_XML_FILE))
    {
        sprintf(strPathBuf, "%s", DTVKIT_CONFIG_XML_FILE);
    }
#else
    bRet = APCFG_AddFilePathForDtvKitConfigFile(strPathBuf, u16PathBufLen, DTVKIT_CONFIG_XML_FILE);
#endif

    STB_SPDebugWrite("%s %d strPathBuf = %s", __FUNCTION__, __LINE__, strPathBuf);

    return bRet;
}

void STB_CfgInitialise(void)
{
    XML_Parser      parser;
    enum XML_Status status;
    FILE           *fp = NULL;
    int             i;

    #ifdef DTVKIT_IN_VENDOR_PARTITION
    char buf[128];
    BOOLEAN ret;

    memset(buf, 0x00, sizeof(buf));
    ret = property_get("persist.vendor.tvconfig.path",buf,NULL);
    if (ret) {
        if ((access(buf, 0) == 0)) {
            CFG_ERR("read from prop, open \"%s\"", buf);
            fp = fopen(buf, "rb");
            if (!fp) {
                CFG_ERR("cannot open \"%s\"", buf);
            }
        }
    }
    #endif

    if (!fp) {
        char strCfgPath[APCFG_DTVKIT_CONFIG_PATH_MAX_LENGTH];

        memset(strCfgPath, 0, APCFG_DTVKIT_CONFIG_PATH_MAX_LENGTH);
        getDtvKitConfigXmlFile(strCfgPath,APCFG_DTVKIT_CONFIG_PATH_MAX_LENGTH);
        fp = fopen(strCfgPath, "rb");
        if (!fp) {
            CFG_ERR("cannot open \"%s\"", strCfgPath);
            return;
        }
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
    aml_hw_cfg.epg_cfg.is_not_match_orignetid = 0;
    aml_hw_cfg.epg_cfg.is_not_match_tsid = 0;
    aml_hw_cfg.epg_cfg.barker_channel_enabled = 0;
    aml_hw_cfg.epg_cfg.eit_search_enabled = 0;

    memset(&(aml_hw_cfg.prop_htab),0,sizeof(struct hsearch_data));
    if(0==hcreate_r(100,&(aml_hw_cfg.prop_htab)))
    {
        CFG_ERR("Hash table, failed to create hash table");
        return;
    }

    while (1) {
        char    buf[CFG_PARSER_BUF_SIZE];
        size_t  len;
        int     is_end;

        len    = fread(buf, 1, CFG_PARSER_BUF_SIZE, fp);
        is_end = (len == 0) ? 1 : 0;

        status = XML_Parse(parser, buf, len, is_end);
        if (status == XML_STATUS_ERROR) {
            CFG_ERR("parse config.xml failed: %s",
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
    CFG_DBG("srate_auto:%d srate_auto_value:%d\n",
                                aml_hw_cfg.demo_cap.srate_auto,
                                aml_hw_cfg.demo_cap.srate_auto_value);
    CFG_DBG("get epg config: is_not_match_orignetid:%d is_not_match_tsid:%d barker_channel_enabled:%d eit_search_enabled:%d\n",
                                aml_hw_cfg.epg_cfg.is_not_match_orignetid,
                                aml_hw_cfg.epg_cfg.is_not_match_tsid,
                                aml_hw_cfg.epg_cfg.barker_channel_enabled,
                                aml_hw_cfg.epg_cfg.eit_search_enabled);

    // B: Starting Up Log
    CERT_Log_StartingUp("tuner_num:%d demux_num:%d ci_slot_num:%d recorder_num:%d vdec_num:%d adec_num:%d demux:%d",
                                aml_hw_cfg.tuner_num,
                                aml_hw_cfg.demux_num,
                                aml_hw_cfg.ci_slot_num,
                                aml_hw_cfg.recorder_num,
                                aml_hw_cfg.vdec_num,
                                aml_hw_cfg.adec_num,
                                aml_hw_cfg.demux);
    CERT_Log_StartingUp("srate_auto:%d srate_auto_value:%d\n",
                                aml_hw_cfg.demo_cap.srate_auto,
                                aml_hw_cfg.demo_cap.srate_auto_value);

    CERT_Log_StartingUp("get epg config: is_not_match_orignetid:%d is_not_match_tsid:%d barker_channel_enabled:%d eit_search_enabled:%d\n",
                                aml_hw_cfg.epg_cfg.is_not_match_orignetid,
                                aml_hw_cfg.epg_cfg.is_not_match_tsid,
                                aml_hw_cfg.epg_cfg.barker_channel_enabled,
                                aml_hw_cfg.epg_cfg.eit_search_enabled);
    // E: Starting Up Log

    for (i = 0; i < aml_hw_cfg.tuner_num; i ++) {
        stb_tuner_cfg *tun = &aml_hw_cfg.tuners[i];

        CFG_DBG("tuner%d ts_input:%d frontend:%d signal_types:%d dvbt2:%d dvbs2:%d",
                                i,
                                tun->ts_input_idx,
                                tun->frontend_idx,
                                tun->signal_types,
                                tun->support_dvbt2,
                                tun->support_dvbs2);

       // B: Starting Up Log
       CERT_Log_StartingUp("tuner%d ts_input:%d frontend:%d signal_types:%d dvbt2:%d dvbs2:%d",
                                i,
                                tun->ts_input_idx,
                                tun->frontend_idx,
                                tun->signal_types,
                                tun->support_dvbt2,
                                tun->support_dvbs2);
       // E: Starting Up Log
    }

    XML_ParserFree(parser);
    fclose(fp);
}

int STB_EpgGetIsNotMatchOrigNetId()
{
	return aml_hw_cfg.epg_cfg.is_not_match_orignetid;
}

int STB_EpgGetIsNotMatchTsId()
{
	return aml_hw_cfg.epg_cfg.is_not_match_tsid;
}

int STB_EpgGetBarkerChannelEnabled()
{
	return aml_hw_cfg.epg_cfg.barker_channel_enabled;
}

int STB_EpgGetEitSearchEnabled()
{
	return aml_hw_cfg.epg_cfg.eit_search_enabled;
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
 * @brief   get ca dev id value
 * @param   slot is used for which device is select
 */
int STB_Get_Ca_devid(int slot)
{
	return aml_hw_cfg.cam[0].dev_id;
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
 * @brief   get dynamic prop
 *          prority1: android property
 *          prority2: config.xml
 *          prority3: other modules
 * @param   name prop name
 * @param   buf returned value
 * @param   len length of buf
 * @return  TRUE if got, FALSE otherwise
 */
BOOLEAN STB_Get_Prop(const char *name, char *buf, int len)
{
    stb_hardware_cfg *cfg = &aml_hw_cfg;
    ENTRY e, *ep;
    int search_ret=0;
    int get_ret=0;

    if ( buf==NULL || len<=0 ) {
        return FALSE;
    }

    e.key = (char*)name;
    search_ret = hsearch_r(e,FIND,&ep,&(cfg->prop_htab));
#ifdef DTVKIT_IN_VENDOR_PARTITION
    get_ret = property_get(name, buf, (search_ret!=0?ep->data:NULL));
#endif

    if ( get_ret>0 ) {
        return TRUE;
    }
    if ( search_ret!=0 ) {
        strncpy(buf,ep->data,len);
        CFG_DBG("Hash table, key:%s, value:%s",name,buf);
        return TRUE;
    }

    return (TRUE == STB_DVRProp_Get(name, buf, len)) ? TRUE : FALSE;
}

/**
 * @brief   Get cam work mode[ci/ciplus]
 * @return  1 for ciplus 0 for ci
 */
int STB_Cam_Is_CIPlus_Mode()
{
	return aml_hw_cfg.cam->is_ciplus_mode;
}

BOOLEAN STB_GetDemoCapabilityByType(E_STB_TUNE_SIGNAL_TYPE eType, U_STB_DEMO_CAPABILITY *pCap)
{
    BOOLEAN ret = TRUE;
    if(NULL == pCap)
    {
        return FALSE;
    }
    switch(eType)
    {
    case TUNE_SIGNAL_QAM:
    {
        pCap->dvbc.symbol_rate_auto = aml_hw_cfg.demo_cap.srate_auto;
        pCap->dvbc.symbol_rate_value = aml_hw_cfg.demo_cap.srate_auto_value;
        break;
    }
    default:
    {
        ret = FALSE;
    }
    }
   return ret;
}
/**
 * @brief   set dynamic prop
   @param   name prop name
   @param   value set value
 */
void STB_Set_Prop(const char *name, const char *value)
{
    if ( name==NULL || value==NULL ) {
        return;
    }
#ifdef DTVKIT_IN_VENDOR_PARTITION
    property_set(name, value);
#else
    stb_hardware_cfg  *cfg = &aml_hw_cfg;
    ENTRY e, *ep;
    int hret;

    e.key = (char *)name;
    hret = hsearch_r(e,FIND,&ep,&(cfg->prop_htab));
    if (hret!=0) {
        STB_MEMFreeSysRAM(ep->data);
        ep->data=strdup(value);
        CFG_DBG("Hash table key:%s, value:%s",ep->key,ep->data);
    } else {

        STB_DVRProp_Set(name, value);
    }
#endif
}


/**
 * @brief   Check if FCC/PIP is enabled.
 * @return  0: Either FCC or PIP is enabled.
 *          1: Both are disabled.
 */
int STB_GetFccPipCfgStatus(void)
{
    return (STB_Is_PIP_Enabled() || STB_Is_FCC_Enabled()) ? 0 : 1;
}

/**
 * @brief   get cam card data in and out.
 * @return  TRUE if yes.
 */
BOOLEAN STB_GetCamSource(U8BIT* input_with_card, U8BIT* input_without_card)
{
    if (input_with_card)
    {
        STB_SPDebugWrite("tssource %d", aml_hw_cfg.cam->camPlug_tssource);
        *input_with_card = aml_hw_cfg.cam->camPlug_tssource;
    }
    if (input_without_card)
        *input_without_card = aml_hw_cfg.cam->camUnplug_tssource;
    return TRUE;
}

S_CAPTURE_ADC_CFG STB_GetCaptureADCCfg()
{
    return aml_hw_cfg.capture_adc;
}

/**
 * @brief   get pvr encrypt enable status
 * @return  TRUE if yes
 */
BOOLEAN STB_Get_PVR_Encrypt()
{
   return aml_hw_cfg.pvr.encrypt == 1 ? 1 : 0;
}

/**
 * @brief   get pvr record ring buffer size
 * @return  ring buffer size;
 */
int STB_Get_PVR_RecRingBufSize()
{
    return aml_hw_cfg.pvr.rec_ringbuf_size;
}

/**
 * @brief   get cam CI host mode.
 * @return  host mode 2:ask user to confirm, others: no need
 */
int STB_GetCIHostMode(void)
{
    return aml_hw_cfg.cam->host_mode;
}

/**
 * @brief   get group which platform belongs to.
 * @return  group by bits
 */
int STB_GetPlatformGroupId(void)
{
    BOOLEAN ret = FALSE;
    char buf[64];
    int group = 0;

    memset(buf, 0, sizeof(buf));
    ret = STB_File_Read("/sys/class/stb/dmx_ver", buf, sizeof(buf));
    if (!ret)
        return 0;

    #define GRP_BIT(_grp) (1 << ((_grp) - 1))

    /*group id 1*/
    if (memcmp(buf, "sc2", 3) == 0 || memcmp(buf, "t7", 2))
        group |= GRP_BIT(1);

    return group;
}

BOOLEAN STB_GetCustomCFGForAutoTime(void)
{
    return aml_custom_config.auto_time_enable;
}

BOOLEAN STB_GetCustomCFGForFOSAudio(void)
{
    return aml_custom_config.fos_ad_enable;
}

BOOLEAN STB_GetCustomCFGForShineDemux(void)
{
    return aml_custom_config.shine_dmx_enable;
}

BOOLEAN STB_GetCustomCFGForMS12AC4(void)
{
    return aml_custom_config.ms12_ac4_enable;
}

BOOLEAN STB_GetCustomCFGForDEULcnSortAfterLast(void)
{
    return aml_custom_config.deu_sort_lcn_after_last;
}

BOOLEAN STB_GetCustomCFGForDEUUseInviableFlag(void)
{
    return aml_custom_config.deu_use_invisible_flag;
}

BOOLEAN STB_GetCustomCFGForDisableAutomaticUpdate(void)
{
    return aml_custom_config.disable_automatic_update;
}

/**
 * @brief   get unsupport service type,such as 4k or 8k and so on
 * @return  E_STB_CFG_SERVICE_UPSOPPORT_TYPE
 */
int STB_CFG_GetServiceUnsupportType(void)
{
   return aml_hw_cfg.service_unsupport_type;
}

/**
 * @brief   check DescriptorTag is unsupport
 * @return  TRUE: this tag is unsupport
 */
BOOLEAN STB_CFG_IsUnsupportDescriptorTag(U8BIT descriptor_tag)
{
    BOOLEAN ret_val = FALSE;
    int idx = 0;
    if (aml_hw_cfg.unsupport_descriptor_tag_num > 0 && NULL != aml_hw_cfg.unsupport_descriptor_tag_list)
    {
        for (idx = 0;idx < aml_hw_cfg.unsupport_descriptor_tag_num;idx++)
        {
            if (descriptor_tag == aml_hw_cfg.unsupport_descriptor_tag_list[idx])
            {
                ret_val = TRUE;
                break;
            }
        }
    }
   return ret_val;
}


static void DVR_Get_Prop(const char *name, char *buf, int len)
{
    BOOLEAN am_success = STB_Get_Prop(name, buf, len);
    if (TRUE == am_success)
    {
        CFG_DBG("%s: succeeded.\r\n", __FUNCTION__);
    }
    else
    {
        CFG_DBG("%s: failed.\r\n", __FUNCTION__);
    }
}

static void DVR_Set_Prop(const char *name, const char *value)
{
    STB_Set_Prop(name, value);
}

BOOLEAN STB_Is_PIP_Enabled()
{
   char prop_buf[16] = {0};
   if (TRUE == STB_Get_Prop("vendor.tv.dtv.enable.pip",prop_buf,16))
   {
      if (strcmp(prop_buf,"true") == 0 || strcmp(prop_buf,"yes") == 0)
      {
         return TRUE;
      }
   }
   return FALSE;
}

BOOLEAN STB_Is_FCC_Enabled()
{
   char prop_buf[16] = {0};
   if (TRUE == STB_Get_Prop("vendor.tv.dtv.enable.fcc",prop_buf,16))
   {
      if (strcmp(prop_buf,"true") == 0 || strcmp(prop_buf,"yes") == 0)
      {
         return TRUE;
      }
   }
   return FALSE;
}

BOOLEAN STB_GetPlatformOui(int *oui)
{
    if (oui == NULL) {
        return FALSE;
    }
    *oui = aml_hw_cfg.oui;
    return TRUE;
}
