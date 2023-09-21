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

#define LOG_TAG "DTVKIT_LOG"

typedef struct s_pid_hal
{
    jobject Jfilter;
    filter_callback cb;
    void* user_data;
} S_HAL;
using namespace std;

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
        package.cb(&package.para);
        //ALOGD("%s pidcallback is %d", __FUNCTION__, package.para.un32filterID);
    }
    ALOGD("end:%s", __FUNCTION__);
}

void FilterCallback(jobject filter, jobjectArray filterEventArray, int filterStatus) {
    //ALOGD("start:%s", __FUNCTION__);
    bool attached = false;
    JNIEnv *env = Am_tuner_getJNIEnv(&attached);
    if (NULL == env) {
        ALOGD("%s : test fail, env is null", __FUNCTION__);
        return;
    }
    //show filter status
    //ALOGD("filterStatus : %d", filterStatus);

    //handle Filter Event
    //pthread_mutex_lock( &gDMXTaskLocked.dmx_mutex);
    if (NULL != filterEventArray) {
        int eventSize = env->GetArrayLength(filterEventArray);
        for (int index = 0; index < eventSize; index++) {
            //1.check section event
            jobject filterEvent = env->GetObjectArrayElement(filterEventArray, index);
            Section_Event stSectionEvent;
            memset(&stSectionEvent, 0, sizeof(Section_Event));
            filter_utils_getSectionEvent(env, filterEvent, &stSectionEvent);

            //ALOGD("tableId :%d, version :%d, section num :%d, data length :%d", stSectionEvent.tableId, stSectionEvent.version,
            //    stSectionEvent.sectionNum, stSectionEvent.dataLength);

            //3.read section data
            char *buffer = new char[stSectionEvent.dataLength];
            int readSize = Am_filter_read(filter, buffer, 0, stSectionEvent.dataLength);

            FILTER_MAP::iterator it = filter_map.find( Am_filter_getId(filter) );
            if (it != filter_map.end())
            {
                S_PID_FILTER_INFO* user_data = (S_PID_FILTER_INFO*)it->second->user_data;
                //ALOGD("user_data.index = %d, user_data.pid = %d, handle =%d,user_data = %p", user_data->index, user_data->pid, user_data->fhandle, user_data);
                //filter_callback callback = it->second->cb;
                ST_CALLBACK_T para;
                para.un32filterID = Am_filter_getId(filter) ;
                para.pun8_buffer = (uint8_t *)buffer ;
                para.un32_length =  readSize;
                user_data->fhandle = Am_filter_getId(filter);
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
            //ALOGD("read callback data size :%d ", readSize);
            if (readSize > stSectionEvent.dataLength) {
                ALOGD("%s : test fail, read data too long than real data size", __FUNCTION__);
            } else {
                /*for (int i = 0; i < readSize; i++) {
                    //ALOGD("0X%x ", buffer[i]);
                }*/

            }
            delete[] buffer;
        }
    }
    if (attached) {
        Am_tuner_detachJNIEnv();
    }
    //pthread_mutex_unlock( &gDMXTaskLocked.dmx_mutex);
    //ALOGD("end:%s", __FUNCTION__);
}

int DMX_OpenFilter(U8BIT path, filter_callback cb, void* user_data,U16BIT type)
{
    ALOGD("start:%s", __FUNCTION__);
    int ClientId = 0xFF;
    if (!gDMXTaskLocked.initDmxLocked )
    {
        gDMXTaskLocked.initDmxLocked = true;
        gDMXTaskLocked.dmx_exit = false;
        pthread_mutex_init( &gDMXTaskLocked.dmx_mutex, NULL);
    }

    //pthread_mutex_lock( &gDMXTaskLocked.dmx_mutex);
    if (type != 0)
    {
        ClientId = Am_tuner_getTunerClientIdByType(TUNER_TYPE_DVR_PLAY);
        ALOGD("===start DMX_CAPS_PLAYBACK filter:%s  ClientId 0x%x", __FUNCTION__,ClientId);
    }
    else
    {
        TUNER_TYPE object_id = TUNER_TYPE_DEFAULT;
        switch (path)
        {
            case 0:
            {
                object_id = TUNER_TYPE_DEFAULT;
                break ;
            }
            case 1:
            {
                object_id = TUNER_TYPE_FCC_TUNE_PREV;
                break ;
            }
            case 2 :
            {
                object_id = TUNER_TYPE_FCC_TUNE_NEXT;
                break ;
            }
            default:
            {
                object_id = TUNER_TYPE_DEFAULT;
                break ;
            }
        }
        ClientId = Am_tuner_getTunerClientIdByType(object_id);
        //ClientId = Am_tuner_getTunerClientIdByType(TUNER_TYPE_DEFAULT);
        ALOGD("===%s start DMX_CAPS_Live filtertuner_path [%d] ClientId[%d] ClientId[%d]", __FUNCTION__,path,ClientId,object_id);
    }
    Am_filter_callback filterCallback = FilterCallback;
    S_HAL *filerInfo;
    filerInfo = new S_HAL();
    filerInfo->cb = cb ;
    filerInfo->Jfilter = Am_tuner_openFilter(ClientId, 1, 1, 8 * 4096, (long)filterCallback);
    filerInfo->user_data  = user_data;
    int filterId = Am_filter_getId(filerInfo->Jfilter);
    FILTER_MAP::iterator it = filter_map.find(filterId);
    if (it != filter_map.end())
    {
        filter_map.erase(it);
        MAP_INSERT_ITEM( filter_map, filterId, filerInfo );
        ALOGI("%s Already Find filterId: %d  update from map.", __FUNCTION__, filterId);
    }
    else
    {
        MAP_INSERT_ITEM( filter_map, filterId, filerInfo );
        ALOGI("%s instert new filerInfo filterId: %d.", __FUNCTION__, filterId);
    }
    ALOGI("==%s  filerInfo %p filerInfo.Jfilter %p, filerInfo->user_data %p", __FUNCTION__, filerInfo, filerInfo->Jfilter, filerInfo->user_data);
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
    //pthread_mutex_unlock( &gDMXTaskLocked.dmx_mutex);
    ALOGD("end:%s", __FUNCTION__);
    return filterId ;
}

BOOLEAN DMX_CloseFilter(int un32filterID)
{
    ALOGD("start:%s", __FUNCTION__);
    //pthread_mutex_lock( &gDMXTaskLocked.dmx_mutex);
    BOOLEAN ret = FALSE;
    FILTER_MAP::iterator it = filter_map.find( un32filterID );
    if (it != filter_map.end())
    {
        ALOGD("DMX_CloseFilter:%d %p", un32filterID,it->second);
        Am_filter_close(it->second->Jfilter);
        if (it->second != NULL)
         {
            delete it->second;
            it->second = NULL;
        }
        filter_map.erase( un32filterID );
        ret =  TRUE;
    }
    ALOGD("end:%s", __FUNCTION__);
    //pthread_mutex_unlock( &gDMXTaskLocked.dmx_mutex);
    return ret ;
}

BOOLEAN DMX_SetupFilter(int un32filterID ,U16BIT pid,S_SECTION_FILTER_INFO* params )
{
    ALOGD("start:%s", __FUNCTION__);
    BOOLEAN ret = FALSE;
    char mode[3] = {0, 0, 0};
    //pthread_mutex_lock( &gDMXTaskLocked.dmx_mutex);
    FILTER_MAP::iterator it = filter_map.find( un32filterID );
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
        tsFilterConfiguration.setting.section_setting.crc_enable = params->check_crc;
        tsFilterConfiguration.setting.section_setting.is_repeat = true;
        tsFilterConfiguration.setting.section_setting.is_raw = false;
        tsFilterConfiguration.setting.section_setting.filter[0] = params->match[0];
        tsFilterConfiguration.setting.section_setting.filter[3] = params->match[1];
        tsFilterConfiguration.setting.section_setting.filter[4] = params->match[2];
        tsFilterConfiguration.setting.section_setting.filter[5] = params->match[3];
        tsFilterConfiguration.setting.section_setting.filter[6] = params->match[4];
        tsFilterConfiguration.setting.section_setting.filter[7] = params->match[5];
        tsFilterConfiguration.setting.section_setting.filter[8] = params->match[6];
        tsFilterConfiguration.setting.section_setting.filter[9] = params->match[7];
        tsFilterConfiguration.setting.section_setting.filter[1] = 0xFF;
        tsFilterConfiguration.setting.section_setting.filter[2] = 0xFF;

        tsFilterConfiguration.setting.section_setting.filter_length = DEMUX_SECTION_FILTER_LENGTH+2;
        tsFilterConfiguration.setting.section_setting.mask[0] = params->mask[0];
        tsFilterConfiguration.setting.section_setting.mask[3] = params->mask[1];
        tsFilterConfiguration.setting.section_setting.mask[4] = params->mask[2];
        tsFilterConfiguration.setting.section_setting.mask[5] = params->mask[3];
        tsFilterConfiguration.setting.section_setting.mask[6] = params->mask[4];
        tsFilterConfiguration.setting.section_setting.mask[7] = params->mask[5];
        tsFilterConfiguration.setting.section_setting.mask[8] = params->mask[5];
        tsFilterConfiguration.setting.section_setting.mask[9] = params->mask[7];
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
            int result = Am_filter_configure((it->second->Jfilter), tsFilterConfigurationObject);
            ret = TRUE;
        }
        delete[] tsFilterConfiguration.setting.section_setting.filter;
        delete[] tsFilterConfiguration.setting.section_setting.mask;
        delete[] tsFilterConfiguration.setting.section_setting.mode;
        ReleaseEnv(attached);
    }
    //pthread_mutex_unlock( &gDMXTaskLocked.dmx_mutex);
    ALOGD("end:%s", __FUNCTION__);
    return ret;
}

