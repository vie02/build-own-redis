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

  char msg[] = "Hello, server!";
  write(fd, msg, strlen(msg));

  char rbuf[64] = {};
  ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
  if (n < 0) {
    die("read() error");
  }
  printf("Server says: %s\n", rbuf);
  close(fd);
}