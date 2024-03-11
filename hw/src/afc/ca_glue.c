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
#define TAG  "CA_GLUE"
#ifdef ANDROID
#include <cutils/properties.h>
#endif

/* Third party header files */

/* Ocean Blue Software header files */
#include <techtype.h>
#include <dbgfuncs.h>
#include <stbhwc.h>
#include "stbhwos.h"
#include "stbhwmem.h"
#include "ca_glue.h"
#include "stbhwcfg.h"

#include "cJSON.h"

#include "JNICasTypes.h"
#include "JNICasWrapper.h"
#include "wrapper_dmx.h"
/*---constant definitions for this file----------------------------------------*/

#ifdef CA_GLUE_DEBUG
#define CA_DBG(X,...)    DTV_LOGI(TAG, X, ##__VA_ARGS__)
#else
#define CA_DBG(X)
#endif


static void *cas_mutex;

typedef struct es_pid_entry
{
    struct es_pid_entry *next;
    U16BIT es_pid;
    int ecm_pid;
    U16BIT private_data_length;
    U8BIT private_data[512];
} ES_PID_INFO;

typedef struct
{
    BOOLEAN has_global_ca;
    BOOLEAN has_component_ca;
    U8BIT scramble_algo;
    ES_PID_INFO *ca_pid_list;
    ES_PID_INFO *last_pid_entry;
} CA_PMT_INFO;

typedef struct
{
    U8BIT path;
    U16BIT service_id;
    CA_PMT_INFO pmt_info;
    BOOLEAN start_descrambling;
    CasHandle ca_handle;
    CasHandle ca_session_handle;
    AM_CasPluginInfo plug_info;
    int match_ca_id;
} CA_HANDLE;

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


/*---global function definitions-----------------------------------------------*/
/*!**************************************************************************
 * @brief   Called once on system startup to allow initialisation of the CA systems
 * @return  TRUE if initialisation is successful, FALSE otherwise
 ****************************************************************************/
BOOLEAN STB_CAInitialise(void)
{
    return FALSE;
}

/*!**************************************************************************
 * @brief   This function can get from other module, to judge cas type
 * @return  cas type E_CAS_TYPE
 ****************************************************************************/
E_CAS_TYPE STB_CAGetCASType()
{
    return CAS_TYPE_NONE;
}

/*!**************************************************************************
 * @brief   This function can get from other module, to judge under TSE mode
 *          or not
 * @return  true under TSE mode, false not TSE mode
 ****************************************************************************/
BOOLEAN STB_CAIsTSEMode()
{
    return FALSE;
}

static void STB_FreePidList(UINTPTR handle)
{
    //free pid info list
    ES_PID_INFO *head = ((CA_HANDLE *)handle)->pmt_info.ca_pid_list;
    ES_PID_INFO *pid_entry = head;

    while (pid_entry != NULL)
    {
        head = pid_entry->next;
        STB_MEMFreeSysRAM(pid_entry);
        pid_entry = head;
    }

    ((CA_HANDLE *)handle)->pmt_info.ca_pid_list = NULL;
}

static void DebugPrintBuffer(U8BIT *buff, U32BIT len)
{
   #define LINE_LEN  (16 * 3)
   const char hexdigits[] = "0123456789abcdef";
   char printline[LINE_LEN + 2];
   U32BIT ii, jj;
   printline[LINE_LEN] = '\n';
   printline[LINE_LEN + 1] = '\0';
   for (ii = 0, jj = 0; jj != len; ++jj)
   {
      printline[ii++] = ' ';
      printline[ii++] = hexdigits[(buff[jj] >> 4) & 0xF];
      printline[ii++] = hexdigits[buff[jj] & 0xF];
      if (ii == LINE_LEN)
      {
         STB_SPDebugWrite(printline);
         ii = 0;
      }
   }
   if (ii != LINE_LEN)
   {
      printline[ii++] = '\n';
      printline[ii] = '\0';
      STB_SPDebugWrite(printline);
   }
}

