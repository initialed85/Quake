#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "../quakedef.h"

#include <emscripten.h>
#include "emscripten/websocket.h"

#include "websockets.h"

int string_to_addr(char *string, struct qsockaddr *addr) {
  long long ha1, ha2, ha3, ha4;
  int hp;
  long long ipaddr;

  sscanf(string, "%lld.%lld.%lld.%lld:%d", &ha1, &ha2, &ha3, &ha4, &hp);
  ipaddr = (ha1 << 24) | (ha2 << 16) | (ha3 << 8) | ha4;

  addr->sa_family = AF_INET;
  ((struct sockaddr_in *)addr)->sin_addr.s_addr = htonl(ipaddr);
  ((struct sockaddr_in *)addr)->sin_port = htons(hp);
  return 0;
}

// warning: static return value; really only suitable for immediate usage
char *addr_to_string_static(struct qsockaddr *addr) {
  static char buffer[22];
  long long haddr;

  haddr = ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr);

  sprintf(buffer, "%lld.%lld.%lld.%lld:%d", (haddr >> 24) & 0xff,
          (haddr >> 16) & 0xff, (haddr >> 8) & 0xff, haddr & 0xff,
          ntohs(((struct sockaddr_in *)addr)->sin_port));

  return buffer;
}

int addr_to_string(struct qsockaddr *addr, char *buffer) {
  long long haddr;

  haddr = ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr);

  sprintf(buffer, "%lld.%lld.%lld.%lld:%d", (haddr >> 24) & 0xff,
          (haddr >> 16) & 0xff, (haddr >> 8) & 0xff, haddr & 0xff,
          ntohs(((struct sockaddr_in *)addr)->sin_port));

  return 0;
}

EM_BOOL _websocket_onopen(int eventType,
                          const EmscriptenWebSocketOpenEvent *websocketEvent,
                          void *userData) {
  websocket_t *self = (websocket_t *)userData;

  printf("websockets.c %s : onopen\n", self->srcstr);

  self->connected = true;

  struct qsockaddr *tx_src = calloc(1, sizeof(struct qsockaddr));
  struct qsockaddr *tx_dst = calloc(1, sizeof(struct qsockaddr));
  char *tx_buf = calloc(1024, sizeof(char));

  sprintf(tx_buf, "g'day!");

  websocket_write(self, tx_buf, (unsigned long)strlen(tx_buf), tx_dst, true,
                  false);

  free(tx_src);
  free(tx_dst);
  free(tx_buf);

  return EM_TRUE;
}

EM_BOOL _websocket_onerror(int eventType,
                           const EmscriptenWebSocketErrorEvent *websocketEvent,
                           void *userData) {
  websocket_t *self = (websocket_t *)userData;

  printf("websockets.c %s : onerror\n", self->srcstr);

  return EM_TRUE;
}

EM_BOOL _websocket_onclose(int eventType,
                           const EmscriptenWebSocketCloseEvent *websocketEvent,
                           void *userData) {
  websocket_t *self = (websocket_t *)userData;

  printf("websockets.c %s : onclose\n", self->srcstr);

  self->sock = 0;
  self->connected = false;

  return EM_TRUE;
}

EM_BOOL
_websocket_onmessage(int eventType,
                     const EmscriptenWebSocketMessageEvent *websocketEvent,
                     void *userData) {
  websocket_t *self = (websocket_t *)userData;

  // printf("websockets.c %s : onmessage; read %d bytes\n", self->srcstr,
  //        websocketEvent->numBytes);

  int delta = self->messages.write_index - self->messages.read_index;
  if (delta < 0) {
    delta += MAX_WS_MESSAGES;
  }

  if (self->messages.write_index + 1 == self->messages.read_index) {
    printf("websockets.c %s : onmessage; error: self->messages buffer overflow "
           "(r: %d, w: "
           "%d, d: %d)\n",
           self->srcstr, self->messages.read_index, self->messages.write_index,
           delta);
    return EM_TRUE;
  }

  if (websocketEvent->numBytes <
      HEADER_LEN + RELEVANT_QSOCKADDR_DATA_LEN + RELEVANT_QSOCKADDR_DATA_LEN) {
    printf("websockets.c %s : onmessage; warning: message too short\n",
           self->srcstr);
    return EM_FALSE;
  }

  memcpy(self->messages.messages[self->messages.write_index].src.sa_data,
         websocketEvent->data + HEADER_LEN, RELEVANT_QSOCKADDR_DATA_LEN);

  memcpy(self->messages.messages[self->messages.write_index].dst.sa_data,
         websocketEvent->data + HEADER_LEN + RELEVANT_QSOCKADDR_DATA_LEN,
         RELEVANT_QSOCKADDR_DATA_LEN);

  self->messages.messages[self->messages.write_index].length =
      (unsigned int)(websocketEvent->numBytes -
                     (HEADER_LEN + RELEVANT_QSOCKADDR_DATA_LEN +
                      RELEVANT_QSOCKADDR_DATA_LEN));

  memcpy(self->messages.messages[self->messages.write_index].data,
         websocketEvent->data + (HEADER_LEN + RELEVANT_QSOCKADDR_DATA_LEN +
                                 RELEVANT_QSOCKADDR_DATA_LEN),
         websocketEvent->numBytes - (HEADER_LEN + RELEVANT_QSOCKADDR_DATA_LEN +
                                     RELEVANT_QSOCKADDR_DATA_LEN));

  char srcstring[22] = {};
  char dststring[22] = {};
  addr_to_string(&self->messages.messages[self->messages.read_index].src,
                 srcstring);
  addr_to_string(&self->messages.messages[self->messages.read_index].dst,
                 dststring);
  // printf("websockets.c %s : onmessage; read %d bytes (%s -> %s)\n",
  //        self->srcstr,
  //        self->messages.messages[self->messages.write_index].length, srcstring,
  //        dststring);

  self->messages.write_index++;

  return EM_TRUE;
}

