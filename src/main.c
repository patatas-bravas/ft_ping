#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ping.h"

int main(int argc, char *argv[]) {

  if (argc == 1) {
    fprintf(stderr, "[WARNING][ping]: missing host operand\n");
    return 0;
  }

  hostname = argv[argc - 1];
  memset(&rtt, 0, sizeof(rtt));
  memset(&opt, 0, sizeof(ping_opt));
  rtt.min = FLT_MAX;
  rtt.max = FLT_MIN;
  run = 1;
  err = 1;
  send_packet = 0;
  recv_packet = 0;
  bytes_read = 0;
  opt.size = ICMP_PAYLOAD_SIZE;

  if (handle_opt(argc, argv) == ERR_FATAL)
    return 1;

  struct sockaddr_in addr_dest;
  if (dns_resolver(&addr_dest) == ERR_FATAL)
    return 2;

  socket_t fd = init_icmp_socket();
  if (fd == ERR_FATAL)
    return 3;

  buffer = malloc(RECV_BUFFER_SIZE);
  if (buffer == NULL) {
    perror("[ERROR][malloc]");
    return ERR_FATAL;
  }

  if (ft_ping(fd, &addr_dest) == ERR_FATAL) {
    free(buffer);
    close(fd);
    return 4;
  }

  close(fd);
  free(buffer);
  return 0;
}
