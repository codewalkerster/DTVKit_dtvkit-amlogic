/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _DROIDLOGIC_DTVKIT_TUNER_
#define _DROIDLOGIC_DTVKIT_TUNER_
#include <jni.h>
#include <utils/Log.h>
#include <utils/RefBase.h>
#include <assert.h>
#include <string>
#include <vector>
#include <sys/system_properties.h>

#define TUNER_CLASS "droidlogic/dtvkit/tuner/TunerAdapter"
#define FILTER_CLASS "droidlogic/dtvkit/tuner/FilterAdapter"
#define LNB_CLASS "droidlogic/dtvkit/tuner/LnbAdapter"
#define DESCRAMBLER_CLASS "droidlogic/dtvkit/tuner/DescramblerAdapter"

#define INVALID_VALUE  0xFFFF;
#define ANDROID_SDK_R 30
#define ANDROID_SDK_T 33

using namespace android;
using ::android::sp;
using ::android::RefBase;


/**
* Extended Frontend Type.
*/
enum FRONTEND_TYPE {
    UNDEFINED = 0,
    ANALOG,
    ATSC,
    ATSC3,
    DVBC,
    DVBS,
    DVBT,
    ISDBS,
    ISDBS3,
    ISDBT,
    DTMB,
};

enum TUNER_CONSTANT{
    TUNER_CONSTANT_INVALID_TUNER_CLIENT_ID = 0xFFFF,
    /**
     * An invalid packet ID in transport stream according to ISO/IEC 13818-1.
     */
    TUNER_CONSTANT_INVALID_TS_PID = 0xFFFF,

    /**
     * An invalid Stream ID.
     */
    TUNER_CONSTANT_INVALID_STREAM_ID = 0xFFFF,

    /**
     * An invalid Filter ID.
     */
    TUNER_CONSTANT_INVALID_FILTER_ID = 0xFFFFFFFF,

    /**
     * An invalid AV sync hardware ID.
     */
    TUNER_CONSTANT_INVALID_AV_SYNC_ID = 0xFFFFFFFF,

    /**
     * An invalid mpuSequenceNumber.
     */
    TUNER_CONSTANT_INVALID_MMTP_RECORD_EVENT_MPT_SEQUENCE_NUM = 0xFFFFFFFF,

    /**
     * An invalid first macroblock address.
     */
    TUNER_CONSTANT_INVALID_FIRST_MACROBLOCK_IN_SLICE = 0xFFFFFFFF,

    /**
     * An invalid frequency that can be used as the default value of the frontend setting.
     */
    TUNER_CONSTANT_INVALID_FRONTEND_SETTING_FREQUENCY = 0xFFFFFFFF,

    /**
     * An invalid context id that can be used as the default value of the unconfigured id. It can
     * be used to reset the configured ip context id.
     */
    TUNER_CONSTANT_INVALID_IP_FILTER_CONTEXT_ID = 0xFFFFFFFF,

    /**
     * An invalid local transport stream id used as the return value on a failed operation of
     * IFrontend.linkCiCam.
     */
    TUNER_CONSTANT_INVALID_LTS_ID = 0xFFFFFFFF,

    /**
     * An invalid frontend ID.
     */
    TUNER_CONSTANT_INVALID_FRONTEND_ID = 0xFFFFFFFF,

    /**
     * An invalid LNB ID.
     */
    TUNER_CONSTANT_INVALID_LNB_ID = 0xFFFFFFFF,

    /**
     * An invalid key token. It is used to remove the current key from the descrambler.
     */
    TUNER_CONSTANT_INVALID_KEYTOKEN = 0x00,

     /**
     * An invalid section filter version number.
     */
    TUNER_CONSTANT_INVALID_TABINFO_VERSION = 0xFFFFFFFF,
};

enum TUNER_LIFECYCLE_STATUS {
    TUNER_RELEASE = 0,
    TUNER_CREATE = 1,
};

