#include "dhcp.h"

typedef struct {
    unsigned char mac_address[16];
    unsigned long ip_address;
} DHCP_LEASE;

typedef struct {
    unsigned char code;
    unsigned char length;
    unsigned long value;
} dhcp_option;

struct sockaddr_in serv, client;

typedef struct {
    unsigned char op;
    unsigned char htype;
    unsigned char hlen;
    unsigned char hops;
    unsigned long xid;
    unsigned short secs;
    unsigned short flags;
    unsigned long ciaddr;
    unsigned long yiaddr;
    unsigned long siaddr;
    unsigned long giaddr;
    unsigned char chaddr[16];
    unsigned long magic_cookie;
    dhcp_option options[308];
} DHCP_PACKET;

DHCP_LEASE leases[MAX_LEASES];
int lease_count = 0;

const char *pool_start_str = "192.168.1.100";
const char *pool_end_str = "192.168.1.200";
unsigned long pool_start;
unsigned long pool_end;
unsigned long next_ip;

void print_mac_address(unsigned char *mac) {
    printf("%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void print_leases() {
    printf("Текущие аренды IP:\n");
    for (int i = 0; i < lease_count; i++) {
        printf("MAC: ");
        print_mac_address(leases[i].mac_address);
        printf(" -> IP: %s\n", inet_ntoa(*(struct in_addr *)&leases[i].ip_address));
    }
}

void addOption(dhcp_option *options, unsigned char option, unsigned char length, unsigned long value) {
    options[option].code = option;
    options[option].length = length;
    options[option].value = value;
}

int DHCP_CLIENT(DHCP_PACKET *packet, int type) {
    switch (type) {
    case DHCPDISCOVER:
        if (packet->options[53].value == DHCPDISCOVER) {
            return DHCPDISCOVER;
        }
        break;
    case DHCPREQUEST:
        if (packet->options[53].value == DHCPREQUEST) {
            return DHCPREQUEST;
        }
        break;
    case DHCPRELEASE:
        if (packet->options[53].value == DHCPRELEASE) {
            return DHCPRELEASE;
        }
        break;
    case DHCPINFORM:
        if (packet->options[53].value == DHCPINFORM) {
            return DHCPINFORM;
        }
        break;
    default:
        break;
    }
    return -1;
}

void fillDhcpOptions_server(DHCP_PACKET *packet, int type) {
    unsigned long netmask = inet_addr("255.255.255.0");

    if (type == DHCPOFFER) {
        addOption(packet->options, DHCP_MESSAGE_TYPE, 1, DHCPOFFER);
        addOption(packet->options, DHCP_SERVER_IDENTIFIER, 4, inet_addr("192.168.1.1"));
        addOption(packet->options, DHCP_LEASE_TIME, 4, htonl(3600));
        addOption(packet->options, DHCP_SUBNET_MASK, 4, netmask);
        addOption(packet->options, DHCP_END, 1, 0);
    } else if (type == DHCPACK) {
        addOption(packet->options, DHCP_MESSAGE_TYPE, 1, DHCPACK);
        addOption(packet->options, DHCP_SERVER_IDENTIFIER, 4, inet_addr("192.168.1.1"));
        addOption(packet->options, DHCP_LEASE_TIME, 4, htonl(3600));
        addOption(packet->options, DHCP_SUBNET_MASK, 4, netmask);
        addOption(packet->options, DHCP_END, 1, 0);
    }
}

unsigned long allocate_ip(unsigned char *mac_address) {
    for (int i = 0; i < lease_count; i++) {
        if (memcmp(leases[i].mac_address, mac_address, 16) == 0) {
            return leases[i].ip_address;
        }
    }

    if (lease_count < MAX_LEASES && next_ip <= pool_end) {
        memcpy(leases[lease_count].mac_address, mac_address, 16);
        leases[lease_count].ip_address = next_ip++;
        return leases[lease_count++].ip_address;
    }

    return 0;
}

void Fill_Server(DHCP_PACKET *packet, int type) {
    packet->op = BOOTREPLY;
    packet->htype = ETHERNET;
    packet->hlen = ETHERNET_LEN;
    packet->xid = htonl(123456789);
    packet->ciaddr = 0;
    packet->flags = 0;
    packet->yiaddr = allocate_ip(packet->chaddr);
    packet->magic_cookie = htonl(0x63825363);
    fillDhcpOptions_server(packet, type);
}

void Dhcp_Offer(DHCP_PACKET *packet, struct sockaddr_in client, int fd) {
    Fill_Server(packet, DHCPOFFER);

    if (sendto(fd, packet, sizeof(DHCP_PACKET), 0, (struct sockaddr *)&client, sizeof(client)) == -1) {
        perror("Ошибка при отправке DHCPOFFER");
        exit(EXIT_FAILURE);
    }
    printf("Отправлен DHCP Offer для MAC: ");
    print_mac_address(packet->chaddr);
    printf(", IP: %s\n", inet_ntoa(*(struct in_addr *)&packet->yiaddr));
    print_leases();
}

void Dhcp_ACK(DHCP_PACKET *packet, struct sockaddr_in client, int fd) {
    Fill_Server(packet, DHCPACK);

    if (sendto(fd, packet, sizeof(DHCP_PACKET), 0, (struct sockaddr *)&client, sizeof(client)) == -1) {
        perror("Ошибка при отправке DHCPACK");
        exit(EXIT_FAILURE);
    }
    printf("Отправлен DHCPACK для MAC: ");
    print_mac_address(packet->chaddr);
    printf(", IP: %s\n", inet_ntoa(*(struct in_addr *)&packet->yiaddr));
    print_leases();
}

void Dhcp_Release_Handler(DHCP_PACKET *packet, struct sockaddr_in client, int fd) {
    printf("Получен DHCPRELEASE от клиента: %s\n", inet_ntoa(*(struct in_addr *)&packet->yiaddr));
    for (int i = 0; i < lease_count; i++) {
        if (leases[i].ip_address == packet->yiaddr) {
            memcpy(&leases[i], &leases[lease_count - 1], sizeof(DHCP_LEASE));
            lease_count--;
            break;
        }
    }
    print_leases();
}

void Dhcp_Inform_Handler(DHCP_PACKET *packet, struct sockaddr_in client, int fd) {
    printf("Получен DHCPINFORM от клиента: %s\n", inet_ntoa(*(struct in_addr *)&packet->yiaddr));
    print_leases();
}

int main() {
    printf("Запуск DHCP сервера\n");

    pool_start = inet_addr(pool_start_str);
    pool_end = inet_addr(pool_end_str);
    next_ip = pool_start;

    if (pool_start == INADDR_NONE || pool_end == INADDR_NONE) {
        fprintf(stderr, "Ошибка: неверные IP-адреса в диапазоне пула\n");
        return EXIT_FAILURE;
    }

    int check = 0;
    DHCP_PACKET *packet = malloc(sizeof(DHCP_PACKET));

    serv.sin_family = AF_INET;
    serv.sin_port = htons(SERVER_PORT);
    serv.sin_addr.s_addr = INADDR_ANY;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("Ошибка при создании сокета (socket)");
        exit(EXIT_FAILURE);
    }

    if (bind(fd, (const struct sockaddr *)&serv, sizeof(serv)) == -1) {
        perror("Ошибка при связывании (bind)");
        exit(EXIT_FAILURE);
    }

    socklen_t size = sizeof(client);

    while (1) {
        check = recvfrom(fd, packet, sizeof(DHCP_PACKET), 0, (struct sockaddr *)&client, &size);
        if (check != -1) {
            printf("Получен DHCP пакет от клиента с MAC: ");
            print_mac_address(packet->chaddr);
            printf("\n");
            
            int msg_type = DHCP_CLIENT(packet, packet->options[53].value);
            switch (msg_type) {
            case DHCPDISCOVER:
                Dhcp_Offer(packet, client, fd);
                break;
            case DHCPREQUEST:
                Dhcp_ACK(packet, client, fd);
                break;
            case DHCPRELEASE:
                Dhcp_Release_Handler(packet, client, fd);
                break;
            case DHCPINFORM:
                Dhcp_Inform_Handler(packet, client, fd);
                break;
            default:
                printf("Неизвестный тип DHCP сообщения\n");
            }
        }
    }

    free(packet);
    close(fd);
    return 0;
}
