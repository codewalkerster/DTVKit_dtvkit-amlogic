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
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <errno.h>
#include <linux/ethtool.h>
#include <netinet/in.h>
//#include <arpa/inet.h>

/* third party header files */

/* DVBCore header files */
#include "techtype.h"
#include "dbgfuncs.h"
#include "stbhwos.h"
#include "stbhwmem.h"
#include "stbhwnet.h"

/*---constant definitions for this file--------------------------------------*/

#define NW_TASK_STACK_SIZE 1024
#define NW_TASK_PRIORITY 8
#define NETWORK_ERROR

#ifdef NETWORK_ERROR
#define NET_ERR(x, ...) STB_SPDebugWrite("======NET======>%s:%d " x, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define NET_ERR(x, ...)
#endif

#ifdef NETWORK_DEBUG
#define NET_DBG(x, ...) STB_SPDebugWrite("%s:%d " x, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define NET_DBG(x, ...)
#endif

/*---local typedef structs for this file-------------------------------------*/

typedef struct s_nw_eth_monitor
{
   struct s_nw_eth_monitor *next;
   NW_eth_callback function;
} S_NW_ETH_MONITOR;

/*---local (static) variable declarations for this file----------------------*/

static S_NW_ETH_MONITOR *nw_monitor_list = NULL;
static void *nw_eth_task_ptr = NULL;
static void *nw_mutex = NULL;
static BOOLEAN nw_eth_status_running = FALSE;
static E_NW_LINK_STATUS current_ethernet_status = NW_LINK_DISABLED;

/*---local function prototypes for this file---------------------------------*/

static void EthernetMonitorTask(void *arg);

/*---global function definitions---------------------------------------------*/

/**
 * @brief   Initialises the socket API, must be called once before using API
 * @return  TRUE if initialised ok, FALSE otherwise
 */
BOOLEAN STB_NWInitialise(void)
{
   FUNCTION_START(STB_NWInitialise);

   if (!nw_eth_status_running)
   {
      nw_mutex = STB_OSCreateMutex();
      if (nw_mutex != NULL)
      {
         nw_eth_status_running = TRUE;
         nw_eth_task_ptr = STB_OSCreateTask(EthernetMonitorTask, NULL, NW_TASK_STACK_SIZE,
                                            NW_TASK_PRIORITY, (U8BIT *)"ethtsk");
         NET_DBG("Net task running\n");
         if (nw_eth_task_ptr == NULL)
         {
            nw_eth_status_running = FALSE;
            STB_OSDeleteMutex(nw_mutex);
            nw_mutex = NULL;
         }
      }
   }

   FUNCTION_FINISH(STB_NWInitialise);

   return nw_eth_status_running;
}

/**
 * @brief   Sets the network interface that will be used for all network or IP operations
 * @param   interface - network interface type to use
 * @return  TRUE if interface is available, FALSE otherwise
 */
BOOLEAN STB_NWSelectInterface(E_NW_INTERFACE interface)
{
   NET_ERR("enter");
   FUNCTION_START(STB_NWSelectInterface);
   USE_UNWANTED_PARAM(interface);
   FUNCTION_FINISH(STB_NWSelectInterface);
   return (TRUE);
}

/**
 * @brief   Returns the selected interface
 * @return  Selected interface
 */
E_NW_INTERFACE STB_NWGetSelectedInterface(void)
{
   NET_ERR("enter");
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
   NET_ERR("enter");

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
   NET_ERR("enter");
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
   NET_ERR("enter");
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
   NET_ERR("enter");
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
   NET_ERR("enter");
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
   NET_ERR("enter");
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
   NET_ERR("enter");
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
   NET_ERR("enter");
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
   NET_ERR("enter");
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
   NET_ERR("enter");
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

   NET_ERR("enter");
   *nw_addrs = NULL;

   FUNCTION_FINISH(STB_NWLookupAddress);

   return (0);
}

/**
 * @brief    Opens (creates) a new socket for subsequent use
 * @param    af - the address family that the socket will be used with
 * @param    type - the stream type the socket will be used with
 * @param    protocol - the protocol the socket will be used with
 * @return   The socket handle, or NULL if failed
 */
