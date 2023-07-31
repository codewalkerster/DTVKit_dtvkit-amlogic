/*******************************************************************************
 * Copyright (c) 2018 The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
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
 * @brief   Set Top Box - Hardware Layer, Media player API
 * @file    stbhwmp.c
 * @date    October 2018
 */

/*#define MEDIA_DEBUG*/

/*---includes for this file--------------------------------------------------*/
/* compiler library header files */

/* third party header files */

/* STB header files */
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwav.h"
#include "stbhwmediaplayer.h"
#include "dtv_log.h"
#define TAG  "STBHWMP"

/*---macro definitions for this file-----------------------------------------*/
#ifdef MEDIA_DEBUG
#define MEDIA_DBG(x,...) DTV_LOG(ANDROID_LOG_INFO, TAG, "%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
#define MEDIA_DBG(X)
#endif

/*---constant definitions for this file--------------------------------------*/

/*---local typedef structs for this file-------------------------------------*/

/*---local (static) variable declarations for this file----------------------*/

/*---local function prototypes for this file---------------------------------*/

/*---local function definitions----------------------------------------------*/

/*---global function definitions---------------------------------------------*/

/**
 * @brief   Initialises the media player module for the specified source
 * @param   source_url source URL to be presented
 * @return  media player handle
 */
void* STB_MPInit(U8BIT *source_name)
{
   FUNCTION_START(STB_MPInit);
   USE_UNWANTED_PARAM(source_name);
   FUNCTION_FINISH(STB_MPInit);

   return NULL;
}

/**
 * @brief   Starts the presentation of content
 * @param   handle media player handle
 * @param   params parameters to control the display
 * @return  HW_OK if successful, error code otherwise
 */
E_HW_STATUS STB_MPStart(void* handle, S_STB_MP_START_PARAMS *params)
{
   FUNCTION_START(STB_MPStart);
   USE_UNWANTED_PARAM(handle);
   USE_UNWANTED_PARAM(params);
   FUNCTION_FINISH(STB_MPStart);

   return(HW_GEN_ERROR);
}

/**
 * @brief   Pauses the presentation of content
 * @param   handle media player handle
 * @return  HW_OK if successful, error code otherwise
 */
E_HW_STATUS STB_MPPause(void* handle)
{
   FUNCTION_START(STB_MPPause);
   USE_UNWANTED_PARAM(handle);
   FUNCTION_FINISH(STB_MPPause);

   return(HW_GEN_ERROR);
}

/**
 * @brief   Resumes the presentation of content
 * @param   handle media player handle
 * @return  HW_OK if successful, error code otherwise
 */
E_HW_STATUS STB_MPResume(void* handle)
{
   FUNCTION_START(STB_MPResume);
   USE_UNWANTED_PARAM(handle);
   FUNCTION_FINISH(STB_MPResume);

   return(HW_GEN_ERROR);
}

/**
 * @brief   Changes position of video on screen
 * @param   handle media player handle
 * @param   rect rectangle structure representing the expected position of video
 * @return  HW_OK if successful, error code otherwise
 */
E_HW_STATUS STB_MPResize(void  *handle, S_RECTANGLE *rect)
{
   FUNCTION_START(STB_MPResize);
   USE_UNWANTED_PARAM(handle);
   USE_UNWANTED_PARAM(rect);
   FUNCTION_FINISH(STB_MPResize);

   return(HW_GEN_ERROR);
}

/**
 * @brief   Stops the presentation of content
 * @param   handle media player handle
 * @return  HW_OK if successful, error code otherwise
 */
E_HW_STATUS STB_MPStop(void* handle)
{
   FUNCTION_START(STB_MPStop);
   USE_UNWANTED_PARAM(handle);
   FUNCTION_FINISH(STB_MPStop);

   return(HW_GEN_ERROR);
}

/**
 * @brief   Uninitialises the media player module and invalidate the handle
 * @param   handle media player handle
 */
void STB_MPExit(void* handle)
{
   FUNCTION_START(STB_MPExit);
   USE_UNWANTED_PARAM(handle);
   FUNCTION_FINISH(STB_MPExit);
}

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
E_HW_STATUS STB_MPGetTimes(void* handle, U32BIT *begin, U32BIT *current, U32BIT *end)
{
   FUNCTION_START(STB_MPGetTimes);
   USE_UNWANTED_PARAM(handle);

   *begin = 0;
   *current = 0;
   *end = 0;

   FUNCTION_FINISH(STB_MPGetTimes);

   return(HW_GEN_ERROR);
}

