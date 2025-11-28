/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// net_udp.c

#include "quakedef.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <errno.h>

#ifdef __sun__
#include <sys/filio.h>
#endif

#ifdef NeXT
#include <libc.h>
#endif

#ifdef __EMSCRIPTEN__
#include <arpa/inet.h>
#endif

#ifdef LINUX
#include <arpa/inet.h>
#include <unistd.h>
#endif

// changed by initialed85
extern int gethostname(char *, size_t);
extern int close(int);

extern cvar_t hostname;

static int net_acceptsocket = -1; // socket for fielding new connections
static int net_controlsocket; // socket for discovering other servers, before
                              // it's been made into a broadcast socket
static int net_broadcastsocket =
    0; // net_controlsocket, but after it's been made into a broadcast socket
static struct qsockaddr broadcastaddr;

static unsigned long myAddr;

#include "net_udp.h"

//=============================================================================

int UDP_Init(void) {
  struct hostent *local;
  char buff[MAXHOSTNAMELEN];
  struct qsockaddr addr;
  char *colon;

  if (COM_CheckParm("-noudp"))
    return -1;

  Con_Printf("UDP Initializing...\n");

  // determine my name & address
  gethostname(buff, MAXHOSTNAMELEN);
  local = gethostbyname(buff);
  myAddr = *(int *)local->h_addr_list[0];

  // if the quake hostname isn't set, set it to the machine name
  if (Q_strcmp(hostname.string, "UNNAMED") == 0) {
    buff[15] = 0;
    Cvar_Set("hostname", buff);
  }

  // net_controlsocket is used by the client to carry broadcasts while searching
  // for servers; I figure we can leave this behaviour as-is because broadcast
  // only works on your layer 2 segment (and so the NAT issues we're trying to
  // work around don't apply)
  Con_Printf("UDP Opening control socket on port %d for server discovery...\n",
             0);
  if ((net_controlsocket = UDP_OpenSocket(0)) == -1)
    Sys_Error("UDP_Init: Unable to open control socket\n");

  ((struct sockaddr_in *)&broadcastaddr)->sin_family = AF_INET;
  ((struct sockaddr_in *)&broadcastaddr)->sin_addr.s_addr = INADDR_BROADCAST;
  ((struct sockaddr_in *)&broadcastaddr)->sin_port = htons(DEFAULTnet_hostport);

  UDP_GetSocketAddr(net_controlsocket, &addr);
  Q_strcpy(my_tcpip_address, UDP_AddrToString(&addr));
  colon = Q_strrchr(my_tcpip_address, ':');
  if (colon)
    *colon = 0;

  Con_Printf("UDP Initialized at %s\n", UDP_AddrToString(&addr));
  tcpipAvailable = true;

  return net_controlsocket;
}

//=============================================================================

void UDP_Shutdown(void) {
  UDP_Listen(false);
  UDP_CloseSocket(net_controlsocket);
}

//=============================================================================

void UDP_Listen(qboolean state) {
  // enable listening
  if (state) {
    if (net_acceptsocket != -1)
      return;

    Con_Printf("UDP Opening socket on port %d to run a server...\n",
               net_hostport);
    if ((net_acceptsocket = UDP_OpenSocket(net_hostport)) == -1)
      Sys_Error("UDP_Listen: Unable to open accept socket\n");

    Con_Printf("UDP Re-opening control socket on port %d to answer server "
               "discovery...\n",
               DEFAULTnet_hostport);
    UDP_CloseSocket(net_controlsocket);
    if ((net_controlsocket = UDP_OpenSocket(DEFAULTnet_hostport)) == -1)
      Sys_Error("UDP_Init: Unable to open control socket\n");

    return;
  }

  // disable listening
  if (net_acceptsocket == -1)
    return;
  UDP_CloseSocket(net_acceptsocket);
  net_acceptsocket = -1;
}

//=============================================================================

void UDP_AddrToString2(struct qsockaddr *addr, char buffer[22]);

