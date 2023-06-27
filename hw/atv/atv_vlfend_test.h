/***************************************************************************
 * Copyright (c) 2023 Amlogic, Inc. All rights reserved.
 *
 *
 * Description:
 */

/**\file
 * \brief Test atv_vlfend Module
 *
 *
 * \author by Robin <Bin.Luo@Amlogic.com>
 * \date 2023-06-16:
 ***************************************************************************/

#ifndef _ATV_VFFEND_TEST_H_
#define _ATV_VFFEND_TEST_H_



#ifdef __cplusplus
extern "C"
{
#endif

typedef unsigned int u32;

int test_atv_vlfend_open();
int test_atv_vlfend_lockfreq(u32 freq, u32 afc_range, u32 flag, u32 std);
int test_atv_vlfend_getstatus();
int test_atv_vlfend_getpara();

#ifdef __cplusplus
}
#endif

#endif