void *STB_NWOpenSocket(E_NW_AF af, E_NW_TYPE type, E_NW_PROTOCOL protocol, BOOLEAN nonblock)
{
   int s_domain;
   int s_protocol;
   int s_type;
   int sock;
   S_SOCKET_CTX *ctx = (S_SOCKET_CTX *)malloc(sizeof(S_SOCKET_CTX));

   USE_UNWANTED_PARAM(protocol); /* used in windows socket implementation */
   STB_SPDebugWrite("sock %d type %d protocol %d", sock, type, protocol);
   FUNCTION_START(STB_NWOpenSocket);

   if (af == NW_AF_INET)
      s_domain = AF_INET;
   else if (af == NW_AF_INET6)
      s_domain = AF_INET6;
   else
      STB_SPDebugWrite("%s: s_protocol invalid %d\n", af);

   if (protocol == NW_PROTOCOL_UDP)
      s_type = SOCK_DGRAM;
   else if (protocol == NW_PROTOCOL_TCP)
      s_type = SOCK_STREAM;
   else
      STB_SPDebugWrite("%s: type invalid %d\n", type);

   USE_UNWANTED_PARAM(nonblock);
   sock = socket(s_domain, s_type, 0);
   if (sock < 0)
      return NULL;

   ctx->af = af;
   ctx->type = type;
   ctx->protocol = protocol;
   ctx->nonblock = nonblock;
   ctx->sock = sock;
   FUNCTION_FINISH(STB_NWOpenSocket);

   return (void *)ctx;
}

/**
 * @brief    Closes (destroys) a socket instance
 * @param    socket - handle of the socket to be closed
 */
BOOLEAN STB_NWCloseSocket(void *socket)
{
   S_SOCKET_CTX *ctx = socket;
   FUNCTION_START(STB_NWCloseSocket);
   if (ctx)
   {
      close(ctx->sock);
      free(ctx);
   }
   FUNCTION_FINISH(STB_NWCloseSocket);

   return TRUE;
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
   S_SOCKET_CTX *ctx = socket;
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
   NET_ERR("enter\n");
   return TRUE;
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

   NET_ERR("enter\n");
   return TRUE;
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
   NET_ERR("enter\n");
   return TRUE;
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
   NET_ERR("enter\n");
   FUNCTION_FINISH(STB_NWDropMembership);

   return TRUE;
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
   S_SOCKET_CTX *ctx = socket;
   int i;
   FUNCTION_START(STB_NWGetSocketName);
   NET_ERR("enter\n");
   if (!ctx)
      return FALSE;
   *af = ctx->af;
   for(i=0; i<ctx->addr_len; i++)
      address[i] = ctx->addr[i];
   *port = ctx->port;
   NET_ERR("exit\n");
   FUNCTION_FINISH(STB_NWGetSocketName);

   return TRUE;
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
   S_SOCKET_CTX* ctx = socket;
   struct sockaddr_in in_addr;
   char *err_msg;
   int ret;
   FUNCTION_START(STB_NWConnect);
   NET_ERR("enter");

   if (!ctx)
   {
      NET_ERR("input socket is null");
      return NW_ERROR;
   }

   if (ctx->af == NW_AF_INET)
      in_addr.sin_family = AF_INET;
   else if (ctx->af = NW_AF_INET6)
      in_addr.sin_family = AF_INET6;
   else
   {
      err_msg = "socket family invalid";
      goto ERR;
   }
   in_addr.sin_port = htons(port);
   in_addr.sin_addr.s_addr = inet_addr(address);
   NET_ERR("sock %d family %d port %d addr %s", ctx->sock, in_addr.sin_family, port, address);
   ret = connect(ctx->sock, (struct sockaddr *)&in_addr, sizeof(in_addr));
   if (ret < 0)
   {
      err_msg = strerror(errno);
      goto ERR;
   }

   FUNCTION_FINISH(STB_NWConnect);
   return NW_OK;
ERR:
   NET_ERR("%s\n", err_msg);
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
   S_SOCKET_CTX *ctx = socket;

   FUNCTION_START(STB_NWListen);
   NET_ERR("enter");
   if (!ctx)
      return FALSE;
   if (listen(ctx->sock, backlog) < 0)
   {
      NET_ERR("listen failed\n");
      return FALSE;
   }

   FUNCTION_FINISH(STB_NWListen);

   return TRUE;
}

