/*
 * Copyright (c) 2020 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef _STBHWDMXUSB_H
#define _STBHWDMXUSB_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SUBFRAG 16
#define FRAG_SIZE 10

/*
 * Negative Error values for read/write operations
 * Note: CIUSB_NOT_ENABLED is treated as though CAM remains plugged in
 */
#define CIUSB_GEN_ERROR -1
#define CIUSB_NO_DEVICE -19  /* value = -ENODEV */

typedef enum {
   NOT_INSERTED,
   INSERTED,
   INSERTED_STATE
} USB_INSERTED_STATE;

/**
 * \brief   set usbcam insertion state.
 * \param   inserted_state set to TRUE if cam is inserted.
 * \return  TRUE if cam is inserted.
 */
BOOLEAN STB_CIUsbInsertedState(USB_INSERTED_STATE inserted_state);

#ifdef __cplusplus
}
#endif

#endif