JCAS_JNI_RESULT CAS_CallBack(CasHandle casHandle, int event, int args, uint8_t* data, int dataLen)
{
    U8BIT i;

    CA_DBG("%s casHandle[%#x] event[%d] args[%d] data[%p] dataLen[%d]", __FUNCTION__, casHandle, event, args, data, dataLen);
    if (NULL != data)
    {
        DebugPrintBuffer(data, dataLen);
    }
    else
    {
        CA_DBG("%s data==NULL", __FUNCTION__);
    }
    return AM_CAS_JNI_OK;
}

JCAS_JNI_RESULT CAS_SessionCallBack(CasHandle casHandle, CasSessionHandle sessionHandle, int event, int args, uint8_t* data, int dataLen)
{
    U8BIT i;

    CA_DBG("%s casHandle[%#x] sessionHandle[%#x] event[%d] args[%d] data[%p] dataLen[%d]", __FUNCTION__,\
        casHandle, sessionHandle, event, args, data, dataLen);
    if (NULL != data)
    {
        DebugPrintBuffer(data, dataLen);
    }
    else
    {
        CA_DBG("%s data==NULL", __FUNCTION__);
    }
    return AM_CAS_JNI_OK;
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
    U8BIT j;
    U32BIT ret = 0;
    static int init_flag = 0;

    CA_DBG("%s(path=%u, serv_id=%u, ca_ids=%p, num_ca_ids=%u, init_flag=%d)",
            __FUNCTION__, path, serv_id, ca_ids, num_ca_ids, init_flag);
    ASSERT(handle);

    if (init_flag == 0)
    {
        ret = MediaCAS_Init();
        init_flag = 1;
        if (ret)
        {
            CA_DBG("am cas init failed [%d]", ret);
        }

        cas_mutex = (void *)STB_OSCreateMutex();
    }

    if (num_ca_ids == 0)
    {
        CA_DBG("Free channel, no need descrambler");
        return FALSE;
    }

    STB_OSMutexLock(cas_mutex);

    *handle = (UINTPTR)STB_MEMGetSysRAM(sizeof(CA_HANDLE));
    ASSERT(*handle);
    memset((void *)*handle, 0x0, sizeof(CA_HANDLE));
    ((CA_HANDLE *)(*handle))->path = path;
    ((CA_HANDLE *)(*handle))->service_id = serv_id;
    CA_DBG("%s handle[%#x]", __FUNCTION__, *handle);

    memset(&(((CA_HANDLE *)(*handle))->plug_info), 0, sizeof(AM_CasPluginInfo));
    for (j = 0; j < num_ca_ids; j++)
    {
        CA_DBG("%s CA Id[%#x]", __FUNCTION__, ca_ids[j]);
        ((CA_HANDLE *)(*handle))->plug_info.tisSessionId = path;
        ((CA_HANDLE *)(*handle))->plug_info.tisUseCase = LIVE;
        ((CA_HANDLE *)(*handle))->plug_info.casCallback = (CAS_Callback_t)CAS_CallBack;
        ((CA_HANDLE *)(*handle))->plug_info.casSessionCallback = (CAS_SessionCallback_t)CAS_SessionCallBack;
        CA_DBG("%s casCallback [%p] casSessionCallback[%p]", __FUNCTION__, ((CA_HANDLE *)(*handle))->plug_info.casCallback,((CA_HANDLE *)(*handle))->plug_info.casSessionCallback);
        CA_DBG("%s casCallback [%p] casSessionCallback[%p]", __FUNCTION__, (CAS_Callback_t)CAS_CallBack,(CAS_SessionCallback_t)CAS_SessionCallBack);

        if (MediaCAS_IsSystemIdSupported(ca_ids[j]))
        {
            ((CA_HANDLE *)(*handle))->plug_info.caSystemId = ca_ids[j];
            ((CA_HANDLE *)(*handle))->match_ca_id  = ca_ids[j];
            CA_DBG("%s Found supported CA Id[%#x]", __FUNCTION__, ((CA_HANDLE *)(*handle))->match_ca_id);
            break;
        }
        else
        {
            CA_DBG("%s Not supported CA Id",__FUNCTION__);
        }
    }

    if (j >= num_ca_ids)
    {
        CA_DBG("%s Not found supported CA Id",__FUNCTION__);
        STB_OSMutexUnlock(cas_mutex);
        return FALSE;
    }

    ret = MediaCAS_CreatePlugin(path, &(((CA_HANDLE *)(*handle))->plug_info), &(((CA_HANDLE *)(*handle))->ca_handle));
    if (ret)
    {
        CA_DBG("%s MediaCAS_CreatePlugin failed,ret=%d",__FUNCTION__,ret);
        STB_OSMutexUnlock(cas_mutex);
        return FALSE;
    }

    STB_OSMutexUnlock(cas_mutex);

    return TRUE;
}
/*!**************************************************************************
 * @brief   Will be called when a CA descrambler is no longer required.
 * @param   handle - CA descrambler handle being released
 * @return  TRUE if the descrambler is released, FALSE otherwise
 ****************************************************************************/
