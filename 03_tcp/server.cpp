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

static void do_something(int connfd) {
  char rbuf[64] = {};
  ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);

  if (n < 0) {
    msg("read() error");
    return;
  }
  printf("client says: %s\n", rbuf);

  char wbuf[] = "Hello, client!";
  write(connfd, wbuf, strlen(wbuf));
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

    do_something(connfd);
    close(connfd);
  }
}