// warning: this allocates websocket_t *self for you; it's on you to free it
// when you're done!
websocket_t *websocket_init() {
  websocket_t *self = calloc(1, sizeof(websocket_t));

  int i = 0;
  for (i = 0; i < MAX_WS_MESSAGES; i++) {
    self->messages.messages[i].length = -1;
  }

  return self;
}

// warning: this allocates char *buf for you; it won't be free'd until you call
// websocket_close (ish)!
int websocket_open(websocket_t *self, const char *url, struct qsockaddr *src) {
  char srcstr[32];
  addr_to_string(src, srcstr);

  if (self == NULL) {
    printf("websockets.c %s : websocket_open; error - cannot open; self "
           "unexpectedly NULL\n",
           srcstr);
    return -1;
  }

  if (self->sock > 0) {
    printf(
        "websockets.c %s : websocket_open; error - cannot open; sock already "
        "initialized!\n",
        srcstr);
    return -1;
  }

  if (!emscripten_websocket_is_supported()) {
    printf("websockets.c %s : websocket_open; error - cannot open; emscripten "
           "says WebSockets aren't available\n",
           srcstr);
    return -1;
  }

  EmscriptenWebSocketCreateAttributes ws_attrs = {
      url,
      NULL,
      EM_TRUE,
  };

  self->src = src;
  memcpy(self->srcstr, srcstr, sizeof(struct qsockaddr));

  if (self->buf == NULL) {
    self->buf = calloc(MAX_WS_MESSAGE_SIZE, sizeof(char));
  }

  self->sock = emscripten_websocket_new(&ws_attrs);
  if (self->sock < 1) {
    printf(
        "websockets.c %s : websocket_open; error - cannot open; unknown error "
        "%d\n",
        self->srcstr, self->sock);
    self->sock = 0;
    return -1;
  }

  self->connected = false;

  emscripten_websocket_set_onopen_callback(self->sock, (void *)self,
                                           _websocket_onopen);

  emscripten_websocket_set_onerror_callback(self->sock, (void *)self,
                                            _websocket_onerror);

  emscripten_websocket_set_onclose_callback(self->sock, (void *)self,
                                            _websocket_onclose);

  emscripten_websocket_set_onmessage_callback(self->sock, (void *)self,
                                              _websocket_onmessage);

  printf("websockets.c %s : websocket_open; opening %s as %s...\n",
         self->srcstr, url, addr_to_string_static(self->src));

  return 0;
};

int websocket_close(websocket_t *self) {
  if (self == NULL) {
    printf("websockets.c %s : websocket_close; error - cannot close; self "
           "unexpectedly NULL\n",
           self->srcstr);
    return 1;
  }

  if (self->sock < 1) {
    printf("websockets.c %s : websocket_close; error - cannot close; sock not "
           "initialized!\n",
           self->srcstr);
    return 1;
  }

  printf("websockets.c %s : websocket_close\n", self->srcstr);

  emscripten_websocket_close(self->sock, 1000,
                             "going away (websocket_close called)");

  return 0;
};

