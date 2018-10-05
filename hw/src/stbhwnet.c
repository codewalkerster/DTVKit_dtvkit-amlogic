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
 * @brief   Set Top Box - Hardware Layer, Socket functions
 * @file    stbhwnet.c
 * @date    October 2018
 */

/*#define NETWORK_ERROR*/
/*#define NETWORK_DEBUG*/

/*---includes for this file--------------------------------------------------*/
/* compiler library header files */

/* third party header files */

/* DVBCore header files */
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwnet.h"

/*---constant definitions for this file--------------------------------------*/
#ifdef NETWORK_ERROR
   #define NET_ERR(x,...)        STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define NET_ERR(x,...)
#endif

#ifdef NETWORK_DEBUG
   #define NET_DBG(x,...)        STB_SPDebugWrite("%s:%d " x,__FUNCTION__,__LINE__, ##__VA_ARGS__ )
#else
   #define NET_DBG(x,...)
#endif

/*---local typedef structs for this file-------------------------------------*/

/*---local (static) variable declarations for this file----------------------*/

/*---local function prototypes for this file---------------------------------*/

/*---global function definitions---------------------------------------------*/

/**
 * @brief   Initialises the socket API, must be called once before using API
 * @return  TRUE if initialised ok, FALSE otherwise
 */
BOOLEAN STB_NWInitialise(void)
{
   FUNCTION_START(STB_NWInitialise);
   FUNCTION_FINISH(STB_NWInitialise);

   return FALSE;
}

/**
 * @brief   Sets the network interface that will be used for all network or IP operations
 * @param   interface - network interface type to use
 * @return  TRUE if interface is available, FALSE otherwise
 */
BOOLEAN STB_NWSelectInterface(E_NW_INTERFACE interface)
{
   FUNCTION_START(STB_NWSelectInterface);
   USE_UNWANTED_PARAM(interface);
   FUNCTION_FINISH(STB_NWSelectInterface);
   return(FALSE);
}

/**
 * @brief   Returns the selected interface
 * @return  Selected interface
 */
E_NW_INTERFACE STB_NWGetSelectedInterface(void)
{
   FUNCTION_START(STB_NWGetSelectedInterface);
   FUNCTION_FINISH(STB_NWGetSelectedInterface);

   return NW_WIRED;
}

/**
 * @brief   Gets the current IPv4 address of the default IP connection
 * @param   ip_addr - 4 byte array in which the address will be returned
 *                    with the most significant byte in ip_addr[0]
 * @return  TRUE if address is returned, FALSE otherwise
 */
BOOLEAN STB_IPGetIPAddress(U8BIT ip_addr[4])
{
   FUNCTION_START(STB_IPGetIPAddress);
   FUNCTION_FINISH(STB_IPGetIPAddress);

   return FALSE;
}

/**
 * @brief   Gets the current IPv4 subnet mask of the default IP connection
 * @param   subnet_mask - 4 byte array in which the address will be returned
 *                        with the most significant byte in subnet_mask[0]
 * @return  TRUE if address is returned, FALSE otherwise
 */
BOOLEAN STB_IPGetSubnetMask(U8BIT subnet_mask[4])
{
   FUNCTION_START(STB_IPGetSubnetMask);
   FUNCTION_FINISH(STB_IPGetSubnetMask);

   return FALSE;
}

/**
 * @brief   Gets the current IPv4 gateway IP address
 * @param   gateway_addr - 4 byte array in which the address will be returned
 *                         with the most significant byte in gateway_addr[0]
 * @return  TRUE if address is returned, FALSE otherwise
 */
BOOLEAN STB_IPGetGatewayIPAddress(U8BIT gateway_addr[4])
{
   FUNCTION_START(STB_IPGetGatewayIPAddress);
   FUNCTION_FINISH(STB_IPGetGatewayIPAddress);

   return FALSE;
}

/**
 * @brief   Gets the current IPv4 DNS server IP address
 * @param   dns_addr - 4 byte array in which the address will be returned
 *                     with the most significant byte in dns_addr[0]
 * @return  TRUE if address is returned, FALSE otherwise
 */
