#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


int init_raw_socket() {
  int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (fd < 0) {
    perror("[ERROR][socket]");
    return -1;
  }
  return fd;
}

int get_dns_adress(char *hostname, char ip_name[]) {

  struct addrinfo info;
  struct addrinfo *result;

  memset(&info, 0, sizeof info);
  info.ai_family = AF_INET;
  if (getaddrinfo(hostname, NULL, &info, &result) != 0) {
    fprintf(stderr, "[ERROR][ping]: %s: No address associated with hostname\n",
            hostname);
    return -1;
  }

  struct sockaddr_in *t = (struct sockaddr_in *)result->ai_addr;
  strcpy(ip_name, inet_ntoa(t->sin_addr));
  freeaddrinfo(result);
  return 0;
}

// int checksum() { return ; }

int main(int argc, char *argv[]) {

  if (argc == 1) {
    fprintf(stderr,
            "[WARNING][ping]: usage error: Destination address required");
    return 0;
  }

  char *hostname = argv[argc - 1];
  char ip_name[16];
  if (get_dns_adress(hostname, ip_name) == -1)
    return 1;

  int socket_fd = init_raw_socket();
  if (socket_fd == -1)
    return 2;

  close(socket_fd);
}