BOOLEAN STB_CAReleaseDescrambler(UINTPTR handle)
{
    //AM_CloseCasSession
    //AM_DestroyCasPlugin
    U32BIT ret = 0;

    ASSERT(handle);
    CA_DBG("%s handle=(0x%lx)", __FUNCTION__, handle);

    STB_OSMutexLock(cas_mutex);

    ret =  MediaCAS_DestroyCasPlugin(((CA_HANDLE *)handle)->ca_handle);
    if (ret)
    {
        CA_DBG("MediaCAS_CloseCasSession failed,ret=[%d]",ret);
    }
#if 0
    ret = MediaCAS_CasManagerTerm();
    if (ret)
    {
        CA_DBG("MediaCAS_CloseCasSession failed,ret=[%d]",ret);
    }
#endif

    if (0 != ((CA_HANDLE *)handle)->ca_handle)
    {
        ((CA_HANDLE *)handle)->ca_handle = 0;
    }

    if (0 != ((CA_HANDLE *)handle)->ca_session_handle)
    {
        ((CA_HANDLE *)handle)->ca_session_handle = 0;
    }

    if (((CA_HANDLE *)handle)->pmt_info.ca_pid_list)
    {
        STB_FreePidList(handle);
    }

    STB_MEMFreeSysRAM((void *)handle);

    STB_OSMutexUnlock(cas_mutex);

    return TRUE;
}

/*!**************************************************************************
 * @brief   This function will be called when decoding of a service is about to start
 *          and there's an associated descrambler.
 * @param   handle - CA descrambler handle
 ****************************************************************************/
void STB_CADescrambleServiceStart(UINTPTR handle)
{
    AM_CasSessionInfo ca_session_info ;
    U32BIT ret = 0;
    ES_PID_INFO *pid_entry;
    U32BIT i = 0;
    CA_DBG("%s handle=(0x%lx)", __FUNCTION__, handle);

    ASSERT(handle);

    STB_OSMutexLock(cas_mutex);

    //only support descrambling of global ca descriptor for now.
    //For component level descramble stream, irdeto cas plugin will process
    //cat and pmt by itself, don't need dtvkit to do extra things.
    //it is not suitable for other CAS.
    if (((CA_HANDLE *)handle)->pmt_info.has_global_ca == FALSE &&
        ((CA_HANDLE *)handle)->pmt_info.has_component_ca == FALSE)
    {
        CA_DBG("%s Warning: DO*NOT have ca descriptor", __FUNCTION__);
        //STB_OSMutexUnlock(cas_mutex);
        //return;
    }

    if (((CA_HANDLE *)handle)->start_descrambling == TRUE)
    {
        CA_DBG("%s CA glue service has started.", __FUNCTION__);
        //STB_OSMutexUnlock(cas_mutex);
        //return;
    }

    //AM_StartDescrambling
    memset(&ca_session_info, 0, sizeof(AM_CasSessionInfo));
    ca_session_info.casPluginInfo = ((CA_HANDLE *)handle)->plug_info;
    CA_DBG("%s caSystemId=[%#x]", __FUNCTION__, ca_session_info.casPluginInfo.caSystemId);
    ca_session_info.scramblingMode = 0;
    ca_session_info.isProgramLevel = FALSE;

    pid_entry = ((CA_HANDLE *)handle)->pmt_info.ca_pid_list;

    while (pid_entry != NULL)
    {
        ca_session_info.ecmPid = pid_entry->ecm_pid;
        ca_session_info.scrambledEsPids[ca_session_info.scrambledEsNum++] = pid_entry->es_pid;
        CA_DBG("Descrambling es pid [%#x], ecm_pid [%#x]", pid_entry->es_pid, pid_entry->ecm_pid);
        ca_session_info.privateDataLen = pid_entry->private_data_length;
        if (pid_entry->private_data_length != 0)
        {
            for (i=0; i<pid_entry->private_data_length; i++)
            {
                ca_session_info.privateData[i] = pid_entry->private_data[i];
            }
            DebugPrintBuffer(ca_session_info.privateData, ca_session_info.privateDataLen);
        }

        pid_entry = pid_entry->next;
    }

    if (ca_session_info.scrambledEsNum <= 0)
    {
        CA_DBG("%s Not found scrambled es", __FUNCTION__);
        //STB_OSMutexUnlock(cas_mutex);
        //return;
    }

    ret = MediaCAS_OpenCasSession(((CA_HANDLE *)handle)->ca_handle, &ca_session_info,\
                                  &(((CA_HANDLE *)handle)->ca_session_handle));
    if (ret)
    {
        CA_DBG("MediaCAS_OpenCasSession failed,ret=[%d]",ret);
        //STB_OSMutexUnlock(cas_mutex);
        //return;
    }

    CA_DBG("%s ca_handle:ca_session_handle=[%#x]:[%#x]",__FUNCTION__, ((CA_HANDLE *)handle)->ca_handle,\
            ((CA_HANDLE *)handle)->ca_session_handle);
    ret = MediaCAS_StartDescrambling(((CA_HANDLE *)handle)->ca_handle, ((CA_HANDLE *)handle)->ca_session_handle);
    if (ret)
    {
        CA_DBG("CAS start descrambling failed,ret=[%d]",ret);
        //STB_OSMutexUnlock(cas_mutex);
        //return;
    }
    ((CA_HANDLE *)handle)->start_descrambling = TRUE;

    STB_OSMutexUnlock(cas_mutex);
}

