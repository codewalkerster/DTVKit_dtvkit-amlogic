/***************************************************************************
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 *
 *
 * Description:
 */

/**\file
 * \brief tvin tvafe
 *
 *
 * \author kieth.liu <kieth.liu@amlogic.com>
 * \date 2023-07-09: create the document
 ***************************************************************************/

#ifndef _ATV_VDIN_TVAFE_H_
#define _ATV_VDIN_TVAFE_H_



#ifdef __cplusplus
extern "C"
{
#endif

typedef unsigned int u32;

enum tvin_sig_status_e {
    TVIN_SIG_STATUS_NULL = 0, // processing status from init to the finding of the 1st confirmed status
    TVIN_SIG_STATUS_NOSIG,    // no signal - physically no signal
    TVIN_SIG_STATUS_UNSTABLE, // unstable - physically bad signal
    TVIN_SIG_STATUS_NOTSUP,   // not supported - physically good signal & not supported
    TVIN_SIG_STATUS_STABLE,   // stable - physically good signal & supported
};


/**\brief vdin callback function*/
typedef void (*AM_VDIN_STATUS_Callback_t) (int status);

extern int start_vdin_signal_detect(AM_VDIN_STATUS_Callback_t cb);
extern int stop_vdin_signal_detect();
extern int set_tvafe(int videoStd, int audioStd, int vfmt);
extern int set_atv_path();
extern int getCurrentSignalInfo(int *fmt, int *transFmt, int *status, int *frameRate);
extern void initCurrentSignalInfo();




#ifdef __cplusplus
}
#endif

#endif

