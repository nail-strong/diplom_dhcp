#include "dhcp.h"

struct sockaddr_in serv, client;

typedef struct {
    unsigned char code;
    unsigned char length;
    unsigned long value;
} dhcp_option;

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

void addOption(dhcp_option *options, unsigned char option, unsigned char length, unsigned long value) {
    options[option].code = option;
    options[option].length = length;
    options[option].value = value;
}

void fillDhcpOptions_client(DHCP_PACKET *packet, int type) {
    if (type == DHCPDISCOVER) {
        addOption(packet->options, DHCP_MESSAGE_TYPE, 1, DHCPDISCOVER);
        addOption(packet->options, CLIENT_IDENTIFIER, 6, 0xa85e45c602cd);
        addOption(packet->options, DHCP_END, 1, 0);
    } else if (type == DHCPREQUEST) {
        addOption(packet->options, DHCP_MESSAGE_TYPE, 1, DHCPREQUEST);
        addOption(packet->options, CLIENT_IDENTIFIER, 6, 0xa85e45c602cd);
        addOption(packet->options, DHCP_REQUESTED_IP_ADDRESS, 4, packet->yiaddr);
        addOption(packet->options, DHCP_END, 1, 0);
    }
}

void fillDhcpOptions_client_release(DHCP_PACKET *packet) {
    addOption(packet->options, DHCP_MESSAGE_TYPE, 1, DHCPRELEASE);
    addOption(packet->options, DHCP_END, 1, 0);
}

void fillDhcpOptions_client_inform(DHCP_PACKET *packet) {
    addOption(packet->options, DHCP_MESSAGE_TYPE, 1, DHCPINFORM);
    addOption(packet->options, CLIENT_IDENTIFIER, 6, 0xa85e45c602cd);
    addOption(packet->options, DHCP_END, 1, 0);
}

void Fill_Client(DHCP_PACKET *packet, int type, unsigned char *mac) {
    memset(packet, 0, sizeof(DHCP_PACKET));
    packet->op = BOOTREQUEST;
    packet->htype = ETHERNET;
    packet->hlen = sizeof(packet->chaddr);
    packet->xid = htonl(123456789);
    packet->ciaddr = 0;
    packet->flags = 0;
    memcpy(packet->chaddr, mac, packet->hlen);
    packet->magic_cookie = htonl(0x63825363);
    fillDhcpOptions_client(packet, type);
    packet->yiaddr = 0;
}

void Fill_Client_Release(DHCP_PACKET *packet, unsigned long yiaddr, unsigned char *mac) {
    memset(packet, 0, sizeof(DHCP_PACKET));
    packet->op = BOOTREQUEST;
    packet->htype = ETHERNET;
    packet->hlen = sizeof(packet->chaddr);
    packet->xid = htonl(123456789);
    packet->yiaddr = yiaddr;
    memcpy(packet->chaddr, mac, packet->hlen);
    packet->magic_cookie = htonl(0x63825363);
    fillDhcpOptions_client_release(packet);
}

void Fill_Client_Inform(DHCP_PACKET *packet, unsigned long yiaddr, unsigned char *mac) {
    memset(packet, 0, sizeof(DHCP_PACKET));
    packet->op = BOOTREQUEST;
    packet->htype = ETHERNET;
    packet->hlen = sizeof(packet->chaddr);
    packet->xid = htonl(123456789);
    packet->yiaddr = yiaddr;
    memcpy(packet->chaddr, mac, packet->hlen);
    packet->magic_cookie = htonl(0x63825363);
    fillDhcpOptions_client_inform(packet);
}