/**
 * @brief   Accepts an incoming connection attempt on a socket
 * @param   void* socket the handle of the socket in the listening state
 * @param   U8BIT* address pointer to the returned address (string) of
 *           connecting entity
 * @param   U32BIT* port pointer to the returned port of connecting entity
 * @return  handle of new socket actually connected, NULL if failed
 */
void *STB_NWAccept(void *socket, U8BIT *address, U32BIT *port)
{
   FUNCTION_START(STB_NWAccept);
   NET_ERR("enter");
   S_SOCKET_CTX *ctx = socket;
   struct sockaddr_in client_addr;
   socklen_t client_addr_len = sizeof(client_addr);
   int connfd;
   S_SOCKET_CTX *new_client = NULL;

   if (!ctx)
   {
      NET_ERR("socket is null");
      return NULL;
   }
   connfd = accept(ctx->sock, (struct sockaddr *)&client_addr, &client_addr_len);
   if (connfd < 0)
      return NULL;

   FUNCTION_FINISH(STB_NWAccept);
   new_client = (S_SOCKET_CTX *)malloc(sizeof(S_SOCKET_CTX));
   new_client->sock = connfd;
   return new_client;
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
   int ret;
   S_SOCKET_CTX *ctx = socket;

   FUNCTION_START(STB_NWSend);
   if (!ctx)
      return -1;
   ret = send(ctx->sock, buf, num_bytes, 0);
   NET_ERR("enter, sock %d data_len %d send %d", ctx->sock, num_bytes, ret);
   FUNCTION_FINISH(STB_NWSend);
   return ret;
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
   S_SOCKET_CTX *ctx = socket;
   NET_ERR("enter");
   int ret;

   FUNCTION_START(STB_NWReceive);
   if (!ctx)
      return -1;

   ret = recv(ctx->sock, buf, max_bytes, 0);
   NET_ERR("recv %d data", ret);
   FUNCTION_FINISH(STB_NWReceive);

   return (ret);
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
   NET_ERR("enter");
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
   NET_ERR("enter");
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
   int i;
   S_SOCKET_CTX *ctx;
   FUNCTION_START(STB_NWSockIsSet);
   if (!socks || !socket || (socks->sock_count == 0))
   {
      NET_ERR("Given sock set is null");
      return FALSE;
   }

   ctx = socket;
   for (i=0; i<socks->sock_count; i++)
   {
      if (socks->sock_array[i] == socket)
      {
         NET_ERR("given sock is set, fd %d\n", ctx->sock);
         return TRUE;
      }
   }
   NET_ERR("given sock is not set, fd %d\n", ctx->sock);
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
   if (socks)
   {
      socks->sock_count = 0;
   }
   FUNCTION_FINISH(STB_NWSockZero);
}

/**
 * @brief   Clears the specified socket from the specified set
 * @param   void* socket The socket to clear
 * @param   S_NW_SOCKSET *socks The set of sockets
 */
