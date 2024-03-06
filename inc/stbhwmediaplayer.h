/*******************************************************************************
 * Copyright © 2014 The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
 * Copyright © 2013 Ocean Blue Software Ltd
 *
 * This file is part of a DTVKit Software Component
 * You are permitted to copy, modify or distribute this file subject to the terms
 * of the DTVKit 1.0 Licence which can be found in licence.txt or at www.dtvkit.org
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * If you or your organisation is not a member of DTVKit then you have access
 * to this source code outside of the terms of the licence agreement
 * and you are expected to delete this and any associated files immediately.
 * Further information on DTVKit, membership and terms can be found at www.dtvkit.org
 *******************************************************************************/
/**
 * @brief    Media player API
 * @file     stbhwmediaplayer.h
 * @date     22/03/2013
 * @author   Sergio Panseri
 */

#ifndef _STBHWMEDIAPLAYER_H
#define _STBHWMEDIAPLAYER_H

#include "techtype.h"

/*---Constant and macro definitions for public use-----------------------------*/

/*---Enumerations for public use-----------------------------------------------*/
typedef enum
{
    STB_MP_STATE_STOPPED = 0,
    STB_MP_STATE_PLAYING,
    STB_MP_STATE_PAUSED,
    STB_MP_STATE_CONNECTING,
    STB_MP_STATE_BUFFERING,
    STB_MP_STATE_FINISHED,
    STB_MP_STATE_ERROR
} E_STB_MP_STATE;

typedef enum
{
    STB_MP_COMPONENT_VIDEO = 0,
    STB_MP_COMPONENT_AUDIO,
    STB_MP_COMPONENT_SUBTITLE,
    STB_MP_COMPONENT_ALL
} E_STB_MP_COMPONENT_TYPE;

typedef enum
{
    STB_MP_NO_ERROR = -1,              /* no error */
    STB_MP_FORMAT_NOT_SUPPORTED = 0,   /* 0 - A/V format not supported */
    STB_MP_CONNECTION_ERROR,           /* 1 - cannot connect to server or connection lost.*/
    STB_MP_UNDEFINED,                  /* 2 - unidentified error */
    STB_MP_NO_RESOURCES,               /* 3 – insufficient resources */
    STB_MP_CORRUPT,                    /* 4 – content corrupt or invalid */
    STB_MP_NOT_AVAILABLE,              /* 5 – content not available */
    STB_MP_NOT_AVAILABLE_POSITION,     /* 6 – content not available at given position */
    STB_MP_BLOCKED                     /* 7 - content blocked due to parental control */
} E_STB_MP_ERROR;

/*---Global type defs for public use-------------------------------------------*/
typedef struct
{
    U8BIT component_tag;
    U16BIT pid;
    E_STB_MP_COMPONENT_TYPE type;
    union
    {
        struct
        {
            E_STB_AV_AUDIO_CODEC encoding;
            U32BIT lang_code;
            U8BIT num_channels;
            BOOLEAN audio_description;
        } audio;
        struct
        {
            E_STB_AV_VIDEO_CODEC encoding;
            BOOLEAN hd;
            U8BIT frame_rate;
            E_STB_AV_ASPECT_RATIO aspect_ratio;
        } video;
        struct
        {
            U32BIT lang_code;
            BOOLEAN hearing_impaired;
        } subtitle;
    } av;
    BOOLEAN encrypted;
    BOOLEAN active;
} S_STB_MP_COMPONENT_DETAILS;

typedef void (*STB_MP_CALLBACK)(void *handle, E_STB_MP_STATE state);

typedef struct
{
    BOOLEAN cache; /* If TRUE, the media player is supposed to load the file and play it from memory */
    S32BIT loops; /* Number of times the file has to be played, -1 means infinite */
} S_STB_MP_START_PARAMS;

/*---Global Function prototypes for public use---------------------------------*/
/**
 * @brief   Initialises the media player module for the specified source
 * @param   source_url source URL to be presented
 * @return  media player handle
 */
void* STB_MPInit(U8BIT *source_url);

/**
 * @brief   Starts the presentation of content
 * @param   handle media player handle
 * @param   params parameters to control the display
 * @return  HW_OK if successful, error code otherwise
 */
E_HW_STATUS STB_MPStart(void *handle, S_STB_MP_START_PARAMS *params);

