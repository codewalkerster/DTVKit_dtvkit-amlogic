#ifndef _TF_DMX_H
#define _TF_DMX_H
#include <jni.h>
#include "techtype.h"
#include "wrapper_os.h"
#include "dmx.h"
#include "JNICasTypes.h"
#include "JNICasWrapper.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEMUX_SECTION_FILTER_LENGTH 8
#define MAX_SECTION_FILTERS         16
#define MAX_FILTERS_PER_PID         8


typedef struct _ST_CALLBACK_T
{

    int un32filterID;
    U8BIT *pun8_buffer;
    U32BIT un32_length;
    void* un32_userdata;
} ST_CALLBACK_T;

typedef void(*filter_callback)( ST_CALLBACK_T *param) ;
typedef void (*FILTER_CALLBACK)( U8BIT path, U16BIT bytes, U16BIT pfilt_id );

/**\brief Input source of the demux*/
typedef struct _S_SECTION_FILTER_INFO
{
   BOOLEAN in_use;
   U8BIT match[DEMUX_SECTION_FILTER_LENGTH];
   U8BIT mask[DEMUX_SECTION_FILTER_LENGTH];
   BOOLEAN check_crc;
   BOOLEAN setup;
   BOOLEAN empty_mask;
} S_SECTION_FILTER_INFO;

typedef struct s_pid_filter_info
{
   U8BIT path;
   U8BIT index;
   U16BIT pid;
   int fhandle;
   BOOLEAN started;
   const U8BIT* data_packet;
   U16BIT data_packet_size;
   S_SECTION_FILTER_INFO section_filters[MAX_SECTION_FILTERS];
   FILTER_CALLBACK func_ptr[MAX_FILTERS_PER_PID];
   U8BIT start_count[MAX_FILTERS_PER_PID];
} S_PID_FILTER_INFO;

int DMX_OpenFilter(U8BIT path, filter_callback cb, void* user_data,U16BIT demux_source,U16BIT demux_cap ,U32BIT section_size);

BOOLEAN DMX_CloseFilter(int un32filterID);
BOOLEAN DMX_SetupFilter(int un32filterID, U16BIT pid, const struct dmx_sct_filter_params *params);
BOOLEAN DMX_StartFilter(int un32filterID );
BOOLEAN DMX_StopFilter(int un32filterID );
BOOLEAN DMX_FlushFilter(int un32filterID );


void DMX_Route_TS(int cicamid,BOOLEAN pass_through);

JCAS_JNI_RESULT MediaCAS_Init();

JCAS_JNI_RESULT MediaCAS_CreatePlugin(U8BIT path , AM_CasPluginInfo *casPluginInfo , CasHandle *casHandle);

BOOLEAN MediaCAS_IsSystemIdSupported(int caSystemId);

JCAS_JNI_RESULT MediaCAS_OpenCasSession(CasHandle casHandle, AM_CasSessionInfo *casSessionInfo,
        CasSessionHandle* casSessionHandle);

JCAS_JNI_RESULT MediaCAS_StartDescrambling(CasHandle casHandle, CasSessionHandle casSessionHandle);

JCAS_JNI_RESULT MediaCAS_StopDescrambling(CasHandle casHandle, CasSessionHandle casSessionHandle);

JCAS_JNI_RESULT MediaCAS_CloseCasSession(CasHandle casHandle, CasSessionHandle casSessionHandle);

JCAS_JNI_RESULT MediaCAS_DestroyCasPlugin(CasHandle casHandle);

JCAS_JNI_RESULT MediaCAS_CasManagerTerm();

JCAS_JNI_RESULT MediaCAS_SendCommand(CasHandle casHandle, int event, int arg, uint8_t* data, int dataLen);

JCAS_JNI_RESULT MediaCAS_SendSessionCommand(CasHandle casHandle, CasSessionHandle casSessionHandle, int event, int arg, uint8_t* data, int dataLen);


jobject DESCRAMBLE_Open();
void DESCRAMBLE_AddPid( jobject handle, int pid);
void DESCRAMBLE_RemovePid(jobject handle, int pid);
void DESCRAMBLE_SetKeyToken(jobject handle,uint32_t token);
void DESCRAMBLE_close(jobject handle);

#ifdef __cplusplus
}
#endif

#endif


