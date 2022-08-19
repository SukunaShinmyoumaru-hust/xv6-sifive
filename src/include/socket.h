#ifndef __SOCKET_H
#define __SOCKET_H

#ifndef SOCK_STREAM
#define SOCK_STREAM    1
#define SOCK_DGRAM     2
#endif

#define SOCK_RAW       3
#define SOCK_RDM       4
#define SOCK_SEQPACKET 5
#define SOCK_DCCP      6
#define SOCK_PACKET    10

#define PF_UNSPEC       0
#define PF_LOCAL        1
#define PF_UNIX         PF_LOCAL
#define PF_FILE         PF_LOCAL
#define PF_INET         2
#define PF_AX25         3
#define PF_IPX          4
#define PF_APPLETALK    5
#define PF_NETROM       6
#define PF_BRIDGE       7
#define PF_ATMPVC       8
#define PF_X25          9
#define PF_INET6        10
#define PF_ROSE         11
#define PF_DECnet       12
#define PF_NETBEUI      13
#define PF_SECURITY     14
#define PF_KEY          15
#define PF_NETLINK      16
#define PF_ROUTE        PF_NETLINK
#define PF_PACKET       17
#define PF_ASH          18
#define PF_ECONET       19
#define PF_ATMSVC       20
#define PF_RDS          21
#define PF_SNA          22
#define PF_IRDA         23
#define PF_PPPOX        24
#define PF_WANPIPE      25
#define PF_LLC          26
#define PF_IB           27
#define PF_MPLS         28
#define PF_CAN          29
#define PF_TIPC         30
#define PF_BLUETOOTH    31
#define PF_IUCV         32
#define PF_RXRPC        33
#define PF_ISDN         34
#define PF_PHONET       35
#define PF_IEEE802154   36
#define PF_CAIF         37
#define PF_ALG          38
#define PF_NFC          39
#define PF_VSOCK        40
#define PF_KCM          41
#define PF_QIPCRTR      42
#define PF_SMC          43
#define PF_XDP          44
#define PF_MAX          45

#define AF_UNSPEC       PF_UNSPEC
#define AF_LOCAL        PF_LOCAL
#define AF_UNIX         AF_LOCAL
#define AF_FILE         AF_LOCAL
#define AF_INET         PF_INET
#define AF_AX25         PF_AX25
#define AF_IPX          PF_IPX
#define AF_APPLETALK    PF_APPLETALK
#define AF_NETROM       PF_NETROM
#define AF_BRIDGE       PF_BRIDGE
#define AF_ATMPVC       PF_ATMPVC
#define AF_X25          PF_X25
#define AF_INET6        PF_INET6
#define AF_ROSE         PF_ROSE
#define AF_DECnet       PF_DECnet
#define AF_NETBEUI      PF_NETBEUI
#define AF_SECURITY     PF_SECURITY
#define AF_KEY          PF_KEY
#define AF_NETLINK      PF_NETLINK
#define AF_ROUTE        PF_ROUTE
#define AF_PACKET       PF_PACKET
#define AF_ASH          PF_ASH
#define AF_ECONET       PF_ECONET
#define AF_ATMSVC       PF_ATMSVC
#define AF_RDS          PF_RDS
#define AF_SNA          PF_SNA
#define AF_IRDA         PF_IRDA
#define AF_PPPOX        PF_PPPOX
#define AF_WANPIPE      PF_WANPIPE
#define AF_LLC          PF_LLC
#define AF_IB           PF_IB
#define AF_MPLS         PF_MPLS
#define AF_CAN          PF_CAN
#define AF_TIPC         PF_TIPC
#define AF_BLUETOOTH    PF_BLUETOOTH
#define AF_IUCV         PF_IUCV
#define AF_RXRPC        PF_RXRPC
#define AF_ISDN         PF_ISDN
#define AF_PHONET       PF_PHONET
#define AF_IEEE802154   PF_IEEE802154
#define AF_CAIF         PF_CAIF
#define AF_ALG          PF_ALG
#define AF_NFC          PF_NFC
#define AF_VSOCK        PF_VSOCK
#define AF_KCM          PF_KCM
#define AF_QIPCRTR      PF_QIPCRTR
#define AF_SMC          PF_SMC
#define AF_XDP          PF_XDP
#define AF_MAX          PF_MAX

