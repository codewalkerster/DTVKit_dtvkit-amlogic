/*******************************************************************************
 * Copyright ?2014 The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
 * Copyright ?2013 Ocean Blue Software Ltd
 *
 * This file is part of a DTVKit Software Component
 * You are permitted to copy, modify or distribute this file subject to the terms
 * of the DTVKit 1.0 Licence which can be found in licence.txt or at www.dtvkit.org
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * If you or your organisation is not a member of DTVKit then you have access
 * to this source code outside of the terms of the licence agreement
 * and you are expected to delete this and any associated files immediately.
 * Further information on DTVKit, membership and terms can be found at www.dtvkit.org
 *******************************************************************************/
/**
 * @brief   Glue layer between DVB and conditional access systems
 * @file    ca_glue.c
 * @date    18/09/2013
 * @author  Steve Ford
 */

#define CA_GLUE_DEBUG

#define loff_t off_t

/*---Includes for this file---------------------------------------------------*/
/* compiler library header files */
#include <unistd.h>
#include <string.h>
#include "dtv_log.h"
#define TAG  "CA_GLUE_AMLMP"
#ifdef ANDROID
#include <cutils/properties.h>
#endif

/* Third party header files */

/* Ocean Blue Software header files */
#include <techtype.h>
#include <dbgfuncs.h>
#include <stbhwc.h>
#include "stbhwos.h"
#include "stbhwdmx.h"
#include "stbhwmem.h"
#include "ca_glue.h"
#include "stbhwcfg.h"
#include "stbca.h"

#include "cJSON.h"

/*---constant definitions for this file----------------------------------------*/

#ifdef CA_GLUE_DEBUG
#define CA_DBG(X,...)    DTV_LOGI(TAG, X, ##__VA_ARGS__)
#else
#define CA_DBG(X)
#endif
#define CA_DTAG     0x09
#define SCRAMBLING_DTAG             0x65     /* scramble flag try to get algorithm */

typedef struct ca_desc
{
    U16BIT ca_id;
    U16BIT ca_pid;
    U16BIT private_data_length;
    U8BIT *private_data;
} CA_DESC;

typedef struct stream_entry
{
    struct stream_entry *next;
    U16BIT pid;
    U16BIT num_ca_entries;
    CA_DESC *ca_desc_array;

} STREAM_ENTRY;

typedef struct ca_list
{
    U16BIT serv_id;
    U16BIT num_ca_entries;
    CA_DESC *ca_desc_array;
    U8BIT scramble_algo; /* nagra cas need algo to check can descramble or not */

    U16BIT num_streams;
    STREAM_ENTRY *stream_list;
    STREAM_ENTRY *last_stream_entry;
} CA_LIST;

//#ifdef SUPPORT_CAS
/*---local typedef structs for this file---------------------------------------*/

/*---local (static) variable declarations for this file------------------------*/
/*   (internal variables declared static to make them local) */
#define ITEM_CAS                "cas"
#define ITEM_TYPE               "type"
#define ITEM_PIN_STATE          "pinstate"
#define ITEM_CHECK_PIN          "checkPin"
#define ITEM_ERROR_CODE         "errcode"
#define VMX_CAS_STRING          "Verimatrix"
#define ITEM_CMD                "cmd"
#define MAX_JSON_LEN            (1024)
#define ITEM_DVR_CAS_MODE       "casMode"
#define ITEM_PATH_TYPE          "pathType"
#define DVB_INVALID_ID              0x1fff
#define INVALID_RES_ID     ((U8BIT)0xFF)     /* ID used to represent an invalid resource */

typedef enum {
    PIN_NEED_CHECK,
    PIN_CHECK_SUCCESS,
    PIN_CHECK_FAILED,
    PIN_MAX,
} PIN_STATUS;

typedef enum {
    CAS_SESSION_PATH_LIVE,
    CAS_SESSION_PATH_RECORD,
    CAS_SESSION_PATH_PLAYBACK,
    CAS_SESSION_PATH_ANY,
} CAS_SESSION_PATH_TYPE;

static int g_checkpin_status = PIN_MAX;
static AML_MP_CASSESSION g_pvrplay_session;
static U32BIT g_pvrplay_replay = 0;

static U32BIT g_last_stream_pos = -1;
static void *g_ca_mutex;

static BOOLEAN is_enable_cicam = FALSE;
static E_CAS_TYPE g_cas_type = CAS_TYPE_NONE;
static BOOLEAN is_enable_fta = FALSE;
static const char* IOCTRL_INVOKE_GET_CAS_MODE = "{\"InvokeID\":3}";
static BOOLEAN is_m2m = TRUE;

typedef struct sess_info_entry
{
    struct sess_info_entry *next;
    U16BIT ecm_pid;
    AML_MP_CASSESSION cas_session;
    // AM_CA_ServiceInfo_t ca_serv_info;
} SESSION_INFO;

typedef struct es_pid_entry
{
    struct es_pid_entry *next;
    U16BIT es_pid;
    U16BIT ecm_pid;
    BOOLEAN running;
} CA_INFO;

typedef struct
{
    BOOLEAN has_global_ca;
    BOOLEAN has_component_ca;
    U8BIT scramble_algo;
    CA_INFO *ca_pid_list;
    CA_INFO *last_pid_entry;
} PMT_INFO;

typedef struct
{
    U8BIT path;
    U16BIT service_id;
    PMT_INFO pmt_info;
    SESSION_INFO *session_info;
    SESSION_INFO *last_sess_entry;
    U16BIT emm_pid;
    BOOLEAN is_descrambling;
    BOOLEAN is_recording;
    BOOLEAN is_replay;
    BOOLEAN is_dvr_start;
    BOOLEAN is_timeshift;
} STB_CA_Glue_t;

/*---local function prototypes for this file-----------------------------------*/
static U8BIT _GetPathDemux(U8BIT path)
{
    U8BIT dev_no = 0 ;
    STB_DMXGetDevNo(path , &dev_no);
    return(dev_no);
}
/*   (internal functions declared static to make them local) */
static U16BIT parser_cat_table(U8BIT *data)
{
    U16BIT ca_system_id = 0, ca_pid = 0, sec_len = 0;
    U8BIT desc_tag = 0, desc_len = 0;
    U8BIT *p = NULL;

    p = data;
    sec_len = ((p[1] << 8 | p[2]) & 0x0FFF);

    p += 8;
    sec_len -= 5;

    while ( sec_len > 9 )
    {
        CA_DBG("%s, cat data: %#x, %#x, %#x, %#x, %#x, %#x ", __FUNCTION__,
                p[0], p[1], p[2], p[3], p[4], p[5]);
        desc_tag = p[0];
        desc_len = p[1];
        ca_system_id = ((p[2] << 8) | p[3]);
        ca_pid = ((p[4] << 8) | p[5]) & 0x1FFF;
        sec_len -= (desc_len + 2);
        p += (desc_len + 2);

        if (Aml_MP_CAS_IsSystemIdSupported(ca_system_id))
        {
            CA_DBG("%s found supported ca pid[%#x] in cat", __FUNCTION__, ca_pid);
            return ca_pid;
        }
    }

    return DVB_INVALID_ID;
}

static BOOLEAN pid_on_decoding(U8BIT path, U16BIT es_pid)
{
    /*no need do this action ,STB_CAReportPMT will contail ES PID INFO*/
#if 0
    if (es_pid == STB_DPGetVideoPID(path)
            || es_pid == STB_DPGetAudioPID(path)
            || es_pid == STB_DPGetADPID(path)
            || es_pid == STB_DPGetTextPID(path)
            || es_pid == STB_DPGetDataPID(path))
    {
        return TRUE;
    }
#endif

    return FALSE;
}

static void free_pid_list(UINTPTR handle)
{
    //free pid info list
    CA_INFO *head = ((STB_CA_Glue_t *)handle)->pmt_info.ca_pid_list;
    CA_INFO *pid_entry = head;

    while (pid_entry != NULL)
    {
        head = pid_entry->next;
        STB_MEMFreeSysRAM(pid_entry);
        pid_entry = head;
    }

    ((STB_CA_Glue_t *)handle)->pmt_info.ca_pid_list = NULL;
}

static void add_sess_pid(UINTPTR handle, U16BIT pid)
{
    SESSION_INFO *sess_node;
    SESSION_INFO *sess_entry = (((STB_CA_Glue_t *)handle)->session_info);

    //not add if the pid exists already
    while (sess_entry != NULL)
    {
        if (sess_entry->ecm_pid == pid)
        {
            return;
        }

        sess_entry = sess_entry->next;
    }

    sess_node = (SESSION_INFO *)STB_MEMGetSysRAM(sizeof(SESSION_INFO));
    memset(sess_node, 0, sizeof(SESSION_INFO));
    sess_node->ecm_pid = pid;

    if (((STB_CA_Glue_t *)handle)->last_sess_entry == NULL)
    {
        ((STB_CA_Glue_t *)handle)->session_info = sess_node;
    }
    else
    {
        ((STB_CA_Glue_t *)handle)->last_sess_entry->next = sess_node;
    }

    ((STB_CA_Glue_t *)handle)->last_sess_entry = sess_node;
}

static void free_sess_list(UINTPTR handle)
{
    //free session info list
    SESSION_INFO *sess_head = (((STB_CA_Glue_t *)handle)->session_info);
    SESSION_INFO *sess_entry = sess_head;

    while (sess_entry != NULL)
    {
        sess_head = sess_entry->next;
        STB_MEMFreeSysRAM(sess_entry);
        sess_entry = sess_head;
    }

    ((STB_CA_Glue_t *)handle)->session_info = NULL;
}

static BOOLEAN is_sess_empty(UINTPTR handle)
{
    return (((STB_CA_Glue_t *)handle)->session_info)? 0 : 1;
}

