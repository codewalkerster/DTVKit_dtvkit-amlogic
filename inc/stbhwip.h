/*******************************************************************************
 * Copyright © 2014 The DTVKit Open Software Foundation Ltd (www.dtvkit.org)
 * Copyright © 2009 Ocean Blue Software Ltd
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
 * @file    stbhwip.h
 * @brief   macros and function prototypes for public use
 * @date    06/08/2009
 */

#ifndef _STBIP_H
#define _STBIP_H

#include "techtype.h"

/**
 * @brief   Gets the current IPv4 address of the default IP connection
 * @param   ip_addr 4 byte array in which the address will be returned
 *          with the most significant byte in ip_addr[0]
 * @retval  TRUE if address is returned, FALSE otherwise
 */
BOOLEAN STB_IPGetIPAddress(U8BIT *ip_addr);

/**
 * @brief   Gets the current IPv4 subnet mask of the default IP connection
 * @param   subnet_mask 4 byte array in which the address will be returned
 *          with the most significant byte in subnet_mask[0]
 * @retval  TRUE if address is returned, FALSE otherwise
 */
BOOLEAN STB_IPGetSubnetMask(U8BIT *subnet_mask);

/**
 * @brief   Gets the current IPv4 gateway IP address
 * @param   gateway_addr 4 byte array in which the address will be returned
 *          with the most significant byte in gateway_addr[0]
 * @retval  TRUE if address is returned, FALSE otherwise
 */
BOOLEAN STB_IPGetGatewayIPAddress(U8BIT *gateway_addr);

/**
 * @brief   Gets the current IPv4 DNS server IP address
 * @param   dns_addr 4 byte array in which the address will be returned
 *          with the most significant byte in dns_addr[0]
 * @retval  TRUE if address is returned, FALSE otherwise
 */
BOOLEAN STB_IPGetDnsServerIPAddress(U8BIT *dns_addr);

/**
 * @brief   Sets the IPv4 format IP address of default network connection
 * @param   ip_addr 4 byte array containing the IP address to be set
 *          with the most significant byte in ip_addr[0]
 */
void STB_IPSetIPAddress(const U8BIT *ip_addr);

/**
 * @brief   Sets the IPv4 format subnet mask of default network connection
 * @param   subnet_mask 4 byte array containing the mask to be set
 *          with the most significant byte in subnet_mask[0]
 */
void STB_IPSetSubnetMask(const U8BIT *subnet_mask);

/**
 * @brief   Sets the IPv4 format gateway IP address
 * @param   gateway_addr 4 byte array containing the mask to be set
 *          with the most significant byte in gateway_addr[0]
 */
void STB_IPSetGatewayIPAddress(const U8BIT *gateway_addr);

/**
 * @brief   Sets the IPv4 format DNS server IP address
 * @param   dns_addr 4 byte array containing the mask to be set
 *          with the most significant byte in dns_addr[0]
 */
void STB_IPSetDnsServerIPAddress(const U8BIT *dns_addr);

/**
 * @brief   Cause the IP address to be set using DHCP
 * @param   wait_for_completion set as TRUE if the call shouldn't return until an IP
 *          address has been assigned or times out
 */
void STB_IPGetIPByDhcp(BOOLEAN wait_for_completion);

#endif

