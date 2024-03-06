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
 * @brief   Socket functions
 * @file    stbhwnet.h
 * @date    24/06/2009
 * @author  Sergio Panseri
 */

/* pre-processor mechanism so multiple inclusions don't cause compilation error*/
#ifndef _STBHWNET_H
#define _STBHWNET_H

#include "techtype.h"


/*---Constant and macro definitions for public use-----------------------------*/
/* the maximum size address string that the API will return */
#ifndef MAX_ADDR_STRING_LEN
#define MAX_ADDR_STRING_LEN 255
#endif

#ifndef SOCK_SETSIZE
#define SOCK_SETSIZE 64
#endif

/* removes socket "sock" from set "set" */
#define SOCK_CLR(sock, set) STB_NWSockClear((sock), (set));

/* adds socket "sock" to set "set" */
#define SOCK_SET(sock, set) STB_NWSockSet((sock), (set));

/* empties set "set" */
#define SOCK_ZERO(set) STB_NWSockZero((set))

/* true if socket "sock" is a member of "set" */
#define SOCK_ISSET(sock, set) STB_NWSockIsSet((sock), (set))

#ifndef INVALID_SOCKET
#define INVALID_SOCKET 0
#endif

/*---Enumerations for public use-----------------------------------------------*/
typedef enum
{
    NW_WIRED,
    NW_WIRELESS
} E_NW_INTERFACE;

typedef enum
{
    NW_AF_INET,
    NW_AF_INET6
} E_NW_AF;

typedef enum
{
    NW_SOCK_STREAM,
    NW_SOCK_DGRAM
} E_NW_TYPE;

typedef enum
{
    NW_PROTOCOL_TCP,
    NW_PROTOCOL_UDP
} E_NW_PROTOCOL;

typedef enum
{
    NW_LINK_ACTIVE,
    NW_LINK_INACTIVE,
    NW_LINK_DISABLED
} E_NW_LINK_STATUS;

typedef enum
{
    NW_OK,
    NW_ERROR,
    NW_BAD_PARAM,
    NW_PENDING
} E_NW_ERROR;

typedef void (*NW_eth_callback)(E_NW_INTERFACE iface, E_NW_LINK_STATUS iface_status);
typedef void *NW_handle;

/*---Global type defs for public use-------------------------------------------*/
typedef struct
{
    U16BIT sock_count;
    void *sock_array[SOCK_SETSIZE];
    U8BIT sockset_array[SOCK_SETSIZE];
} S_NW_SOCKSET;

typedef struct
{
    E_NW_AF af;
    E_NW_TYPE type;
    E_NW_PROTOCOL protocol;
    BOOLEAN is_multicast;
    U8BIT addr[48];
} S_NW_ADDR_INFO;

typedef struct
{
    U8BIT *essid;
    U8BIT quality;
    BOOLEAN encrypted;
    BOOLEAN connected;
} S_NW_ACCESS_POINT;

typedef struct
{
    E_NW_AF af;
    E_NW_TYPE type;
    E_NW_PROTOCOL protocol;
    BOOLEAN nonblock;
    int sock;
    U32BIT port;
    U8BIT addr[32];
    int addr_len;
} S_SOCKET_CTX;

/*---Global Function prototypes for public use---------------------------------*/
/**
 * @brief   Initialises the socket API, must be called once before using API
 * @return  TRUE Socket API initialised ok, FALSE error
 */
BOOLEAN STB_NWInitialise(void);

/**
 * @brief   Sets the network interface that will be used for all network or IP operations
 * @param   interface network interface type to use
 * @retval  TRUE if interface is available, FALSE otherwise
 */
BOOLEAN STB_NWSelectInterface(E_NW_INTERFACE iface);

/**
 * @brief   Returns the selected interface
 * @return  Selected interface
 */
E_NW_INTERFACE STB_NWGetSelectedInterface(void);

/**
 * @brief   Get the selected interface link status
 * @return  NW_LINK_ACTIVE   cable connected,
 *          NW_LINK_INACTIVE cable dis-connected
 *          NW_LINK_DISABLED no ethernet device or other error
 */
E_NW_LINK_STATUS STB_NWGetLinkStatus(void);

/**
 * @brief   Start monitoring the ethernet status
 * @param   func callback function to notify status change to ethernet device
 * @return  handle
 */
