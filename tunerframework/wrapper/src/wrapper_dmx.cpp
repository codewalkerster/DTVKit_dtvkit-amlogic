//! C/C++
#include <stdint.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>
#include <map>
#include <vector>
#include <pthread.h>



//!self
#include "JNI_tuner.h"
#include "filter_utils.h"


#include "wrapper_dmx.h"
#include "wrapper_frontend.h"

//!JNI
#include <jni.h>

#define LOG_TAG "DMX_HAL"
using namespace std;
//=========================================
typedef struct
{
    pthread_mutex_t dmx_mutex;
    bool dmx_exit;
    bool initDmxLocked = false;
} DMX_THREAD_PARA;

typedef struct
{
    filter_callback cb;
    ST_CALLBACK_T para;
}PID_TASK_PACKAGE;

DMX_THREAD_PARA gDMXTaskLocked;
//=========================================

typedef struct s_pid_hal
{
    jobject Jfilter;
    filter_callback cb;
    U16BIT pid ;
    void* user_data;
} S_HAL;

typedef map<int, S_HAL*> FILTER_MAP;
#define MAP_INSERT_ITEM(__MAP__, __KEY__, __VALUE__) __MAP__.insert(std::make_pair(__KEY__, __VALUE__))

static FILTER_MAP filter_map;
static int symbol_open = 0;
S_QUEUE *pid_queue = NULL;

static void FilterTask(void *param)
{
    ALOGD("start:%s", __FUNCTION__);
    PID_TASK_PACKAGE package;
    while (1)
    {
        if (!wrapper_OSReadQueue(pid_queue , (void *)&package, sizeof(PID_TASK_PACKAGE), TIMEOUT_NEVER))
        {
            ALOGD("%s read pid_queue failure", __FUNCTION__);
        }
        if (package.cb != NULL)
        {
            package.cb(&package.para);
            delete[] package.para.pun8_buffer;
        }
        //ALOGD("%s pidcallback is %d", __FUNCTION__, package.para.un32filterID);
    }
    ALOGD("end:%s", __FUNCTION__);
}

static void DebugPrintBuffer(U8BIT *buff, U32BIT len)
{
#define LINE_LEN (16 * 3)
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
            ALOGD("%s", printline);
            ii = 0;
        }
    }
    if (ii != LINE_LEN)
    {
        printline[ii++] = '\n';
        printline[ii] = '\0';
        ALOGD("%s", printline);
    }
}

void FilterCallback(jobject filter, jobjectArray filterEventArray, int filterStatus) {
    //ALOGD("start:%s", __FUNCTION__);
    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGD("%s : test fail, env is null", __FUNCTION__);
        return;
    }
    if (NULL != filterEventArray) {
        int eventSize = env->GetArrayLength(filterEventArray);
        for (int index = 0; index < eventSize; index++) {
            //1.check section event
            jobject filterEvent = env->GetObjectArrayElement(filterEventArray, index);
            Section_Event stSectionEvent;
            memset(&stSectionEvent, 0, sizeof(Section_Event));
            filter_utils_getSectionEvent(env, filterEvent, &stSectionEvent);
            //3.read section data
            char *buffer = new char[stSectionEvent.dataLength];
            int readSize = Am_filter_read(filter, buffer, 0, stSectionEvent.dataLength);
            int filterid = Am_filter_getId(filter);
            pthread_mutex_lock( &gDMXTaskLocked.dmx_mutex);
            FILTER_MAP::iterator it = filter_map.find( filterid );
            if (it != filter_map.end())
            {
                S_PID_FILTER_INFO* user_data = (S_PID_FILTER_INFO*)it->second->user_data;
                //ALOGD("user_data.index = %d, user_data.pid = %d, handle =%d,user_data = %p", user_data->index, user_data->pid, user_data->fhandle, user_data);
                //filter_callback callback = it->second->cb;
                ST_CALLBACK_T para;
                para.un32filterID = filterid;
                para.pun8_buffer = (uint8_t *)buffer ;
                para.un32_length =  readSize;
                // DebugPrintBuffer((U8BIT *)buffer, (U32BIT)readSize);
                user_data->fhandle = filterid;
                para.un32_userdata = user_data;
                if ( it->second!= NULL && it->second->cb != NULL )
                {
                    PID_TASK_PACKAGE package;
                    package.cb = it->second->cb;
                    package.para = para;
                    if (!wrapper_OSWriteQueue(pid_queue, (void *)&package, sizeof(PID_TASK_PACKAGE), TIMEOUT_NEVER))
                    {
                        ALOGD("%s: write pid_queue failure", __FUNCTION__);
                    }
                }
            }
            pthread_mutex_unlock( &gDMXTaskLocked.dmx_mutex);
        }
    }
    if (attached) {
        Am_tuner_detachJNIEnv();
    }
}