static AML_MP_CASSESSION get_cas_session(UINTPTR handle, U16BIT es_pid)
{
    U16BIT ecm_pid = 0;
    CA_INFO *pid_entry = ((STB_CA_Glue_t *)handle)->pmt_info.ca_pid_list;

    while (pid_entry != NULL)
    {
        if (pid_entry->es_pid == es_pid)
        {
            ecm_pid = pid_entry->ecm_pid;
            break;
        }

        pid_entry = pid_entry->next;
    }

    SESSION_INFO *sess_entry = (((STB_CA_Glue_t *)handle)->session_info);

    while (sess_entry != NULL)
    {
        if (sess_entry->ecm_pid == ecm_pid)
        {
            return sess_entry->cas_session;
        }

        sess_entry = sess_entry->next;
    }

    return NULL;
}

static void update_desc_pid(UINTPTR handle)
{
    CA_INFO *pid_entry;
    U16BIT old_es_pid = DVB_INVALID_ID;
    U16BIT new_es_pid = DVB_INVALID_ID;

    pid_entry = ((STB_CA_Glue_t *)handle)->pmt_info.ca_pid_list;

    while (pid_entry != NULL)
    {
        if (pid_on_decoding(((STB_CA_Glue_t *)handle)->path, pid_entry->es_pid)
                && pid_entry->running != TRUE)
        {
            new_es_pid = pid_entry->es_pid;
        }

        if ((pid_on_decoding(((STB_CA_Glue_t *)handle)->path, pid_entry->es_pid) == FALSE)
                && pid_entry->running == TRUE)
        {
            old_es_pid = pid_entry->es_pid;
        }

        pid_entry = pid_entry->next;
    }

    //only support pid update, not support add/remove
    if (old_es_pid == DVB_INVALID_ID
            || new_es_pid == DVB_INVALID_ID)
    {
        return;
    }

    Aml_MP_CAS_UpdateDescramblingPid(
        get_cas_session(handle, old_es_pid),
        old_es_pid,
        new_es_pid);
}

static int cas_event_cb(AML_MP_CASSESSION session, const char *json)
{
    static CAS_EVENT_DATA_t cas_event_data;
    BOOLEAN isPlaybackPath = false, has_ca = false;
    U8BIT num_paths, cas_path = INVALID_RES_ID;
    UINTPTR ca_handle = 0;

    cJSON* data = cJSON_Parse(json);
    if (!data)
    {
        CA_DBG("%s:not a valid event- %s", __func__, json);
        return 0;
    }

    if (g_pvrplay_session && (session == g_pvrplay_session))
    {
        CA_DBG("%s:event for dvr playback session", __func__);
        isPlaybackPath = TRUE;
    }

    cJSON* cas = cJSON_GetObjectItemCaseSensitive(data, ITEM_CAS);
    cJSON* type = cJSON_GetObjectItemCaseSensitive(data, ITEM_TYPE);
    cJSON* pathType = cJSON_GetObjectItemCaseSensitive(data, ITEM_PATH_TYPE);
    cJSON* msg_type = cJSON_GetObjectItem(data, "msg_type");

    if (isPlaybackPath &&
    cJSON_IsString(cas) &&
    (!strcmp(cas->valuestring, VMX_CAS_STRING)) &&
    (cJSON_IsString(type)) &&
    type->valuestring)
    {
        if (!strcmp(type->valuestring, ITEM_CHECK_PIN))
        {
            g_checkpin_status = PIN_NEED_CHECK;
            CA_DBG("%s: pin need check\n", __func__);
        }
        else if (!strcmp(type->valuestring, ITEM_PIN_STATE))
        {
            cJSON* state = cJSON_GetObjectItemCaseSensitive(data, ITEM_ERROR_CODE);
            if (cJSON_IsNumber(state))
            {
                if (state->valuedouble)
                {
                    g_checkpin_status = PIN_CHECK_SUCCESS;
                    CA_DBG("%s: pin check success\n", __func__);
                }
                else
                {
                    g_checkpin_status = PIN_CHECK_FAILED;
                    CA_DBG("%s: pin check failed\n", __func__);
                }
            }
        }
    }
    cJSON_AddNumberToObject(data, "session", (UINTPTR)session);

    if (cJSON_IsNumber(pathType))
        cas_event_data.pathType = (U8BIT)(pathType->valuedouble);
    else
        cas_event_data.pathType = CAS_SESSION_PATH_ANY;

    if (isPlaybackPath)
        cas_event_data.pathType = CAS_SESSION_PATH_PLAYBACK;
    else if (cas_path == INVALID_RES_ID)
    {
        CA_DBG("%s:cannot found none playback path, maybe a global cas event.", __func__);
        cas_event_data.pathType = CAS_SESSION_PATH_ANY;
    }

    cas_event_data.path = cas_path;
    cas_event_data.session = (UINTPTR)session;
    U32BIT data_len = strlen(json);
    if (data_len + 1 < CAS_MSG_LEN)
    {
        cJSON_PrintPreallocated(data, cas_event_data.data_str, CAS_MSG_LEN, 0);
        CA_DBG("%s:%s", __func__, cas_event_data.data_str);
        //STB_ERSendEvent(FALSE, FALSE, EV_CLASS_CAS, EV_TYPE_CAS, &cas_event_data, sizeof(cas_event_data));
        STB_OSSendEvent(FALSE, HW_EV_CLASS_CAS, EV_TYPE_CAS_HW, &cas_event_data, sizeof(cas_event_data));

    }
    else
    {
        CAS_EVENT_LONG_DATA_t *cas_event_long_data = (CAS_EVENT_LONG_DATA_t *)STB_MEMGetSysRAM(sizeof(CAS_EVENT_LONG_DATA_t));
        cas_event_long_data->long_data_str = (char *)STB_MEMGetSysRAM((data_len + 1) * sizeof(char));
        cJSON_PrintPreallocated(data, cas_event_long_data->long_data_str, data_len + 1, 0);
        cas_event_long_data->pathType = cas_event_data.pathType;
        cas_event_long_data->path = cas_event_data.path;
        CA_DBG("%s: data is too long data_len %d \n", __func__, data_len);
        CA_DBG("%s:%s", __func__, cas_event_long_data->long_data_str);
        //STB_ERSendEvent(FALSE, FALSE, EV_CLASS_CAS, EV_TYPE_CAS_LONG, &cas_event_long_data, sizeof(void *));
        STB_OSSendEvent(FALSE, HW_EV_CLASS_CAS, EV_TYPE_CAS_LONG_HW, &cas_event_long_data, sizeof(void *));

    }

    if (data)
    {
        cJSON_Delete(data);
        data = NULL;
    }

    return 0;
}

static void get_cas_mode(AML_MP_CASSESSION session)
{
    static BOOLEAN is_gained_cas_mode = FALSE;
    cJSON *input = NULL;
    cJSON *item = NULL;
    char out_json[MAX_JSON_LEN];

    if (is_gained_cas_mode == TRUE) {
        return;
    }

    if (session)
        Aml_MP_CAS_Ioctl(session, IOCTRL_INVOKE_GET_CAS_MODE, out_json, MAX_JSON_LEN);

    input = cJSON_Parse(out_json);
    item = cJSON_GetObjectItemCaseSensitive(input, ITEM_DVR_CAS_MODE);
    if (!cJSON_IsString(item) || item->valuestring[0] == '\0') {
        cJSON_Delete(input);
        return;
    }

    if (strncmp(item->valuestring, "false", strlen(item->valuestring)) == 0) {
        is_m2m = FALSE;
        CA_DBG("%s:isn't M2M", __func__);
    } else if (strncmp(item->valuestring, "true", strlen(item->valuestring)) == 0) {
        is_m2m = TRUE;
        CA_DBG("%s:is M2M", __func__);
    }

    is_gained_cas_mode = TRUE;
    cJSON_Delete(input);
}

static void tms_status_check(char* Json ,U32BIT outLen)
{
    #define msg_len 1024*80
    static char out_json[msg_len];
    cJSON *client_data = NULL;
    cJSON *result_data = NULL;
    if (NULL == Json)
    {
        Aml_MP_CAS_Ioctl(0, "{\"InvokeID\":1003}", out_json, 8192);
    }
    else
    {
        if (outLen <= msg_len)
            memcpy(out_json, Json, outLen);
    }
    client_data = cJSON_Parse(out_json);
    if (TRUE == cJSON_HasObjectItem(client_data, "Result"))
    {
        result_data = cJSON_GetObjectItem(client_data, "Result");
        //check have TMS&Flexi
        CA_DBG("==TmsData [%d]  FlexiCore [%d ]==",\
        cJSON_HasObjectItem(result_data, "TmsData"),\
        cJSON_HasObjectItem(result_data, "FlexiCore") );

        //retrieve FlexiCore
        if (TRUE == cJSON_HasObjectItem(result_data, "FlexiCore"))
        {
            cJSON *FlexiCore =cJSON_GetObjectItem(result_data, "FlexiCore");
            cJSON *message = cJSON_GetObjectItem(FlexiCore, "message");
            CA_DBG("===================>message[%d]" ,  message->valueint );
        }
        //retrieve TmsData
        if (TRUE == cJSON_HasObjectItem(result_data, "TmsData"))
        {
            cJSON *TmsData =cJSON_GetObjectItem(result_data, "TmsData");
            //retrieve each item
            cJSON *TmsData_element = NULL;
            cJSON_ArrayForEach(TmsData_element, TmsData)
            {
                cJSON *CICAM = cJSON_GetObjectItem(TmsData_element, "CICAM");
                if (NULL != CICAM)
                {
                    CA_DBG("===================>CICAM[%d]", CICAM->valueint);
                    /*mark this since for SMDC test*/
                    if (CICAM->valueint == 0)
                    {
                        is_enable_cicam = FALSE;
                    }
                    else
                    {
                        is_enable_cicam = TRUE;
                    }
                }
                else
                {
                    CA_DBG("===================>no CICAM");
                    is_enable_cicam = FALSE;
                }

                cJSON *FTA = cJSON_GetObjectItem(TmsData_element, "FTA");
                if (NULL != FTA)
                {
                    CA_DBG("===================>FTA[%d]" , FTA->valueint);
                    if (FTA->valueint == 0)
                    {
                        is_enable_fta = FALSE;
                    }
                    else
                    {
                        is_enable_fta = TRUE;
                    }
                 }
                 else
                 {
                     CA_DBG("===================>no FTA");
                     is_enable_fta = FALSE;
                 }
             }
         }
    }
    if (client_data)
    {
        cJSON_Delete(client_data);
        client_data = NULL;
    }
}
//#endif
/*---global function definitions-----------------------------------------------*/

