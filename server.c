
#include "evloop.h"
#include "net.h"
#include "dbg.h"

int main(void)
{
    int sfd;
    
    sfd = nb_tcp_socket("127.0.0.1", 10101, 10);

}
