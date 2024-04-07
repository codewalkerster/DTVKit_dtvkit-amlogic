#ifndef __CAM_MANGER_H
#define __CAM_MANGER_H

#include <stdbool.h>
#include "techtype.h"

#define CAM_NUM_MAX 32

typedef enum {
        CAM_TS_BYPASS,
        CAM_TS_PASS_THROUGH
} CAM_TsStatus_t;

typedef enum {
        CAM_DEVICE_USB,//
        CAM_DEVICE_PCMCIA//
} CAM_DeviceType_t;

typedef struct {
        CAM_DeviceType_t device_type;//device type
        int slot_id;//slot id
        bool insert;//plug or not
        CAM_TsStatus_t through;//through or not
} CAM_Info_t;

typedef enum {
        CAM_CARD_NONE,
        CAM_CARD_USB,
        CAM_CARD_PCMCIA
} CAM_Card_State;

typedef struct {
	CAM_Info_t CAM[CAM_NUM_MAX];//all cam information
} CAM_Manager_t;

typedef enum {
        CAM_MAN_STATE_BYPASS,
        CAM_MAN_STATE_USB,
        CAM_MAN_STATE_PCMCIA
} CAM_Manager_State;



int CAM_Insert(int slot_id, CAM_DeviceType_t type);
int CAM_SetTsStatus(int slot_id, CAM_TsStatus_t status);
int CAM_Remove(int slot_id);
void CAM_IsUsbcam(BOOLEAN enable);
CAM_Card_State CAM_GetCamCardState();
void CAM_Register_CallBack(BOOLEAN (*decodingpath_callback)(U8BIT i),BOOLEAN (*recordingpath_callback)(U8BIT i));
#endif