/*!**************************************************************************
 * @brief   Called once on system startup to allow initialisation of the CA systems
 * @return  TRUE if initialisation is successful, FALSE otherwise
 ****************************************************************************/
BOOLEAN STB_CAInitialise(void)
{
#ifdef SUPPORT_CAS
    int ret;
    static int inited = 0;

    FUNCTION_START(STB_CAInitialise);

    CA_DBG("call %s", __FUNCTION__);

    if (!inited)
    {
        inited = 1;
        ret = Aml_MP_CAS_Initialize();

        if (ret)
        {
            CA_DBG("am cas init failed [%d]", ret);
        }
        else
        {
            STB_Set_Prop("vendor.tv.dtv.cas.ready", "true");
        }

        g_ca_mutex = (void *)STB_OSCreateMutex();

        ret = Aml_MP_CAS_RegisterEventCallback(NULL, cas_event_cb, NULL);

        if (ret)
        {
            CA_DBG("CAS RegisterEventCallback failed [%d]", ret);
        }

        char castype[32] = { 0 };
        STB_Get_Prop("vendor.cas.type", castype, 32);
        if (!strncmp(castype, "nagra", 5)) {
            g_cas_type = CAS_TYPE_NAGRA;
        }
        CA_DBG("am cas init g_cas_type=%d", g_cas_type);

    }

    FUNCTION_FINISH(STB_CAInitialise);
#endif
    return(TRUE);
}

/*!**************************************************************************
 * @brief   This function can get from other module, to judge cas type
 * @return  cas type E_CAS_TYPE
 ****************************************************************************/
E_CAS_TYPE STB_CAGetCASType()
{
    char castype[32] = { 0 };
    STB_Get_Prop("vendor.cas.type", castype, 32);
    CA_DBG("STB_CAGetCASType g_cas_type=%d, castype=%s", g_cas_type, castype);
    if (!strncmp(castype, "nagra", 5)) {
        g_cas_type = CAS_TYPE_NAGRA;
    }
    CA_DBG("STB_CAGetCASType exit g_cas_type=%d", g_cas_type);
    return g_cas_type;
}

/*!**************************************************************************
 * @brief   This function can get from other module, to judge under M2M
 *          or not
 * @return  true under M2M, false not M2M
 ****************************************************************************/
BOOLEAN STB_CAIsM2M()
{
    return is_m2m;
}

/*!**************************************************************************
 * @brief   This function is used by the resource manager to acquire a CA descrambler
 *          that's able to descramble a service that uses one of the CA systems
 *          defined by the array of CA system IDs (ca_ids). If a descrambler is
 *          available then a handle should be returned in 'handle' which will be
 *          used in all future calls related to this descrambler.
 *          If the CA software needs to set the demux descrambling keys, or create
 *          any filters to monitor SI data, the given demux handle should be used.
 * @param   path - the ID of the decode path to use if a descrambler is acquired
 * @param   serv_id - ID of the service the descrambler is being acquired for
 * @param   ca_ids - array of CA system IDs for the service
 * @param   num_ca_ids - number of CA system IDs in the array
 * @param   handle - pointer to return a handle to identify the acquired CA descrambler
 * @return  TRUE if a descrambler is acquired, FALSE otherwise
 ****************************************************************************/
BOOLEAN STB_CAAcquireDescrambler(U8BIT path, U16BIT serv_id, U16BIT *ca_ids, U16BIT num_ca_ids,
                                 UINTPTR *handle)
{
#ifdef SUPPORT_CAS
    U16BIT i;

    FUNCTION_START(STB_CAAcquireDescrambler);
    ASSERT(handle);

    CA_DBG("%s(path=%u, serv_id=%u, ca_ids=%p, num_ca_ids=%u)",
            __FUNCTION__, path, serv_id, ca_ids, num_ca_ids);

    if (num_ca_ids == 0)
    {
        CA_DBG("Free channel, no need descrambler");
        return FALSE;
    }

    STB_OSMutexLock(g_ca_mutex);

    for (i = 0; i < num_ca_ids; i++)
    {
        if (Aml_MP_CAS_IsSystemIdSupported(ca_ids[i]))
        {
            CA_DBG("Found supported CA Id[%#x]", ca_ids[i]);
            break;
        }
    }

    if (i >= num_ca_ids)
    {
        CA_DBG("Not found supported CA Id");
        STB_OSMutexUnlock(g_ca_mutex);
        return FALSE;
    }

    *handle = (UINTPTR)STB_MEMGetSysRAM(sizeof(STB_CA_Glue_t));
    ASSERT(*handle);
    memset((void *)*handle, 0x0, sizeof(STB_CA_Glue_t));
    ((STB_CA_Glue_t *)(*handle))->path = path;
    ((STB_CA_Glue_t *)(*handle))->service_id = serv_id;

    CA_DBG("%s handle[%#x]", __FUNCTION__, *handle);
    FUNCTION_FINISH(STB_CAAcquireDescrambler);

    STB_OSMutexUnlock(g_ca_mutex);
    return(TRUE);
#else
    return(FALSE);
#endif
}

/*!**************************************************************************
 * @brief   Will be called when a CA descrambler is no longer required.
 * @param   handle - CA descrambler handle being released
 * @return  TRUE if the descrambler is released, FALSE otherwise
 ****************************************************************************/
BOOLEAN STB_CAReleaseDescrambler(UINTPTR handle)
{
#ifdef SUPPORT_CAS
    FUNCTION_START(STB_CAReleaseDescrambler);

    ASSERT(handle);

    CA_DBG("%s(0x%lx)", __FUNCTION__, handle);

    STB_OSMutexLock(g_ca_mutex);
    CA_DBG(("%s->CAS_HAL_LOCK", __func__));
    if (((STB_CA_Glue_t *)handle)->session_info &&
        ((STB_CA_Glue_t *)handle)->session_info->cas_session)
    {
        if (((STB_CA_Glue_t *)handle)->is_descrambling)
        {
            Aml_MP_CAS_StopDescrambling(((STB_CA_Glue_t *)handle)->session_info->cas_session);
            Aml_MP_CAS_CloseSession(((STB_CA_Glue_t *)handle)->session_info->cas_session);
            ((STB_CA_Glue_t *)handle)->is_descrambling = FALSE;
        }
        else
        {
            Aml_MP_CAS_StopDVRRecord(((STB_CA_Glue_t *)handle)->session_info->cas_session);
            Aml_MP_CAS_CloseSession(((STB_CA_Glue_t *)handle)->session_info->cas_session);
        }
        ((STB_CA_Glue_t *)handle)->session_info->cas_session = NULL;
        CA_DBG(("CA glue close cas session"));
    }

    if (((STB_CA_Glue_t *)handle)->session_info)
    {
        free_sess_list(handle);
    }

    if (((STB_CA_Glue_t *)handle)->pmt_info.ca_pid_list)
    {
        free_pid_list(handle);
    }

    STB_MEMFreeSysRAM((void *)handle);

    FUNCTION_FINISH(STB_CAReleaseDescrambler);

    STB_OSMutexUnlock(g_ca_mutex);
    return(TRUE);
#else
    return(FALSE);
#endif
}

/*!**************************************************************************
 * @brief   This function will be called when decoding of a service is about to start
 *          and there's an associated descrambler.
 * @param   handle - CA descrambler handle
 ****************************************************************************/
