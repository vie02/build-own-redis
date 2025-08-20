#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>  // for printf, perror
#include <stdlib.h> // for exit
#include <string.h> // for strlen
#include <sys/socket.h>
#include <unistd.h> // for read, write, close

/**

A simple binary protocol

┌─────┬──────┬─────┬──────┬────────
│ len │ msg1 │ len │ msg2 │ more...
└─────┴──────┴─────┴──────┴────────
   4B   ...     4B   ...
Each message consists of a 4-byte little-endian integer indicating the length of
the request and the variable-length payload.

*/

const size_t k_max_msg = 4096;

void die(const char *msg) {
  perror(msg);
  exit(1);
}

void msg(const char *msg) { perror(msg); }

static int32_t read_full(int fd, char *buf, size_t len) {
  size_t total = 0;
  while (total < len) {
    ssize_t n = read(fd, buf + total, len - total);
    assert((size_t)n <= len - total);
    if (n <= 0) {
      return -1; // error, or EOF
    }
    total += n;
  }
  return 0; // success
}

static int32_t write_all(int fd, const char *buf, size_t len) {
  size_t total = 0;
  while (total < len) {
    ssize_t n = write(fd, buf + total, len - total);
    assert((size_t)n <= len - total);
    if (n <= 0) {
      return -1; // error
    }
    total += n;
  }
  return 0; // success
}

static int32_t one_request(int connfd) {
  // 4 bytes header
  char rbuf[4 + k_max_msg];
  errno = 0;
  int32_t err = read_full(connfd, rbuf, 4);
  if (err) {
    msg(errno == 0 ? "EOF" : "read() error");
    return err;
  }
  uint32_t len = 0;
  memcpy(&len, rbuf, 4); // assume little endian
  if (len > k_max_msg) {
    msg("too long");
    return -1;
  }
  // request body
  err = read_full(connfd, &rbuf[4], len);
  if (err) {
    msg("read() error");
    return err;
  }
  // do something
  printf("client says: %.*s\n", len, &rbuf[4]);
  // reply using the same protocol
  const char reply[] = "world";
  char wbuf[4 + sizeof(reply)];
  len = (uint32_t)strlen(reply);
  memcpy(wbuf, &len, 4);
  memcpy(&wbuf[4], reply, len);
  return write_all(connfd, wbuf, 4 + len);
}

int main() {
  // IPv4 TCP
  int fd = socket(AF_INET, SOCK_STREAM, 0);

  // set socket options
  int val = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

  // bind to an address
  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(1234);     // port
  addr.sin_addr.s_addr = htonl(0); // wildcard IP 0.0.0.0

  int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
  if (rv) {
    die("bind()");
  }

  // listen
  rv = listen(fd, SOMAXCONN);
  if (rv) {
    die("listen()");
  }

  // accept connection
  while (true) {
    printf("Waiting for a connection...\n");
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
    if (connfd < 0) {
      continue;
    } // error

    while (true) {
      int32_t err = one_request(connfd);
      if (err) {
        msg("one_request() error");
        break;
      }
    }

    close(connfd);
  }

  return 0;
}