void STB_NWSockClear(void *socket, S_NW_SOCKSET *socks)
{
   int i;
   FUNCTION_START(STB_NWSockClear);
   if (!socks || !socks->sock_count)
   {
      NET_ERR("socks sets is empty");
      return;
   }

   for (i=0; i<socks->sock_count; i++)
   {
      if (socks->sock_array[i] == socket)
      {
         socks->sock_array[i] = socks->sock_array[socks->sock_count-1];
         socks->sock_count--;
         NET_ERR("Found socket to clear");
         return;
      }
   }
   NET_ERR("given socket is not found");
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
   if (!socks)
   {
      NET_ERR("given sock set is null, fatal err");
      return;
   }
   socks->sock_array[socks->sock_count] = socket;
   socks->sock_count++;
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
S32BIT STB_NWSelect(S_NW_SOCKSET *read_sockets, S_NW_SOCKSET *write_sockets,
                    S_NW_SOCKSET *except_sockets, S32BIT timeout_ms)
{
   FUNCTION_START(STB_NWSelect);
   fd_set read_fds;
   fd_set write_fds;
   fd_set exception_fds;
   int max_fd = 0;
   int ret = -1;
   int i;
   struct timeval time = {0};
   S_SOCKET_CTX *ctx;

#if 0
   if (read_sockets)
      NET_ERR("read socket %d", read_sockets->sock_count);
   if (write_sockets)
      NET_ERR("write socket %d", write_sockets->sock_count);
   if (except_sockets)
      NET_ERR("except socket %d", except_sockets->sock_count);
#endif

   FD_ZERO(&read_fds);
   FD_ZERO(&write_fds);
   FD_ZERO(&exception_fds);

   if (read_sockets)
   {
      for (i=0; i<read_sockets->sock_count; i++)
      {
         ctx = read_sockets->sock_array[i];
         if (!ctx)
         {
            NET_ERR("Found null ctx in read socks set");
            continue;
         }
         FD_SET(ctx->sock, &read_fds);
         max_fd = (max_fd > ctx->sock) ? max_fd : ctx->sock;
      }
   }
   if (write_sockets)
   {
      for (i = 0; i < write_sockets->sock_count; i++)
      {
         ctx = write_sockets->sock_array[i];
         if (!ctx)
         {
            NET_ERR("Found null ctx in write socks set");
            continue;
         }
         FD_SET(ctx->sock, &write_fds);
         max_fd = (max_fd > ctx->sock) ? max_fd : ctx->sock;
      }
   }
   if (except_sockets)
   {
      for (i = 0; i < except_sockets->sock_count; i++)
      {
         ctx = except_sockets->sock_array[i];
         if (!ctx)
         {
            NET_ERR("Found null ctx in exception socks set");
            continue;
         }
         FD_SET(ctx->sock, &exception_fds);
         max_fd = (max_fd > ctx->sock) ? max_fd : ctx->sock;
      }
   }
   //NET_ERR("select timeout %d", timeout_ms);
   if (timeout_ms == -1)
      ret = select(max_fd + 1, &read_fds, &write_fds, &exception_fds, NULL);
   else
   {
      time.tv_sec = timeout_ms / 1000;
      time.tv_usec = (timeout_ms%1000)*1000;
      ret = select(max_fd + 1, &read_fds, &write_fds, &exception_fds, &time);
   }

   FUNCTION_FINISH(STB_NWSelect);

   return ret;
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
   NET_ERR("enter");
   NET_DBG("STB_NWGetLinkStatus: %s", ((current_ethernet_status == NW_LINK_ACTIVE) ? "active" : (current_ethernet_status == NW_LINK_INACTIVE) ? "inactive" : "disabled"));
   FUNCTION_FINISH(STB_NWGetLinkStatus);
   return current_ethernet_status;
}

/**
 * @brief   Start monitoring the ethernet status
 * @param   func callback function to notify status change to ethernet device
 * @return  handle
 */
NW_handle STB_NWStartEthernetMonitor(NW_eth_callback func)
{
   S_NW_ETH_MONITOR *p_mtr;

   FUNCTION_START(STB_NWStartEthernetMonitor);

   if (func == NULL)
   {
      p_mtr = NULL;
   }
   else
   {
      p_mtr = (S_NW_ETH_MONITOR *)STB_MEMGetSysRAM(sizeof(S_NW_ETH_MONITOR));

      if (p_mtr != NULL)
      {
         p_mtr->function = func;

         STB_OSMutexLock(nw_mutex);
         p_mtr->next = nw_monitor_list;
         nw_monitor_list = p_mtr;
         STB_OSMutexUnlock(nw_mutex);
      }
   }

   FUNCTION_FINISH(STB_NWStartEthernetMonitor);

   return p_mtr;
}

/**
 * @brief   Stop monitoring the ethernet status
 * @param   hdl monitor handle
 */
void STB_NWStopEthernetMonitor(NW_handle hdl)
{
   S_NW_ETH_MONITOR *p_mtr, *prev;

   FUNCTION_START(STB_NWStopEthernetMonitor);
   if (hdl != NULL && nw_monitor_list != NULL)
   {
      p_mtr = (S_NW_ETH_MONITOR *)hdl;
      STB_OSMutexLock(nw_mutex);
      if (p_mtr == nw_monitor_list)
      {
         nw_monitor_list = p_mtr->next;
      }
      else
      {
         prev = nw_monitor_list;
         while (prev->next != NULL)
         {
            if (prev->next == p_mtr)
            {
               prev->next = p_mtr->next;
               break;
            }
            prev = prev->next;
         }
      }
      STB_OSMutexUnlock(nw_mutex);

      STB_MEMFreeSysRAM(hdl);
   }

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
   NET_ERR("enter");
   *access_points = NULL;
   FUNCTION_FINISH(STB_NWGetWirelessAccessPoints);
   return (0);
}

/**
 * @brief   Frees the array of access points returned by STB_NWGetWirelessAccessPoints
 * @param   access_points - array of access points to be freed
 * @param   num_aps - number of access points in the array
 */
void STB_NWFreeWirelessAccessPoints(S_NW_ACCESS_POINT *access_points, U16BIT num_aps)
{
   FUNCTION_START(STB_NWFreeWirelessAccessPoints);
   NET_ERR("enter");
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
   NET_ERR("enter");
   USE_UNWANTED_PARAM(essid);
   USE_UNWANTED_PARAM(password);
   FUNCTION_FINISH(STB_NWConnectToAccessPoint);
   return (FALSE);
}

/*---local function definitions----------------------------------------------*/

static void EthernetMonitorTask(void *arg)
{
   S_NW_ETH_MONITOR *p_nw_monitor;
   E_NW_LINK_STATUS status_now;
   struct ifreq ifr;
   int sock_fd;
   struct ethtool_value edata;

   USE_UNWANTED_PARAM(arg);

   sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
   if (sock_fd < 0)
   {
      NET_ERR("EthernetMonitorTask socket FAILURE");
   }
   else
   {
      strcpy(ifr.ifr_name, "eth0");

      while (nw_eth_status_running)
      {
         if (ioctl(sock_fd, SIOCGIFFLAGS, &ifr) < 0)
         {
            NET_ERR("ioctl SIOCGIFFLAGS failed %s", strerror(errno));
            status_now = NW_LINK_DISABLED;
         }
         else
         {
            if (ifr.ifr_flags & (IFF_UP | IFF_RUNNING))
            {
               edata.cmd = ETHTOOL_GLINK;
               ifr.ifr_data = (char *)&edata;
               if (ioctl(sock_fd, SIOCETHTOOL, &ifr) < 0)
               {
                  NET_ERR("ioctl SIOCETHTOOL failed");
                  status_now = NW_LINK_DISABLED;
               }
               else
               {
                  status_now = (edata.data) ? NW_LINK_ACTIVE : NW_LINK_INACTIVE;
               }
            }
            else
            {
               status_now = NW_LINK_DISABLED;
            }
         }
         if (current_ethernet_status != status_now)
         {
            current_ethernet_status = status_now;

            NET_DBG("EthernetMonitorTask: current ethernet status changed to %s",
                    ((current_ethernet_status == NW_LINK_ACTIVE) ? "active" : (current_ethernet_status == NW_LINK_INACTIVE) ? "inactive" : "disabled"));

            STB_OSMutexLock(nw_mutex);
            p_nw_monitor = nw_monitor_list;
            while (p_nw_monitor != NULL)
            {
               (p_nw_monitor->function)(NW_WIRED, current_ethernet_status);
               p_nw_monitor = p_nw_monitor->next;
            }
            STB_OSMutexUnlock(nw_mutex);
         }

         STB_OSTaskDelay(100);
      }
      close(sock_fd);
   }
}