void STB_CADescrambleServiceStart(UINTPTR handle)
{
#ifdef SUPPORT_CAS
    U32BIT ret;
    CA_INFO *pid_entry;
    AML_MP_CASSESSION cas_session = 0;
    Aml_MP_CASServiceInfo ca_serv_info;

    FUNCTION_START(STB_CADescrambleServiceStart);

    CA_DBG("%s(0x%lx)", __FUNCTION__, handle);

    ASSERT(handle);

    STB_OSMutexLock(g_ca_mutex);

    //only support descrambling of global ca descriptor for now.
    //For component level descramble stream, irdeto cas plugin will process
    //cat and pmt by itself, don't need dtvkit to do extra things.
    //it is not suitable for other CAS.
    if (((STB_CA_Glue_t *)handle)->pmt_info.has_global_ca == FALSE &&
        ((STB_CA_Glue_t *)handle)->pmt_info.has_component_ca == FALSE)
    {
        CA_DBG("%s Warning: DO*NOT have ca descriptor", __FUNCTION__);
        STB_OSMutexUnlock(g_ca_mutex);
        return;
    }

    if (((STB_CA_Glue_t *)handle)->is_descrambling == TRUE)
    {
        CA_DBG("%s CA glue service has started.", __FUNCTION__);
        STB_OSMutexUnlock(g_ca_mutex);
        return;
    }

    ret = Aml_MP_CAS_OpenSession(&cas_session, AML_MP_CAS_SERVICE_LIVE_PLAY);

    if (ret)
    {
        CA_DBG("AM_CA_OpenSession failed [%d]", ret);
        STB_OSMutexUnlock(g_ca_mutex);
        return;
    }

    ret = Aml_MP_CAS_RegisterEventCallback(cas_session, cas_event_cb, NULL);

    if (ret)
    {
        CA_DBG("CAS RegisterEventCallback failed [%d]", ret);
        Aml_MP_CAS_CloseSession(cas_session);
        STB_OSMutexUnlock(g_ca_mutex);
        return;
    }

    memset(&ca_serv_info, 0, sizeof(Aml_MP_CASServiceInfo));
    ca_serv_info.service_id = ((STB_CA_Glue_t *)handle)->service_id;

    //need use the real device num on dual tuner lib.
    ca_serv_info.dmx_dev = _GetPathDemux(((STB_CA_Glue_t *)handle)->path);
    //ca_serv_info.fend_dev = STB_DPGetPathTuner(((STB_CA_Glue_t *)handle)->path);
    ca_serv_info.serviceMode = AML_MP_CAS_SERVICE_DVB;
    ca_serv_info.serviceType = AML_MP_CAS_SERVICE_LIVE_PLAY;
    if (((STB_CA_Glue_t *)handle)->session_info)
    {
        ca_serv_info.ecm_pid = ((STB_CA_Glue_t *)handle)->session_info->ecm_pid;
    }

    pid_entry = ((STB_CA_Glue_t *)handle)->pmt_info.ca_pid_list;

    /* pass scramble algorithm to cas hal */
    ca_serv_info.ca_private_data_len = MAX_DATA_LEN;
    ca_serv_info.ca_private_data[2] =  ((STB_CA_Glue_t *)handle)->pmt_info.scramble_algo;
    CA_DBG("%s algo ca_private_data[2]=%x", __func__, ca_serv_info.ca_private_data[2]);

    while (pid_entry != NULL)
    {
        //if (pid_on_decoding(((STB_CA_Glue_t *)handle)->path, pid_entry->es_pid))
        //{
        pid_entry->running = TRUE;
        ca_serv_info.stream_pids[ca_serv_info.stream_num++] = pid_entry->es_pid;
        CA_DBG("Descrambling es pid [%#x]", pid_entry->es_pid);
        //}
        pid_entry = pid_entry->next;
    }

    if (ca_serv_info.stream_num <= 0)
    {
        CA_DBG("%s Not found scrambled es", __FUNCTION__);
        Aml_MP_CAS_CloseSession(cas_session);
        STB_OSMutexUnlock(g_ca_mutex);
        return;
    }

    ret = Aml_MP_CAS_StartDescrambling(cas_session, &ca_serv_info);

    if (ret)
    {
        CA_DBG("CAS start descrambling failed. handle[%#x], ret = %d\r\n", handle, ret);
        Aml_MP_CAS_CloseSession(cas_session);
        STB_OSMutexUnlock(g_ca_mutex);
        return;
    }

    if (((STB_CA_Glue_t *)handle)->session_info)
    {
        ((STB_CA_Glue_t *)handle)->session_info->cas_session = cas_session;
    }

    ((STB_CA_Glue_t *)handle)->is_descrambling = TRUE;

    FUNCTION_FINISH(STB_CADescrambleServiceStart);
    STB_OSMutexUnlock(g_ca_mutex);

    CA_DBG("%s cas_session: %p", __FUNCTION__, cas_session);
#endif
}

/*!**************************************************************************
 * @brief   This function will be called when decoding of a service is stopped
 * @param   handle - CA descrambler handle
 ****************************************************************************/
void STB_CADescrambleServiceStop(UINTPTR handle)
{
#ifdef SUPPORT_CAS
    FUNCTION_START(STB_CADescrambleServiceStop);

    ASSERT(handle);

    CA_DBG("%s(0x%lx)", __FUNCTION__, handle);

    STB_OSMutexLock(g_ca_mutex);

    if (((STB_CA_Glue_t *)handle)->is_descrambling == FALSE)
    {
        CA_DBG("CA glue service not started.");
        STB_OSMutexUnlock(g_ca_mutex);
        return;
    }

    if (Aml_MP_CAS_StopDescrambling(((STB_CA_Glue_t *)handle)->session_info->cas_session))
    {
        CA_DBG("CA stop descrambling failed.");
        STB_OSMutexUnlock(g_ca_mutex);
        return;
    }

    if (Aml_MP_CAS_CloseSession(((STB_CA_Glue_t *)handle)->session_info->cas_session))
    {
        CA_DBG("CA close session failed.");
        STB_OSMutexUnlock(g_ca_mutex);
        return;
    }

    //free_sess_list(handle);

    ((STB_CA_Glue_t *)handle)->session_info->cas_session = NULL;
    ((STB_CA_Glue_t *)handle)->is_descrambling = FALSE;

    FUNCTION_FINISH(STB_CADescrambleServiceStop);

    STB_OSMutexUnlock(g_ca_mutex);
#endif
}

/*!**************************************************************************
 * @brief   This function will be called when set CA descramble ioctl
 * @param   handle - CA descrambler handle
 ****************************************************************************/
void STB_CADescrambleIoctl(UINTPTR handle, U32BIT session,  const char* inJson, char* outJson, U32BIT outLen)
{
#ifdef SUPPORT_CAS
    FUNCTION_START(STB_CADescrambleIoctl);

    ASSERT(handle);
    STB_OSMutexLock(g_ca_mutex);

    if (0 != session)
    {
        if (Aml_MP_CAS_Ioctl((AML_MP_CASSESSION)(session), inJson, outJson, outLen))
        {
            CA_DBG("CA  descrambling Ioctl failed.");
        }
    }
    else if (0 != handle)
    {
        if (Aml_MP_CAS_Ioctl(((STB_CA_Glue_t *)handle)->session_info->cas_session, inJson, outJson, outLen))
        {
            CA_DBG("CA  descrambling Ioctl failed.");
        }
    }
    else
    {
        Aml_MP_CAS_Ioctl(NULL, inJson, outJson, outLen);
    }
    CA_DBG("%shandle : (0x%lx)  session (0x%lx)  [inJson: %s] [outJson: %s] [outLen: %d]", __FUNCTION__, handle, session , inJson, outJson, outLen);
    FUNCTION_FINISH(STB_CADescrambleIoctl);

    STB_OSMutexUnlock(g_ca_mutex);
#endif
}

BOOLEAN STB_CACheckSessionStatus(UINTPTR handle, UINTPTR session)
{
    ASSERT(handle);
    BOOLEAN ret = FALSE;
    STB_OSMutexLock(g_ca_mutex);
    if ((0 != handle) && (NULL != ((STB_CA_Glue_t *)handle)->session_info))
    {
        if ((UINTPTR)(((STB_CA_Glue_t *)handle)->session_info->cas_session) == session)
            ret = TRUE;
        else
            ret = FALSE;
    }
    else
        ret = FALSE;
    STB_OSMutexUnlock(g_ca_mutex);
    return ret;
}

static U8BIT* _STB_ParseCaDescriptor(U8BIT *dptr, U16BIT *num_ptr, CA_DESC **array_ptr,
                                BOOLEAN db_print)
{
    U8BIT dlen;
    U8BIT *end_ptr;
    U16BIT num_entries;
    CA_DESC *array;
    U16BIT ca_id;
    U16BIT ca_pid;

    FUNCTION_START(_STB_ParseCaDescriptor);

    ASSERT(dptr != NULL);
    ASSERT(num_ptr != NULL);
    ASSERT(array_ptr != NULL);

    dlen = *dptr;
    dptr++;
    end_ptr = dptr + dlen;

    if (dlen >= 4)
    {
        ca_id = (dptr[0] << 8) | dptr[1];
        ca_pid = ((dptr[2] & 0x1f) << 8) | dptr[3];
#ifdef DEBUG_CA_DESC

        if (db_print == TRUE)
        {
            CA_DBG("   CA desc: (%d bytes) id=0x%04x, pid=0x%04x", dlen, ca_id, ca_pid);
        }

#else
        USE_UNWANTED_PARAM(db_print);
#endif

        // check if there are already entries in the array (i.e. already received a descriptor)
        // if so add to the existing array, otherwise create new array
        if (*array_ptr == NULL)
        {
            // no entries already - create new array
            num_entries = 1;
            array = (CA_DESC *)STB_MEMGetSysRAM(sizeof(CA_DESC));
            // memset(array, 0, sizeof(CA_DESC));
        }
        else
        {
            // already got entries - make array bigger
            num_entries = *num_ptr + 1;
            array = (CA_DESC *)STB_MEMGetSysRAM(num_entries * sizeof(CA_DESC));
            // memset(array, 0, sizeof(num_entries * sizeof(CA_DESC)));

            if (array != NULL)
            {
                // copy over previous entries and free old array
                memcpy(array, *array_ptr, (*num_ptr * sizeof(CA_DESC)));
                STB_MEMFreeSysRAM(*array_ptr);
            }
        }

        // add new entry to array
        if (array != NULL)
        {
            array[num_entries - 1].ca_id = ca_id;
            array[num_entries - 1].ca_pid = ca_pid;
            dlen -= 4;
            dptr += 4;
            if (dlen != 0)
            {
                /* The rest is private data */
                array[num_entries - 1].private_data = STB_MEMGetSysRAM(dlen);

                if (array[num_entries - 1].private_data != NULL)
                {
                    array[num_entries - 1].private_data_length = dlen;
                    memcpy(array[num_entries - 1].private_data, dptr, dlen);
                    dlen = 0;
                }
            }
            else
            {
                array[num_entries - 1].private_data = NULL;
                array[num_entries - 1].private_data_length = 0;
            }
            *array_ptr = array;
            *num_ptr = num_entries;
        }
        else
        {
#ifdef DEBUG_CA_DESC

            if (db_print == TRUE)
            {
                CA_DBG("   CAN'T ALLOCATE MEMORY FOR DESCRIPTOR ARRAY ENTRY");
            }

#endif
        }
    }
    else
    {
#ifdef DEBUG_CA_DESC

        if (db_print == TRUE)
        {
            CA_DBG("   Invalid CA desc: (%d bytes)", dlen);
        }

#endif
    }

    FUNCTION_FINISH(_STB_ParseCaDescriptor);
    return(end_ptr);
}

