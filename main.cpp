// Source - https://stackoverflow.com/a/74329116

// Posted by Wladimir Koroy

// Retrieved 2026-06-14, License - CC BY-SA 4.0



#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/ip_icmp.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

// Define the Packet Constants
// ping packet size
#define PING_PKT_S 64

// Automatic port number
#define PORT_NO 0

// Automatic port number
#define PING_SLEEP_RATE 1000000

// Gives the timeout delay for receiving packets
// in seconds
#define RECV_TIMEOUT 1

// Performs a DNS lookup
char* dns_lookup(char* addr_host, struct sockaddr_in* addr_con) {
  // printf("\nResolving DNS..\n");
  struct hostent* host_entity;
  char* ip = (char*)malloc(NI_MAXHOST * sizeof(char));
  int i;

  if ((host_entity = gethostbyname(addr_host)) == NULL) {
    // No ip found for hostname
    return NULL;
  }

  // filling up address structure
  strcpy(ip, inet_ntoa(*(struct in_addr*)host_entity->h_addr));

  (*addr_con).sin_family = host_entity->h_addrtype;
  (*addr_con).sin_port = htons(PORT_NO);
  (*addr_con).sin_addr.s_addr = *(long*)host_entity->h_addr;

  return ip;
}

// Resolves the reverse lookup of the hostname
char* reverse_dns_lookup(char* ip_addr) {
  struct sockaddr_in temp_addr;
  socklen_t len;
  char buf[NI_MAXHOST], *ret_buf;

  temp_addr.sin_family = AF_INET;
  temp_addr.sin_addr.s_addr = inet_addr(ip_addr);
  len = sizeof(struct sockaddr_in);

  if (getnameinfo((struct sockaddr*)&temp_addr, len, buf, sizeof(buf), NULL, 0,
                  NI_NAMEREQD)) {
    // printf("Could not resolve reverse lookup of hostname\n");
    return NULL;
  }
  ret_buf = (char*)malloc((strlen(buf) + 1) * sizeof(char));
  strcpy(ret_buf, buf);
  return ret_buf;
}

// Driver Code
int main(int argc, char* argv[]) {
  int sockfd;
  char *ip_addr, *reverse_hostname;
  struct sockaddr_in addr_con;
  int addrlen = sizeof(addr_con);
  char net_buf[NI_MAXHOST];

  int i = 0;
  for (int i = 1; i < 255; ++i) {
    char ip[80];
    sprintf(ip, "192.168.2.%d", i);

    ip_addr = dns_lookup(ip, &addr_con);
    if (ip_addr == NULL) {
      // printf("\nDNS lookup failed! Could not resolve hostname!\n");
      continue;
    }

    reverse_hostname = reverse_dns_lookup(ip_addr);

    if (reverse_hostname == NULL) {
      // printf("\nDNS lookup failed! Could not resolve hostname!\n");
      continue;
    }
    // printf("\nTrying to connect to '%s' IP: %s\n",ip, ip_addr);
    printf("\nReverse Lookup domain: %s", reverse_hostname);

    printf("\n %s \n", ip);
  }

  return 0;
}

