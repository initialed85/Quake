#include "emscripten/websocket.h"

#define HEADER_LEN 4
#define RELEVANT_QSOCKADDR_DATA_LEN 6
#define MAX_WS_MESSAGE_SIZE 65520
#define REAL_MAX_WS_MESSAGE_SIZE 65536
#define MAX_WS_MESSAGES 256

// ['0x55', '0x5b', '0x14', '0x18']
static char bridge_header[HEADER_LEN] = {85, 91, 20, 24};

// ['0x42', '0x63', '0x2c', '0x16']
static char control_header[HEADER_LEN] = {66, 99, 44, 22};

// '0xb', '0x21', '0x21', '0x4d'
static char data_header[HEADER_LEN] = {11, 33, 33, 77};

typedef struct {
  struct qsockaddr src;
  struct qsockaddr dst;
  int length;
  byte data[REAL_MAX_WS_MESSAGE_SIZE];
} message_t;

typedef struct {
  byte read_index;
  byte write_index;
  message_t messages[MAX_WS_MESSAGES];
} messages_t;

typedef struct {
  const char *url;
  struct qsockaddr *src;
  char srcstr[22];
  char *buf;
  messages_t messages;
  EMSCRIPTEN_WEBSOCKET_T sock;
  qboolean should_be_connected;
  qboolean is_connected;
} websocket_t;

int string_to_addr(char *string, struct qsockaddr *addr);

char *addr_to_string_static(struct qsockaddr *addr);

int addr_to_string(struct qsockaddr *addr, char *string);

websocket_t *websocket_init();

int websocket_open(websocket_t *self, const char *url, struct qsockaddr *src);

int websocket_close(websocket_t *self);

int websocket_write_spoof(websocket_t *self, char *buf, int len,
                          struct qsockaddr *dst, qboolean bridge,
                          qboolean control, struct qsockaddr *src,
                          char *srcstr);

int websocket_write(websocket_t *self, char *buf, int len,
                    struct qsockaddr *dst, qboolean bridge, qboolean control);

int websocket_read(websocket_t *self, struct qsockaddr *src,
                   struct qsockaddr *dst, char *buf);

int websocket_peek(websocket_t *self);

int websocket_free(websocket_t *self);