NW_handle STB_NWStartEthernetMonitor( NW_eth_callback func );

/**
 * @brief   Stop monitoring the ethernet status
 */
void STB_NWStopEthernetMonitor( NW_handle hdl );

/**
 * @brief   Gets the MAC address of the default ethernet connection
 * @param   interface NW_WIRED or NW_WIRELESS
 * @param   mac_address 6 byte array in which the address will be returned
 *                        with the most significant byte in mac_addr[0]
 * @retval  TRUE if address is returned, FALSE otherwise
 */
BOOLEAN STB_NWGetMACAddress(E_NW_INTERFACE iface, U8BIT *mac_addr);

/**
 * @brief   Performs a lookup to find the IP address(es) of the given host name
 * @param   name host name
 * @param   nw_addrs array of structures containing the results of the lookup.
 *          This array must be deleted using STB_MEMFreeSysRAM.
 * @return  The number of addresses returned in the array
 */
U16BIT STB_NWLookupAddress(U8BIT *name, S_NW_ADDR_INFO **nw_addrs);

/**
 * @brief   Opens (creates) a new socket for subsequent use
 * @param   af the address family that the socket will be used with
 * @param   type the stream type the socket will be used with
 * @param   protocol the protocol the socket will be used with
 * @return  The socket handle, or NULL if failed
 */
void* STB_NWOpenSocket(E_NW_AF af, E_NW_TYPE type, E_NW_PROTOCOL protocol, BOOLEAN nonblock);

/**
 * @brief   Closes (destroys) a socket instance
 * @param   socket handle of the socket to be closed
 * @return  TRUE success, FALSE error
 */
BOOLEAN STB_NWCloseSocket(void *socket);

/**
 * @brief   Binds (names) a socket in the local address space
 * @param   socket the handle of the socket to be bound
 * @param   address the string address (NULL to let OS choose)
 * @param   port the port number to be bound to (zero to let OS choose)
 * @return  TRUE successfully bound, FALSE failed to bind
 */
BOOLEAN STB_NWBind(void *socket, U8BIT *address, U32BIT port);

/**
 * @brief   Sets the socket option SO_REUSEADDR
 * @param   void *socket the handle of the socket to be bound
 * @param   BOOLEAN state TRUE or FALSE to activate/deactivate the
 *          option REUSEADDR
 * @return  TRUE successfully set, FALSE failed to set
 */
BOOLEAN STB_NWSetReuseaddr(void *socket, BOOLEAN state);

/**
 * @brief   Gets the socket option SO_REUSEADDR
 * @param   void *socket the handle of the socket
 * @param   BOOLEAN *state pointer to the returned state, TRUE or FALSE
 *          if the option is active/inactive
 * @return  TRUE successfully got, FALSE failed to get
 */
BOOLEAN STB_NWGetReuseaddr(void *socket, BOOLEAN *state);

/**
 * @brief   Sets the ip option IP_ADD_MEMBERSHIP
 * @param   void* socket the handle of the socket
 * @param   U8BIT *group_address address to add
 * @return  TRUE successfully set, FALSE failed to set
 */
BOOLEAN STB_NWAddMembership(void *socket, U8BIT *group_address);

/**
 * @brief   Sets the ip option IP_DROP_MEMBERSHIP
 * @param   void* socket the handle of the socket
 * @param   U8BIT *group_address address to drop
 * @return  TRUE successfully set, FALSE failed to set
 */
BOOLEAN STB_NWDropMembership(void *socket, U8BIT *group_address);

/**
 * @brief   Gets the socket's "name" (family, address and port)
 * @param   void* socket the handle of the socket
 * @param   E_NW_AF* af pointer to the returned family
 * @param   U8BIT* address pointer to the returned address string
 * @param   U32BIT* port pointer to the returned port
 * @return  TRUE successfully set, FALSE failed to set
 */
BOOLEAN STB_NWGetSocketName(void *socket, E_NW_AF *af, U8BIT *address, U32BIT *port);

/**
 * @brief   Connects the socket to a remote host
 * @param   void* socket the handle of the socket
 * @param   U8BIT* address address string
 * @param   U32BIT port port
 * @return  TRUE successfully connected, FALSE failed to set
 */
E_NW_ERROR STB_NWConnect(void *socket, U8BIT *address, U32BIT port);