enum TUNER_TYPE {
    TUNER_TYPE_LIVE_0               = 0,
    TUNER_TYPE_LIVE_1               = 1,
    TUNER_TYPE_DVR_RECORD           = 2,
    TUNER_TYPE_DVR_TIMESHIFT_RECORD = 3,
    TUNER_TYPE_DVR_PLAY             = 4,
    TUNER_TYPE_SCAN                 = 5,
    TUNER_TYPE_LIVE_2               = 6,
    TUNER_TYPE_BACKGROUND           = 7,
};
/*
struct AMTuner : public RefBase {
    AMTuner(JNIEnv *env, jobject thiz);
    ~AMTuner();
    jobject getTuner();
private:
    jweak mTuner;
};
*/

/**
 * get open tuner client id.
 * @return tuner client id.
 */
int Am_tuner_getTunerClientId();

/**
 * get open tuner client id by type.
 * @param tunerType see@enmu TUNER_TYPE.
 * @return tuner client id.
 */
int Am_tuner_getTunerClientIdByType(int tunerType);

/**
 * get tuner object by client id.
 * @param tunerClientId the current open tuner clientId.
 * @return tuner jobject or null.
 */
jobject Am_tuner_getTunerObjectByClientId(int clientId);

/**
 * get original tuner object.
 * @param tunerClientId the current open tuner clientId.
 * @return tuner jobject or null.
 */
jobject Am_tuner_getOriginalTuner(int tunerClientId);

/**
 * get record tuner object.
 * @return record tuner jobject or null.
 */
jobject Am_tuner_getRecordTuner();

/**
 * get dvr tuner object.
 * @param tunerType see@enmu TUNER_TYPE.
 * @return dvr tuner jobject or null.
 */
jobject Am_tuner_getDvrTunerByType(int tunerType);

/**
 * register listener to receive tuner jobject life cycle status change.
 * @param listenerContext The listener context.
 * @return null.
 */
void Am_tuner_addTunerLifeCycleListener(long listenerContext);

/**
 * remove listener which receive tuner jobject life cycle status change.
 * @param listenerContext The listener context.
 * @return null.
 */
void Am_tuner_removeTunerLifeCycleListener(long listenerContext);

/**
 * get frontend id.
 *
 * @param tunerClientId the current open tuner clientId.
 * @return frontend id list.
 * @jni local reference corresponds to the java reference List<Integer>.
 */
jobject Am_tuner_getFrontendIds(int tunerClientId);

/**
 * Gets the frontend information.
 *
 * @param tunerClientId the current open tuner clientId.
 * @return the frontend information.
 * @jni local reference corresponds to the java reference FrontendInfo.
 */
jobject Am_tuner_getFrontendInfo(int tunerClientId);

/**
 * Gets the frontend information by id.
 *
 * @param tunerClientId the current open tuner clientId.
 * @param id frontend id.
 * @return the frontend information.
 * @jni local reference corresponds to the java reference FrontendInfo.
 */
jobject Am_tuner_getFrontendInfoById(int tunerClientId, int id);

/**
 * Tunes the frontend to using the settings given.
 *
 * @param tunerClientId the current open tuner clientId.
 * @param frontendSettings settings Signal delivery information the frontend uses to
 *  search and lock the signal.corresponds to the java reference FrontendSettings.
 * @return result status of tune operation.
 */
jint Am_tuner_tune(int tunerClientId, jobject frontendSettings);

/**
 * Stops a previous tuning.
 *
 * @param tunerClientId the current open tuner clientId.
 * @return result status of tune operation.
 */
jint Am_tuner_cancelTuning(int tunerClientId);

/**
 * Scan for channels..
 *
 * @param tunerClientId the current open tuner clientId.
 * @param frontendSettings settings Signal delivery information the frontend uses to
 *  search and lock the signal.corresponds to the java reference FrontendSettings.
 * @param scanType The scan type.
 * @param callbackContext The scan message callback.
 * @return result status of tune operation.
 */
jint Am_tuner_scan(int tunerClientId, jobject frontendSettings, int scanType, long callbackContext);