BOOLEAN STB_IPGetDnsServerIPAddress(U8BIT *dns_addr)
{
   FUNCTION_START(STB_IPGetDnsServerIPAddress);
   FUNCTION_FINISH(STB_IPGetDnsServerIPAddress);

   return FALSE;
}

/**
 * @brief   Gets the MAC address of the default ethernet connection
 * @param   interface - NW_WIRED or NW_WIRELESS
 * @param   mac_address - 6 byte array in which the address will be returned
 *                        with the most significant byte in mac_addr[0]
 * @return  TRUE if address is returned, FALSE otherwise
 */
BOOLEAN STB_NWGetMACAddress(E_NW_INTERFACE interface, U8BIT *mac_addr)
{
   FUNCTION_START(STB_NWGetMACAddress);
   USE_UNWANTED_PARAM(interface);
   FUNCTION_FINISH(STB_NWGetMACAddress);

   return FALSE;
}

/**
 * @brief   Sets the IPv4 format IP address of default network connection
 * @param   ip_addr - 4 byte array containing the IP address to be set
 *                    with the most significant byte in ip_addr[0]
 */
void STB_IPSetIPAddress(const U8BIT *ip_addr)
{
   FUNCTION_START(STB_IPSetIPAddress);
   FUNCTION_FINISH(STB_IPSetIPAddress);
}

/**
 * @brief   Sets the IPv4 format subnet mask of default network connection
 * @param   subnet_mask - 4 byte array containing the mask to be set
 *                        with the most significant byte in subnet_mask[0]
 */
void STB_IPSetSubnetMask(const U8BIT *subnet_mask)
{
   FUNCTION_START(STB_IPSetSubnetMask);
   FUNCTION_FINISH(STB_IPSetSubnetMask);
}

/**
 * @brief   Sets the IPv4 format gateway IP address
 * @param   gateway_addr - 4 byte array containing the mask to be set
 *                         with the most significant byte in gateway_addr[0]
 */
void STB_IPSetGatewayIPAddress(const U8BIT *gateway_addr)
{
   FUNCTION_START(STB_IPSetGatewayIPAddress);
   FUNCTION_FINISH(STB_IPSetGatewayIPAddress);
}

/**
 * @brief   Sets the IPv4 format DNS server IP address
 * @param   dns_addr - 4 byte array containing the mask to be set
 *                     with the most significant byte in dns_addr[0]
 */
void STB_IPSetDnsServerIPAddress(const U8BIT *dns_addr)
{
   FUNCTION_START(STB_IPSetDnsServerIPAddress);
   FUNCTION_FINISH(STB_IPSetDnsServerIPAddress);
}

/**
 * @brief   Cause the IP address to be set using DHCP
 * @param   wait_for_completion - set as TRUE if the call shouldn't return until an IP
 *                                address has been assigned or times out
 */
void STB_IPGetIPByDhcp(BOOLEAN wait_for_completion)
{
   FUNCTION_START(STB_IPGetIPByDhcp);
   USE_UNWANTED_PARAM(wait_for_completion);
   FUNCTION_FINISH(STB_IPGetIPByDhcp);
}

/**
 * @brief   Performs a lookup to find the IP address(es) of the given host name
 * @param   name - host name
 * @param   nw_addrs - array of structures containing the results of the lookup.
 *                     This array must be deleted using STB_MEMFreeSysRAM.
 * @return  The number of addresses returned in the array
 */
U16BIT STB_NWLookupAddress(U8BIT *name, S_NW_ADDR_INFO **nw_addrs)
{
   FUNCTION_START(STB_NWLookupAddress);
   USE_UNWANTED_PARAM(name);

   *nw_addrs = NULL;

   FUNCTION_FINISH(STB_NWLookupAddress);

   return(0);
}

/**
 * @brief    Opens (creates) a new socket for subsequent use
 * @param    af - the address family that the socket will be used with
 * @param    type - the stream type the socket will be used with
 * @param    protocol - the protocol the socket will be used with
 * @return   The socket handle, or NULL if failed
 */
