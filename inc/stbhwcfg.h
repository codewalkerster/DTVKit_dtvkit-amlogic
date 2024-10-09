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

#include "techtype.h"
#include "stbhwtun.h"

#undef  _GNU_SOURCE
#define _GNU_SOURCE
#undef  __USE_GNU
#define __USE_GNU
#include <search.h>

typedef struct
{
    int ts_input_idx;
    int ori_tsinput_idx;
    int frontend_idx;
    int signal_types;
    int support_dvbt2;
    int support_dvbs2;
} stb_tuner_cfg;

#define AML_MAX_TUNER_NUM 8
#define AML_MAX_CAM_NUM 2
#define AML_MAX_DMX_NUM 16
#define AML_MAX_MEM_LEVEL_NUM 7

typedef enum e_stb_cfg_service_unsupport_type
{
    E_STB_CFG_SERVICE_UPSOPPORT_TYPE_4K = 1,
    E_STB_CFG_SERVICE_UPSOPPORT_TYPE_HDR_HLG = 2,
} E_STB_CFG_SERVICE_UPSOPPORT_TYPE;


typedef struct
{
    int           CICAM_TSI;
} stb_cam_cfg;

typedef struct
{
    int           encrypt;
    int           rec_ringbuf_size;
    int           rec_hwbuf_size;
} stb_pvr_cfg;

typedef struct
{
    int           net_id_max;
    int           orig_net_id_max;
    int           usr_def_orig_net_id;
    int           net_id_change_update;
} stb_network_id_cfg;

typedef struct
{
    int           is_not_match_orignetid;
    int           is_not_match_tsid;
    int           barker_channel_enabled;
    int           eit_search_enabled;
} stb_epg_cfg;

typedef struct
{
    BOOLEAN tvp_enable;
    BOOLEAN secmem_enable;
} stb_secure_cfg;

typedef struct
{
    int           sdt_timeout;
    int           pat_timeout;
    int           pmt_timeout;
    int           nit_timeout;
    int           cat_timeout;
    int           bat_timeout;
    int           tot_timeout;
    int           tdt_timeout;
    int           eit_timeout;
} stb_sipsi_timeout;

typedef struct
{
    unsigned char     srate_auto;
    unsigned int     srate_auto_value;
} stb_demo_cap;


typedef enum
{
    SIPSI_PAT = 0,
    SIPSI_PMT,
    SIPSI_SDT,
    SIPSI_NIT,
    SIPSI_CAT,
    SIPSI_BAT,
    SIPSI_TOT,
    SIPSI_TDT,
    SIPSI_EIT
} E_SI_PSI_TYPE;

typedef struct
{
    BOOLEAN analog_enabled;
    BOOLEAN dvbs_enabled;
    BOOLEAN dvbt_enabled;
    BOOLEAN dvbc_enabled;
    BOOLEAN isdbt_enabled;
    char file_path[128];
} S_CAPTURE_ADC_CFG;

typedef struct
{
    int level;
    int size;
} stb_dmc_mem;

typedef struct
{
    BOOLEAN tvin_db_reg_en;
} stb_tvin_cfg;

typedef struct
{
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
    int           oui;
    stb_pvr_cfg   pvr;
    char          country_code[3];
    stb_network_id_cfg network;
    stb_sipsi_timeout sipsi;
    stb_demo_cap demo_cap;
    int           service_unsupport_type;
    int           fcc_pip;
    S_CAPTURE_ADC_CFG capture_adc;
    stb_epg_cfg epg_cfg;
    stb_secure_cfg secure_cfg;
    struct hsearch_data  prop_htab;
    BOOLEAN prop_htab_ok;
    int           mem_level_num;
    stb_dmc_mem dmc_mem[AML_MAX_MEM_LEVEL_NUM];
    int unsupport_descriptor_tag_num;
    U8BIT *unsupport_descriptor_tag_list;
    stb_tvin_cfg tvin_cfg;
} stb_hardware_cfg;

typedef struct {
    BOOLEAN ms12_ac4_enable;
    BOOLEAN deu_sort_lcn_after_last;
    BOOLEAN deu_use_invisible_flag;//when LCN is invalid, if inviable flag is valid.
    BOOLEAN disable_automatic_update;
    BOOLEAN a_a_1;
    int scrambled_flag_control;
} stb_custom_config;

typedef union
{
    struct
    {
        U8BIT symbol_rate_auto;
        U8BIT symbol_rate_value;
    } dvbc;
    struct
    {
        U8BIT unused;
    } dvbt;
} U_STB_DEMO_CAPABILITY;

extern stb_hardware_cfg aml_hw_cfg;

