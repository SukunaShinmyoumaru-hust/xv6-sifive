#include "include/types.h"
#include "include/riscv.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/spinlock.h"
#include "include/proc.h"
#include "include/timer.h"
#include "include/kalloc.h"
#include "include/string.h"
#include "include/printf.h"
#include "include/kalloc.h"
#include "include/console.h"
#include "include/string.h"
#include "include/stat.h"
#include "include/vm.h"
#include "include/uname.h"
#include "include/socket.h"

struct netIP localIP[LOCALIPNUM];
int skid;
int
netinit()
{
  skid = 0;
  uint64 local_ipv4 = 0x100007f;
  uint64 local_ipv6[2] = {0,0};
  
  for(int i=0;i<LOCALIPNUM;i++)
  {
    localIP[i].type=IPn;
  } 
  IPinit(localIP+0, &local_ipv4, IPv4);
  IPinit(localIP+1, &local_ipv6, IPv6);
  return 0;
}

void
portinit(struct netport* port,uint64 portid)
{
  memset(port,0,sizeof(struct netport));
  port->portid = portid;
  initlock(&port->lk, "net port");
  list_init(&port->msg);
}

void
IPinit(struct netIP* ip,void* addr,int type)
{
  ip->type = type;
  int cplen = type==IPv4?4:16;
  memcpy(ip->addr, addr, cplen);
  for(int i = 0;i<PORTNUM;i++){
    portinit(ip->ports+i, i);
  }
}

int
bindaddr(struct socket* sk){
    int ret = -1;
    struct netport* port = findport(&sk->bind_addr);
    if(!port){
      __debug_warn("[bind addr] addr not found\n");
      return ret;
    }
    acquire(&port->lk);
    if(!port->sk){
      port->sk = sk;
      ret = 0;
    }
    release(&port->lk);
    return ret;
}

int
bindalloc(struct socket* sk){
  struct netport* ports = localIP->ports;
  for(int i = 0;i<PORTNUM;i++){
    acquire(&ports[i].lk);
    if(!ports[i].sk){
      sk->bind_addr = (sockaddr){
        .addr4 = {
          .sin_family = AF_INET,
          .sin_addr = 0x100007f,
          .sin_port = i
        }
      };
      release(&ports[i].lk);
      return 0;
    }
    release(&ports[i].lk);
  }
  return -1;
}

int
connect(struct socket* sk)
{
  struct netport* myport = findport(&sk->bind_addr);
  struct netport* port = findport(&sk->addr);
  if(!port){
    return -1;
  }
  acquire(&port->lk);
  if(!port->sk){
    release(&port->lk);
    return -1;
  }
  acquire(&myport->lk);
  myport->conn = port;
  port->conn = myport;
  release(&myport->lk);
  release(&port->lk);
  return 0;
}

struct netIP*
findIP(void* addr,int type)
{
  int cmplen = type==IPv4?4:16;
  uint64 zero[2] = {0,0};
  for(int i=0;i<LOCALIPNUM;i++)
  {
    if(localIP[i].type!=type)continue;
    if(memcmp(localIP[i].addr,addr,cmplen)==0
     ||memcmp(addr,zero,cmplen)==0){
      return localIP+i;
    }
  } 
  return NULL;
}

struct netport*
findport(sockaddr* addr)
{
  int family = ADDR_FAMILY(addr);
  int type = 0;
  void* ipaddr;
  uint32 portid;
  if(family == 0x2){
    type = IPv4;
    ipaddr = &addr->addr4.sin_addr;
    portid = addr->addr4.sin_port;
  }else if(family == 0xa){
    type = IPv6;
    ipaddr = addr->addr6.sin6_addr.__u6_addr.__u6_addr8;
    portid = addr->addr6.sin6_port;
  }else{
    return NULL;
  }
  struct netIP* ip = findIP(ipaddr,type);
  if(!ip||portid<0||portid>PORTNUM)return NULL;
  return ip->ports+portid;
}

struct socket*
socketalloc()
{
  struct socket* sk = kmalloc(sizeof(struct socket));
  memset(sk,0,sizeof(struct socket));
  sk->id = ++skid;
  initlock(&sk->lk,"socket");
  return sk;
}