int DMX_OpenFilter(U8BIT path, filter_callback cb, void* user_data,U16BIT source_type ,U16BIT demux_cap  ,U32BIT section_size)
{
    int ClientId = 0xFF;
    if (!gDMXTaskLocked.initDmxLocked )
    {
        gDMXTaskLocked.initDmxLocked = true;
        gDMXTaskLocked.dmx_exit = false;
        pthread_mutex_init( &gDMXTaskLocked.dmx_mutex, NULL);
    }
    /*
    typedef enum
    {
        DMX_TUNER,
        DMX_1394,
        DMX_MEMORY
    } E_STB_DMX_DEMUX_SOURCE;
    */
    if (source_type  != 0)
    {
        ClientId = Am_tuner_getTunerClientIdByType(TUNER_TYPE_DVR_PLAY);
        ALOGD("start DMX_CAPS_PLAYBACK filter ClientId 0x%x",ClientId);
    }
    else
    {
        TUNER_TYPE tuner_type = TUNER_TYPE_LIVE_0;
        /*
        DMX_CAPS_RECORDING = 0x04,
        */
        if (demux_cap == 0x04)
        {
            tuner_type = TUNER_TYPE_DVR_RECORD;
        }
        else if (Wrapper_TuneIsSearchMode(path))
        {
            tuner_type = TUNER_TYPE_SCAN;
        }
        else
        {
            switch (path)
            {
                case 0:
                {
                    tuner_type = TUNER_TYPE_LIVE_0;
                    break ;
                }
                case 1:
                {
                    tuner_type = TUNER_TYPE_LIVE_1;
                    break ;
                }
                case 2 :
                {
                    tuner_type = TUNER_TYPE_LIVE_2;
                    break ;
                }
                default:
                {
                    tuner_type = TUNER_TYPE_LIVE_0;
                    break ;
                }
            }
        }
        ClientId = Am_tuner_getTunerClientIdByType(tuner_type);
        ALOGD("start DMX_CAPS_Live filter path [%d] demux_cap [0x%x] ClientId[%d] tuner_type[%d]",path,demux_cap ,ClientId,tuner_type);
    }
    Am_filter_callback filterCallback = FilterCallback;
    S_HAL *filerInfo;
    filerInfo = new S_HAL();
    filerInfo->cb = cb ;
    if (section_size > 8 * 4096)
    {
        ALOGI("Executor@large section_size  [%d]",section_size);
        filerInfo->Jfilter = Am_tuner_openFilter(ClientId, 1, 1, section_size, (long)filterCallback, 1);
    }
    else
    {
        filerInfo->Jfilter = Am_tuner_openFilter(ClientId, 1, 1, section_size, (long)filterCallback, 0);
    }
    filerInfo->user_data  = user_data;
    int filterId = Am_filter_getId(filerInfo->Jfilter);

    pthread_mutex_lock( &gDMXTaskLocked.dmx_mutex);
    FILTER_MAP::iterator it = filter_map.find(filterId);
    if (it != filter_map.end())
    {    /* this flow may happen: case 1 tuner object release before player.stop, case 2 monitor flow bug*/
         filter_map.erase( filterId );
        ALOGI("%s -----------------Already Find filterId: 0x%x  update from map.", __FUNCTION__, filterId);
    }
    MAP_INSERT_ITEM( filter_map, filterId, filerInfo );
    ALOGI("Insert new filerInfo filterId: %d.", filterId);
    pthread_mutex_unlock( &gDMXTaskLocked.dmx_mutex);
    ALOGI("DMX_HAL_%s  filerInfo %p filterId 0x%x Jfilter %p, user_data %p", __FUNCTION__, filerInfo, filterId , filerInfo->Jfilter, filerInfo->user_data);
    if (symbol_open == 0)
    {
        if (pid_queue == NULL)
        {
            pid_queue = (S_QUEUE*)wrapper_OSCreateQueue(sizeof(PID_TASK_PACKAGE),  20);
        }
        else
        {
            ALOGI("%s error:pid_queue initialization failure", __FUNCTION__);
        }
        if (wrapper_OSCreateTask(FilterTask, NULL, (U8BIT *)"FilterTask") == NULL)
        {
            ALOGI("%s error:Failed to create task for filter", __FUNCTION__);
        }
        symbol_open = 1;
    }
    return filterId ;
}