/**
 * @brief   Pauses the presentation of content
 * @param   handle media player handle
 * @return  HW_OK if successful, error code otherwise
 */
E_HW_STATUS STB_MPPause(void *handle);

/**
 * @brief   Resumes the presentation of content
 * @param   handle media player handle
 * @return  HW_OK if successful, error code otherwise
 */
E_HW_STATUS STB_MPResume(void *handle);

/**
 * @brief   Changes position of video on screen
 * @param   handle media player handle
 * @param   rect rectangle structure representing the expected position of video
 * @return  HW_OK if successful, error code otherwise
 */
E_HW_STATUS STB_MPResize(void *handle, S_RECTANGLE *rect);

/**
 * @brief   Stops the presentation of content
 * @param   handle media player handle
 * @return  HW_OK if successful, error code otherwise
 */
E_HW_STATUS STB_MPStop(void *handle);

/**
 * @brief   Uninitialises the media player module and invalidate the handle
 * @param   handle media player handle
 */
void STB_MPExit(void *handle);

/**
 * @brief   Returns start, current and end times in milliseconds for the content currently being
 *          presented.
 * @param   handle media player handle
 * @param   begin pointer to the variable where the begin time is stored. The value returned for the
 *          begin time depend on the platform implementation and might not be always 0.
 * @param   current pointer to the variable where the current time is stored.
 * @param   end pointer to the variable where the end time is stored
 * @return  HW_OK if successful, error code otherwise
 */
E_HW_STATUS STB_MPGetTimes(void *handle, U32BIT *begin, U32BIT *current, U32BIT *end);

/**
 * @brief   Registers a callback to receive notifications of media player change of state
 * @param   handle media player handle
 * @param   callback pointer to the callback function
 */
void STB_MPRegisterCallback(void *handle, STB_MP_CALLBACK callback);

/**
 * @brief   Seeks the currently presented content to the specified position
 * @param   handle media player handle
 * @param   position position in milliseconds
 * @return  HW_OK if successful, error code otherwise
 */
E_HW_STATUS STB_MPSeek(void *handle, U32BIT position);

/**
 * @brief   Returns a list of components of the specified type available in the currently presented
 *          content.
 * @param   handle media player handle
 * @param   type type of component or STB_MP_COMPONENT_ALL to receive all of them
 * @param   num_ptr pointer to the number of components found
 * @param   list_ptr pointer to the list, must be freed using STB_MPReleaseComponentList
 * @return  HW_OK if successful, error code otherwise
 */
E_HW_STATUS STB_MPObtainComponentList(void *handle, E_STB_MP_COMPONENT_TYPE type,
                                      U32BIT *num_ptr, S_STB_MP_COMPONENT_DETAILS **list_ptr);

/**
 * @brief   Releases the list of components returned by STB_MPObtainComponentList
 * @param   handle media player handle
 * @param   list_ptr list to be freed
 */
void STB_MPReleaseComponentList(void *handle, S_STB_MP_COMPONENT_DETAILS *list_ptr);

/**
 * @brief   Forces the specifed component to be presented, if another component of the same type is
 *          already presented, it will be removed.
 * @param   handle media player handle
 * @param   component pointer to the component to be presented, this must be one of the elements of
 *          the component list returned by STB_MPObtainComponentList.
 * @return  HW_OK if successful, error code otherwise
 */
E_HW_STATUS STB_MPSelectComponent(void *handle, S_STB_MP_COMPONENT_DETAILS *component);

/**
 * @brief   Forces the specified component to be stopped
 * @param   handle media player handle
 * @param   component pointer to the component to be presented, this must be one of the elements of
 *          the component list returned by STB_MPObtainComponentList.
 * @return  HW_OK if successful, error code otherwise
 */
E_HW_STATUS STB_MPUnselectComponent(void *handle, S_STB_MP_COMPONENT_DETAILS *component);

/**
 * @brief   Returns the actual value of the error when the media player status is STB_MP_STATE_ERROR
 * @return  Error code when the media player is in STB_MP_STATE_ERROR status, STB_MP_NO_ERROR
 *          otherwise
 */
E_STB_MP_ERROR STB_MPGetError(void *handle);

#endif /*  _STBHWMEDIAPLAYER_H */

/******************************************************************************
 * End of file
 ******************************************************************************/
