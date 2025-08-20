#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>  // for printf, perror
#include <stdlib.h> // for exit
#include <string.h> // for strlen
#include <sys/socket.h>
#include <unistd.h> // for read, write, close

void die(const char *msg) {
  perror(msg);
  exit(1);
}

void msg(const char *msg) { perror(msg); }

const size_t k_max_msg = 4096;

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

static int32_t query(int fd, const char *text) {
  uint32_t len = (uint32_t)strlen(text);
  if (len > k_max_msg) {
    return -1;
  }
  // send request
  char wbuf[4 + k_max_msg];
  memcpy(wbuf, &len, 4); // assume little endian
  memcpy(&wbuf[4], text, len);
  if (int32_t err = write_all(fd, wbuf, 4 + len)) {
    return err;
  }
  // 4 bytes header
  char rbuf[4 + k_max_msg];
  errno = 0;
  int32_t err = read_full(fd, rbuf, 4);
  if (err) {
    msg(errno == 0 ? "EOF" : "read() error");
    return err;
  }
  memcpy(&len, rbuf, 4); // assume little endian
  if (len > k_max_msg) {
    msg("too long");
    return -1;
  }
  // reply body
  err = read_full(fd, &rbuf[4], len);
  if (err) {
    msg("read() error");
    return err;
  }
  // do something
  printf("server says: %.*s\n", len, &rbuf[4]);
  return 0;
}

int main() {
  // IPv4 TCP
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    die("socket() error");
  }

  // bind to an address
  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(1234);                   // port
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1

  int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
  if (rv < 0) {
    die("connect() err");
  }

  // send multiple requests
  int32_t err = query(fd, "hello 1");
  if (err) {
    goto L_DONE;
  }
  err = query(fd, "hello 2");
  if (err) {
    goto L_DONE;
  }

L_DONE:
  close(fd);
  return 0;
}