BOOLEAN DMX_CloseFilter(int un32filterID)
{
    BOOLEAN ret = FALSE;
    pthread_mutex_lock( &gDMXTaskLocked.dmx_mutex);
    FILTER_MAP::iterator it = filter_map.find( un32filterID );
    pthread_mutex_unlock( &gDMXTaskLocked.dmx_mutex);
    if (it != filter_map.end())
    {
        ALOGD("DMX_HAL_%s  filerInfo %p filterId 0x%x Jfilter %p pid [0x%x]",  __FUNCTION__,  it->second , un32filterID, it->second->Jfilter, it->second->pid);
        Am_filter_close(it->second->Jfilter);

        pthread_mutex_lock( &gDMXTaskLocked.dmx_mutex);
        if (it->second != NULL)
         {
            delete it->second;
            it->second = NULL;
        }
        filter_map.erase( un32filterID );
        pthread_mutex_unlock( &gDMXTaskLocked.dmx_mutex);
        ret =  TRUE;
    }
    return ret ;
}

BOOLEAN DMX_SetupFilter(int un32filterID, U16BIT pid, const struct dmx_sct_filter_params* params)
{
    BOOLEAN ret = FALSE;
    char mode[3] = {0, 0, 0};
    pthread_mutex_lock( &gDMXTaskLocked.dmx_mutex);
    FILTER_MAP::iterator it = filter_map.find( un32filterID );
    pthread_mutex_unlock( &gDMXTaskLocked.dmx_mutex);
    if (it != filter_map.end())
    {
        TS_Filter_Configuration tsFilterConfiguration;
        memset(&tsFilterConfiguration, 0, sizeof(TS_Filter_Configuration));

        tsFilterConfiguration.setting.section_setting.filter =new char[DEMUX_SECTION_FILTER_LENGTH+2];
        memset(tsFilterConfiguration.setting.section_setting.filter, 0, DEMUX_SECTION_FILTER_LENGTH+2);

        tsFilterConfiguration.setting.section_setting.mask =new char[DEMUX_SECTION_FILTER_LENGTH+2];
        memset(tsFilterConfiguration.setting.section_setting.mask, 0, DEMUX_SECTION_FILTER_LENGTH+2);

        tsFilterConfiguration.setting.section_setting.mode =new char[DEMUX_SECTION_FILTER_LENGTH];
        memset(tsFilterConfiguration.setting.section_setting.mode, 0, DEMUX_SECTION_FILTER_LENGTH);

        tsFilterConfiguration.pid = pid;
        tsFilterConfiguration.type = MAIN_TYPE_TS;
        tsFilterConfiguration.setting.section_setting.crc_enable = params->flags;
        tsFilterConfiguration.setting.section_setting.is_repeat = true;
        tsFilterConfiguration.setting.section_setting.is_raw = false;
        tsFilterConfiguration.setting.section_setting.filter[0] = params->filter.filter[0];
        tsFilterConfiguration.setting.section_setting.filter[3] = params->filter.filter[1];
        tsFilterConfiguration.setting.section_setting.filter[4] = params->filter.filter[2];
        tsFilterConfiguration.setting.section_setting.filter[5] = params->filter.filter[3];
        tsFilterConfiguration.setting.section_setting.filter[6] = params->filter.filter[4];
        tsFilterConfiguration.setting.section_setting.filter[7] = params->filter.filter[5];
        tsFilterConfiguration.setting.section_setting.filter[8] = params->filter.filter[6];
        tsFilterConfiguration.setting.section_setting.filter[9] = params->filter.filter[7];
        tsFilterConfiguration.setting.section_setting.filter[1] = 0xFF;
        tsFilterConfiguration.setting.section_setting.filter[2] = 0xFF;

        tsFilterConfiguration.setting.section_setting.filter_length = DEMUX_SECTION_FILTER_LENGTH+2;
        tsFilterConfiguration.setting.section_setting.mask[0] = params->filter.mask[0];
        tsFilterConfiguration.setting.section_setting.mask[3] = params->filter.mask[1];
        tsFilterConfiguration.setting.section_setting.mask[4] = params->filter.mask[2];
        tsFilterConfiguration.setting.section_setting.mask[5] = params->filter.mask[3];
        tsFilterConfiguration.setting.section_setting.mask[6] = params->filter.mask[4];
        tsFilterConfiguration.setting.section_setting.mask[7] = params->filter.mask[5];
        tsFilterConfiguration.setting.section_setting.mask[8] = params->filter.mask[5];
        tsFilterConfiguration.setting.section_setting.mask[9] = params->filter.mask[7];
        tsFilterConfiguration.setting.section_setting.mask[1] = 0xFF;
        tsFilterConfiguration.setting.section_setting.mask[2] = 0xFF;

        tsFilterConfiguration.setting.section_setting.mask_length = DEMUX_SECTION_FILTER_LENGTH+2;
        tsFilterConfiguration.setting.section_setting.mode_length = DEMUX_SECTION_FILTER_LENGTH;

        bool attached = false;
        JNIEnv *env = Am_tuner_getJNIEnv(&attached);
        if (NULL == env) {
            ALOGE("%s: input parameter error", __FUNCTION__);
            return ret;
        }

        jobject tsFilterConfigurationObject = filter_utils_getSectionTsFilterConfiguration(env, tsFilterConfiguration);
        if (tsFilterConfigurationObject)
        {
            ALOGD("DMX_HAL_%s  Jfilter %p pid [0x%x]",  __FUNCTION__, it->second->Jfilter, pid);
            it->second->pid = pid ;
            int result = Am_filter_configure((it->second->Jfilter), tsFilterConfigurationObject);
            if (result == RETURN_ERROR)
            {
                ReleaseEnv(attached);
                return FALSE;
            }
            ret = TRUE;
        }
        delete[] tsFilterConfiguration.setting.section_setting.filter;
        delete[] tsFilterConfiguration.setting.section_setting.mask;
        delete[] tsFilterConfiguration.setting.section_setting.mode;
        ReleaseEnv(attached);
    }
    return ret;
}