extern void STB_CfgInitialise(void);

int STB_EpgGetIsNotMatchOrigNetId();

int STB_EpgGetIsNotMatchTsId();

int STB_EpgGetBarkerChannelEnabled();

int STB_EpgGetEitSearchEnabled();

/**
 * @brief   get country code from cfg
 * @param   country code
 */
int STB_Get_Country_Code(char *country_code);

/**
 * @brief   get max value that orig_net_id_max and net_id_max from cfg
 * @param   net_id_max
 * @param   orig_net_id_max
 */
void STB_Get_Max_Network_Id(int            *net_id_max, int *orig_net_id_max);

/**
 * @brief   get user define value about orig_net_id from cfg
 * @return  usr_def_orig_net_id
 */
int STB_Get_Usr_Orig_Net_Id();

/**
 * @brief   get support that network id changed whether auto update channels
 * @return  net_id_change_update
 */
int STB_NetId_Change_Update_Ch();

/**
 * @brief   get si/psi timeout from cfg
 * @return  usr_def_orig_net_id
 */
int STB_Get_SI_PSI_Timeout(E_SI_PSI_TYPE sipsi_type);

/**
 * @brief   get unsupport service type,such as 4k or 8k and so on
 * @return  E_STB_CFG_SERVICE_UPSOPPORT_TYPE
 */
int STB_CFG_GetServiceUnsupportType(void);

/**
 * @brief   check DescriptorTag is unsupport
 * @return  TRUE: this tag is unsupport
 */
BOOLEAN STB_CFG_IsUnsupportDescriptorTag(U8BIT descriptor_tag);

/**
 * @brief   get dynamic prop
   @param   name prop name
   @param   buf returned value
   @param   len length of buf
   @return  TRUE if got, FALSE otherwise
 */
BOOLEAN STB_Get_Prop(const char *name, char *buf, int len);

/**
 * @brief   set dynamic prop
   @param   name prop name
   @param   value set value
 */
void STB_Set_Prop(const char *name, const char *value);

/**
 * @brief   get ca dev id value
 * @param   slot is used for which device is select
 */
int STB_Get_Ca_devid(int slot);

/**
 * @brief   get demo Capability
 * @param   eType demo type
 */
BOOLEAN STB_GetDemoCapabilityByType(E_STB_TUNE_SIGNAL_TYPE eType, U_STB_DEMO_CAPABILITY *pCap);

/**
 * @brief   get config of fcc pip status
 * @return  // 0 enable 1 disable
 */
int STB_GetFccPipCfgStatus(void);

/**
 * @brief   get config of capture ADC Data
 * @return  S_CAPTURE_ADC_CFG cfg.
 */
S_CAPTURE_ADC_CFG STB_GetCaptureADCCfg();

/**
 * @brief   get pvr encrypt enable status
 * @return  TRUE if yes
 */
BOOLEAN STB_Get_PVR_Encrypt();

/**
 * @brief   get pvr record ring buffer size
 * @return  ring buffer size;
 */
int STB_Get_PVR_RecRingBufSize();

/**
 * @brief   get pvr record driver internal buffer size
 * @return  buffer size;
 */
int STB_Get_PVR_RecHwBufSize();

/**
 * @brief   get group which platform belongs to.
 * @return  group by bits
 */
int STB_GetPlatformGroupId(void);

BOOLEAN STB_GetTvpEnable(void);

BOOLEAN STB_GetSecMemEnable(void);

BOOLEAN STB_GetCustomCFGForA_A_1(void);

BOOLEAN STB_GetCustomCFGForMS12AC4(void);

BOOLEAN STB_GetCustomCFGForDEULcnSortAfterLast(void);

BOOLEAN STB_GetCustomCFGForDEUUseInviableFlag(void);

BOOLEAN STB_GetCustomCFGForDisableAutomaticUpdate(void);

U8BIT STB_GetCustomCFGForScrambledFlagControl(void);

BOOLEAN STB_GetPlatformOui(int *oui);
/**
 * @brief   Check if PIP is allowed by reading a config item.
 * @return  TRUE if allowed, FALSE otherwise
 */
BOOLEAN STB_Is_PIP_Enabled();

/**
 * @brief   Check if FCC is allowed by reading a config item.
 * @return  TRUE if allowed, FALSE otherwise
 */
BOOLEAN STB_Is_FCC_Enabled();

/**
 * @brief   Check if tuner framework enabled.
 * @return  TRUE if enabled, FALSE otherwise
 */
BOOLEAN STB_Is_TunerFramework_Enabled();

BOOLEAN STB_Get_Tvin_Db_Reg_Enabled();
#endif