/**
 * @brief   Put socket into state of waiting for incoming connection
 * @param   void* socket the handle of the socket to begin listening
 * @param   backlog the maximum length of the queue of pending connections
 * @return  TRUE successfully connected, FALSE failed to set
 */
BOOLEAN STB_NWListen(void *socket, S32BIT backlog);

/**
 * @brief   Accepts an incoming connection attempt on a socket
 * @param   void* socket the handle of the socket in the listening state
 * @param   U8BIT* address pointer to the returned address (string) of
 *           connecting entity
 * @param   U32BIT* port pointer to the returned port of connecting entity
 * @return  handle of new socket actually connected, NULL if failed
 */
void* STB_NWAccept(void *socket, U8BIT *address, U32BIT *port);

/**
 * @brief   Sends data on a connected socket
 * @param   void* socket the handle of the (connected) socket on which to
 *          send
 * @param   U8BIT* buf the buffer holding the data to be sent
 * @param   U32BIT num_bytes the number of bytes in buf to be sent
 * @return  the number of bytes sent (may be less than num_bytes)' -1 if
 *          failed
 */
S32BIT STB_NWSend(void *socket, U8BIT *buf, U32BIT num_bytes);

/**
 * @brief   Receives data from a connected socket
 * @param   void* socket the handle of the socket from which to read
 * @param   U8BIT* buf the buffer to hold the read data
 * @param   U32BIT max_bytes the maximum bytes the caller can accept in
 *          the buffer
 * @return  the number of bytes read, -1 if there was an error reading
 *          0 if the connection was closed
 */
S32BIT STB_NWReceive(void *socket, U8BIT *buf, U32BIT max_bytes);

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
S32BIT STB_NWReceiveFrom(void *socket, U8BIT *buf, U32BIT max_bytes, U8BIT *address, U32BIT *port);

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
                    U8BIT *address, U32BIT port);

/**
 * @brief   Returns whether a specified socket is a member of a specified set
 * @param   void* socket The socket to check
 * @param   S_NW_SOCKSET *socks The set of sockets to check
 * @return  TRUE socket is a member of set, FALSE socket is not a member
 *          of set
 */
BOOLEAN STB_NWSockIsSet(void *socket, S_NW_SOCKSET *socks);

/**
 * @brief   Clears the socket set
 * @param   S_NW_SOCKSET *socks The set of sockets to check
 */
void STB_NWSockZero(S_NW_SOCKSET *socks);

/**
 * @brief   Clears the specified socket from the specified set
 * @param   void* socket The socket to clear
 * @param   S_NW_SOCKSET *socks The set of sockets
 */
void STB_NWSockClear(void *socket, S_NW_SOCKSET *socks);

/**
 * @brief   Sets a specificed socket in a specified set
 * @param   void* socket The socket to set
 * @param   S_NW_SOCKSET *socks The set of sockets
 */
void STB_NWSockSet(void *socket, S_NW_SOCKSET *socks);

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
 *          -1 an error occurred
 */
S32BIT  STB_NWSelect(S_NW_SOCKSET *read_sockets, S_NW_SOCKSET *write_sockets,
                     S_NW_SOCKSET *except_sockets, S32BIT timeout_ms);


/**
 * @brief   Scans and returns all access points visible on the wireless network
 * @param   access_points pointer to an array that will be allocated containing
 *          info on each access point found
 * @return  number of access points found
 */
U16BIT STB_NWGetWirelessAccessPoints(S_NW_ACCESS_POINT **access_points);

/**
 * @brief   Frees the array of access points returned by STB_NWGetWirelessAccessPoints
 * @param   access_points array of access points to be freed
 * @param   num_aps number of access points in the array
 */
void STB_NWFreeWirelessAccessPoints(S_NW_ACCESS_POINT *access_points, U16BIT num_aps);

/**
 * @brief   Attempts to connect to the wireless network with the given SSID.
 *          If the network is open then 'password' can be NULL.
 * @param   essid network to connect to
 * @param   password password to be used for an encrypted network,
 *          can be NULL for an open network
 * @return  TRUE if connected successfully to the network, FALSE otherwise
 */
BOOLEAN STB_NWConnectToAccessPoint(U8BIT *essid, U8BIT *password);


#endif /* _STBHWNET_H */