/*!**************************************************************************
 * @brief   This function will be called when decoding of a service is stopped
 * @param   handle - CA descrambler handle
 ****************************************************************************/
void STB_CADescrambleServiceStop(UINTPTR handle)
{
    U32BIT ret = 0;
    ASSERT(handle);

    CA_DBG("%s handle=(0x%lx)", __FUNCTION__, handle);

    STB_OSMutexLock(cas_mutex);

    //AM_StopDescrambling
    if (0 != handle)
    {
        if (((CA_HANDLE *)handle)->start_descrambling == FALSE)
        {
            CA_DBG("CA glue service not started.");
            STB_OSMutexUnlock(cas_mutex);
            return;
        }
    }

    CA_DBG("%s ca_handle:ca_session_handle=[%#x]:[%#x]",__FUNCTION__, ((CA_HANDLE *)handle)->ca_handle,\
            ((CA_HANDLE *)handle)->ca_session_handle);
    ret = MediaCAS_StopDescrambling(((CA_HANDLE *)handle)->ca_handle, ((CA_HANDLE *)handle)->ca_session_handle);
    if (ret)
    {
        CA_DBG("CAS stop descrambling failed.");
        //return;
    }

    ret = MediaCAS_CloseCasSession(((CA_HANDLE *)handle)->ca_handle, ((CA_HANDLE *)handle)->ca_session_handle);
    if (ret)
    {
        CA_DBG("MediaCAS_CloseCasSession failed,ret=[%d]",ret);
    }

    ((CA_HANDLE *)handle)->start_descrambling = FALSE;

    STB_OSMutexUnlock(cas_mutex);
}

/*!**************************************************************************
 * @brief   This function will be called when set CA descramble ioctl
 * @param   handle - CA descrambler handle
 ****************************************************************************/