void
socketclose(struct socket* sk)
{
  //acquire(&sk->lk);
  //release(&sk->lk);
  kfree(sk);
}


void slock(struct socket* sk){
  acquire(&sk->lk);
}


void sunlock(struct socket* sk){
  release(&sk->lk);
}

void
socketkstat(struct socket* sk, struct kstat* kst)
{
  
}

int
socketread(struct socket* sk, int user, uint64 addr, int n)
{
  __debug_warn("socket read addr:%p n:%p\n", addr, n);
  return 0;
}

int
socketwrite(struct socket* sk, int user, uint64 addr, int n)
{
  __debug_warn("socket write addr:%p n:%p\n", addr, n);
  return 0;
}

struct msg*
createmsg(char* data, uint64 len)
{
  struct msg* msg = kmalloc(sizeof(struct msg));
  msg->data = data;
  msg->len = len;
  list_init(&msg->list);
  return msg;
}

void
destroymsg(struct msg* msg)
{
  kfree(msg->data);
  kfree(msg);
}


struct msg*
msgcopy(struct msg* msg){
  struct msg* cp = kmalloc(sizeof(struct msg));
  cp->data = kmalloc(msg->len);
  cp->len = msg->len;
  memcpy(cp->data,msg->data,msg->len);
  list_init(&cp->list);
  return cp;
}

void
sendmsgto(sockaddr* addr, struct msg* msg)
{
  print_sockaddr(addr);
  struct msg* cp = msgcopy(msg);
  struct netport* port = findport(addr);
  if(!port)return;
  port_recv_msg(port, cp);
}

struct msg*
recvmsgfrom(sockaddr* addr)
{
  return NULL;
}

void
print_sockaddr(sockaddr* addr){
  int family = ADDR_FAMILY(addr);
  if(family == 0x2){
    char ipv4_1 = (addr->addr4.sin_addr)>>0;
    char ipv4_2 = (addr->addr4.sin_addr)>>8;
    char ipv4_3 = (addr->addr4.sin_addr)>>16;
    char ipv4_4 = (addr->addr4.sin_addr)>>24;
    printf("sockaddr family %p\taddr %d:%d:%d:%d\tport %p\n", addr->addr4.sin_family
    							, ipv4_1, ipv4_2, ipv4_3, ipv4_4
    							, addr->addr4.sin_port);
  }
  else if(family == 0xa){
    char* addr6 = addr->addr6.sin6_addr.__u6_addr.__u6_addr8;
    uint16 ipv6_1 = ((uint16)(addr6[1])<<8)+addr6[0];
    uint16 ipv6_2 = ((uint16)(addr6[3])<<8)+addr6[2];
    uint16 ipv6_3 = ((uint16)(addr6[5])<<8)+addr6[4];
    uint16 ipv6_4 = ((uint16)(addr6[7])<<8)+addr6[6];
    uint16 ipv6_5 = ((uint16)(addr6[9])<<8)+addr6[8];
    uint16 ipv6_6 = ((uint16)(addr6[11])<<8)+addr6[10];
    uint16 ipv6_7 = ((uint16)(addr6[13])<<8)+addr6[12];
    uint16 ipv6_8 = ((uint16)(addr6[15])<<8)+addr6[14];
    printf("sockaddr family %p\taddr %o:%o:%o:%o:%o:%o:%o:%o\tport %p\n",addr->addr6.sin6_family
    							, ipv6_1, ipv6_2, ipv6_3, ipv6_4
    							, ipv6_5, ipv6_6, ipv6_7, ipv6_8
    							, addr->addr6.sin6_port);
  }
}

void
print_msg(struct msg* msg)
{
  printf("-----msg len:%d S-----\n");
  printf("%s\n",msg->data);
  printf("-----msg len:%d E-----\n");
}

void
print_port_info(struct netport* port)
{
  printf("[PORT]portid:%p\n",port->portid);
  acquire(&port->lk);
  printf("[PORT]msgnum:%p\n",port->msgnum);
  struct list* head = &port->msg;
  struct list* it = list_next(head);
  while(it!=head){
    struct msg* msg = dlist_entry(it,struct msg, list);
    print_msg(msg);
    it = list_next(it);
  }
  printf("[PORT]end\n");
  release(&port->lk);
}