int UDP_OpenSocket(int port) {
  int newsocket;
  struct sockaddr_in address;
  qboolean _true = true;
  int i = 1;

  if ((newsocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
    Con_Printf("UDP_OpenSocket: failed at socket()\n");
    return -1;
  }

  if (ioctl(newsocket, FIONBIO, (char *)&_true) == -1) {
    Con_Printf("UDP_OpenSocket: failed at ioctl()\n");
    goto ErrorReturn;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
  if (bind(newsocket, (void *)&address, sizeof(address)) == -1) {
    char addr[22];
    UDP_AddrToString2((struct qsockaddr *)&address, addr);
    Con_Printf("UDP_OpenSocket: failed at bind(%s)\n", addr);
    goto ErrorReturn;
  }

  struct qsockaddr boundaddr;

  UDP_GetSocketAddr(newsocket, &boundaddr);

  Con_Printf("UDP_OpenSocket: bind socket for %s returned %s\n",
             UDP_AddrToString((struct qsockaddr *)&address),
             UDP_AddrToString((struct qsockaddr *)&boundaddr));

  return newsocket;

ErrorReturn:
  close(newsocket);
  return -1;
}

//=============================================================================

int UDP_CloseSocket(int socket) {
  if (socket == net_broadcastsocket)
    net_broadcastsocket = 0;

  if (socket == net_acceptsocket) {
    Con_Printf(
        "UDP Re-opening control socket on port %d for server discovery...\n",
        0);

    close(net_controlsocket);

    net_controlsocket = 0;

    if ((net_controlsocket = UDP_OpenSocket(0)) == -1)
      Sys_Error("UDP_Init: Unable to open control socket\n");
  }

  return close(socket);
}

//=============================================================================
/*
============
PartialIPAddress

this lets you type only as much of the net address as required, using
the local network components to fill in the rest
============
*/
static int PartialIPAddress(char *in, struct qsockaddr *hostaddr) {
  char buff[256];
  char *b;
  int addr;
  int num;
  int mask;
  int run;
  int port;

  buff[0] = '.';
  b = buff;
  strcpy(buff + 1, in);
  if (buff[1] == '.')
    b++;

  addr = 0;
  mask = -1;
  while (*b == '.') {
    b++;
    num = 0;
    run = 0;
    while (!(*b < '0' || *b > '9')) {
      num = num * 10 + *b++ - '0';
      if (++run > 3)
        return -1;
    }
    if ((*b < '0' || *b > '9') && *b != '.' && *b != ':' && *b != 0)
      return -1;
    if (num < 0 || num > 255)
      return -1;
    mask <<= 8;
    addr = (addr << 8) + num;
  }

  if (*b++ == ':')
    port = Q_atoi(b);
  else
    port = net_hostport;

  hostaddr->sa_family = AF_INET;
  ((struct sockaddr_in *)hostaddr)->sin_port = htons((short)port);
  ((struct sockaddr_in *)hostaddr)->sin_addr.s_addr =
      (myAddr & htonl(mask)) | htonl(addr);

  return 0;
}
//=============================================================================

int UDP_Connect(int socket, struct qsockaddr *addr) { return 0; }

//=============================================================================

int UDP_CheckNewConnections(void) {
  unsigned long available;

  if (net_acceptsocket == -1)
    return -1;

  if (ioctl(net_acceptsocket, FIONREAD, &available) == -1)
    Sys_Error("UDP: ioctlsocket (FIONREAD) failed for net_acceptsocket\n");
  if (available)
    return net_acceptsocket;
  return -1;
}

//=============================================================================

int UDP_CheckNewConnectionsDiscoveryOnly(void) {
  unsigned long available;

  if (net_controlsocket == -1)
    return -1;

  if (ioctl(net_controlsocket, FIONREAD, &available) == -1)
    Sys_Error("UDP: ioctlsocket (FIONREAD) failed for net_controlsocket\n");
  if (available)
    return net_controlsocket;
  return -1;
}

//=============================================================================

int UDP_Read(int socket, byte *buf, int len, struct qsockaddr *addr) {
  int addrlen = sizeof(struct qsockaddr);
  int ret;

  ret = recvfrom(socket, buf, len, 0, (struct sockaddr *)addr, &addrlen);

  // in macOS, sockaddr has a "__uint8_t sa_len" in front of "sa_family_t
  // sa_family" so we have to shift to avoid that byte ending up qsockaddr
  // "short sa_family"
#ifdef NeXT
  addr->sa_family >>= 8;
#endif

  if (ret == -1 && (errno == EWOULDBLOCK || errno == ECONNREFUSED))
    return 0;

  return ret;
}

//=============================================================================

int UDP_MakeSocketBroadcastCapable(int socket) {
  int i = 1;

  // make this socket broadcast capable
  if (setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) < 0)
    return -1;
  net_broadcastsocket = socket;

  return 0;
}

//=============================================================================

int UDP_Broadcast(int socket, byte *buf, int len) {
  int ret;

  if (socket != net_broadcastsocket) {
    if (net_broadcastsocket != 0)
      Sys_Error("Attempted to use multiple broadcasts sockets\n");
    ret = UDP_MakeSocketBroadcastCapable(socket);
    if (ret == -1) {
      Con_Printf("Unable to make socket broadcast capable\n");
      return ret;
    }
  }

  return UDP_Write(socket, buf, len, &broadcastaddr);
}

//=============================================================================

int UDP_Write(int socket, byte *buf, int len, struct qsockaddr *addr) {
  int ret;

  ret = sendto(socket, buf, len, 0, (struct sockaddr *)addr,
               sizeof(struct qsockaddr));
  if (ret == -1 && errno == EWOULDBLOCK)
    return 0;
  return ret;
}

//=============================================================================

// TODO: you have to be careful with this function because the buffer is
// statically assigned
char *UDP_AddrToString(struct qsockaddr *addr) {
  static char buffer[22];
  int haddr;

  haddr = ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr);

  sprintf(buffer, "%d.%d.%d.%d:%d", (haddr >> 24) & 0xff, (haddr >> 16) & 0xff,
          (haddr >> 8) & 0xff, haddr & 0xff,
          ntohs(((struct sockaddr_in *)addr)->sin_port));

  return buffer;
}

