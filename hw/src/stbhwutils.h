/*******************************************************************************
 *  Copyright @ 2023
 *
 *  header file for stbhwutils.c
 *
 *
 *******************************************************************************/

/**
 * @brief    header file for stbhwutils.c
 * @file    stbhwutils.h
 * @date    2023-12-19
 */

#ifndef __STBHWUTILS_H__
#define __STBHWUTILS_H__

U8BIT STB_Utils_StrengthToSSI(U8BIT path, S16BIT strength);
U8BIT STB_Utils_SNR10ToSQI(U8BIT path, S16BIT snr);

#endif