/**
 * Stops scan and delete scan callback.
 *
 * @param tunerClientId the current open tuner clientId.
 * @return result status of tune operation.
 */
jint Am_tuner_cancelScanning(int tunerClientId);

/**
 * release frontend resource.
 *
 * @param tunerClientId the current open tuner clientId.
 * @return.
 */
void Am_tuner_closeFrontend(int tunerClientId);

/**
 * close tuner instance to release tuner resource.
 *
 * @param tunerClientId the current open tuner clientId.
 * @return.
 */
void Am_tuner_close(int tunerClientId);

/**
 * Gets the statuses of the frontend.
 *
 * @param tunerClientId the current open tuner clientId.
 * @param statusTypes an array of status types which the caller requests.
 * @return statuses which response the caller's requests.
 * @jni local reference corresponds to the java reference FrontendStatus.
 */
jobject Am_tuner_getFrontendStatus(int tunerClientId, jintArray statusTypes);

/**
 * Listens for tune events.
 *
 * @param tunerClientId the current open tuner clientId.
 * @param callbackContext native callback context.
 */
void Am_tuner_setOnTuneEventListener(int tunerClientId, long callbackContext);

/**
 * stop to listen for tune events.
 *
 * @param tunerClientId the current open tuner clientId.
 */
void Am_tuner_clearOnTuneEventListener(int tunerClientId);

/**
 * Opens a filter object based on the given types and buffer size.
 *
 * @param tunerClientId the current open tuner clientId.
 * @param mainType the main type of the filter.
 * @param subType the subtype of the filter.
 * @param bufferSize the buffer size of the filter to be opened in bytes. The buffer holds the
 * data output from the filter.
 * @param callbackContext native callback context to listen filter event.
 * @param privateCallback make filter use new thread for callback or not.
 * @return the opened filter.
 * @jni weak reference corresponds to the java reference Filter.
 */
jobject Am_tuner_openFilter(int tunerClientId, int mainType, int subType, long bufferSize, long callbackContext, int privateCallback);

/**
 * Gets hardware sync ID for audio and video.
 *
 * @param tunerClientId the current open tuner clientId.
 * @param filter the filter instance for the hardware sync ID.
 * @return the id of hardware A/V sync.
 */
jint Am_tuner_getAvSyncHwId(int tunerClientId, jobject filter);

/**
 * Gets the current timestamp for Audio/Video sync.
 *
 * @param tunerClientId the current open tuner clientId.
 * @param avSyncHwId the hardware id of A/V sync.
 * @return the current timestamp of hardware A/V sync.
 */
jlong Am_tuner_getAvSyncTime(int tunerClientId, jint avSyncHwId);


/**
 * Opens an LNB (low-noise block downconverter) object.
 *
 * @param tunerClientId the current open tuner clientId.
 * @param callbackContext the lnb message callback(@link Am_lnb_callback).
 * @return lnb weak global ref.
 */
jobject Am_tuner_openLnb(int tunerClientId, long callbackContext);

/**
 * Opens an LNB (low-noise block downconverter) object.
 *
 * @param tunerClientId the current open tuner clientId.
 * @param name the LNB name.
 * @param callbackContext the lnb message callback(@link Am_lnb_callback).
 * @return lnb weak global ref.
 */
jobject Am_tuner_openLnbByName(int tunerClientId, const std::string &name, long callbackContext);
/**
 * alloc the Descrambler instance.
 *
 * @param descrambler the open descrambler instance.
 */
jobject Am_tuner_openDescrambler(int tunerClientId) ;
/**
 * Connects Conditional Access Modules (CAM) through Common Interface (CI).
 *
 * @param tunerClientId the current open tuner clientId.
 * @param ciCamId specify CI-CAM Id to connect
 * @result status of the operation.
 */
int Am_tuner_connectCiCam(int tunerClientId, int ciCamId);

/**
 * Disconnects Conditional Access Modules (CAM).
 *
 * @param tunerClientId the current open tuner clientId.
 * @result status of the operation.
 */
