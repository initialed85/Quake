/*
added by initialed85
*/

//
// net_ws.h: WebSocket network implementation (for WASM / Emscripten only)
//

int WS_Init(void);
void WS_Shutdown(void);
void WS_Listen(qboolean state);
int WS_OpenSocket(int port);
int WS_CloseSocket(int socket);
int WS_Connect(int socket, struct qsockaddr *addr);
int WS_CheckNewConnections(void);
int WS_CheckNewConnectionsDiscoveryOnly(void);
int WS_Read(int socket, byte *buf, int len, struct qsockaddr *addr);
int WS_Write(int socket, byte *buf, int len, struct qsockaddr *addr);
int WS_Broadcast(int socket, byte *buf, int len);
char *WS_AddrToString(struct qsockaddr *addr);
int WS_StringToAddr(char *string, struct qsockaddr *addr);
int WS_GetSocketAddr(int socket, struct qsockaddr *addr);
int WS_GetNameFromAddr(struct qsockaddr *addr, char *name);
int WS_GetAddrFromName(char *name, struct qsockaddr *addr);
int WS_AddrCompare(struct qsockaddr *addr1, struct qsockaddr *addr2);
int WS_GetSocketPort(struct qsockaddr *addr);
int WS_SetSocketPort(struct qsockaddr *addr, int port);