#define SOCK_NONBLOCK  0x800
#define SOCK_CLOEXEC   0x80000

#define MAX_LENGTH_OF_SOCKET 256
#define ADDR_FAMILY(x) (*(sa_family_t*)(x))

#include"utils/list.h"
#include"poll.h"
#include"file.h"
#include"epoll.h"

typedef int socklen_t;
typedef unsigned short sa_family_t;

struct file;
struct poll_table;

struct sockaddr_in {
    sa_family_t sin_family;
    uint16      sin_port;
    uint32      sin_addr;
    char        data[8];
};

struct in6_addr {
	union {
		char   __u6_addr8[16];
	} __u6_addr;			/* 128-bit IP6 address */
};

struct sockaddr_in6 {
    unsigned short int  sin6_family;    /* AF_INET6 */
    uint16    sin6_port;      /* Transport layer port # */
    uint32    sin6_flowinfo;  /* IPv6 flow information */
    struct in6_addr sin6_addr;      /* IPv6 address */
    uint32   sin6_scope_id;  /* scope id (new in RFC2553) */
};

typedef union{
  struct sockaddr_in addr4;
  struct sockaddr_in6 addr6;
}sockaddr;


struct netIP;

#define SESSIONNUM 2
struct netport {
  int portid;
  int msgnum;
  struct netIP* IP;
  struct netport* conn;
  struct socket* sk;
  struct socket* sesk[SESSIONNUM];
  struct list msg;
  struct list req;
  struct spinlock lk;
};



struct msg{
    char* data;
    struct netport* port;
    uint64 len;
    struct list list;
};

#define PORTNUM 0x10000
#define LOCALIPNUM 10
struct netIP {
  enum {IPn,IPv4,IPv6} type;
  char addr[16];
  struct netport ports[PORTNUM];
};

struct socket{
    int id;
    int domain;
    int type;
    int protocol;
    int nonblock;
    struct wait_queue	rqueue;
    enum {SK_NONE, SK_BIND, SK_CONNECT} sk_type;
    char temp[MAX_LENGTH_OF_SOCKET];
    struct netport* bind_port;
    struct netport* conn_port;
    struct spinlock lk;
};


void slock(struct socket* sk);
void sunlock(struct socket* sk);
void socketwakeup(struct socket *sk);
int netinit();
void portinit(struct netport* port,uint64 portid);
void IPinit(struct netIP* ip,void* addr,int type);
void IPaddr(struct netIP* ip,sockaddr* addr);
void portaddr(struct netport* port,sockaddr* addr);
struct netport* findport(sockaddr* addr);

void port_recv_req(struct netport* port,struct msg* msg);
int port_has_req(struct netport* port);
struct msg* port_pop_req(struct netport* port);

int bindalloc(struct socket* sk);
int bindaddr(struct socket* sk, sockaddr* addr);
struct socket* socketalloc();
int connect(struct socket* sk, sockaddr* addr);
struct netport* getconn(struct socket* sk, struct netport* port);
void socketclose(struct socket* sk);
int socketread(struct socket* sk, int user, uint64 addr, int n);
int socketwrite(struct socket* sk, int user, uint64 addr, int n);
void socketkstat(struct socket* sk, struct kstat* kst);


struct msg* createmsg(char* data, uint64 len);
struct msg* nullmsg();
struct msg* msgcopy(struct msg* msg);
void destroymsg(struct msg* msg);


void sendmsg(struct socket* sock, sockaddr* addr, struct msg* msg);
void sendreq(struct netport* port, struct netport* pport);
struct msg* recvmsgfrom(struct socket* sock,sockaddr* addr);

uint32 acceptepoll(struct file *fp, struct poll_table *pt);
uint32 socketepoll(struct file *fp, struct poll_table *pt);
uint32 socketnoepoll(struct file *fp, struct poll_table *pt);
void socketlock(struct socket *sk, struct wait_node *wait);
void socketunlock(struct socket *sk, struct wait_node *wait);
void print_msg(struct msg* msg);
void print_sockaddr(sockaddr* addr);
void print_port_info(struct netport* port);
#endif

