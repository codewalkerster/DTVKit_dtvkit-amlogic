#ifndef AM_JNI_CAS_TYPES_H
#define AM_JNI_CAS_TYPES_H

#include <stdbool.h>

#define MAX_PRIVATE_DATA_SIZE 512
#define MAX_SCRAMBLED_ES_NUM 8

typedef size_t CasHandle;
typedef size_t CasSessionHandle;
typedef void* TunerObject;

typedef enum {
    AM_CAS_JNI_OK  = 0,
    AM_CAS_JNI_ERR_BASE = 100,
    AM_CAS_JNI_ERR = AM_CAS_JNI_ERR_BASE,
    AM_CAS_JNI_PARAM_ERR = AM_CAS_JNI_ERR_BASE + 1,
    AM_CAS_JNI_NOT_IMPLEMENT_ERR = AM_CAS_JNI_ERR_BASE + 2,
} JCAS_JNI_RESULT;

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

typedef struct {
    int caSystemId;
    int tisSessionId;
    SessionIntent tisUseCase;
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