void STB_CADescrambleIoctl(UINTPTR handle, U32BIT session, const char* inJson, char* outJson, U32BIT outLen)
{
    FUNCTION_START(STB_CADescrambleIoctl);
    ASSERT(handle);
    CA_DBG("%s handle=(0x%lx)", __FUNCTION__, handle);
    STB_OSMutexLock(cas_mutex);

    if (0 != session)
    {
        if (MediaCAS_SendCommand((CasHandle)session, CAS_EVENT_TYPE_PROVIDER , CAS_EVENT_TYPE_STATUS, (uint8_t*)inJson, strlen(inJson)))
        {
            CA_DBG("%s MediaCAS_SendCommand failed.", __FUNCTION__);
        }
    }
    else if (0 != handle)
    {
        if (MediaCAS_SendSessionCommand(((CA_HANDLE *)handle)->ca_handle, ((CA_HANDLE *)handle)->ca_session_handle, CAS_EVENT_TYPE_PROVIDER, CAS_EVENT_TYPE_STATUS, (uint8_t*)inJson, strlen(inJson)))
        {
            CA_DBG("%s MediaCAS_SendSessionCommand failed.", __FUNCTION__);
        }
    }
    else
    {
        CA_DBG("%s ca_handle = NULL", __FUNCTION__);
    }

    CA_DBG("%s handle:(0x%lx) session:(0x%lx) [inJson: %s] [inLen: %d]", __FUNCTION__, handle, session, inJson, strlen(inJson));
    STB_OSMutexUnlock(cas_mutex);
    FUNCTION_FINISH(STB_CADescrambleIoctl);
}

