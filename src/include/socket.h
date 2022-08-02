#ifndef __SOCKET_H
#define __SOCKET_H
#define MAX_LENGTH_OF_SOCKET 256
struct socket_connection{
    int IP;
    int sock_opt;
    uint64 sock_addr;
    int passive_socket;
    char temp[MAX_LENGTH_OF_SOCKET];
};

void socket_init(void);
int add_socket(int IP,int op);
#endif