//=============================================================================

void UDP_AddrToString2(struct qsockaddr *addr, char buffer[22]) {
  int haddr;

  haddr = ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr);

  sprintf(buffer, "%d.%d.%d.%d:%d", (haddr >> 24) & 0xff, (haddr >> 16) & 0xff,
          (haddr >> 8) & 0xff, haddr & 0xff,
          ntohs(((struct sockaddr_in *)addr)->sin_port));
}

//=============================================================================

int UDP_StringToAddr(char *string, struct qsockaddr *addr) {
  int ha1, ha2, ha3, ha4, hp;
  int ipaddr;

  sscanf(string, "%d.%d.%d.%d:%d", &ha1, &ha2, &ha3, &ha4, &hp);
  ipaddr = (ha1 << 24) | (ha2 << 16) | (ha3 << 8) | ha4;

  addr->sa_family = AF_INET;
  ((struct sockaddr_in *)addr)->sin_addr.s_addr = htonl(ipaddr);
  ((struct sockaddr_in *)addr)->sin_port = htons(hp);
  return 0;
}

//=============================================================================

int UDP_GetSocketAddr(int socket, struct qsockaddr *addr) {
  int addrlen = sizeof(struct qsockaddr);

  Q_memset(addr, 0, sizeof(struct qsockaddr));
  getsockname(socket, (struct sockaddr *)addr, &addrlen);

  // commented by initialed85
  // unsigned int a;
  // a = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
  // if (a == 0 || a == inet_addr("127.0.0.1"))
  //   ((struct sockaddr_in *)addr)->sin_addr.s_addr = myAddr;

  return 0;
}

//=============================================================================

int UDP_GetNameFromAddr(struct qsockaddr *addr, char *name) {
  struct hostent *hostentry;

  hostentry = gethostbyaddr((char *)&((struct sockaddr_in *)addr)->sin_addr,
                            sizeof(struct in_addr), AF_INET);
  if (hostentry) {
    Q_strncpy(name, (char *)hostentry->h_name, NET_NAMELEN - 1);
    return 0;
  }

  Q_strcpy(name, UDP_AddrToString(addr));
  return 0;
}

//=============================================================================

int UDP_GetAddrFromName(char *name, struct qsockaddr *addr) {
  struct hostent *hostentry;

  if (name[0] >= '0' && name[0] <= '9')
    return PartialIPAddress(name, addr);

  hostentry = gethostbyname(name);
  if (!hostentry)
    return -1;

  addr->sa_family = AF_INET;
  ((struct sockaddr_in *)addr)->sin_port = htons(net_hostport);
  ((struct sockaddr_in *)addr)->sin_addr.s_addr =
      *(int *)hostentry->h_addr_list[0];

  return 0;
}

//=============================================================================

int UDP_AddrCompare(struct qsockaddr *addr1, struct qsockaddr *addr2) {
  // TODO: because in macOS sockaddr has a "__uint8_t sa_len" in front of
  // "sa_family_t sa_family" so we have to shift to avoid that byte ending up
  // qsockaddr "short sa_family"- unfortunately its hard to know where we do
  // and don't have to do this so we just shoddily compare the strings for now
  // if (addr1->sa_family != addr2->sa_family)
  //   return -1;

  // if (((struct sockaddr_in *)addr1)->sin_addr.s_addr !=
  //     ((struct sockaddr_in *)addr2)->sin_addr.s_addr)
  //   return -1;

  // if (((struct sockaddr_in *)addr1)->sin_port !=
  //     ((struct sockaddr_in *)addr2)->sin_port)
  //   return 1;

  // return 0;

  char a[22];
  char b[22];

  UDP_AddrToString2(addr1, a);
  UDP_AddrToString2(addr2, b);

  char *colon_a = strchr(a, ':');
  char *colon_b = strchr(b, ':');

  if (colon_a != NULL && colon_b != NULL) {
    *colon_a = '\0';
    char *host_a = a;
    char *port_a = colon_a + 1;
    if (strcmp(host_a, "127.0.0.1") == 0) {
      host_a = "0.0.0.0";
    }

    *colon_b = '\0';
    char *host_b = b;
    char *port_b = colon_b + 1;
    if (strcmp(host_b, "127.0.0.1") == 0) {
      host_b = "0.0.0.0";
    }

    return strcmp(host_a, host_b) == 0 && strcmp(port_a, port_b) == 0 ? 0 : -1;
  }

  return strcmp(a, b) == 0 ? 0 : -1;
}

//=============================================================================

int UDP_GetSocketPort(struct qsockaddr *addr) {
  return ntohs(((struct sockaddr_in *)addr)->sin_port);
}

int UDP_SetSocketPort(struct qsockaddr *addr, int port) {
  ((struct sockaddr_in *)addr)->sin_port = htons(port);
  return 0;
}

//=============================================================================