int Am_tuner_disconnectCiCam(int tunerClientId);

/**
 * Connect Conditional Access Modules (CAM) Frontend to support Common Interface (CI)
 * by-pass mode.
 * @param tunerClientId the current open tuner clientId.
 * @param ciCamId specify CI-CAM Id to connect
 * @return Local transport stream id when connection is successfully established.
 */
int Am_tuner_connectFrontendToCiCam(int tunerClientId, int ciCamId);

/**
 * Disconnect Conditional Access Modules (CAM) Frontend.
 *
 * @param tunerClientId the current open tuner clientId.
 * @param ciCamId specify CI-CAM Id to connect
 * @return result status of the operation.
 */
int Am_tuner_disconnectFrontendToCiCam(int tunerClientId, int ciCamId);

/**
 * Get TvInputSession.setSurface surfece instance by tuner client id.
 *
 * @param tunerClientId the current open tuner clientId.
 * @return result surface object or null.
 */
jobject Am_tuner_getSurfaceByTunerClient(int tunerClientId);

/**
 * Sets the filter type.
 *
 * @param filter the open filter instance.
 * @param mainType the main type of the filter.
 * @param subType the subtype of the filter.
 */
void Am_filter_setType(jobject filter, int mainType, int subType);

/**
 * Listens for filter events.
 *
 * @param filter the open filter instance.
 * @param callbackContext native callback context to listen filter event.
 */
void Am_filter_setCallback(jobject filter, long callbackContext);

/**
 * Configures the filter.
 *
 * @param filter the open filter instance.
 * @param config the configuration of the filter.
 *  corresponds to the java reference FilterConfiguration.
 * @return result status of the operation.
 */
jint Am_filter_configure(jobject filter, jobject config);

/**
 * Gets the filter Id.
 *
 * @param filter the open filter instance.
 * @return the filter Id.
 */
jint Am_filter_getId(jobject filter);

/**
 * Sets the filter's data source.
 *
 * @param filter the open filter instance.
 * @param source the filter instance which provides data input. Switch to
 * use demux as data source if the filter instance is NULL.
 * corresponds to the java reference Filter.
 * @return result status of the operation.
 */
jint Am_filter_setDataSource(jobject filter, jobject source);

/**
 * Starts filtering data.
 *
 * @param filter the open filter instance.
 * @return result status of the operation.
 */
jint Am_filter_start(jobject filter);

/**
 * Stops filtering data.
 *
 * @param filter the open filter instance.
 * @return result status of the operation.
 */
jint Am_filter_stop(jobject filter);

/**
 * Flushes the filter.The data which is already produced by filter but not consumed yet will
 * be cleared.
 *
 * @param filter the open filter instance.
 * @return result status of the operation.
 */
jint Am_filter_flush(jobject filter);

/**
 * Copies filtered data from filter output to the given byte array.
 *
 * @param filter the open filter instance.
 * @param buffer the buffer to store the filtered data.
 * @param offset the index of the first byte in {@code buffer} to write.
 * @param size the maximum number of bytes to read.
 * @return the number of bytes read.
 */
jint Am_filter_read(jobject filter, char * buffer, long offset, long size);

/**
 * Stops filtering data and releases the Filter instance.
 *
 * @param filter the open filter instance.
 */
void Am_filter_close(jobject filter);

/**
 * Sets the LNB's power voltage.
 *
 * @param lnb the open lnb instance.
 * @param voltage the power voltage constant the Lnb to use.
 * @return result status of the operation.
 */
int Am_lnb_setVoltage(jobject lnb, int voltage);

/**
 * Sets the LNB's tone mode.
 *
 * @param lnb the open lnb instance.
 * @param tone the tone mode the Lnb to use.
 * @return result status of the operation.
 */
int Am_lnb_setTone(jobject lnb, int tone);

/**
 * Selects the LNB's position.
 *
 * @param lnb the open lnb instance.
 * @param position the position the Lnb to use.
 * @return result status of the operation.
 */