BOOLEAN DMX_StartFilter(int un32filterID )
{
    ALOGD("start:%s", __FUNCTION__);
    BOOLEAN ret = FALSE;
    //pthread_mutex_lock( &gDMXTaskLocked.dmx_mutex);
    FILTER_MAP::iterator it = filter_map.find( un32filterID );
    if (it != filter_map.end())
    {
        if (it->second != NULL)
        {
            jint result = Am_filter_start(it->second->Jfilter);
            ret =  TRUE;
        }
    }
    //pthread_mutex_unlock( &gDMXTaskLocked.dmx_mutex);
    ALOGD("end:%s", __FUNCTION__);
    return ret ;
}

BOOLEAN  DMX_StopFilter(int un32filterID )
{
    ALOGD("start:%s", __FUNCTION__);
    BOOLEAN ret = FALSE;
    //pthread_mutex_lock( &gDMXTaskLocked.dmx_mutex);
    FILTER_MAP::iterator it = filter_map.find( un32filterID );
    if (it != filter_map.end())
    {
       if (it->second != NULL)
       {
           jint result = Am_filter_stop(it->second->Jfilter);
           ret =  TRUE;
       }
    }
    //pthread_mutex_unlock( &gDMXTaskLocked.dmx_mutex);
    ALOGD("end:%s", __FUNCTION__);
    return ret ;
}


void DMX_Route_TS(int cicamid,BOOLEAN pass_through)
{
    if (INVALID_TUNER_ID == Am_tuner_getTunerClientId())
    {
        ALOGD("%s : gTunerClient is invalid", __FUNCTION__);
        return ;
    }

    if (true == pass_through)
    {
        ALOGD("======>TS change to passthough");
        Am_tuner_connectCiCam(Am_tuner_getTunerClientId(),cicamid);
        Am_tuner_connectFrontendToCiCam(Am_tuner_getTunerClientId(),cicamid);
    }
    else
    {
        ALOGD("======>TS change to bypass");
        Am_tuner_disconnectCiCam(Am_tuner_getTunerClientId());
        Am_tuner_disconnectFrontendToCiCam(Am_tuner_getTunerClientId(),cicamid);
    }
    ALOGD("END:%s", __FUNCTION__);
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

jobject DESCRAMBLE_Open()
{
    ALOGD("IN:%s", __FUNCTION__);
    jobject handle = NULL;
    handle =  Am_tuner_openDescrambler(Am_tuner_getTunerClientId()) ;
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