BOOLEAN DMX_StartFilter(int un32filterID )
{
    BOOLEAN ret = FALSE;
    pthread_mutex_lock( &gDMXTaskLocked.dmx_mutex);
    FILTER_MAP::iterator it = filter_map.find( un32filterID );
    pthread_mutex_unlock( &gDMXTaskLocked.dmx_mutex);
    if (it != filter_map.end())
    {
        if (it->second != NULL)
        {
            ALOGD("DMX_HAL_%s  filerInfo %p filterId 0x%x Jfilter %p pid [0x%x]",  __FUNCTION__,  it->second , un32filterID, it->second->Jfilter,it->second->pid);
            jint result = Am_filter_start(it->second->Jfilter);
            if (result == RETURN_ERROR)
            {
                return FALSE;
            }
            ret =  TRUE;
        }
    }
    return ret ;
}

BOOLEAN  DMX_StopFilter(int un32filterID )
{
    BOOLEAN ret = FALSE;
    pthread_mutex_lock( &gDMXTaskLocked.dmx_mutex);
    FILTER_MAP::iterator it = filter_map.find( un32filterID );
    pthread_mutex_unlock( &gDMXTaskLocked.dmx_mutex);
    if (it != filter_map.end())
    {
       if (it->second != NULL)
       {
           ALOGD("DMX_HAL_%s  filerInfo %p filterId 0x%x Jfilter %p pid [0x%x]",  __FUNCTION__,  it->second , un32filterID, it->second->Jfilter ,it->second->pid);
           jint result = Am_filter_stop(it->second->Jfilter);
           ret =  TRUE;
       }
    }
    return ret ;
}

