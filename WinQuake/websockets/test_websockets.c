#include "../quakedef.h"

#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "websockets.h"

// warning: static return value; really only suitable for immediate usage
char *now() {
  static char buf[32];

  struct timeval tv;
  struct tm *tm_info;
  char time_str[30];

  gettimeofday(&tv, NULL);
  tm_info = gmtime(&tv.tv_sec);

  strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%S", tm_info);
  snprintf(buf, 32, "%s.%03dZ", time_str, tv.tv_usec / 1000);

  return buf;
}

int test_websocket_happy_path_one_client() {
  char *tx_srcstr = "127.0.0.1:26001";
  struct qsockaddr *tx_src = calloc(1, sizeof(struct qsockaddr));
  assert(string_to_addr(tx_srcstr, tx_src) == 0);

  char *tx_dststr = "255.255.255.255:26000";
  struct qsockaddr *tx_dst = calloc(1, sizeof(struct qsockaddr));
  assert(string_to_addr(tx_dststr, tx_dst) == 0);

  struct qsockaddr *rx_src = calloc(1, sizeof(struct qsockaddr));
  struct qsockaddr *rx_dst = calloc(1, sizeof(struct qsockaddr));
  char *tx_buf = calloc(1024, sizeof(char));
  char *rx_buf = calloc(1024, sizeof(char));

  websocket_t *websocket = websocket_init();

  int i;

  assert(websocket_open(websocket, "ws://127.0.0.1:8080/ws", tx_src) == 0);
  emscripten_sleep(100);

  for (i = 0; i < 3; i++) {
    sprintf(tx_buf, "Hello, world! #%d", i);
    assert(websocket_write(websocket, tx_buf, (unsigned long)strlen(tx_buf),
                           tx_dst, true, false) == 16);
    emscripten_sleep(100);
  }

  for (i = 0; i < 3; i++) {
    sprintf(tx_buf, "Hello, world! #%d", i);
    assert(websocket_read(websocket, rx_src, rx_dst, rx_buf) == 16);
    assert(strcmp(addr_to_string_static(rx_src), "255.255.255.255:26000") == 0);
    assert(strcmp(addr_to_string_static(rx_dst), "127.0.0.1:26001") == 0);
    assert(strcmp(rx_buf, tx_buf) == 0);
    emscripten_sleep(100);
  }

  assert(websocket_read(websocket, rx_src, rx_dst, rx_buf) == -1);
  emscripten_sleep(100);

  assert(websocket_close(websocket) == 0);
  emscripten_sleep(100);

  free(tx_src);
  free(tx_dst);
  free(tx_buf);
  free(rx_src);
  free(rx_dst);
  free(rx_buf);
  free(websocket);

  return 0;
}