static CA_LIST * _STB_CAGetPmtDescArrayList(U8BIT *pmt_data)
{
    U8BIT *data_ptr;
    U16BIT sec_len;
    U8BIT *data_end;
    U16BIT dloop_len;
    U8BIT *dloop_end;
    U8BIT dtag;
    U16BIT i, num_ca_entries;
    U16BIT stream_entry_pid;
    CA_DESC *ca_desc_array;
    STREAM_ENTRY *stream_entry;
    CA_LIST * ca_list = NULL;
    U8BIT scramble_algo = 0;
    U16BIT program_number;
    FUNCTION_START(_STB_CAGetPmtDescArrayList);

    if (pmt_data == NULL)
    {
        return NULL;
    }

    ca_list = (CA_LIST *)STB_MEMGetSysRAM(sizeof(CA_LIST));
    memset(ca_list, 0, sizeof(CA_LIST));

    /* Get pointer to section data and end of section */
    data_ptr = pmt_data;
    sec_len = (((data_ptr[1] & 0x0f) << 8) | data_ptr[2]) + 3;
    data_end = data_ptr + sec_len - 4;   // -4 for crc

    /* Skip section header */
    data_ptr += 8;

    /* Get descriptor loop length */
    dloop_len = ((data_ptr[2] & 0x0f) << 8) | data_ptr[3];
    data_ptr += 4;
    program_number = pmt_data[3] << 8 | pmt_data[4];

    num_ca_entries = 0;
    ca_desc_array = NULL;

    /* Process first descriptor loop */
    dloop_end = data_ptr + dloop_len;

    while (data_ptr < dloop_end)
    {
        dtag = data_ptr[0];
        data_ptr++;

        switch (dtag)
        {
            case CA_DTAG:
            {
                data_ptr = _STB_ParseCaDescriptor(data_ptr, &num_ca_entries, &ca_desc_array, FALSE);
                break;
            }
            case SCRAMBLING_DTAG:
            {
                /* record scramble algorithm */
                scramble_algo = data_ptr[1];
                data_ptr += (*data_ptr + 1);
                break;
            }
            default:
            {
                /* Skip the descriptor */
                data_ptr += (*data_ptr + 1);
                break;
            }
        }
    }

    if (num_ca_entries > 0)
    {
        ca_list->num_ca_entries = num_ca_entries;
        ca_list->ca_desc_array = ca_desc_array;
    }
    ca_list->serv_id = program_number;
    ca_list->scramble_algo = scramble_algo;
    /* Read entry for each stream */
    while (data_ptr < data_end)
    {
        dloop_len = ((data_ptr[3] & 0x0f) << 8) | data_ptr[4];
        stream_entry_pid = ((data_ptr[1] & 0x1f) << 8) | data_ptr[2];
        data_ptr += 5;

        num_ca_entries = 0;
        ca_desc_array = NULL;
        /* Process stream descriptor loop */
        dloop_end = data_ptr + dloop_len;

        while ((data_ptr < dloop_end) && (data_ptr < data_end))
        {
            dtag = data_ptr[0];
            data_ptr++;

            switch (dtag)
            {
                case CA_DTAG:
                {
                    data_ptr = _STB_ParseCaDescriptor(data_ptr, &num_ca_entries, &ca_desc_array, FALSE);
                    break;
                }

                default:
                {
                    /* Skip the descriptor */
                    data_ptr += (*data_ptr + 1);
                    break;
                }
            }
        }

        {
            stream_entry = (STREAM_ENTRY *)STB_MEMGetSysRAM(sizeof(STREAM_ENTRY));

            if (stream_entry != NULL)
            {
                // initialise new stream structure
                memset(stream_entry, 0, sizeof(STREAM_ENTRY));

                // add to the end of the stream list in the pmt table
                if (ca_list->last_stream_entry == NULL)
                {
                    // first entry in the list
                    ca_list->stream_list = stream_entry;
                }
                else
                {
                    // not the first entry
                    ca_list->last_stream_entry->next = stream_entry;
                }

                ca_list->last_stream_entry = stream_entry;
                ca_list->num_streams++;
                stream_entry->num_ca_entries = num_ca_entries;
                stream_entry->ca_desc_array = ca_desc_array;
                stream_entry->pid = stream_entry_pid;
            }
        }
    }

    if (ca_list != NULL)
    {
        for (i = 0; i < ca_list->num_ca_entries; i++)
        {
            CA_DBG("CAS_  ca_list->ca_desc_array[%d].ca_id 0x%x", i, ca_list->ca_desc_array[i].ca_id);
        }

        STREAM_ENTRY *stream_list = ca_list->stream_list;
        U16BIT num = 0;
        while (stream_list != NULL)
        {
            CA_DBG("CAS_ ES ID 0x%x", stream_list->pid);
            for (i = 0; i < stream_list->num_ca_entries; i++)
            {
                CA_DBG("CAS_  %d   ca_list->ca_desc_array[%d].ca_id 0x%x ",
                        num, i, stream_list->ca_desc_array[i].ca_id);
            }
            num++;
            stream_list = stream_list->next;
        }
    }

    FUNCTION_FINISH(_STB_CAGetPmtDescArrayList);
    return ca_list;
}
static void  _STB_CAFreeDescArrayList(CA_LIST * ca_list)
{
    U16BIT i, num_ca_entries;
    STREAM_ENTRY *stream_entry = NULL;
    STREAM_ENTRY *next_entry = NULL;
    if (ca_list == NULL)
    {
        return;
    }
//first loop ca descriptor free
    num_ca_entries = ca_list->num_ca_entries;
    for (i= 0 ; i < num_ca_entries ; i++)
    {
        STB_MEMFreeSysRAM(ca_list->ca_desc_array[i].private_data);
    }
    STB_MEMFreeSysRAM(ca_list->ca_desc_array);
//second loop ca descriptor free
    stream_entry = ca_list->stream_list ;
    while (stream_entry != NULL)
    {
        next_entry = stream_entry->next;
        //ca
        num_ca_entries= stream_entry->num_ca_entries;
        for (i= 0 ; i < num_ca_entries ; i++)
        {
            STB_MEMFreeSysRAM(stream_entry->ca_desc_array[i].private_data);
        }
        STB_MEMFreeSysRAM(stream_entry->ca_desc_array)  ;
        //last
        STB_MEMFreeSysRAM(stream_entry)    ;
        stream_entry = next_entry;
    }
    STB_MEMFreeSysRAM(ca_list)    ;
}

/*!**************************************************************************
 * @brief   When there's an update to the PMT for a service, the updated PMT
 *          will be reported to the CA system using this function.
 * @param   handle - CA descrambler handle
 * @param   pmt_data - raw PMT section data
 * @param   data_len - number of bytes in the PMT
 ****************************************************************************/
    static void _STB_CollectESCaInfo(UINTPTR handle, PMT_INFO *pmt_info, CA_LIST * ca_list, U16BIT global_ecm_pid)
    {
        STREAM_ENTRY *stream_entry;
        CA_INFO *ca_pid_info;
        int i;

        stream_entry = ca_list->stream_list;

        while (stream_entry != NULL)
        {
            ca_pid_info = (CA_INFO *)STB_MEMGetSysRAM(sizeof(CA_INFO));
            memset(ca_pid_info, 0, sizeof(CA_INFO));

            if (pmt_info->has_global_ca)
            {
                ca_pid_info->ecm_pid = global_ecm_pid;
                ca_pid_info->es_pid = stream_entry->pid;

                if (pmt_info->last_pid_entry == NULL)
                {
                    pmt_info->ca_pid_list = ca_pid_info;
                }
                else
                {
                    pmt_info->last_pid_entry->next = ca_pid_info;
                }

                pmt_info->last_pid_entry = ca_pid_info;
                stream_entry = stream_entry->next;
                continue;
            }

            for (i = 0; i < stream_entry->num_ca_entries; i++)
            {
                if (Aml_MP_CAS_IsSystemIdSupported(stream_entry->ca_desc_array[i].ca_id))
                {
                    ca_pid_info->es_pid = stream_entry->pid;
                    ca_pid_info->ecm_pid = stream_entry->ca_desc_array[i].ca_pid;
                    add_sess_pid(handle, stream_entry->ca_desc_array[i].ca_pid);

                    if (pmt_info->last_pid_entry == NULL)
                    {
                        pmt_info->ca_pid_list = ca_pid_info;
                    }
                    else
                    {
                        pmt_info->last_pid_entry->next = ca_pid_info;
                    }

                    pmt_info->last_pid_entry = ca_pid_info;

                    pmt_info->has_component_ca = TRUE;
                    CA_DBG("%s supported component CA desc found", __FUNCTION__);
                    break;
                }
            }

            stream_entry = stream_entry->next;
        }
        /* record the scramble algorithm */
        pmt_info->scramble_algo = ca_list->scramble_algo;
    }

/*!**************************************************************************
 * @brief   When there's an update to the PMT for a service, the updated PMT
 *          will be reported to the CA system using this function.
 * @param   handle - CA descrambler handle
 * @param   pmt_data - raw PMT section data
 * @param   data_len - number of bytes in the PMT
 ****************************************************************************/