BOOLEAN  DMX_FlushFilter(int un32filterID )
{
    BOOLEAN ret = FALSE;
    pthread_mutex_lock( &gDMXTaskLocked.dmx_mutex);
    FILTER_MAP::iterator it = filter_map.find( un32filterID );
    pthread_mutex_unlock( &gDMXTaskLocked.dmx_mutex);
    if (it != filter_map.end())
    {
       if (it->second != NULL)
       {
           ALOGD("DMX_HAL_%s  filerInfo %p filterId 0x%x Jfilter %p pid [0x%x]",  __FUNCTION__,  it->second , un32filterID, it->second->Jfilter ,it->second->pid);
           jint result = Am_filter_flush(it->second->Jfilter);
           ret =  TRUE;
       }
    }
    return ret ;
}

static int CiCamId;
static BOOLEAN Pass_through;
static int Listener;

static void tuner_status_listener (int tunerClientId, TUNER_LIFECYCLE_STATUS lifecycleStatus)
{
    if (lifecycleStatus == TUNER_CREATE)
    {
        if (Pass_through)
        {
            Am_tuner_connectCiCam(tunerClientId, CiCamId);
            Am_tuner_connectFrontendToCiCam(tunerClientId, CiCamId);
        }
    }
}

void DMX_Route_TS(int tuner_no, int cicamid,BOOLEAN pass_through)
{
    int i;
    CiCamId = cicamid;
    Pass_through = pass_through;
#if 0
    switch (tuner_no)
    {
        case 0:
        {
            tuner_type = TUNER_TYPE_LIVE_0;
            break ;
        }
        case 1:
        {
            tuner_type = TUNER_TYPE_LIVE_1;
            break ;
        }
        case 2 :
        {
            tuner_type = TUNER_TYPE_LIVE_2;
            break ;
        }
        case 4 :
        {
            tuner_type = TUNER_TYPE_DVR_PLAY;
            break ;
        }
        default:
        {
            tuner_type = TUNER_TYPE_LIVE_0;
            break ;
        }
    }
#endif

    for (i = TUNER_TYPE_LIVE_0; i <= TUNER_TYPE_BACKGROUND; i ++)
    {
        TUNER_TYPE tuner_type = (TUNER_TYPE)i;

        if (INVALID_TUNER_ID == Am_tuner_getTunerClientIdByType(tuner_type))
            continue;

        if (true == pass_through)
        {
            ALOGD("======>TS change to passthough @ %d" , tuner_type);
            Am_tuner_connectCiCam(Am_tuner_getTunerClientIdByType(tuner_type),cicamid);
            Am_tuner_connectFrontendToCiCam(Am_tuner_getTunerClientIdByType(tuner_type),cicamid);
        }
        else
        {
            ALOGD("======>TS change to bypass @ %d" , tuner_type );
            Am_tuner_disconnectCiCam(Am_tuner_getTunerClientIdByType(tuner_type));
            Am_tuner_disconnectFrontendToCiCam(Am_tuner_getTunerClientIdByType(tuner_type),cicamid);
        }
    }
    if (Listener == 0)
    {
        Am_tuner_addTunerLifeCycleListener((long)tuner_status_listener);
        Listener++;
    }

    ALOGD("END:%s", __FUNCTION__);
}



JCAS_JNI_RESULT MediaCAS_Init()
{
    return AM_CasManagerInit();
}