void* STB_NWOpenSocket(E_NW_AF af, E_NW_TYPE type, E_NW_PROTOCOL protocol, BOOLEAN nonblock)
{
   FUNCTION_START(STB_NWOpenSocket);
   USE_UNWANTED_PARAM(af);
   USE_UNWANTED_PARAM(type);
   USE_UNWANTED_PARAM(protocol); /* used in windows socket implementation */
   USE_UNWANTED_PARAM(nonblock);
   FUNCTION_FINISH(STB_NWOpenSocket);

   return NULL;
}

/**
 * @brief    Closes (destroys) a socket instance
 * @param    socket - handle of the socket to be closed
 */
BOOLEAN STB_NWCloseSocket(void *socket)
{
   FUNCTION_START(STB_NWCloseSocket);
   USE_UNWANTED_PARAM(socket);
   FUNCTION_FINISH(STB_NWCloseSocket);

   return FALSE;
}

/**
 * @brief    Binds (names) a socket in the local address space
 * @param    socket - the handle of the socket to be bound
 * @param    af - the address family to be bound to
 * @param    address - the string address (NULL to let OS choose)
 * @param    port - the port number to be bound to (zero to let OS choose)
 * @return   TRUE - successfully bound, FALSE failed to bind
 */
BOOLEAN STB_NWBind(void *socket, U8BIT *address, U32BIT port)
{
   FUNCTION_START(STB_NWBind);
   USE_UNWANTED_PARAM(socket);
   USE_UNWANTED_PARAM(address);
   USE_UNWANTED_PARAM(port);
   FUNCTION_FINISH(STB_NWBind);

   return FALSE;
}

/**
 * @brief    Sets the socket option SO_REUSEADDR
 * @param    socket - the handle of the socket to be bound
 * @param    state - TRUE or FALSE to activate/deactivate the
 *           option REUSEADDR
 * @return   TRUE successfully set, FALSE failed to set
 */
BOOLEAN STB_NWSetReuseaddr(void *socket, BOOLEAN state)
{
   FUNCTION_START(STB_NWSetReuseaddr);
   USE_UNWANTED_PARAM(socket);
   USE_UNWANTED_PARAM(state);
   FUNCTION_FINISH(STB_NWSetReuseaddr);

   return FALSE;
}

/**
 * @brief    Gets the socket option SO_REUSEADDR
 * @param    socket - the handle of the socket
 * @param    state - TRUE or FALSE if the option is actie/inactivate
 * @return   TRUE successfully got, FALSE failed to get
 */
BOOLEAN STB_NWGetReuseaddr(void *socket, BOOLEAN *state)
{
   FUNCTION_START(STB_NWGetReuseaddr);
   USE_UNWANTED_PARAM(socket);
   USE_UNWANTED_PARAM(state);
   FUNCTION_FINISH(STB_NWGetReuseaddr);

   return FALSE;
}

/**
 * @brief    Sets the ip option IP_ADD_MEMBERSHIP
 * @param    socket - the handle of the socket
 * @param    group_address address to add
 * @return   TRUE successfully set, FALSE failed to set
 */
BOOLEAN STB_NWAddMembership(void *socket, U8BIT *group_address)
{
   FUNCTION_START(STB_NWAddMembership);
   USE_UNWANTED_PARAM(socket);
   USE_UNWANTED_PARAM(group_address);
   FUNCTION_FINISH(STB_NWAddMembership);

   return FALSE;
}

/**
 * @brief    Sets the ip option IP_DROP_MEMBERSHIP
 * @param    socket - the handle of the socket
 * @param    group_address address to drop
 * @return   TRUE successfully set, FALSE failed to set
 */
BOOLEAN STB_NWDropMembership(void *socket, U8BIT *group_address)
{
   FUNCTION_START(STB_NWDropMembership);
   USE_UNWANTED_PARAM(socket);
   USE_UNWANTED_PARAM(group_address);
   FUNCTION_FINISH(STB_NWDropMembership);

   return FALSE;
}

