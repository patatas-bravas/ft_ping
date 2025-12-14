#include <float.h>
#include <stdio.h>

#include "ping.h"

int main(int argc, char *argv[]) {

  if (argc == 1) {
    fprintf(stderr, "[WARNING][ping]: missing host operand\n");
    return 0;
  }

  hostname = argv[argc - 1];
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

  if (ft_ping(fd, &addr_dest) == ERR_FATAL) {
    close(fd);
    return 4;
  }

  close(fd);
  return 0;
}