void STB_CAReportPMT(UINTPTR handle, U8BIT *pmt_data, U16BIT data_len)
{
    U16BIT i;
    PMT_INFO pmt_info;
    CA_LIST *ca_list =NULL;
    FUNCTION_START(STB_CAReportPMT);
    U16BIT global_ecm_pid = 0x1fff;

    CA_DBG("%s(handle=0x%lx, pmt_data=%p, data_len=%u)", __FUNCTION__, handle, pmt_data, data_len);

    /*if PMT update when playback, we need re-start descrambling with
    **the new es pid.but now we try to handle that in Video/Audio decoding
    **status notify function. And not support ecm pid update for now.
    */

    ASSERT(handle);
    if(NULL == pmt_data||data_len<=3)
    {
        return;
    }
    STB_OSMutexLock(g_ca_mutex);

    if (Aml_MP_CAS_IsNeedWholeSection())
    {
        Aml_MP_CASSectionReportAttr attr;

        memset(&attr, 0, sizeof(attr));
        attr.dmxDev = (Aml_MP_DemuxId)_GetPathDemux(((STB_CA_Glue_t *)handle)->path);
        attr.serviceId = ((STB_CA_Glue_t *)handle)->service_id;
        attr.sectionType = AML_MP_CAS_SECTION_PMT;
        Aml_MP_CAS_ReportSection(&attr, pmt_data, data_len);
        //STB_OSMutexUnlock(g_ca_mutex);
        //return;
    }

    ca_list = _STB_CAGetPmtDescArrayList(pmt_data);
    if (NULL != ca_list)
    {
        CA_DBG("%s svc_id[%#x], num_ca_entries[%d], num_streams[%d]",
                __FUNCTION__, ca_list->serv_id, ca_list->num_ca_entries, ca_list->num_streams);

        memset(&pmt_info, 0, sizeof(PMT_INFO));

        for (i = 0; i < ca_list->num_ca_entries; i++)
        {
            if (Aml_MP_CAS_IsSystemIdSupported(ca_list->ca_desc_array[i].ca_id))
            {
                pmt_info.has_global_ca = TRUE;
                global_ecm_pid = ca_list->ca_desc_array[i].ca_pid;
                add_sess_pid(handle, global_ecm_pid);
                break;
            }
        }

        if (i >= ca_list->num_ca_entries)
        {
            CA_DBG("%s not found supported global CA desc", __FUNCTION__);
        }

        if (((STB_CA_Glue_t *)handle)->pmt_info.ca_pid_list)
        {
            CA_DBG("%s free previous pmt pid list", __FUNCTION__);
            free_pid_list(handle);
        }
        _STB_CollectESCaInfo(handle, &pmt_info, ca_list, global_ecm_pid);
        memcpy(&(((STB_CA_Glue_t *)handle)->pmt_info), &pmt_info, sizeof(PMT_INFO));
        ((STB_CA_Glue_t *)handle)->service_id = ca_list->serv_id;
        _STB_CAFreeDescArrayList(ca_list);
   }

    FUNCTION_FINISH(STB_CAReportPMT);

    STB_OSMutexUnlock(g_ca_mutex);
}

/*!**************************************************************************
 * @brief   When there's an update to the CAT for a service, the updated CAT
 *          will be reported to the CA system using this function. The data is
 *          provided a section at a time, rather than as a complete table.
 * @param   path - the decoder path
 * @param   cat_data - raw CAT section data
 * @param   data_len - number of bytes in the CAT section
 ****************************************************************************/
void STB_CAReportCAT(U8BIT path, U8BIT *cat_data, U16BIT data_len)
{
#ifdef SUPPORT_CAS
    int dmx_dev;
    U16BIT ca_pid;
    FUNCTION_START(STB_CAReportCAT);

    CA_DBG("%s(handle=0x%lx, cat_data=%p, data_len=%u)", __FUNCTION__, path, cat_data, data_len);

    STB_OSMutexLock(g_ca_mutex);

    if (Aml_MP_CAS_IsNeedWholeSection())
    {
        Aml_MP_CASSectionReportAttr attr;

        memset(&attr, 0, sizeof(attr));
        attr.dmxDev = (Aml_MP_DemuxId)_GetPathDemux(path);
        attr.sectionType = AML_MP_CAS_SECTION_CAT;
        Aml_MP_CAS_ReportSection(&attr, cat_data, data_len);
        //STB_OSMutexUnlock(g_ca_mutex);
        //return;
    }

    ca_pid = parser_cat_table(cat_data);

    if (ca_pid == DVB_INVALID_ID)
    {
        CA_DBG("%s no match ca pid", __FUNCTION__);
        STB_OSMutexUnlock(g_ca_mutex);
        return;
    }

    dmx_dev = _GetPathDemux(path);
    Aml_MP_CAS_SetEmmPid(dmx_dev, ca_pid);

    FUNCTION_FINISH(STB_CAReportCAT);

    STB_OSMutexUnlock(g_ca_mutex);
#endif
}

/*!**************************************************************************
 * @brief   When there's an update to the BAT, the updated BAT will be reported
 *          to the CA system using this function. The data is provided a section
 *          at a time, rather than as a complete table.
 * @param   handle - CA descrambler handle
 * @param   bat_data - raw BAT section data
 * @param   data_len - number of bytes in the BAT section
 ****************************************************************************/
void STB_CAReportBAT(UINTPTR handle, U8BIT *bat_data, U16BIT data_len)
{
    FUNCTION_START(STB_CAReportBAT);

    CA_DBG("%s(handle=0x%lx, bat_data=%p, data_len=%u)", __FUNCTION__, handle, bat_data, data_len);

    USE_UNWANTED_PARAM(handle);
    USE_UNWANTED_PARAM(bat_data);
    USE_UNWANTED_PARAM(data_len);

    FUNCTION_FINISH(STB_CAReportBAT);
}

/*!**************************************************************************
 * @brief   When there's an update to the NIT, the updated NIT will be reported
 *          to the CA system using this function. The data is provided a section
 *          at a time, rather than as a complete table.
 * @param   path - the decoder path
 * @param   nit_data - raw NIT section data
 * @param   data_len - number of bytes in the NIT section
 ****************************************************************************/
void STB_CAReportNIT(U8BIT path, U8BIT *nit_data, U16BIT data_len)
{
#ifdef SUPPORT_CAS
    int dmx_dev;
    U16BIT ca_pid;
    FUNCTION_START(STB_CAReportNIT);

    CA_DBG("%s(path=0x%lx, nit_data=%p, data_len=%u)", __FUNCTION__, path, nit_data, data_len);

    STB_OSMutexLock(g_ca_mutex);

    if (Aml_MP_CAS_IsNeedWholeSection())
    {
        Aml_MP_CASSectionReportAttr attr;

        memset(&attr, 0, sizeof(attr));
        attr.dmxDev = (Aml_MP_DemuxId)_GetPathDemux(path);
        attr.sectionType = AML_MP_CAS_SECTION_NIT;
        Aml_MP_CAS_ReportSection(&attr, nit_data, data_len);
    }

    FUNCTION_FINISH(STB_CAReportNIT);

    STB_OSMutexUnlock(g_ca_mutex);
#endif
}

/*!**************************************************************************
 * @brief   Notifies the CA system of a change in the video decoding state
 * @param   handle - CA descrambler handle
 * @param   decode_status - decoding status
 ****************************************************************************/
void STB_CADecodeVideoStatus(U32BIT handle, E_CA_DECODE_STATUS decode_status)
{
#ifdef SUPPORT_CAS
    FUNCTION_START(STB_CADecodeVideoStatus);

    CA_DBG("%s(handle=0x%lx, status=%u)", __FUNCTION__, handle, decode_status);

    if (decode_status != CA_DECODE_STATUS_STARTED)
    {
        return;
    }

    update_desc_pid(handle);

    FUNCTION_FINISH(STB_CADecodeVideoStatus);
#endif
}

/*!**************************************************************************
 * @brief   Notifies the CA system of a change in the audio decoding state
 * @param   handle - CA descrambler handle
 * @param   decode_status - decoding status
 ****************************************************************************/
void STB_CADecodeAudioStatus(U32BIT handle, E_CA_DECODE_STATUS decode_status)
{
#ifdef SUPPORT_CAS
    FUNCTION_START(STB_CADecodeAudioStatus);

    CA_DBG("%s(handle=0x%lx, status=%u)", __FUNCTION__, handle, decode_status);

    if (decode_status != CA_DECODE_STATUS_STARTED)
    {
        return;
    }

    update_desc_pid(handle);

    FUNCTION_FINISH(STB_CADecodeAudioStatus);
#endif
}

/*!**************************************************************************
 * @brief   Notifies the CA system of a change in the AD decoding state
 * @param   handle - CA descrambler handle
 * @param   decode_status - decoding status
 ****************************************************************************/
void STB_CADecodeADStatus(U32BIT handle, E_CA_DECODE_STATUS decode_status)
{
#ifdef SUPPORT_CAS
    FUNCTION_START(STB_CADecodeADStatus);

    CA_DBG("%s(handle=0x%lx, status=%u)", __FUNCTION__, handle, decode_status);

    if (decode_status != CA_DECODE_STATUS_STARTED)
    {
        return;
    }

    update_desc_pid(handle);

    FUNCTION_FINISH(STB_CADecodeADStatus);
#endif
}

/*!**************************************************************************
 * @brief   This function will be called when there's a change to the running status
 *          of a service being descrambled as indicated by the running_status field
 *          in the SDT.
 * @param   handle - CA descrambler handle
 * @param   status - running status as defined in the SDT
 ****************************************************************************/
void STB_CANotifyRunningStatus(UINTPTR handle, U8BIT status)
{
    FUNCTION_START(STB_CANotifyRunningStatus);

    CA_DBG("%s(handle=0x%lx, status=%u)", __FUNCTION__, handle, status);

    USE_UNWANTED_PARAM(handle);
    USE_UNWANTED_PARAM(status);

    FUNCTION_FINISH(STB_CANotifyRunningStatus);
}