/**
 * @brief   Gets the socket's "name" (family, address and port)
 * @param   void* socket the handle of the socket
 * @param   E_NW_AF* af pointer to the returned family
 * @param   U8BIT* address pointer to the returned address string
 * @param   U32BIT* port pointer to the returned port
 * @return  TRUE successfully set, FALSE failed to set
 */
BOOLEAN STB_NWGetSocketName(void *socket, E_NW_AF *af, U8BIT *address, U32BIT *port)
{
   FUNCTION_START(STB_NWGetSocketName);
   USE_UNWANTED_PARAM(socket);
   USE_UNWANTED_PARAM(af);
   USE_UNWANTED_PARAM(address);
   USE_UNWANTED_PARAM(port);
   FUNCTION_FINISH(STB_NWGetSocketName);

   return FALSE;
}

/**
 * @brief   Connects the socket to a remote host
 * @param   void* socket the handle of the socket
 * @param   U8BIT* address address string
 * @param   U32BIT port port
 * @return  TRUE successfully connected, FALSE failed to set
 */
E_NW_ERROR STB_NWConnect(void *socket, U8BIT *address, U32BIT port)
{
   FUNCTION_START(STB_NWConnect);
   USE_UNWANTED_PARAM(socket);
   USE_UNWANTED_PARAM(address);
   USE_UNWANTED_PARAM(port);
   FUNCTION_FINISH(STB_NWConnect);

   return NW_ERROR;
}

/**
 * @brief   Put socket into state of waiting for incoming connection
 * @param   void* socket the handle of the socket to begin listening
 * @param   backlog the maximum length of the queue of pending connections
 * @return  TRUE successfully connected, FALSE failed to set
 */
BOOLEAN STB_NWListen(void *socket, S32BIT backlog)
{
   FUNCTION_START(STB_NWListen);
   USE_UNWANTED_PARAM(socket);
   USE_UNWANTED_PARAM(backlog);
   FUNCTION_FINISH(STB_NWListen);

   return FALSE;
}

/**
 * @brief   Accepts an incoming connection attempt on a socket
 * @param   void* socket the handle of the socket in the listening state
 * @param   U8BIT* address pointer to the returned address (string) of
 *           connecting entity
 * @param   U32BIT* port pointer to the returned port of connecting entity
 * @return  handle of new socket actually connected, NULL if failed
 */
void* STB_NWAccept(void *socket, U8BIT *address, U32BIT *port)
{
   FUNCTION_START(STB_NWAccept);
   USE_UNWANTED_PARAM(socket);
   USE_UNWANTED_PARAM(address);
   USE_UNWANTED_PARAM(port);
   FUNCTION_FINISH(STB_NWAccept);

   return NULL;
}

/**
 * @brief   Sends data on a connected socket
 * @param   void* socket the handle of the (connected) socket on which to
 *          send
 * @param   U8BIT* buf the buffer holding the data to be sent
 * @param   U32BIT num_bytes the number of bytes in buf to be sent
 * @return  the number of bytes sent (may be less than num_bytes)' -1 if failed
 */
S32BIT STB_NWSend(void *socket, U8BIT *buf, U32BIT num_bytes)
{
   FUNCTION_START(STB_NWSend);
   USE_UNWANTED_PARAM(socket);
   USE_UNWANTED_PARAM(buf);
   USE_UNWANTED_PARAM(num_bytes);
   FUNCTION_FINISH(STB_NWSend);

   return -1;
}

/**
 * @brief   Receives data from a connected socket
 * @param   void* socket the handle of the socket from which to read
 * @param   U8BIT* buf the buffer to hold the read data
 * @param   U32BIT max_bytes the maximum bytes the caller can accept in
 *          the buffer
 * @return  the number of bytes read, -1 if there was an error reading
 *          0 if the connection was closed
 */
S32BIT STB_NWReceive(void *socket, U8BIT *buf, U32BIT max_bytes)
{
   FUNCTION_START(STB_NWReceive);
   USE_UNWANTED_PARAM(socket);
   USE_UNWANTED_PARAM(buf);
   USE_UNWANTED_PARAM(max_bytes);
   FUNCTION_FINISH(STB_NWReceive);

   return(0);
}

