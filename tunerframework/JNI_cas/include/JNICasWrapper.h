#ifndef AM_JNI_CAS_WRAPPER_H
#define AM_JNI_CAS_WRAPPER_H

#include "JNICasTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Create and prepare the CasManager.
 *          It will prepare the CasWrapperManager and make the default CasWrapper
 * @param   void.
 * @retval  AM_CAS_JNI_OK if function succeeds.
 *          AM_CAS_JNI_ERR or AM_CAS_JNI_PARAM_ERR if If some exception occurs.
 */
JCAS_JNI_RESULT AM_CasManagerInit();

/**
 * @brief   Determine whether the specified ca system id is supported.
 * @param   caSystemId: The specified ca system id.
 * @retval  true if the ca system id is supported.
 *          false if the ca system id isn't supported.
 */
BOOLEAN AM_IsSystemIdSupported(int caSystemId);

/**
 * @brief   Create a new CasPlugin.
 *          It will create and prepare the CasConnection and the CasExtractor, and configure the CasConnection
 * @param   casPluginInfo: include the basic info for create a MediaCas instance.
 * @param   clientId: The tuner client Id.
 * @param   casHandle: The cas plugin handle.
 * @retval  AM_CAS_JNI_OK if function succeeds.
 *          AM_CAS_JNI_ERR or AM_CAS_JNI_PARAM_ERR if If some exception occurs.
 */
JCAS_JNI_RESULT AM_CreateCasPlugin(AM_CasPluginInfo *casPluginInfo, int clientId, CasHandle *casHandle);

/**
 * @brief   Try to open a cas session.
 *          It will create a CasInfo instance and a Descrambler instance
 * @param   casHandle: The cas plugin handle.
 * @param   casSessionInfo: include the basic info for create a MediaCas Session instance.
 * @param   casSessionHandle: The cas session handle.
 * @retval  AM_CAS_JNI_OK if function succeeds.
 *          AM_CAS_JNI_ERR or AM_CAS_JNI_PARAM_ERR if If some exception occurs.
 */
JCAS_JNI_RESULT AM_OpenCasSession(CasHandle casHandle, AM_CasSessionInfo *casSessionInfo,
        CasSessionHandle* casSessionHandle);

/**
 * @brief   Start descrambling with the cas session.
 *          It will set the scramble status, start ecm filter add descrambler pids
 * @param   casHandle: The cas plugin handle.
 * @param   casSessionHandle: The cas session handle.
 * @retval  AM_CAS_JNI_OK if function succeeds.
 *          AM_CAS_JNI_ERR or AM_CAS_JNI_PARAM_ERR if If some exception occurs.
 */
JCAS_JNI_RESULT AM_StartDescrambling(CasHandle casHandle, CasSessionHandle casSessionHandle);

/**
 * @brief   Stop descrambling with the cas session.
 *          It will set the scramble status and remove descrambler pids
 * @param   casHandle: The cas plugin handle.
 * @param   casSessionHandle: The cas session handle.
 * @retval  AM_CAS_JNI_OK if function succeeds.
 *          AM_CAS_JNI_ERR or AM_CAS_JNI_PARAM_ERR if If some exception occurs.
 */
JCAS_JNI_RESULT AM_StopDescrambling(CasHandle casHandle, CasSessionHandle casSessionHandle);

/**
 * @brief   Close the cas session.
 *          It will clear the CasInfo and stop ecm filter
 * @param   casHandle: The cas plugin handle.
 * @param   casSessionHandle: The cas session handle.
 * @retval  AM_CAS_JNI_OK if function succeeds.
 *          AM_CAS_JNI_ERR or AM_CAS_JNI_PARAM_ERR if If some exception occurs.
 */
JCAS_JNI_RESULT AM_CloseCasSession(CasHandle casHandle, CasSessionHandle casSessionHandle);

/**
 * @brief   Destroy a CasPlugin.
 *          It will delete the CasConnection and clear the CasExtractor
 * @param   The cas handle.
 * @retval  AM_CAS_JNI_OK if function succeeds.
 *          AM_CAS_JNI_ERR or AM_CAS_JNI_PARAM_ERR if If some exception occurs.
 */
JCAS_JNI_RESULT AM_DestroyCasPlugin(CasHandle casHandle);

/**
 * @brief   Release the CasManager.
 *          It will close all CasConnection instance and release the CasWrapperManager
 * @param   void.
 * @retval  AM_CAS_JNI_OK if function succeeds.
 *          AM_CAS_JNI_ERR or AM_CAS_JNI_PARAM_ERR if If some exception occurs.
 */
JCAS_JNI_RESULT AM_CasManagerTerm();

/**
 * @brief   Send the command to cas plugin.
 *          It will send the command of middleware or app to cas plugin
 * @param   casHandle: The cas plugin handle.
 * @param   event: The command event.
 * @param   arg: The specified arg of the command.
 * @param   data: The specified data of the command.
 * @param   dataLen: The dataLen of the command data.
 * @retval  AM_CAS_JNI_OK if function succeeds.
 *          AM_CAS_JNI_ERR or AM_CAS_JNI_PARAM_ERR if some exception occurs.
 */
JCAS_JNI_RESULT AM_SendCommand(CasHandle casHandle, int event, int arg, uint8_t* data, int dataLen);

/**
 * @brief   Send the command to cas plugin.
 *          It will send the command of middleware or app to cas session
 * @param   casHandle: The cas plugin handle.
 * @param   casSessionHandle: The cas session handle.
 * @param   event: The command event.
 * @param   arg: The specified arg of the command.
 * @param   data: The specified data of the command.
 * @param   dataLen: The dataLen of the command data.
 * @retval  AM_CAS_JNI_OK if function succeeds.
 *          AM_CAS_JNI_ERR or AM_CAS_JNI_PARAM_ERR if some exception occurs.
 */
JCAS_JNI_RESULT AM_SendSessionCommand(CasHandle casHandle, CasSessionHandle casSessionHandle, int event, int arg, uint8_t* data, int dataLen);

#ifdef __cplusplus
};
#endif

#endif //AM_JNI_CAS_WRAPPRE_H