JCAS_JNI_RESULT MediaCAS_CreatePlugin(U8BIT path ,U16BIT source_type  ,U16BIT demux_cap  , AM_CasPluginInfo *casPluginInfo , CasHandle *casHandle)
{
    int ClientId = 0xFFFF;
    static jobject sTunerJcas = NULL;

    /*
    typedef enum
    {
        DMX_TUNER,
        DMX_1394,
        DMX_MEMORY
    } E_STB_DMX_DEMUX_SOURCE;
    */
    if (source_type  != 0)
    {
        ClientId = Am_tuner_getTunerClientIdByType(TUNER_TYPE_DVR_PLAY);
        ALOGD("start DMX_CAPS_PLAYBACK filter ClientId 0x%x",ClientId);
    }
    else
    {
        TUNER_TYPE tuner_type = TUNER_TYPE_LIVE_0;
        if (demux_cap == 0x0004)
        {
            tuner_type = TUNER_TYPE_DVR_RECORD;
        }
        else if (demux_cap == 0x0080)
        {
           tuner_type = TUNER_TYPE_DVR_TIMESHIFT_RECORD;
        }
        else
        {
            switch (path)
            {
                case 0:
                {
                    tuner_type = TUNER_TYPE_LIVE_0;
                    break ;
                }
                case 1:
                {
                    tuner_type = TUNER_TYPE_LIVE_1;
                    break ;
                }
                case 2 :
                {
                    tuner_type = TUNER_TYPE_LIVE_2;
                    break ;
                }
                default:
                {
                    tuner_type = TUNER_TYPE_LIVE_0;
                    break ;
                }
            }
        }
        ClientId = Am_tuner_getTunerClientIdByType(tuner_type);
        ALOGD("start MediaCAS_CreatePlugin filter path [%d] demux_cap [0x%x] ClientId[%d] tuner_type[%d]",path,demux_cap ,ClientId,tuner_type);
    }

    if (0xFF == path)
    {
        ALOGD("%s : get global ClientId ok = %d ", __FUNCTION__, ClientId);
        return AM_CreateCasPlugin(casPluginInfo, INVALID_TUNER_ID, casHandle);
    }
    else
    {
        ALOGD("%s : get ClientId ok = %d ", __FUNCTION__, ClientId);
        return AM_CreateCasPlugin(casPluginInfo, ClientId, casHandle);
    }
    #if 0
    sTunerJcas = Am_tuner_getOriginalTuner(ClientId);
    if (NULL != sTunerJcas)
    {
        ALOGD("%s : get TunerHandle ok = %p ", __FUNCTION__,sTunerJcas);
        return AM_CreateCasPlugin(casPluginInfo, ClientId, casHandle);
    }
    else
    {
        ALOGD("%s : get TunerHandle fail", __FUNCTION__);
        return AM_CAS_JNI_ERR_BASE;
    }
    #endif
}

BOOLEAN MediaCAS_IsSystemIdSupported(int caSystemId)
{
    return AM_IsSystemIdSupported(caSystemId);
}

JCAS_JNI_RESULT MediaCAS_OpenCasSession(CasHandle casHandle, AM_CasSessionInfo *casSessionInfo,
        CasSessionHandle* casSessionHandle)
{
    return AM_OpenCasSession(casHandle, casSessionInfo, casSessionHandle);
}

JCAS_JNI_RESULT MediaCAS_StartDescrambling(CasHandle casHandle, CasSessionHandle casSessionHandle)
{
    return AM_StartDescrambling(casHandle, casSessionHandle);
}

JCAS_JNI_RESULT MediaCAS_StopDescrambling(CasHandle casHandle, CasSessionHandle casSessionHandle)
{
    return AM_StopDescrambling(casHandle, casSessionHandle);
}

JCAS_JNI_RESULT MediaCAS_CloseCasSession(CasHandle casHandle, CasSessionHandle casSessionHandle)
{
    return AM_CloseCasSession(casHandle, casSessionHandle);
}

JCAS_JNI_RESULT MediaCAS_DestroyCasPlugin(CasHandle casHandle)
{
    return AM_DestroyCasPlugin(casHandle);
}

JCAS_JNI_RESULT MediaCAS_CasManagerTerm()
{
    return AM_CasManagerTerm();
}

JCAS_JNI_RESULT MediaCAS_SendCommand(CasHandle casHandle, int event, int arg, uint8_t* data, int dataLen)
{
    return AM_SendCommand(casHandle, event, arg, data, dataLen);
}

JCAS_JNI_RESULT MediaCAS_SendSessionCommand(CasHandle casHandle, CasSessionHandle casSessionHandle, int event, int arg, uint8_t* data, int dataLen)
{
    return AM_SendSessionCommand(casHandle, casSessionHandle, event, arg, data, dataLen);
}

JCAS_JNI_RESULT MediaCAS_GetDefaultCaSystemIds(int* caSystemIds)
{
    return AM_GetDefaultCaSystemIds(caSystemIds);
}
////////////////////////////
 //tuner hal flow
