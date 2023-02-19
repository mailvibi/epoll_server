#ifndef __NET_H__
#define __NET_H__
#include <stdint.h>

int nb_tcp_socket(char *ip, uint16_t port, int backlog);

#endif /* __NET_H__*/