/**
 * @brief   Registers a callback to receive notifications of media player change of state
 * @param   handle media player handle
 * @param   callback pointer to the callback function
 */
void STB_MPRegisterCallback(void *handle, STB_MP_CALLBACK callback)
{
   FUNCTION_START(STB_MPRegisterCallback);
   USE_UNWANTED_PARAM(handle);
   USE_UNWANTED_PARAM(callback);
   FUNCTION_FINISH(STB_MPRegisterCallback);
}

/**
 * @brief   Seeks the currently presented content to the specified position
 * @param   handle media player handle
 * @param   position position in milliseconds
 * @return  HW_OK if successful, error code otherwise
 */
E_HW_STATUS STB_MPSeek(void* handle, U32BIT position)
{
   FUNCTION_START(STB_MPSeek);
   USE_UNWANTED_PARAM(handle);
   USE_UNWANTED_PARAM(position);
   FUNCTION_FINISH(STB_MPSeek);
   return(HW_GEN_ERROR);
}

/**
 * @brief   Returns a list of components of the specified type available in the currently presented
 *          content.
 * @param   handle media player handle
 * @param   type type of component or STB_MP_COMPONENT_ALL to receive all of them
 * @param   num_ptr pointer to the number of components found
 * @param   list_ptr pointer to the list, must be freed using STB_MPReleaseComponentList
 * @return  HW_OK if successful, error code otherwise
 */
E_HW_STATUS STB_MPObtainComponentList(void *handle, E_STB_MP_COMPONENT_TYPE type, U32BIT *num_ptr,
   S_STB_MP_COMPONENT_DETAILS** list_ptr)
{
   FUNCTION_START(STB_MPObtainComponentList);
   USE_UNWANTED_PARAM(handle);
   USE_UNWANTED_PARAM(type);

   *num_ptr = 0;
   *list_ptr = NULL;

   FUNCTION_FINISH(STB_MPObtainComponentList);
   return(HW_GEN_ERROR);
}

/**
 * @brief   Releases the list of components returned by STB_MPObtainComponentList
 * @param   handle media player handle
 * @param   list_ptr list to be freed
 */
void STB_MPReleaseComponentList(void *handle, S_STB_MP_COMPONENT_DETAILS* list_ptr)
{
   FUNCTION_START(STB_MPReleaseComponentList);
   USE_UNWANTED_PARAM(handle);
   USE_UNWANTED_PARAM(list_ptr);
   FUNCTION_FINISH(STB_MPReleaseComponentList);
}

/**
 * @brief   Forces the specifed component to be presented, if another component of the same type is
 *          already presented, it will be removed.
 * @param   handle media player handle
 * @param   component pointer to the componente to be presented, this must be one of the elements of
 *          the component list retured by STB_MPObtainComponentList.
 * @return  HW_OK if successful, error code otherwise
 */
E_HW_STATUS STB_MPSelectComponent(void *handle, S_STB_MP_COMPONENT_DETAILS *component)
{
   FUNCTION_START(STB_MPSelectComponent);
   USE_UNWANTED_PARAM(handle);
   USE_UNWANTED_PARAM(component);
   FUNCTION_FINISH(STB_MPSelectComponent);
   return(HW_GEN_ERROR);
}

/**
 * @brief   Forces the specified component to be stopped
 * @param   handle media player handle
 * @param   component pointer to the componente to be presented, this must be one of the elements of
 *          the component list retured by STB_MPObtainComponentList.
 * @return  HW_OK if successful, error code otherwise
 */
E_HW_STATUS STB_MPUnselectComponent(void *handle, S_STB_MP_COMPONENT_DETAILS *component)
{
   FUNCTION_START(STB_MPUnselectComponent);
   USE_UNWANTED_PARAM(handle);
   USE_UNWANTED_PARAM(component);
   FUNCTION_FINISH(STB_MPUnselectComponent);
   return(HW_GEN_ERROR);
}

/**
 * @brief   Returns the actual value of the error when the media player status is STB_MP_STATE_ERROR
 * @return  Error code when the media player is in STB_MP_STATE_ERROR status, STB_MP_NO_ERROR
 *          otherwise
 */
E_STB_MP_ERROR STB_MPGetError(void *handle)
{
   E_STB_MP_ERROR error;

   FUNCTION_START(STB_MPGetError);
   USE_UNWANTED_PARAM(handle);

   error = STB_MP_UNDEFINED;

   FUNCTION_FINISH(STB_MPGetError);

   return error;
}