int websocket_write_spoof(websocket_t *self, char *buf, int len,
                          struct qsockaddr *dst, qboolean bridge,
                          qboolean control, struct qsockaddr *src,
                          char *srcstr) {
  if (self == NULL) {
    printf("websockets.c %s : websocket_write; error - cannot write; self "
           "unexpectedly NULL\n",
           srcstr);
    return 1;
  }

  if (self->sock < 1) {
    printf("websockets.c %s : websocket_write; error - cannot write; sock not "
           "initialized!\n",
           srcstr);
    return 1;
  }

  if (!self->connected) {
    printf("websockets.c %s : websocket_write; error - cannot write; sock yet "
           "connected!\n",
           srcstr);
    return 1;
  }

  if (bridge && control) {
    printf(
        "websockets.c %s : websocket_write; error - cannot write; both bridge "
        "and control "
        "arguments cannot be set!\n",
        srcstr);
    return 1;
  }

  char *header;
  if (bridge)
    header = bridge_header;
  else if (control)
    header = control_header;
  else
    header = data_header;

  memcpy(self->buf, header, HEADER_LEN);

  memcpy(self->buf + HEADER_LEN, src->sa_data, RELEVANT_QSOCKADDR_DATA_LEN);

  memcpy(self->buf + HEADER_LEN + RELEVANT_QSOCKADDR_DATA_LEN, dst->sa_data,
         RELEVANT_QSOCKADDR_DATA_LEN);

  memcpy(self->buf + HEADER_LEN + RELEVANT_QSOCKADDR_DATA_LEN +
             RELEVANT_QSOCKADDR_DATA_LEN,
         buf, len);

  EMSCRIPTEN_RESULT res;
  int real_len = len + HEADER_LEN + RELEVANT_QSOCKADDR_DATA_LEN +
                 RELEVANT_QSOCKADDR_DATA_LEN;

  if ((res = emscripten_websocket_send_binary(self->sock, (void *)self->buf,
                                              real_len)) != 0) {
    printf("websockets.c %s : websocket_write; error - cannot write; unknown "
           "error %d!\n",
           srcstr, res);

    return 1;
  }

  char srcstring[22] = {};
  char dststring[22] = {};
  addr_to_string(src, srcstring);
  addr_to_string(dst, dststring);
  // printf("websockets.c %s : websocket_write; wrote %d bytes (%s -> %s)\n",
  //        srcstr, len, srcstring, dststring);

  return len;
}

int websocket_write(websocket_t *self, char *buf, int len,
                    struct qsockaddr *dst, qboolean bridge, qboolean control) {
  return websocket_write_spoof(self, buf, len, dst, bridge, control, self->src,
                               self->srcstr);
}

int websocket_read(websocket_t *self, struct qsockaddr *src,
                   struct qsockaddr *dst, char *buf) {
  if (self == NULL) {
    printf("websockets.c %s : websocket_read; error - cannot read; self "
           "unexpectedly NULL\n",
           self->srcstr);
    return -1;
  }

  if (self->messages.messages[self->messages.read_index].length < 0) {
    // printf("websockets.c %s : websocket_read; nothing to read (length =
    // -1)\n",
    //        self->srcstr);
    return -1;
  }

  int len = self->messages.messages[self->messages.read_index].length;

  memcpy(src, &self->messages.messages[self->messages.read_index].src,
         sizeof(struct qsockaddr));

  memcpy(dst, &self->messages.messages[self->messages.read_index].dst,
         sizeof(struct qsockaddr));

  memcpy(buf, &self->messages.messages[self->messages.read_index].data, len);

  char srcstring[22] = {};
  char dststring[22] = {};
  addr_to_string(&self->messages.messages[self->messages.read_index].src,
                 srcstring);
  addr_to_string(&self->messages.messages[self->messages.read_index].dst,
                 dststring);
  // printf("websockets.c %s : websocket_read; read %d bytes (%s -> %s)\n",
  //        self->srcstr, len, srcstring, dststring);

  self->messages.messages[self->messages.read_index].length = -1;
  self->messages.read_index++;

  return len;
}

int websocket_peek(websocket_t *self) {
  if (self == NULL) {
    printf("websockets.c %s : websocket_peek; error - cannot peek; self "
           "unexpectedly NULL\n",
           self->srcstr);
    return -1;
  }

  if (self->messages.messages[self->messages.read_index].length < 0) {
    // printf("websockets.c %s : websocket_peek; nothing to peek (length = -1)\n",
    //        self->srcstr);
    return -1;
  }

  int len = self->messages.messages[self->messages.read_index].length;

  char srcstring[22] = {};
  char dststring[22] = {};
  addr_to_string(&self->messages.messages[self->messages.read_index].src,
                 srcstring);
  addr_to_string(&self->messages.messages[self->messages.read_index].dst,
                 dststring);
  // printf("websockets.c %s : websocket_peek; peeked %d bytes (%s -> %s)\n",
  //        self->srcstr, len, srcstring, dststring);

  return len;
}

int websocket_free(websocket_t *self) {
  if (self == NULL) {
    return -1;
  }

  // TODO
  // if (self->src != NULL) {
  //   free(self->src);
  // }

  if (self->buf != NULL) {
    free(self->buf);
  }

  free(self);

  return 0;
}