/*!**************************************************************************
 * @brief   This function specifies whether a CA descrambler is required
 *          a recording with one of the given CA system IDs.
 * @param   ca_ids - array of CA system IDs
 * @param   num_ca_ids - number of CA system IDs in the array
 * @return  TRUE if a CA descrambler is required, FALSE otherwise
 ****************************************************************************/
BOOLEAN STB_CADescramblerRequired(U16BIT *ca_ids, U16BIT num_ca_ids)
{
#ifdef SUPPORT_CAS
    U16BIT i;
    BOOLEAN ret = FALSE;

    FUNCTION_START(STB_CADescramblerRequired);

    CA_DBG("%s(ca_ids=%p, num_ca_ids=%u)", __FUNCTION__, ca_ids, num_ca_ids);
    for (i = 0; i < num_ca_ids; i++)
    {
        if (Aml_MP_CAS_IsSystemIdSupported(ca_ids[i]))
        {
            ret = TRUE;
            break;
        }
    }

    FUNCTION_FINISH(STB_CADescramblerRequired);

    return(ret);
#else
    return(FALSE);
#endif

}

/*!**************************************************************************
 * @brief   This function specifies whether a CA descrambler is required to playback
 *          a recording with one of the given CA system IDs.
 * @param   ca_ids - array of CA system IDs
 * @param   num_ca_ids - number of CA system IDs in the array
 * @return  TRUE if a CA descrambler is required, FALSE otherwise
 ****************************************************************************/
BOOLEAN STB_CADescramblerRequiredForPlayback(U16BIT *ca_ids, U16BIT num_ca_ids)
{
    BOOLEAN ret = FALSE;

    FUNCTION_START(STB_CADescramblerRequiredForPlayback);

    CA_DBG("%s(ca_ids=%p, num_ca_ids=%u)", __FUNCTION__, ca_ids, num_ca_ids);

    /* STB_CADescramblerRequiredForPlayback always return FALSE */
//  ret = STB_CADescramblerRequired(ca_ids, num_ca_ids);

    FUNCTION_FINISH(STB_CADescramblerRequiredForPlayback);

    return ret;
}

/*!**************************************************************************
 * @brief   This function specifies whether a CA descrambler is required to record
 *          a service with one of the given CA system IDs.
 * @param   ca_ids - array of CA system IDs
 * @param   num_ca_ids - number of CA system IDs in the array
 * @return  TRUE if a CA descrambler is required, FALSE otherwise
 ****************************************************************************/
BOOLEAN STB_CADescramblerRequiredForRecording(U16BIT *ca_ids, U16BIT num_ca_ids)
{
    BOOLEAN ret = FALSE;

    FUNCTION_START(STB_CADescramblerRequiredForRecording);

    CA_DBG("%s(ca_ids=%p, num_ca_ids=%u)", __FUNCTION__, ca_ids, num_ca_ids);
    ret = STB_CADescramblerRequired(ca_ids, num_ca_ids);

    FUNCTION_FINISH(STB_CADescramblerRequiredForRecording);

    return ret;
}

/*!**************************************************************************
 * @brief   This function is called to get an array of PIDs that need to be recorded
 *          for the CA system required for the given PMT. The array must be allocated
 *          by this function, which also returns the number of items in the array.
 * @param   pmt_data - raw PMT section data
 * @param   pid_array - pointer to an array allocated by this function on return,
 *                      containing the PIDs to be recorded
 * @return  the number of PIDs in the returned array
 ****************************************************************************/
U16BIT STB_CAGetRecordingPids(U8BIT *pmt_data, U16BIT **pid_array)
{
    U16BIT num_pids;

    FUNCTION_START(STB_CAGetRecordingPids);

    num_pids = 0;

    USE_UNWANTED_PARAM(pmt_data);
    USE_UNWANTED_PARAM(pid_array);

    CA_DBG("%s(pmt_data=%p, pid_array=%p): %u", __FUNCTION__, pmt_data, pid_array, num_pids);

    FUNCTION_FINISH(STB_CAGetRecordingPids);

    return(num_pids);
}

void STB_CAPVRRecodingEncrypt(void *handle, void *param)
{
    int ret;
#ifdef SUPPORT_CAS
    AML_MP_CASSESSION cas_session = 0;

    CA_INFO *pid_entry;
    Aml_MP_CASServiceInfo ca_serv_info;
    Aml_MP_CASCryptoParams *cryptoPara= (Aml_MP_CASCryptoParams *)param;

    ASSERT(handle);
    ASSERT(cryptoPara);

#if 0
    CA_DBG("%s handle(%#x), session(%#x), (%#x, %#x, %#x)", __FUNCTION__,
            handle,
            ((STB_CA_Glue_t *)handle)->session_info->cas_session,
            cryptoPara->buf_in, cryptoPara->buf_out, cryptoPara->buf_len);
#endif

           if (!(((STB_CA_Glue_t *)handle)->session_info))
{
    CA_DBG("CA glue PVR recoding encrypt session_info is null");
        return;
    }

    if (FALSE && !(((STB_CA_Glue_t *)handle)->session_info->cas_session))
{
        if (((STB_CA_Glue_t *)handle)->is_timeshift)
            ret = Aml_MP_CAS_OpenSession(&cas_session, AML_MP_CAS_SERVICE_PVR_TIMESHIFT_RECORDING);
        else
            ret = Aml_MP_CAS_OpenSession(&cas_session, AML_MP_CAS_SERVICE_PVR_RECORDING);

        if (ret)
        {
            CA_DBG("AM_CA_OpenSession failed [%d]", ret);
            return;
        }

        ((STB_CA_Glue_t *)handle)->session_info->cas_session = cas_session;

        memset(&ca_serv_info, 0, sizeof(Aml_MP_CASServiceInfo));
        ca_serv_info.service_id = ((STB_CA_Glue_t *)handle)->service_id;

        ca_serv_info.dmx_dev = _GetPathDemux(((STB_CA_Glue_t *)handle)->path);
        //ca_serv_info.dvr_dev = _GetPathDemux(((STB_CA_Glue_t *)handle)->path);

        ca_serv_info.serviceMode = AML_MP_CAS_SERVICE_DVB;
        if (((STB_CA_Glue_t *)handle)->is_timeshift)
            ca_serv_info.serviceType = AML_MP_CAS_SERVICE_PVR_TIMESHIFT_RECORDING;
        else
            ca_serv_info.serviceType = AML_MP_CAS_SERVICE_PVR_RECORDING;
        ca_serv_info.ecm_pid = ((STB_CA_Glue_t *)handle)->pmt_info.ca_pid_list->ecm_pid;

        pid_entry = ((STB_CA_Glue_t *)handle)->pmt_info.ca_pid_list;

        while (pid_entry != NULL)
        {
            pid_entry->running = TRUE;
            ca_serv_info.stream_pids[ca_serv_info.stream_num++] = pid_entry->es_pid;
            pid_entry = pid_entry->next;
        }

        ret = Aml_MP_CAS_StartDVRRecord(
                  cas_session,
                  &ca_serv_info);

        if (ret)
        {
            CA_DBG("CAS start DVR failed. ret = %d\r\n", ret);
            return;
        }
    }

    ret = Aml_MP_CAS_DVREncrypt(
              ((STB_CA_Glue_t *)handle)->session_info->cas_session,
              cryptoPara);

          if (ret)
{
    cryptoPara->outputSize = 0;
    cryptoPara->outputBuffer.size = 0;
}
#endif
}

void STB_CAPVRPlayDecrypt(void *handle, void *param)
{
#ifdef SUPPORT_CAS
    int ret;
    Aml_MP_CASCryptoParams *cryptoPara= (Aml_MP_CASCryptoParams *)param;

    //For now, it will be zero set by STB_PVRStartPlaying
    if (handle != 0)
    {
        CA_DBG("CAS Decrypt, wrong param");
        return;
    }

    if (!g_pvrplay_replay)
    {
        ret = Aml_MP_CAS_DVRReplay(g_pvrplay_session, param);
        if (!ret)
        {
            g_pvrplay_replay = 1;
        }
        else
        {
            cryptoPara->outputBuffer.size = cryptoPara->inputBuffer.size;
            cryptoPara->outputSize = cryptoPara->inputBuffer.size;
            return;
        }
    }
    if (g_checkpin_status != PIN_MAX)
    {
        if (g_checkpin_status == PIN_NEED_CHECK)
        {
            usleep(50*1000);
            cryptoPara->outputBuffer.size = cryptoPara->inputBuffer.size;
            cryptoPara->outputSize = cryptoPara->inputBuffer.size;
            return;
        }
        else if (g_checkpin_status == PIN_CHECK_SUCCESS)
        {
            ret = Aml_MP_CAS_DVRReplay(g_pvrplay_session, param);
            CA_DBG("PIN checked. CAS start Replay. ret = %d\r\n", ret);
            g_checkpin_status = PIN_MAX;
        }
    }
    ret = Aml_MP_CAS_DVRDecrypt(g_pvrplay_session, cryptoPara);
    if (ret)
    {
        cryptoPara->outputBuffer.size = 0;
        cryptoPara->outputSize = 0;
    }

    CA_DBG("CAS DEC ####");
#endif
}


#ifdef SUPPORT_CAS
int STB_CAPVRGetPlaySection(AML_MP_CASSESSION *sec)
{
    *sec = g_pvrplay_session;
    CA_DBG("STB_CAPVRGetPlaySection [%p]", *sec);
    return 0;
}

int STB_CAPVRGetDvrSection(UINTPTR handle, AML_MP_CASSESSION *sec)
{
    if (handle && ((STB_CA_Glue_t *)handle)->session_info)
    {
        *sec = ((STB_CA_Glue_t *)handle)->session_info->cas_session;
        CA_DBG("STB_CAPVRGetDvrSection [%p]", *sec);
    }
    else
    {
        *sec = 0;
    }

    return 0;
}