/**
 * @brief   Receives a datagram and returns the data and its source address
 * @param   void* socket the handle of the socket from which to read
 * @param   U8BIT* buf the buffer to hold the read data
 * @param   U32BIT max_bytes the maximum bytes the caller can accept in
 *          the buffer
 * @param   U8BIT* address the source address of the data in string form
 * @param   U32BIT* port the source port of the data
 * @return  the number of bytes read, -1 if there was an error reading
 *          0 if the connection was closed
 */
S32BIT STB_NWReceiveFrom(void *socket, U8BIT *buf, U32BIT max_bytes, U8BIT *address, U32BIT *port)
{
   FUNCTION_START(STB_NWReceiveFrom);
   USE_UNWANTED_PARAM(socket);
   USE_UNWANTED_PARAM(buf);
   USE_UNWANTED_PARAM(max_bytes);
   USE_UNWANTED_PARAM(address);
   USE_UNWANTED_PARAM(port);
   FUNCTION_FINISH(STB_NWReceiveFrom);

   return -1;
}

/**
 * @brief   Sends data to a specific destination
 * @param   void* socket the handle of the socket on which to send
 * @param   U8BIT* buf the buffer holding the data to be sent
 * @param   U32BIT num_bytes the number of bytes in buf to be sent
 * @param   U8BIT* address the address (in string form) of the target socket
 * @param   U32BIT port the port number of the target socket
 * @return  the number of bytes sent (may be less than num_bytes), -1 if
 *          failed
 */
S32BIT STB_NWSendTo(void *socket, U8BIT *buf, U32BIT num_bytes,
   U8BIT *address, U32BIT port)
{
   FUNCTION_START(STB_NWSendTo);
   USE_UNWANTED_PARAM(socket);
   USE_UNWANTED_PARAM(buf);
   USE_UNWANTED_PARAM(num_bytes);
   USE_UNWANTED_PARAM(address);
   USE_UNWANTED_PARAM(port);
   FUNCTION_FINISH(STB_NWSendTo);

   return -1;
}

/**
 * @brief   Returns whether a specified socket is a member of a specified set
 * @param   void* socket The socket to check
 * @param   S_NW_SOCKSET *socks The set of sockets to check
 * @return  TRUE socket is a member of set, FALSE socket is not a member
 *          of set
 */
BOOLEAN STB_NWSockIsSet(void *socket, S_NW_SOCKSET *socks)
{
   FUNCTION_START(STB_NWSockIsSet);
   USE_UNWANTED_PARAM(socket);
   USE_UNWANTED_PARAM(socks);
   FUNCTION_FINISH(STB_NWSockIsSet);

   return FALSE;
}

/**
 * @brief   Clears the socket set
 * @param   S_NW_SOCKSET *socks The set of sockets to check
 */
void STB_NWSockZero(S_NW_SOCKSET *socks)
{
   FUNCTION_START(STB_NWSockZero);
   USE_UNWANTED_PARAM(socks);
   FUNCTION_FINISH(STB_NWSockZero);
}

/**
 * @brief   Clears the specified socket from the specified set
 * @param   void* socket The socket to clear
 * @param   S_NW_SOCKSET *socks The set of sockets
 */
void STB_NWSockClear(void *socket, S_NW_SOCKSET *socks)
{
   FUNCTION_START(STB_NWSockClear);
   USE_UNWANTED_PARAM(socket);
   USE_UNWANTED_PARAM(socks);
   FUNCTION_FINISH(STB_NWSockClear);
}

/**
 * @brief   Sets a specificed socket in a specified set
 * @param   void* socket The socket to set
 * @param   S_NW_SOCKSET *socks The set of sockets
 */
void STB_NWSockSet(void *socket, S_NW_SOCKSET *socks)
{
   FUNCTION_START(STB_NWSockSet);
   USE_UNWANTED_PARAM(socket);
   USE_UNWANTED_PARAM(socks);
   FUNCTION_FINISH(STB_NWSockSet);
}