BOOLEAN STB_CACheckSessionStatus(UINTPTR handle, UINTPTR session)
{
    ASSERT(handle);
    BOOLEAN ret = FALSE;
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
static void _STB_CollectESCaInfo(UINTPTR handle, CA_PMT_INFO *pmt_info, CA_LIST * ca_list,U16BIT global_ecm_pid, U8BIT *private_data, U16BIT private_data_length)
{
    STREAM_ENTRY *stream_entry;
    ES_PID_INFO *ca_pid_info;
    int i;

    stream_entry = ca_list->stream_list;

    while (stream_entry != NULL)
    {
        ca_pid_info = (ES_PID_INFO *)STB_MEMGetSysRAM(sizeof(ES_PID_INFO));
        memset(ca_pid_info, 0, sizeof(ES_PID_INFO));

        if (pmt_info->has_global_ca)
        {
            ca_pid_info->ecm_pid = global_ecm_pid;
            ca_pid_info->es_pid = stream_entry->pid;
            if ((private_data_length < 512) && (private_data != NULL))
            {
                ca_pid_info->private_data_length = private_data_length;
                memcpy(ca_pid_info->private_data, private_data, private_data_length);
            }

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
            if (((CA_HANDLE *)handle)->match_ca_id == stream_entry->ca_desc_array[i].ca_id)
            {
                ca_pid_info->es_pid = stream_entry->pid;
                ca_pid_info->ecm_pid = stream_entry->ca_desc_array[i].ca_pid;
                CA_DBG("%s Descrambling es pid [%#x], ecm_pid [%#x]",__FUNCTION__, ca_pid_info->es_pid, ca_pid_info->ecm_pid);

                if ((stream_entry->ca_desc_array[i].private_data_length < 512) && (stream_entry->ca_desc_array[i].private_data != NULL))
                {
                    memcpy(ca_pid_info->private_data,stream_entry->ca_desc_array[i].private_data ,\
                        stream_entry->ca_desc_array[i].private_data_length);
                    ca_pid_info->private_data_length = stream_entry->ca_desc_array[i].private_data_length;
                    DebugPrintBuffer(ca_pid_info->private_data,ca_pid_info->private_data_length);
                }

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
    U8BIT i;
    CA_PMT_INFO pmt_info;
    CA_LIST *ca_list =NULL;
    U16BIT global_ecm_pid = 0x1fff;
    U16BIT private_data_length = 0;
    U8BIT *private_data = NULL;

    FUNCTION_START(STB_CAReportPMT);

    CA_DBG("%s(handle=0x%lx, pmt_data=%p, data_len=%u)", __FUNCTION__, handle, pmt_data, data_len);

    /*if PMT update when playback, we need re-start descrambling with
    **the new es pid.but now we try to handle that in Video/Audio decoding
    **status notify function. And not support ecm pid update for now.
    */

    ASSERT(handle);
    if (NULL == pmt_data || data_len <= 3)
    {
        return;
    }

    STB_OSMutexLock(cas_mutex);

    ca_list = _STB_CAGetPmtDescArrayList(pmt_data);
    if (NULL != ca_list)
    {
        CA_DBG("%s svc_id[%#x], num_ca_entries[%d], num_streams[%d]",
                __FUNCTION__, ca_list->serv_id, ca_list->num_ca_entries, ca_list->num_streams);

        memset(&pmt_info, 0, sizeof(CA_PMT_INFO));
        for (i = 0; i < ca_list->num_ca_entries; i++)
        {
            if (((CA_HANDLE *)handle)->match_ca_id == ca_list->ca_desc_array[i].ca_id)
            {
                CA_DBG("Found supported global CA Id[%#x]", ca_list->ca_desc_array[i].ca_id);
                pmt_info.has_global_ca = TRUE;
                global_ecm_pid = ca_list->ca_desc_array[i].ca_pid;
                if (ca_list->ca_desc_array[i].private_data != NULL)
                {
                    private_data_length = ca_list->ca_desc_array[i].private_data_length;
                    private_data = ca_list->ca_desc_array[i].private_data;
                    DebugPrintBuffer(private_data, private_data_length);
                }
                break;
            }
            else
            {
                CA_DBG("ca_id not match CA Id[%#x]",ca_list->ca_desc_array[i].ca_id);
            }
        }

        if (i >= ca_list->num_ca_entries)
        {
            CA_DBG("%s not found supported global CA desc", __FUNCTION__);
        }

        if (((CA_HANDLE *)handle)->pmt_info.ca_pid_list)
        {
            CA_DBG("%s free previous pmt pid list", __FUNCTION__);
            STB_FreePidList(handle);
        }

        _STB_CollectESCaInfo(handle, &pmt_info, ca_list, global_ecm_pid, private_data, private_data_length);
        memcpy(&(((CA_HANDLE *)handle)->pmt_info), &pmt_info, sizeof(CA_PMT_INFO));
        ((CA_HANDLE *)handle)->service_id = ca_list->serv_id;

        _STB_CAFreeDescArrayList(ca_list);
   }
    STB_OSMutexUnlock(cas_mutex);
    FUNCTION_FINISH(STB_CAReportPMT);
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
}

/*!**************************************************************************
 * @brief   Notifies the CA system of a change in the video decoding state
 * @param   handle - CA descrambler handle
 * @param   decode_status - decoding status
 ****************************************************************************/
void STB_CADecodeVideoStatus(U32BIT handle, E_CA_DECODE_STATUS decode_status)
{
}

/*!**************************************************************************
 * @brief   Notifies the CA system of a change in the audio decoding state
 * @param   handle - CA descrambler handle
 * @param   decode_status - decoding status
 ****************************************************************************/
void STB_CADecodeAudioStatus(U32BIT handle, E_CA_DECODE_STATUS decode_status)
{
}

/*!**************************************************************************
 * @brief   Notifies the CA system of a change in the AD decoding state
 * @param   handle - CA descrambler handle
 * @param   decode_status - decoding status
 ****************************************************************************/
void STB_CADecodeADStatus(U32BIT handle, E_CA_DECODE_STATUS decode_status)
{
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
    return FALSE;
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
    return FALSE;
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



/*!**************************************************************************
 * @brief   Called to free the array of PIDs allocated by STB_CAGetRecordingPids.
 * @param   pid_array - array of PIDs to be freed
 * @param   num_pids - number of PIDs in the array
 ****************************************************************************/
void STB_CAReleaseRecordingPids(U16BIT *pid_array, U16BIT num_pids)
{
}


/*!**************************************************************************
 * @brief   This function is called when a record is stoped
 * @param   handle - CA descrambler handle
 ****************************************************************************/
int STB_CAPVRRecordStart(UINTPTR handle)
{
    return -1;
}


/*!**************************************************************************
 * @brief   This function is called when a record is stoped
 * @param   handle - CA descrambler handle
 ****************************************************************************/
void STB_CAPVRRecordStop(UINTPTR handle)
{
}

/*!**************************************************************************
 * @brief   This function is called when a recording starts and when it stops
 * @param   handle - CA descrambler handle
 * @param   status - TRUE when a recording starts, FALSE when it stops
 ****************************************************************************/
void STB_CANotifyRecordingStatus(UINTPTR handle, BOOLEAN status)
{
}

/******************************************************************************
** End of file
******************************************************************************/