void STB_CAPVRPlayStart(struct Aml_MP_CASDVRReplayParams *param, BOOLEAN isTimeShift)
{
    int ret;

    //For now, it will be zero set by STB_PVRStartPlaying
    if (!g_pvrplay_session)
    {
        if (isTimeShift)
            ret = Aml_MP_CAS_OpenSession(&g_pvrplay_session, AML_MP_CAS_SERVICE_PVR_TIMESHIFT_PLAY);
        else
            ret = Aml_MP_CAS_OpenSession(&g_pvrplay_session, AML_MP_CAS_SERVICE_PVR_PLAY);

        if (ret)
        {
            CA_DBG("CAS open session failed. ret = %d\r\n", ret);
            return;
        }

        ret = Aml_MP_CAS_RegisterEventCallback(g_pvrplay_session, cas_event_cb, NULL);
        if (ret)
        {
            CA_DBG("CAS(PVR replay) RegisterEventCallback failed [%d]", ret);
        }

        get_cas_mode(g_pvrplay_session);

        CA_DBG("PVRPlay CAS open session = %p", g_pvrplay_session);
        if (Aml_MP_CAS_StartDVRReplay(g_pvrplay_session, param))
        {
            CA_DBG("Start DVR Replay failed\n");
        }
    }
}
#endif

void STB_CAPVRPlayStop(void)
{
#ifdef SUPPORT_CAS
    CA_DBG("%s(session=%p)", __FUNCTION__, g_pvrplay_session);

    if (g_pvrplay_session)
    {
        Aml_MP_CAS_StopDVRReplay(g_pvrplay_session);
        Aml_MP_CAS_CloseSession(g_pvrplay_session);
        g_pvrplay_session = 0;
        g_last_stream_pos = -1;
        g_pvrplay_replay = 0;
        g_checkpin_status = PIN_MAX;
    }

#endif
}

/*!**************************************************************************
 * @brief   Called to free the array of PIDs allocated by STB_CAGetRecordingPids.
 * @param   pid_array - array of PIDs to be freed
 * @param   num_pids - number of PIDs in the array
 ****************************************************************************/
void STB_CAReleaseRecordingPids(U16BIT *pid_array, U16BIT num_pids)
{
    FUNCTION_START(STB_CAReleaseRecordingPids);

    CA_DBG("%s(pid_array=%p, num_pids=%u)", __FUNCTION__, pid_array, num_pids);

    USE_UNWANTED_PARAM(pid_array);
    USE_UNWANTED_PARAM(num_pids);

    FUNCTION_FINISH(STB_CAReleaseRecordingPids);
}


/*!**************************************************************************
 * @brief   This function is called when a record is stoped
 * @param   handle - CA descrambler handle
 ****************************************************************************/
int STB_CAPVRRecordStart(UINTPTR handle)
{
    int ret;
#ifdef SUPPORT_CAS
    AML_MP_CASSESSION cas_session = 0;

    CA_INFO *pid_entry;
    Aml_MP_CASServiceInfo ca_serv_info;

    ASSERT(handle);
    //ASSERT(cryptoPara);

    if (!(((STB_CA_Glue_t *)handle)->session_info))
    {
        CA_DBG("CA glue PVR recoding encrypt session_info is null");
        return -1;
    }

    if (!(((STB_CA_Glue_t *)handle)->session_info->cas_session))
    {
        if (((STB_CA_Glue_t *)handle)->is_timeshift)
            ret = Aml_MP_CAS_OpenSession(&cas_session, AML_MP_CAS_SERVICE_PVR_TIMESHIFT_RECORDING);
        else
            ret = Aml_MP_CAS_OpenSession(&cas_session, AML_MP_CAS_SERVICE_PVR_RECORDING);
        if (ret)
        {
            CA_DBG("AM_CA_OpenSession failed [%d]", ret);
            return -1;
        }

        ret = Aml_MP_CAS_RegisterEventCallback(cas_session, cas_event_cb, NULL);
        if (ret)
        {
            CA_DBG("CAS(PVR record) RegisterEventCallback failed [%d]", ret);
        }

        get_cas_mode(cas_session);

        CA_DBG("AM_CA_OpenSession rec start cas_session [%p] is_timeshift=%d", cas_session,
            ((STB_CA_Glue_t *)handle)->is_timeshift);
        ((STB_CA_Glue_t *)handle)->session_info->cas_session = cas_session;

        memset(&ca_serv_info, 0, sizeof(Aml_MP_CASServiceInfo));
        ca_serv_info.service_id = ((STB_CA_Glue_t *)handle)->service_id;

        ca_serv_info.dmx_dev = _GetPathDemux(((STB_CA_Glue_t *)handle)->path);
        //ca_serv_info.dvr_dev = _GetPathDemux(((STB_CA_Glue_t *)handle)->path);

        ca_serv_info.serviceMode = AML_MP_CAS_SERVICE_DVB;

        if (((STB_CA_Glue_t *)handle)->is_timeshift) {
            ca_serv_info.serviceType = AML_MP_CAS_SERVICE_PVR_TIMESHIFT_RECORDING;
        } else
            ca_serv_info.serviceType = AML_MP_CAS_SERVICE_PVR_RECORDING;
        ca_serv_info.ecm_pid = ((STB_CA_Glue_t *)handle)->pmt_info.ca_pid_list->ecm_pid;

        pid_entry = ((STB_CA_Glue_t *)handle)->pmt_info.ca_pid_list;

        /* if cas type is nagra, we need get emi and scramble algo then set to cas hal */
        ca_serv_info.ca_private_data_len = MAX_DATA_LEN;
        ca_serv_info.ca_private_data[2] =  ((STB_CA_Glue_t *)handle)->pmt_info.scramble_algo;
        CA_DBG("rec start cas_session [%p] ca_private_data[2]=%x", cas_session, ca_serv_info.ca_private_data[2]);

        while (pid_entry != NULL)
        {
            pid_entry->running = TRUE;
            ca_serv_info.stream_pids[ca_serv_info.stream_num++] = pid_entry->es_pid;
            pid_entry = pid_entry->next;
        }

        ret = Aml_MP_CAS_StartDVRRecord(
                  cas_session,
                  &ca_serv_info);

        if (ret)
        {
            CA_DBG("CAS start DVR failed. ret = %d\r\n", ret);
            return -1;
        }
    }

#endif
    return 0;
}


/*!**************************************************************************
 * @brief   This function is called when a record is stoped
 * @param   handle - CA descrambler handle
 ****************************************************************************/
void STB_CAPVRRecordStop(UINTPTR handle)
{
#ifdef SUPPORT_CAS
    U32BIT ret;

    FUNCTION_START(STB_CAPVRRecordStop);

    CA_DBG("%s(%#x)", __FUNCTION__, handle);

    ASSERT(handle);

    if (TRUE)
    {
        CA_DBG("%s(%#x): Stop recording", __FUNCTION__, handle);

        if (!(((STB_CA_Glue_t *)handle)->session_info))
        {
            CA_DBG("CA glue stop recording status session_info is null");
            return;
        }

        if (!(((STB_CA_Glue_t *)handle)->session_info->cas_session))
        {
            CA_DBG(("CA glue stop recording status cas session is null"));
            return;
        }
        Aml_MP_CAS_StopDVRRecord(((STB_CA_Glue_t *)handle)->session_info->cas_session);
        Aml_MP_CAS_CloseSession(((STB_CA_Glue_t *)handle)->session_info->cas_session);
        ((STB_CA_Glue_t *)handle)->session_info->cas_session=NULL;
        return;
    }

    CA_DBG("%s(%#x): Started recording", __FUNCTION__, handle);

    FUNCTION_FINISH(STB_CAPVRRecordStop);
#endif
}

/*!**************************************************************************
 * @brief   This function is called when in timeshift state
 * @param   handle - CA descrambler handle
 * @param   On - TRUE in timeshfit, FALSE normal record or replay
 ****************************************************************************/
void STB_CASetTimeShiftOn(UINTPTR handle, BOOLEAN On)
{
    ASSERT(handle);
    ((STB_CA_Glue_t *)handle)->is_timeshift = On;

    CA_DBG("%s(%#x): ON=%d", __func__, handle, On);
}

/*!**************************************************************************
 * @brief   This function is called when a recording starts and when it stops
 * @param   handle - CA descrambler handle
 * @param   status - TRUE when a recording starts, FALSE when it stops
 ****************************************************************************/
void STB_CANotifyRecordingStatus(UINTPTR handle, BOOLEAN status)
{
#ifdef SUPPORT_CAS
    U32BIT ret;

    FUNCTION_START(STB_CANotifyRecordingStatus);

    CA_DBG("%s(%#x): %u", __FUNCTION__, handle, status);

    ASSERT(handle);

    if (!status)
    {
        CA_DBG("%s(%#x): Stop recording", __FUNCTION__, handle);

        if (!(((STB_CA_Glue_t *)handle)->session_info))
        {
            CA_DBG("CA glue notify recording status session_info is null");
            return;
        }
        if(((STB_CA_Glue_t *)handle)->session_info->cas_session !=NULL)
        {
            Aml_MP_CAS_StopDVRRecord(((STB_CA_Glue_t *)handle)->session_info->cas_session);
            Aml_MP_CAS_CloseSession(((STB_CA_Glue_t *)handle)->session_info->cas_session);
            ((STB_CA_Glue_t *)handle)->session_info->cas_session=NULL;

        }

        return;
    }

    CA_DBG("%s(%#x): Started recording", __FUNCTION__, handle);

    FUNCTION_FINISH(STB_CANotifyRecordingStatus);
#endif
}

BOOLEAN STB_CATMSCicamBit()
{
    //return TRUE;
    return is_enable_cicam;
}

BOOLEAN STB_CATMSFtaBit()
{
    return is_enable_fta;
}


/******************************************************************************
** End of file
******************************************************************************/
