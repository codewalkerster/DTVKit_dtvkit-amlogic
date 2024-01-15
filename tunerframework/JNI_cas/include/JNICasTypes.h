#ifndef AM_JNI_CAS_TYPES_H
#define AM_JNI_CAS_TYPES_H

#include <stdbool.h>

#define MAX_PRIVATE_DATA_SIZE 512
#define MAX_SCRAMBLED_ES_NUM 8

typedef size_t CasHandle;
typedef size_t CasSessionHandle;

typedef enum {
    AM_CAS_JNI_OK  = 0,
    AM_CAS_JNI_ERR_BASE = 100,
    AM_CAS_JNI_ERR = AM_CAS_JNI_ERR_BASE,
    AM_CAS_JNI_PARAM_ERR = AM_CAS_JNI_ERR_BASE + 1,
    AM_CAS_JNI_NOT_IMPLEMENT_ERR = AM_CAS_JNI_ERR_BASE + 2,
} JCAS_JNI_RESULT;

/**CAS callback.*/
typedef JCAS_JNI_RESULT (*CAS_Callback_t)(CasHandle casHandle, int event, int args, uint8_t* data, int dataLen);
typedef JCAS_JNI_RESULT (*CAS_SessionCallback_t)(CasHandle casHandle, CasSessionHandle sessionHandle, int event, int args, uint8_t* data, int dataLen);

typedef enum {
    LIVE,
    PLAYBACK,
    RECORD,
    TIMESHIFT,
} SessionIntent;

typedef enum {
    RESERVED = 0,
    DVB_CSA1,
    DVB_CSA2,
    DVB_CSA3_STANDARD,
    DVB_CSA3_MINIMAL,
    DVB_CSA3_ENHANCE,
    DVB_CISSA_V1,
    DVB_IDSA,
    MULTI2,
    AES128,
    AES_ECB,
    AES_SCTE52,
    TDES_ECB,
    TDES_SCTE52,
    AES_CBC,
} ScramblingMode;

/**
 * CasEvent types
 */
typedef enum {
    /*Used for notify Cas Vendor provider event*/
    CAS_EVENT_TYPE_PROVIDER = 4,
    /*Used for notify TvInputManager event*/
    CAS_EVENT_TYPE_STATUS = 5,
    /*Used for notify content rating event*/
    CAS_EVENT_TYPE_CONTENT_RATING = 6,
} CasEventType;

/**
 * Provider event arg to share TV app
 */
typedef enum {
    PROVIDER_EVENT_TYPE_SYSTEM_INFO = 0,
    PROVIDER_EVENT_TYPE_ENTITLEMENT_INFO = 1,
    PROVIDER_EVENT_TYPE_USAGE_RULES = 2,
    PROVIDER_EVENT_TYPE_FINGERPRINT = 3,
    PROVIDER_EVENT_TYPE_WATERMARKING = 4,
    PROVIDER_EVENT_TYPE_IRD_COMMAND = 5,
    PROVIDER_EVENT_TYPE_STATUS = 6,
    PROVIDER_EVENT_TYPE_SESSION_STATUS = 7,
    PROVIDER_EVENT_TYPE_PLUGIN_STATUS = 8,
    PROVIDER_EVENT_TYPE_PROVISION_STATUS = 9,
    PROVIDER_EVENT_TYPE_LICENSE_STATUS = 10,
    PROVIDER_EVENT_TYPE_PLUGIN_VERSION = 11,
    PROVIDER_EVENT_TYPE_CLEAR_SESSION = 12,
    PROVIDER_EVENT_TYPE_CLEAR_CONNECTION = 13,
    PROVIDER_EVENT_TYPE_EXTENSION_INFO = 14,
    PROVIDER_EVENT_TYPE_UNKNOWN = 99,
} CasProviderEventArg;

typedef struct {
    int caSystemId;
    int tisSessionId;
    SessionIntent tisUseCase;
    /*Cas Event Callback*/
    CAS_Callback_t casCallback;
    /*Cas Session Event Callback*/
    CAS_SessionCallback_t casSessionCallback;
} AM_CasPluginInfo;

typedef struct {
    AM_CasPluginInfo casPluginInfo;
    int ecmPid;
    ScramblingMode scramblingMode;
    BOOLEAN isProgramLevel;
    uint8_t privateData[MAX_PRIVATE_DATA_SIZE];
    int privateDataLen;
    uint16_t scrambledEsPids[MAX_SCRAMBLED_ES_NUM];
    int scrambledEsNum;
} AM_CasSessionInfo;

#endif //AM_JNI_CAS_TYPES_H