int Am_lnb_setSatellitePosition(jobject lnb, int position);

/**
 * Selects the LNB's position.
 *
 * @param lnb the open lnb instance.
 * @param diseqcMessage a byte array of data for DiSEqC message which is specified by EUTELSAT Bus
 *        Functional Specification Version 4.2.
 * @return result status of the operation.
 */
int Am_lnb_sendDiseqcMessage(jobject lnb, const std::vector<char> diseqcMessage);

/**
 * Releases the LNB instance.
 *
 * @param lnb the open lnb instance.
 */
void Am_lnb_close(jobject lnb);

/**
 * Add packets' PID to the descrambler for descrambling.
 *
 * @param descrambler the open descrambler instance.
 * @param pidType the type of the PID.
 * @param pid the PID of packets to start to be descrambled.
 * @param filter an optional filter instance to identify upper stream.
 * @return result status of the operation.
 */
int Am_descrambler_addPid(jobject descrambler, int pidType, int pid, jobject filter);

/**
 * Remove packets' PID from the descrambler.
 *
 * @param descrambler the open descrambler instance.
 * @param pidType the type of the PID.
 * @param pid the PID of packets to start to be descrambled.
 * @param filter an optional filter instance to identify upper stream.
 * @return result status of the operation.
 */
int Am_descrambler_removePid(jobject descrambler, int pidType, int pid, jobject filter);

/**
 * Remove packets' PID from the descrambler.
 *
 * @param descrambler the open descrambler instance.
 * @param keyToken the token to be used to link the key slot.Use {@link TUNER_CONSTANT_INVALID_KEYTOKEN}
 *        to remove the current key from descrambler. If the current keyToken comes from a
 *        MediaCas session, use {@link TTUNER_CONSTANT_INVALID_KEYTOKEN} to remove current key before
 *        closing the MediaCas session.
 * @return result status of the operation.
 */
int Am_descrambler_setKeyToken(jobject descrambler, const std::vector<char> keyToken);

/**
 * Releases the Descrambler instance.
 *
 * @param descrambler the open descrambler instance.
 */
void Am_descrambler_close(jobject descrambler);

/**
 * Tuner event callback function
 *
 * @param event the tuner status.
 */
typedef void (*Am_tuner_notifyTunerEvent) (int tunerClientId, int event);

/**
 * scan callback function
 *
 * @param scanMessageType the scan callback message type.
 * @param scanMessage the scan callback message.
 */
typedef void (*Am_tuner_notifyScanCallbackEvent) (int tunerClientId, int scanMessageType, jobjectArray scanMessage);

/**
 * Filter event callback function
 *
 * @param filter the open filter instance.
 * @param filterEvent the filter events sent from the filter, may be null if not have filter event.
 * @param filterStatus the new status of the filter.
 */
typedef void (*Am_filter_callback) (jobject filter, jobjectArray filterEvent, int filterStatus);

/**
 * Lnb event callback function
 *
 * @param lnb the open lnb instance.
 * @param messageType the lnb callback event.
 * @param diseqcMessage the lnb callback diseqc message.
 */
typedef void (*Am_lnb_callback) (jobject lnb, int tunerClientId, int eventType, jbyteArray diseqcMessage);

/**
 * Tuner instance status listener
 *
 * @param tunerClientId the status change tuner clientId
 * @param lifecycleStatus 0 is tuner release, 1 is tuenr create.
 */
typedef void (*Am_tuner_status_listener) (int tunerClientId, TUNER_LIFECYCLE_STATUS lifecycleStatus);

JNIEnv* Am_tuner_getJNIEnv(bool* needsDetach);
void Am_tuner_detachJNIEnv();
int Am_tuner_getAndroidVersion();
jobject Am_tuner_getSurface();
auto ReleaseEnv = [](bool attached){if (attached) Am_tuner_detachJNIEnv();};

#endif/*_DROIDLOGIC_DTVKIT_TUNER_*/

