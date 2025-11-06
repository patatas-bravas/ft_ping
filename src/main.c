#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <bits/time.h>
#include <bits/types/struct_iovec.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define IP_NAME_LEN 16
#define TTL_UNIX_SIZE 64

bool run = true;

typedef struct {
  struct icmphdr header;
  char payload[64 - sizeof(struct icmphdr)];

} icmppkt;

int init_imcp_socket() {

  int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (fd < 0) {
    perror("[ERROR][socket]");
    return -1;
  }

  struct timeval timeout;
  timeout.tv_sec = 2;
  timeout.tv_usec = 0;
  if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) == -1) {
    perror("[ERROR][setsockopt][SO_RCVTIMEO]");
    close(fd);
    return -1;
  }

  uint8_t size_ttl = TTL_UNIX_SIZE;
  if (setsockopt(fd, IPPROTO_IP, IP_TTL, &size_ttl, sizeof(size_ttl)) == -1) {
    perror("[ERROR][setsockopt][IP_TTL]");
    close(fd);
    return -1;
  }

  bool on = true;
  if (setsockopt(fd, IPPROTO_IP, IP_RECVTTL, &on, sizeof(on)) == -1) {
    perror("[ERROR][setsockopt][IP_RECVTTL]");
    close(fd);
    return -1;
  }
  return fd;
}

bool dns_resolver(char *hostname, char *ipname, struct sockaddr_in *dest_addr) {

  struct addrinfo info;
  struct addrinfo *result;
  memset(&info, 0, sizeof info);
  info.ai_family = AF_INET;

  if (getaddrinfo(hostname, NULL, &info, &result) != 0) {
    fprintf(stderr, "[ERROR][ping]: %s: No address associated with hostname\n", hostname);
    return false;
  }

  *dest_addr = *(struct sockaddr_in *)result->ai_addr;
  freeaddrinfo(result);

  if (inet_ntop(AF_INET, &dest_addr->sin_addr, ipname, IP_NAME_LEN) == NULL) {
    perror("[ERROR][inet_ntop]");
    return false;
  }

  return true;
}

uint16_t checksum(void *addr, size_t size) {
  uint16_t *buffer = addr;
  uint32_t sum = 0;

  while (size > 1) {
    sum += *buffer++;
    size -= 2;
  }
  if (size == 1)
    sum += *(uint8_t *)buffer;

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);

  return (uint16_t)~sum;
}

bool send_packet(int socket_fd, struct sockaddr_in *dest_addr) {

  icmppkt packet;

  memset(&packet, 0, sizeof(packet));
  memset(&packet.payload, 'M', sizeof(packet.payload));
  packet.header.type = ICMP_ECHO;
  packet.header.un.echo.id = getpid();
  packet.header.un.echo.sequence += 1;
  packet.header.checksum = checksum(&packet, sizeof(packet));
  if (sendto(socket_fd, &packet, sizeof(packet), 0, (struct sockaddr *)dest_addr, sizeof(struct sockaddr_in)) == -1) {
    perror("[ERROR][setsockopt]");
    return false;
  }
  return true;
}

bool recv_packet(int socket_fd) {

  char buffer[1024];
  struct msghdr msg;
  struct iovec io;
  char control[CMSG_SPACE(sizeof(int))];
  io.iov_base = buffer;
  io.iov_len = sizeof(buffer);
  memset(&msg, 0, sizeof(msg));
  msg.msg_iov = &io;
  msg.msg_iovlen = 1;
  msg.msg_control = control;
  msg.msg_controllen = sizeof(control);

  ssize_t size_read;
  size_read = recvmsg(socket_fd, &msg, 0);
  printf("size_read = %ld\n", size_read);
  if (size_read == -1) {
    perror("[ERROR][recvmsg]");
    return false;
  }

  int16_t ttl = -1;
  struct cmsghdr *cmsg;
  for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
    if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_RECVTTL) {
      ttl = *(int *)CMSG_DATA(cmsg);
      break;
    }
  }
  printf("ttl = %d\n", ttl);

  return true;
}

bool ping(int socket_fd, struct sockaddr_in *dest_addr) {

  struct timespec start, end;
  while (run) {
    clock_gettime(CLOCK_MONOTONIC, &start);
    if (send_packet(socket_fd, dest_addr) == false)
      return false;
    if (recv_packet(socket_fd) == false)
      return false;
    clock_gettime(CLOCK_MONOTONIC, &end);
    sleep(1);
  }

  return true;
}

int main(int argc, char *argv[]) {

  if (argc == 1) {
    fprintf(stderr, "[WARNING][ping]: usage error: Destination address required\n");
    return 0;
  }

  char *hostname = argv[argc - 1];
  char ipname[IP_NAME_LEN];
  struct sockaddr_in addr_dest;
  if (dns_resolver(hostname, ipname, &addr_dest) == false)
    return 1;
  printf("ip: %s\nhostname: %s\n", ipname, hostname);

  int socket_fd = init_imcp_socket();
  if (socket_fd == -1)
    return 2;

  ping(socket_fd, &addr_dest);

  close(socket_fd);
}
