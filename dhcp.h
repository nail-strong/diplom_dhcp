#ifndef DHCP_H
#define DHCP_H

#define FILENAME_SOCK "/tmp/Socket"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/un.h>
#include <time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define OPTION_FIELD_SIZE 312
#define FILENAME_SOCK "/tmp/Socket"
#define LEASE_TIME 3600 
#define SERVER_PORT 2067
#define CLIENT_PORT 2068
#define BUF_SIZE 1024
#define DHCPDISCOVER  1
#define DHCPOFFER 2
#define DHCPREQUEST 3
#define DHCPACK 4
#define DHCP_OP 0
#define DHCP_HTYPE 1
#define DHCP_HLEN 2
#define DHCP_HOPS 3
#define DHCP_XID 4
#define DHCPRELEASE 7
#define DHCP_SECS 8
#define DHCPINFORM 9
#define DHCP_FLAGS 10
#define DHCP_CIADDR 12
#define DHCP_YIADDR 16
#define DHCP_SIADDR 20
#define DHCP_GIADDR 24
#define DHCP_CHADDR 28
#define DHCP_MAGIC_COOKIE 236
#define MAX_LEASES 256
#define DHCP_MESSAGE_TYPE 53
#define DHCP_SERVER_IDENTIFIER 54
#define DHCP_LEASE_TIME 51
#define DHCP_SUBNET_MASK 1
#define CLIENT_IDENTIFIER 61
#define DHCP_ROUTER 3
#define DHCP_DOMAIN_NAME_SERVER 6
#define DHCP_END 255
#define DHCP_REQUESTED_IP_ADDRESS 50


enum ports {
    BOOTPS = 67,
    BOOTPC = 68
};

enum op_types {
    BOOTREQUEST = 1,
    BOOTREPLY   = 2,   
};

enum hardware_address_types {
    ETHERNET     = 0x01,
    ETHERNET_LEN = 0x06,
};

enum {
    DHCP_HEADER_SIZE = 236
};
enum DhcpOption {
    DHCP_OPTION_SUBNET_MASK = 1,
    DHCP_OPTION_ROUTER = 3,
    DHCP_OPTION_DOMAIN_NAME_SERVER = 6,
    
    DHCP_OPTION_IP_ADDRESS_LEASE_TIME = 51,
    DHCP_OPTION_DHCP_MESSAGE_TYPE = 53,
    DHCP_OPTION_DHCP_SERVER_IDENTIFIER = 54,
    DHCP_OPTION_PARAMETER_REQUEST_LIST = 55,
    DHCP_OPTION_END = 255
};

#endif