/*
added by initialed85
*/

//
// net_ws.c: WebSocket network implementation (for WASM / Emscripten only)
//

#include "quakedef.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <arpa/inet.h>

#include "websockets/websockets.h"

#include "net_ws.h"

#define DEFAULT_WEBSOCKET_URL "ws://localhost/ws"
#define MAX_WEBSOCKETS 256
#define SOCKFD_OFFSET 3

int next_websocket = 0;
websocket_t *websockets[MAX_WEBSOCKETS] = {};

static char websocketurl[1024] = {};

char *ourhoststr;
char *ouraddrstr;
struct qsockaddr *ouraddr;

char *broadcastaddrstr;
struct qsockaddr *broadcastaddr;

struct qsockaddr *tempaddr;

static int net_controlsocket = -1;
static int net_acceptsocket = -1;

//=============================================================================

int WS_Init(void) {
  int i;
  char clientid[15];

  i = COM_CheckParm("-websocketurl");
  if ((i) && (i != (com_argc - 1))) {
    strcpy(websocketurl, com_argv[i + 1]);
  } else {
    strcpy(websocketurl, DEFAULT_WEBSOCKET_URL);
  }
  printf("websocketurl: %s\n", websocketurl);

  i = COM_CheckParm("-clientid");
  if ((i) && (i != (com_argc - 1))) {
    strcpy(clientid, com_argv[i + 1]);
  } else {
    strcpy(clientid, "127.0.0.1");
  }
  printf("clientid: %s\n", clientid);

  ourhoststr = calloc(15, sizeof(char));
  sprintf(ourhoststr, "%s", clientid);
  printf("ourhoststr: %s\n", ourhoststr);

  ouraddrstr = calloc(22, sizeof(char));
  sprintf(ouraddrstr, "%s:%d", ourhoststr, net_hostport);
  printf("ouraddrstr: %s\n", ouraddrstr);

  ouraddr = calloc(1, sizeof(struct qsockaddr));
  string_to_addr(ouraddrstr, ouraddr);
  printf("ouraddr: %s\n", addr_to_string_static(ouraddr));

  broadcastaddrstr = calloc(22, sizeof(char));
  sprintf(broadcastaddrstr, "255.255.255.255:%d", DEFAULTnet_hostport);
  printf("broadcastaddrstr: %s\n", broadcastaddrstr);

  broadcastaddr = calloc(1, sizeof(struct qsockaddr));
  string_to_addr(broadcastaddrstr, broadcastaddr);
  printf("broadcastaddr: %s\n", addr_to_string_static(ouraddr));

  tempaddr = calloc(1, sizeof(struct qsockaddr));

  srand(time(NULL));
  // net_controlsocket = WS_OpenSocket(0);
  net_controlsocket = WS_OpenSocket(DEFAULTnet_hostport);
  printf("net_controlsocket: %d\n", net_controlsocket);

  tcpipAvailable = true;
  printf("net_controlsocket: %d\n", net_controlsocket);

  Con_Printf("WS_Init: WebSockets initialized\n");

  return net_controlsocket;
}

//=============================================================================

void WS_Shutdown(void) {
  WS_CloseSocket(net_controlsocket);

  Con_Printf("WS_Shutdown: WebSockets shut down\n");
}

//=============================================================================

void WS_Listen(qboolean state) {
  if (state) {
    if (net_acceptsocket != -1) {
      return;
    }

    net_acceptsocket = WS_OpenSocket(net_hostport);
    Con_Printf("WS_Listen : state=%d, net_acceptsocket = %d\n", state,
               net_acceptsocket);

    // WS_CloseSocket(net_controlsocket);
    // net_controlsocket = WS_OpenSocket(DEFAULTnet_hostport);
    // Con_Printf("WS_Listen : state=%d, net_controlsocket = %d\n", state,
    //            net_acceptsocket);
  } else {
    WS_CloseSocket(net_acceptsocket);
    Con_Printf("WS_Listen : state=%d, net_acceptsocket = %d\n", state,
               net_acceptsocket);

    // WS_CloseSocket(net_controlsocket);
    // net_controlsocket = WS_OpenSocket(0);
    // Con_Printf("WS_Listen : state=%d, net_controlsocket = %d\n", state,
    //            net_acceptsocket);
  }
}

//=============================================================================

void WS_AddrToString2(struct qsockaddr *addr, char buffer[22]);