int test_websocket_happy_path_many_clients_unicast() {
  char *addrstr_1 = "127.0.0.1:26001";
  struct qsockaddr *addr_1 = calloc(1, sizeof(struct qsockaddr));
  assert(string_to_addr(addrstr_1, addr_1) == 0);

  char *addrstr_2 = "127.0.0.2:26001";
  struct qsockaddr *addr_2 = calloc(1, sizeof(struct qsockaddr));
  assert(string_to_addr(addrstr_2, addr_2) == 0);

  char *addrstr_3 = "127.0.0.3:26001";
  struct qsockaddr *addr_3 = calloc(1, sizeof(struct qsockaddr));
  assert(string_to_addr(addrstr_3, addr_3) == 0);

  char *addrstr_255 = "255.255.255.255:26001";
  struct qsockaddr *addr_255 = calloc(1, sizeof(struct qsockaddr));
  assert(string_to_addr(addrstr_255, addr_255) == 0);

  char *tx_buf = calloc(1024, sizeof(char));
  struct qsockaddr *rx_src = calloc(1, sizeof(struct qsockaddr));
  struct qsockaddr *rx_dst = calloc(1, sizeof(struct qsockaddr));
  char *rx_buf = calloc(1024, sizeof(char));

  websocket_t *websocket_1 = websocket_init();
  websocket_t *websocket_2 = websocket_init();
  websocket_t *websocket_3 = websocket_init();

  assert(websocket_open(websocket_1, "ws://127.0.0.1:8081/ws", addr_1) == 0);
  emscripten_sleep(100);

  assert(websocket_open(websocket_2, "ws://127.0.0.1:8081/ws", addr_2) == 0);
  emscripten_sleep(100);

  assert(websocket_open(websocket_3, "ws://127.0.0.1:8081/ws", addr_3) == 0);
  emscripten_sleep(100);

  sprintf(tx_buf, "Hello, world! #%d", 2);
  assert(websocket_write(websocket_1, tx_buf, (unsigned long)strlen(tx_buf),
                         addr_2, false, true) == 16);
  emscripten_sleep(100);

  sprintf(tx_buf, "Hello, world! #%d", 3);
  assert(websocket_write(websocket_1, tx_buf, (unsigned long)strlen(tx_buf),
                         addr_3, false, true) == 16);
  emscripten_sleep(100);

  sprintf(tx_buf, "Hello, world! #%d", 255);
  assert(websocket_write(websocket_1, tx_buf, (unsigned long)strlen(tx_buf),
                         addr_255, false, true) == 18);
  emscripten_sleep(100);

  sprintf(tx_buf, "Hello, world! #%d", 2);
  assert(websocket_read(websocket_2, rx_src, rx_dst, rx_buf) == 16);
  assert(strcmp(addr_to_string_static(rx_src), "127.0.0.1:26001") == 0);
  assert(strcmp(addr_to_string_static(rx_dst), "127.0.0.2:26001") == 0);
  assert(strcmp(rx_buf, tx_buf) == 0);
  emscripten_sleep(100);

  sprintf(tx_buf, "Hello, world! #%d", 3);
  assert(websocket_read(websocket_3, rx_src, rx_dst, rx_buf) == 16);
  assert(strcmp(addr_to_string_static(rx_src), "127.0.0.1:26001") == 0);
  assert(strcmp(addr_to_string_static(rx_dst), "127.0.0.3:26001") == 0);
  assert(strcmp(rx_buf, tx_buf) == 0);
  emscripten_sleep(100);

  sprintf(tx_buf, "Hello, world! #%d", 255);
  assert(websocket_read(websocket_2, rx_src, rx_dst, rx_buf) == 18);
  assert(strcmp(addr_to_string_static(rx_src), "127.0.0.1:26001") == 0);
  assert(strcmp(addr_to_string_static(rx_dst), "255.255.255.255:26001") == 0);
  assert(strcmp(rx_buf, tx_buf) == 0);
  emscripten_sleep(100);

  sprintf(tx_buf, "Hello, world! #%d", 255);
  assert(websocket_read(websocket_3, rx_src, rx_dst, rx_buf) == 18);
  assert(strcmp(addr_to_string_static(rx_src), "127.0.0.1:26001") == 0);
  assert(strcmp(addr_to_string_static(rx_dst), "255.255.255.255:26001") == 0);
  assert(strcmp(rx_buf, tx_buf) == 0);
  emscripten_sleep(100);

  assert(websocket_read(websocket_2, rx_src, rx_dst, rx_buf) == -1);
  emscripten_sleep(100);

  assert(websocket_read(websocket_3, rx_src, rx_dst, rx_buf) == -1);
  emscripten_sleep(100);

  assert(websocket_close(websocket_1) == 0);
  emscripten_sleep(100);

  assert(websocket_close(websocket_2) == 0);
  emscripten_sleep(100);

  assert(websocket_close(websocket_3) == 0);
  emscripten_sleep(100);

  free(addr_1);
  free(addr_2);
  free(addr_3);
  free(websocket_1);
  free(websocket_2);
  free(websocket_3);
  free(tx_buf);
  free(rx_src);
  free(rx_dst);
  free(rx_buf);

  return 0;
}

int do_test(const char *name, int test_fn()) {
  printf("%s - %s...\n", now(), name);
  assert(test_fn() == 0);
  printf("\n");

  return 0;
}

int do_tests() {
  assert(do_test("test_websocket_happy_path_one_client",
                 test_websocket_happy_path_one_client) == 0);

  assert(do_test("test_websocket_happy_path_many_clients_unicast",
                 test_websocket_happy_path_many_clients_unicast) == 0);

  return 0;
}

int main(void) {
  printf("%s - running tests...\n\n", now());

  assert(do_tests() == 0);

  printf("%s - tests passed.\n\n", now());
  return 0;
}