/**
 * @brief   Determines the status of one or more sockets, blocking if necessary
 * @param   S_NW_SOCKSET *read_sockets set of sockets to be checked for
 *          readability
 * @param   S_NW_SOCKSET *write_sockets set of sockets to be checked for
 *          writability
 * @param   S_NW_SOCKSET *exceptfds set of sockets to be checked for errors
 * @param   U32BIT timeout_ms maximum number of milliseconds to wait
 *          (-1 to block, 0 to return immediately)
 * @return  the total number of sockets that are ready, 0 time out exceeded,
 *          -1 an error occured
 */
S32BIT  STB_NWSelect(S_NW_SOCKSET *read_sockets, S_NW_SOCKSET *write_sockets,
   S_NW_SOCKSET *except_sockets, S32BIT timeout_ms)
{
   FUNCTION_START(STB_NWSelect);
   USE_UNWANTED_PARAM(read_sockets);
   USE_UNWANTED_PARAM(write_sockets);
   USE_UNWANTED_PARAM(except_sockets);
   USE_UNWANTED_PARAM(timeout_ms);
   FUNCTION_FINISH(STB_NWSelect);

   return -1;
}

/**
 * @brief   Get the selected interface link status
 * @return  NW_LINK_ACTIVE   cable connected,
 *          NW_LINK_INACTIVE cable dis-connected
 *          NW_LINK_DISABLED no ethernet device or other error
 */
E_NW_LINK_STATUS STB_NWGetLinkStatus(void)
{
   FUNCTION_START(STB_NWGetLinkStatus);
   FUNCTION_FINISH(STB_NWGetLinkStatus);
   return NW_LINK_DISABLED;
}

/**
 * @brief   Start monitoring the ethernet status
 * @param   func callback function to notify status change to ethernet device
 * @return  handle
 */
NW_handle STB_NWStartEthernetMonitor( NW_eth_callback func )
{
   FUNCTION_START(STB_NWStartEthernetMonitor);
   FUNCTION_FINISH(STB_NWStartEthernetMonitor);

   return NULL;
}

/**
 * @brief   Stop monitoring the ethernet status
 * @param   hdl monitor handle
 */
void STB_NWStopEthernetMonitor(NW_handle hdl)
{
   FUNCTION_START(STB_NWStopEthernetMonitor);
   USE_UNWANTED_PARAM(hdl);
   FUNCTION_FINISH(STB_NWStopEthernetMonitor);
}

/**
 * @brief   Scans and returns all access points visible on the wireless network
 * @param   access_points - pointer to an array that will be allocated containing
 *                          info on each access point found
 * @return  number of access points found
 */
U16BIT STB_NWGetWirelessAccessPoints(S_NW_ACCESS_POINT **access_points)
{
   FUNCTION_START(STB_NWGetWirelessAccessPoints);
   *access_points = NULL;
   FUNCTION_FINISH(STB_NWGetWirelessAccessPoints);
   return(0);
}

/**
 * @brief   Frees the array of access points returned by STB_NWGetWirelessAccessPoints
 * @param   access_points - array of access points to be freed
 * @param   num_aps - number of access points in the array
 */
void STB_NWFreeWirelessAccessPoints(S_NW_ACCESS_POINT *access_points, U16BIT num_aps)
{
   FUNCTION_START(STB_NWFreeWirelessAccessPoints);
   USE_UNWANTED_PARAM(access_points);
   USE_UNWANTED_PARAM(num_aps);
   FUNCTION_FINISH(STB_NWFreeWirelessAccessPoints);
}

/**
 * @brief   Attempts to connect to the wireless network with the given SSID.
 *          If the network is open then 'password' can be NULL.
 * @param   essid - network to connect to
 * @param   password - password to be used for an encrypted network,
 *                     can be NULL for an open network
 * @return  TRUE if connected successfully to the network, FALSE otherwise
 */
BOOLEAN STB_NWConnectToAccessPoint(U8BIT *essid, U8BIT *password)
{
   FUNCTION_START(STB_NWConnectToAccessPoint);
   USE_UNWANTED_PARAM(essid);
   USE_UNWANTED_PARAM(password);
   FUNCTION_FINISH(STB_NWConnectToAccessPoint);
   return(FALSE);
}

/*---local function definitions----------------------------------------------*/