int WS_OpenSocket(int port) {
  int adjusted_port =
      (port == 0) ? (32768 + (rand() % (65535 - 32768 + 1))) : port;

  int i;
  int socket;

  char thisaddrstr[22] = {};
  struct qsockaddr *thisaddr = calloc(1, sizeof(struct qsockaddr));

  sprintf(thisaddrstr, "%s:%d", ourhoststr, adjusted_port);
  string_to_addr(thisaddrstr, thisaddr);

  for (i = 0; i < MAX_WEBSOCKETS; i++) {
    if (websockets[i] != NULL) {
      continue;
    }

    websockets[i] = websocket_init();
    if (websocket_open(websockets[i], websocketurl, thisaddr) != 0) {
      websocket_free(websockets[i]);
      websockets[i] = NULL;
      return -1;
    }

    socket = i + SOCKFD_OFFSET;

    Con_Printf("WS_OpenSocket : port = %d, adjusted_port = %d, i = %d, "
               "socket = %d\n",
               port, adjusted_port, i, socket);

    emscripten_sleep(1);

    return socket;
  }

  return -1;
}

//=============================================================================

int WS_CloseSocket(int socket) {
  int i = socket - SOCKFD_OFFSET;

  if (websockets[i] != NULL) {
    websocket_close(websockets[i]);
    websocket_free(websockets[i]);
    Con_Printf("WS_CloseSocket : socket = %d, i = %d\n", socket, i);
  } else {
    Con_Printf("WS_CloseSocket : socket = %d, i = %d (was already closed?)\n",
               socket, i);
  }

  if (socket == net_controlsocket) {
    net_controlsocket = -1;
  } else if (socket == net_acceptsocket) {
    net_acceptsocket = -1;
  }

  return 0;
}

//=============================================================================

int WS_Connect(int socket, struct qsockaddr *addr) {
  emscripten_sleep(1);

  return 0;
}

//=============================================================================

int WS_CheckNewConnections(void) {
  if (net_acceptsocket < 0) {
    return -1;
  }

  int i = net_acceptsocket - SOCKFD_OFFSET;

  return websocket_peek(websockets[i]) > 0 ? net_acceptsocket : -1;
}

//=============================================================================

int WS_CheckNewConnectionsDiscoveryOnly(void) {
  if (net_controlsocket < 0) {
    return -1;
  }

  int i = net_controlsocket - SOCKFD_OFFSET;

  return websocket_peek(websockets[i]) > 0 ? net_controlsocket : -1;
}

//=============================================================================

int WS_Read(int socket, byte *buf, int len, struct qsockaddr *addr) {
  if (socket < 0) {
    return -1;
  }

  int i = socket - SOCKFD_OFFSET;

  int ret = websocket_read(websockets[i], addr, tempaddr, buf);

  return ret == -1 ? 0 : ret;
}

//=============================================================================

int WS_MakeSocketBroadcastCapable(int socket) { return socket; }

//=============================================================================

int WS_Broadcast(int socket, byte *buf, int len) {
  if (socket < 0) {
    return -1;
  }

  int i = socket - SOCKFD_OFFSET;

  return websocket_write(websockets[i], buf, len, broadcastaddr, false,
                         socket == net_controlsocket);
}

//=============================================================================

int WS_Write(int socket, byte *buf, int len, struct qsockaddr *addr) {
  if (socket < 0) {
    return -1;
  }

  int i = socket - SOCKFD_OFFSET;

  return websocket_write(websockets[i], buf, len, addr, false,
                         socket == net_controlsocket);
}

//=============================================================================

// TODO: you have to be careful with this function because the buffer is
// statically assigned
char *WS_AddrToString(struct qsockaddr *addr) {
  return addr_to_string_static(addr);
}

//=============================================================================

void WS_AddrToString2(struct qsockaddr *addr, char buffer[22]) {
  addr_to_string(addr, buffer);
}

//=============================================================================

int WS_StringToAddr(char *string, struct qsockaddr *addr) {
  return string_to_addr(string, addr);
}

//=============================================================================

int WS_GetSocketAddr(int socket, struct qsockaddr *addr) {
  if (socket < 0) {
    return -1;
  }

  int i = socket - SOCKFD_OFFSET;

  memcpy(addr, websockets[i]->src, sizeof(struct qsockaddr));

  return 0;
}

//=============================================================================

int WS_GetNameFromAddr(struct qsockaddr *addr, char *name) {
  strcpy(name, addr_to_string_static(addr));

  return 0;
}

//=============================================================================

int WS_GetAddrFromName(char *name, struct qsockaddr *addr) {
  return string_to_addr(name, addr);
}

//=============================================================================

int WS_AddrCompare(struct qsockaddr *addr1, struct qsockaddr *addr2) {
  char addr1str[22] = {};
  char addr2str[22] = {};

  addr_to_string(addr1, addr1str);
  addr_to_string(addr2, addr2str);

  int eq = strcmp(addr1str, addr2str);

  return eq == 0 ? 0 : -1;
}

//=============================================================================

int WS_GetSocketPort(struct qsockaddr *addr) {
  return ntohs(((struct sockaddr_in *)addr)->sin_port);
}

int WS_SetSocketPort(struct qsockaddr *addr, int port) {
  ((struct sockaddr_in *)addr)->sin_port = htons(port);
  return 0;
}

//=============================================================================
