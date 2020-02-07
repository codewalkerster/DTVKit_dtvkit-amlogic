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
 * @brief   Set Top Box - Hardware Layer, Serial Port (Debug) functions
 * @file    stbhwsp.c
 * @date    October 2018
 */

/*---includes for this file---------------------------------------------------*/
/* compiler library header files */
#include <stdio.h>
#include <stdarg.h>

/* third party header files */
/* DVBCore header files*/
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwc.h"

#include <android/log.h>
#ifndef TAG_EXT
#define TAG_EXT
#endif

#define DTVKIT_MACRO_BEGIN   do {

/**\brief 添加在命令行式定义的末尾*/
#define DTVKIT_MACRO_END     } while(0)

#define log_print(...) __android_log_print(ANDROID_LOG_INFO, "DTVKIT_LOG" TAG_EXT, __VA_ARGS__)
#define DTVKIT_DEBUG(_fmt...) \
   DTVKIT_MACRO_BEGIN\
   if (1)\
   {\
      log_print(_fmt);\
   }\
   DTVKIT_MACRO_END

/*---constant definitions for this file---------------------------------------*/

/*---macro definitions for this file------------------------------------------*/

/*---local typedef structs for this file--------------------------------------*/

/*---local (static) variable declarations for this file-----------------------*/
/*   (internal variables declared static to make them local) */
static char debug_msg_buff[512];

/*---local function prototypes for this file----------------------------------*/
/*   (internal functions declared static to make them local) */

/*---local function definitions-----------------------------------------------*/
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "am_misc.h"

static char *lf_file()
{
    char buf[256] = { 0 };
    char log_prop[] = "tv.dtvkit.logfile";
    //AM_PropRead(log_prop, buf, sizeof(buf));
    strncpy(buf, "/data/data/org.dtvkit.inputsource/dtvkit.log", sizeof(buf));
    return strlen(buf)? buf : NULL;
}

static void lf_write(const char *log)
{
   static int fd = -1;
   char *rn = "\n";

   if (fd == -1) {
       char *f = lf_file();
       if (f)
           fd = open(f, O_WRONLY | O_APPEND | O_CREAT,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
   }
   if (fd != -1) {
       write(fd, log, strlen(log));
       write(fd, rn, strlen(rn));
   } else {
       DTVKIT_DEBUG("lf open fail(%d:%s)", errno, strerror(errno));
   }
}

/*---global function definitions----------------------------------------------*/


/**
 * @brief   Write debug string to serial/debug port. <CR><LF> characters will be automatically
 *          added to the end of the string
 * @param   format string & format
 */
void STB_SPDebugWrite(const char *format, ... )
{
   va_list vparams;

   FUNCTION_START(STB_SPDebugWrite);

   ASSERT(format != NULL);

   va_start(vparams, format);
   vsnprintf(debug_msg_buff, sizeof(debug_msg_buff), format, vparams);
   va_end(vparams);

   //printf("%s\n", debug_msg_buff);
   DTVKIT_DEBUG("%s", debug_msg_buff);
   lf_write(debug_msg_buff);
   //fflush(stdout);

   FUNCTION_FINISH(STB_SPDebugWrite);
}

/**
 * @brief   Writes debug string to the serial port without <CR><LF>
 * @param   format string & format
 */
void STB_SPDebugNoCnWrite(const char *format, ... )
{
   va_list vparams;

   FUNCTION_START(STB_SPDebugNoCnWrite);

   ASSERT(format != NULL);

   va_start(vparams, format);
   vsnprintf(debug_msg_buff, sizeof(debug_msg_buff), format, vparams);
   va_end(vparams);

   DTVKIT_DEBUG("%s", debug_msg_buff);
   //printf("%s", debug_msg_buff);
   //fflush(stdout);

   FUNCTION_FINISH(STB_SPDebugNoCnWrite);
}

/**
 * @brief   Report Assertion failure
 * @param   file name of source file
 * @param   line line number of source file
 * @param   eval_str evaluation string that failed
 */
void STB_SPDebugAssertFail(const char *file, int line, const char *eval_str)
{
   FUNCTION_START(STB_SPDebugAssertFail);
   STB_SPDebugNoCnWrite("ASSERT FAILURE at %s:%d (%s)\n",file,line,eval_str);
   FUNCTION_FINISH(STB_SPDebugAssertFail);
}