// open descramble
//////////////////////////////
//tuner hal flow
//setKeyToken
//add pid here
///////////////////////////////
//tuner hal flow
//remove pid
//close descramble

jobject DESCRAMBLE_Open(U8BIT path ,   U16BIT source_type  ,U16BIT demux_cap)
{
    ALOGD("IN:%s", __FUNCTION__);
    jobject handle = NULL;
    int ClientId = 0xFF;
    /*
    typedef enum
    {
        DMX_TUNER,
        DMX_1394,
        DMX_MEMORY
    } E_STB_DMX_DEMUX_SOURCE;
    */
    ALOGD("%s source_type %d demux_cap %d path %d", __FUNCTION__,source_type ,demux_cap,path);
    if (source_type  != 0)
    {
        ClientId = Am_tuner_getTunerClientIdByType(TUNER_TYPE_DVR_PLAY);
        ALOGD("DESCRAMBLE_Open start DMX_CAPS_PLAYBACK filter ClientId 0x%x",ClientId);
    }
    else
    {
        TUNER_TYPE tuner_type = TUNER_TYPE_LIVE_0;
        /*
        DMX_CAPS_RECORDING = 0x04,
        */
        if (demux_cap == 0x04)
        {
            tuner_type = TUNER_TYPE_DVR_RECORD ;
        }
        else if (demux_cap == 0x80)
        {
            tuner_type = TUNER_TYPE_DVR_TIMESHIFT_RECORD;
        }
        else
        {
            switch (path)
            {
                case 0:
                {
                    tuner_type = TUNER_TYPE_LIVE_0;
                    break ;
                }
                case 1:
                {
                    tuner_type = TUNER_TYPE_LIVE_1;
                    break ;
                }
                case 2 :
                {
                    tuner_type = TUNER_TYPE_LIVE_2;
                    break ;
                }
                default:
                {
                    tuner_type = TUNER_TYPE_LIVE_0;
                    break ;
                }
            }
        }
        ALOGD("%s tuner_type %d", __FUNCTION__,tuner_type);
        ClientId = Am_tuner_getTunerClientIdByType(tuner_type);
    }
    handle =  Am_tuner_openDescrambler(ClientId) ;
    ALOGD("OUT:%s handle%p", __FUNCTION__,handle);
    return handle;
}
void DESCRAMBLE_AddPid( jobject handle, int pid)
{
// rule define here :
//public static final int PID_TYPE_T = 1;
//public static final int PID_TYPE_MMTP = 2;
    ALOGD("IN :%s descramble_handle%p", __FUNCTION__,handle);
    Am_descrambler_addPid(handle, 1, pid,NULL);
    ALOGD("OUT:%s", __FUNCTION__);
}
void DESCRAMBLE_RemovePid(jobject handle, int pid)
{
    ALOGD("IN :%s descramble_handle%p", __FUNCTION__,handle);
    Am_descrambler_removePid(handle, 1, pid,NULL);
    ALOGD("OUT:%s", __FUNCTION__);
}
void DESCRAMBLE_SetKeyToken(jobject handle,uint32_t token)
{
//tuner hal define rule:
//    for (int token_idx = sizeof(mCasSessionToken) - 1; token_idx >= 0; --token_idx) {
 //       mCasSessionToken = (mCasSessionToken << 8) | keyToken[token_idx];


 //android_media_tv_Tuner_descrambler_set_key_token
//env->GetByteArrayRegion(keyToken, 0, size, reinterpret_cast<jbyte*>(&v[0]));
    ALOGD("IN :%s descramble_handle%p", __FUNCTION__,handle);
    std::vector<char> keyToken;
    keyToken.push_back((char)(token & 0xFF));
    keyToken.push_back((char)((token & 0xFF00) >> 8));
    keyToken.push_back((char)((token & 0xFF0000) >> 16));
    keyToken.push_back((char)((token >> 24) & 0xFF));
    Am_descrambler_setKeyToken(handle, keyToken);
    ALOGD("OUT:%s", __FUNCTION__);
}
void DESCRAMBLE_close(jobject handle)
{
    ALOGD("IN :%s descramble_handle%p", __FUNCTION__,handle);
    Am_descrambler_close(handle);
    ALOGD("OUT:%s", __FUNCTION__);
}