void Dhcp_Discover(DHCP_PACKET *packet, struct sockaddr_in serv, int fd, unsigned char *mac) {
    int check = 0;
    socklen_t size = sizeof(serv);
    int type = DHCPDISCOVER;

    Fill_Client(packet, type, mac);

    if (sendto(fd, packet, sizeof(DHCP_PACKET), 0, (struct sockaddr *)&serv, sizeof(serv)) == -1) {
        perror("Ошибка при отправке (sendto)");
        exit(EXIT_FAILURE);
    }
    printf("DHCPDISCOVER отправлен.\n");

    check = recvfrom(fd, packet, sizeof(DHCP_PACKET), 0, (struct sockaddr *)&serv, &size);
    if (check != -1) {
        if (packet->options[53].value == DHCPOFFER) {
            printf("Получен DHCPOFFER.\n");
            packet->yiaddr = packet->yiaddr;
        }
    } else {
        perror("Ошибка при получении (recvfrom)");
    }
}

void Dhcp_Request(DHCP_PACKET *packet, struct sockaddr_in serv, int fd, unsigned char *mac) {
    int check = 0;
    socklen_t size = sizeof(serv);
    int type = DHCPREQUEST;

    Fill_Client(packet, type, mac);

    if (sendto(fd, packet, sizeof(DHCP_PACKET), 0, (struct sockaddr *)&serv, sizeof(serv)) == -1) {
        perror("Ошибка при отправке (sendto)");
        exit(EXIT_FAILURE);
    }
    printf("DHCPREQUEST отправлен.\n");

    check = recvfrom(fd, packet, sizeof(DHCP_PACKET), 0, (struct sockaddr *)&serv, &size);
    if (check != -1) {
        if (packet->options[53].value == DHCPACK) {
            printf("Получен DHCPACK.\n");
            printf("Тип сообщения ACK: %ld\n", packet->options[53].value);
        }
    } else {
        perror("Ошибка при получении (recvfrom)");
    }
}

void Dhcp_Release(DHCP_PACKET *packet, struct sockaddr_in serv, int fd, unsigned long yiaddr, unsigned char *mac) {
    Fill_Client_Release(packet, yiaddr, mac);
    if (sendto(fd, packet, sizeof(DHCP_PACKET), 0, (struct sockaddr *)&serv, sizeof(serv)) == -1) {
        perror("Ошибка при отправке DHCPRELEASE");
        exit(EXIT_FAILURE);
    }
    printf("DHCPRELEASE отправлен.\n");
}

void Dhcp_Inform(DHCP_PACKET *packet, struct sockaddr_in serv, int fd, unsigned long yiaddr, unsigned char *mac) {
    Fill_Client_Inform(packet, yiaddr, mac);
    if (sendto(fd, packet, sizeof(DHCP_PACKET), 0, (struct sockaddr *)&serv, sizeof(serv)) == -1) {
        perror("Ошибка при отправке DHCPINFORM");
        exit(EXIT_FAILURE);
    }
    printf("DHCPINFORM отправлен.\n");
}

void generate_unique_mac(unsigned char *mac) {
    for (int i = 0; i < 6; i++) {
        mac[i] = rand() % 256;
    }
}

int main() {
    printf("Запуск DHCP клиента\n");

    struct sockaddr_in serv, client;
    int fd;
    DHCP_PACKET *packet = (DHCP_PACKET *)malloc(sizeof(DHCP_PACKET));
    if (packet == NULL) {
        perror("Ошибка выделения памяти (malloc)");
        exit(EXIT_FAILURE);
    }

    int flag = 1;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(SERVER_PORT);
    client.sin_family = AF_INET;
    client.sin_port = htons(CLIENT_PORT);
    client.sin_addr.s_addr = htonl(INADDR_ANY);
    inet_pton(AF_INET, "255.255.255.255", &(serv.sin_addr));

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("Ошибка при создании сокета (socket)");
        exit(EXIT_FAILURE);
    }
    setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag));

    socklen_t size = sizeof(serv);

    unsigned char mac[6];
    generate_unique_mac(mac);

    Dhcp_Discover(packet, serv, fd, mac);
    Dhcp_Request(packet, serv, fd, mac);

    while (1) {
        sleep(3559);
        Dhcp_Request(packet, serv, fd, mac);
    }

    free(packet);
    return 0;
}
