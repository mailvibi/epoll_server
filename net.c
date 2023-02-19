#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "dbg.h"

int nb_tcp_socket(char *ip, uint16_t port, int backlog)
{
    struct sockaddr_in addr;
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sockfd < 0 ) {
        perr("socket");
        return sockfd;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_aton(ip, &addr.sin_addr);
    int r = bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
    if (r < 0) {
        perr("bind");
        close(sockfd);
        return r;
    }

    r = listen(sockfd, backlog);
    if (r < 0) {
        perr("listen");
        close(sockfd);
        return r;
    }
    return sockfd;
}

static int test(void)
{
    struct sockaddr saddr;
    socklen_t slen;
    int sfd = nb_tcp_socket("127.0.0.1", 10101, 10);
    if (sfd < 0) {
        err("invalid socket fd");
        return -1;
    }

    for (int r = 0 ;;) {
        int cfd = accept(sfd, NULL, NULL);
        if (cfd < 0 && errno == EAGAIN)
            continue;
        if (cfd < 0) {
            perr("accept");
            return cfd;
        }
        uint8_t b[255] = {0};
        r = read(cfd, b, sizeof(b) - 1);
        if (r > 0) {
            fprintf(stderr, "%s", b);
        } else if (r < 0 && errno != EAGAIN) {
            perr("read");
            break;
        } else if (r == 0) {
            dbg("Connection Closed");
            break;
        }
    }
}