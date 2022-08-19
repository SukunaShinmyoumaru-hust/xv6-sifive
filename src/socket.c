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
#include "include/copy.h"
#include "include/console.h"
#include "include/string.h"
#include "include/stat.h"
#include "include/vm.h"
#include "include/uname.h"
#include "include/socket.h"

struct netIP localIP[LOCALIPNUM];
int skid;
int testbit;

int
netinit()
{
  testbit = 0;
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
  list_init(&port->req);
}

void
IPinit(struct netIP* ip,void* addr,int type)
{
  ip->type = type;
  int cplen = type==IPv4?4:16;
  memcpy(ip->addr, addr, cplen);
  for(int i = 0;i<PORTNUM;i++){
    portinit(ip->ports+i, i);
    ip->ports[i].IP = ip;
  }
}

void
IPaddr(struct netIP* ip,sockaddr* addr)
{
  if(ip->type == IPv4){
    addr->addr4.sin_family = 0x2;
    memcpy(&addr->addr4.sin_addr, ip->addr, 4);
  }else if(ip->type == IPv6){
    addr->addr6.sin6_family = 0xa;
    memcpy(addr->addr6.sin6_addr.__u6_addr.__u6_addr8, ip->addr, 16);
  }
}

void
portaddr(struct netport* port,sockaddr* addr)
{
  IPaddr(port->IP,addr);
  int family = ADDR_FAMILY(addr);
  if(family == 0x2){
    addr->addr4.sin_port = port->portid;
  }else if(family == 0xa){
    addr->addr6.sin6_port = port->portid;
  }
}

int
bindaddr(struct socket* sk, sockaddr* addr){
    int ret = -1;
    struct netport* port = findport(addr);
    if(!port){
      __debug_warn("[bind addr] addr not found\n");
      return ret;
    }
    acquire(&port->lk);
    if(!port->sk){
      sk->bind_port = port;
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
      sk->bind_port = ports+i;
      ports[i].sk = sk;
      release(&ports[i].lk);
      return 0;
    }
    release(&ports[i].lk);
  }
  return -1;
}

int
connect(struct socket* sk, sockaddr* addr)
{
  struct netport* myport = sk->bind_port;
  struct netport* port = findport(addr);
  if(!port||!myport){
    return -1;
  }
  acquire(&myport->lk);
  sk->conn_port = port; 
  myport->conn = port;
  sendreq(myport,port);
  release(&myport->lk);
  sleep(port,&sk->lk);
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

static inline void port_recv_msg(struct netport* port,struct msg* msg)
{
  acquire(&port->lk);
  port->msgnum++;
  list_add_before(&port->msg,&msg->list);
  struct socket* sk = NULL;
  for(int i = 0;i<PORTNUM;i++){
     if(!port->sesk[i])continue;
     if(port->sesk[i]->conn_port==msg->port){
       sk = port->sesk[i];
       break;
     }
  }
  if(sk){
    slock(sk);
    socketwakeup(sk);
    sunlock(sk);
  }
  release(&port->lk);
}

static int port_has_msg(struct netport* port,struct netport* pport)
{
  int ret = 0;
  acquire(&port->lk);
  struct list* head = &port->msg;
  struct list* it = head;
  while(it!=head){
    struct msg* msg = dlist_entry(it,struct msg,list);
    if(msg->port == pport){
      ret = 1;
      break;
    }
    it = list_next(it);
  }
  release(&port->lk);
  return ret;
}

struct msg* port_pop_msg(struct netport* port,struct netport* pport)
{
  struct msg* msg = NULL;
  acquire(&port->lk);
  struct list* head = &port->msg;
  struct list* it = head;
  while(it!=head){
    msg = dlist_entry(it,struct msg,list);
    if(msg->port == pport){
      break;
    }
    it = list_next(it);
  }
  release(&port->lk);
  return msg;
}

void port_recv_req(struct netport* port,struct msg* msg)
{
  acquire(&port->lk);
  list_add_before(&port->req,&msg->list);
  struct socket* sk = port->sk;
  if(sk){
    slock(sk);
    socketwakeup(sk);
    sunlock(sk);
  }
  release(&port->lk);
}

int port_has_req(struct netport* port)
{
  int ret;
  acquire(&port->lk);
  ret = !list_empty(&port->req);
  release(&port->lk);
  return ret;
}

struct msg* port_pop_req(struct netport* port)
{
  struct msg* msg = NULL;
  acquire(&port->lk);
  if(!list_empty(&port->req)){
    msg = dlist_entry(list_next(&port->req),struct msg, list);
    list_del(&msg->list);
    port->msgnum--;
  }
  release(&port->lk);
  return msg;
}

struct netport*
getconn(struct socket* sk, struct netport* port)
{
  int nonblock = sk->nonblock;
  if(!port)return NULL;
  struct msg* msg = NULL;
  struct wait_node node;
  node.chan = &msg;
  socketlock(sk,&node);
  while(1){
    msg = port_pop_req(port);
    if(msg||nonblock)break;
    else{
      sleep(&msg,&sk->lk);
    }
  }
  socketunlock(sk,&node);
  struct netport* ret = msg?msg->port:NULL;
  if(msg)destroymsg(msg);
  wakeup(port);
  return ret;
}


struct socket*
socketalloc()
{
  struct socket* sk = kmalloc(sizeof(struct socket));
  memset(sk,0,sizeof(struct socket));
  sk->id = ++skid;
  initlock(&sk->lk,"socket");
  wait_queue_init(&sk->rqueue, "socket read queue");
  return sk;
}

void
socketclose(struct socket* sk)
{
  //acquire(&sk->lk);
  //release(&sk->lk);
  kfree(sk);
}

void socketwakeup(struct socket *sk)
{
	struct wait_queue *queue= &sk->rqueue;
	acquire(&queue->lock);
	if (!wait_queue_empty(queue)) {
		struct wait_node *wno = wait_queue_next(queue);
		wakeup(wno->chan);
	}
	release(&queue->lock);
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

void socketlock(struct socket *sk, struct wait_node *wait)
{
	struct wait_queue *q = &sk->rqueue;

	acquire(&q->lock);
	wait_queue_add(q, wait);	// stay in line

	// whether we are the first arrival
	while (!wait_queue_is_first(q, wait)) {
		sleep(wait->chan, &q->lock);
	}
	release(&q->lock);
}

void socketunlock(struct socket *sk, struct wait_node *wait)
{
	struct wait_queue *q = &sk->rqueue;

	acquire(&q->lock);
	// if (wait_queue_empty(q))
	// 	panic("pipeunlock: empty queue");
	// wait_queue_empty(q);

	// if (wait != wait_queue_next(q))
	// 	panic("pipeunlock: not next");
	
	wait_queue_del(wait);			// walk out the queue
	if (!wait_queue_empty(q)) {		// wake up the next one
		wait = wait_queue_next(q);
		wakeup(wait->chan);
	}
	release(&q->lock);
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
  char* data = kmalloc(n);
  if(!data)return 0;
  if(either_copyin(user,data,addr,n)<0){
    return 0;
  }
  struct msg* msg =createmsg(data,n);
  if(!msg)return 0;
  sendmsg(sk,NULL,msg);
  destroymsg(msg);
  print_port_info(localIP->ports+0);
  printf("[socket write]ret %d\n",n);
  return n;
}

struct msg*
createmsg(char* data, uint64 len)
{
  struct msg* msg = kmalloc(sizeof(struct msg));
  if(!msg)return NULL;
  msg->data = data;
  msg->len = len;
  msg->port = NULL;
  list_init(&msg->list);
  return msg;
}

struct msg*
nullmsg()
{
  struct msg* msg = kmalloc(sizeof(struct msg));
  msg->data = NULL;
  return msg;
}

void
destroymsg(struct msg* msg)
{
  if(msg->data)kfree(msg->data);
  kfree(msg);
}


struct msg*
msgcopy(struct msg* msg){
  struct msg* cp = kmalloc(sizeof(struct msg));
  cp->data = kmalloc(msg->len);
  cp->len = msg->len;
  cp->port = NULL;
  memcpy(cp->data,msg->data,msg->len);
  list_init(&cp->list);
  return cp;
}

void
sendmsg(struct socket* sock, sockaddr* addr, struct msg* msg)
{
  struct netport* port;
  if(sock->sk_type==SK_CONNECT){
    port = sock->conn_port;
  }
  else port = findport(addr);
  if(!port){
    __debug_warn("send fail\n");
    return;
  }
  struct msg* cp = msgcopy(msg);
  cp->port = sock->bind_port;
  printf("[sendmsg]");
  print_port_info(port);
  testbit = 1;
  port_recv_msg(port, cp);
}

void
sendreq(struct netport* port, struct netport* pport)
{
  struct msg* msg = nullmsg();
  msg->port = port;
  port_recv_req(pport,msg);
}

struct msg*
recvmsgfrom(struct socket* sk,sockaddr* addr)
{
  struct netport* port;
  if(sk->sk_type==SK_CONNECT){
    port = sk->bind_port;
  }
  else port = findport(addr);
  if(!port){
    __debug_warn("recv fail\n");
    return NULL;
  }
  struct msg* msg =NULL;
  struct wait_node node;
  node.chan = &msg;
  socketlock(sk,&node);
  while(1){
    msg = port_pop_msg(port,sk->conn_port);
    if(msg)break;
    else{
      printf("sleep on %p\n",&msg);
      sleep(&msg,&sk->lk);
    }
  }
  socketunlock(sk,&node);
  
  return msg;
}

uint32
acceptepoll(struct file *fp, struct poll_table *pt)
{
	uint32 mask = 0;
	struct socket* sk = fp->sk;
	if(!sk)return 0;
	if (fp->readable)
		poll_wait(fp, &sk->rqueue, pt);
	if(sk->bind_port&&port_has_req(sk->bind_port))mask|=EPOLLIN;
	mask|=EPOLLOUT;
	return mask;
}


uint32
socketepoll(struct file *fp, struct poll_table *pt)
{
	uint32 mask = 0;
	struct socket* sk = fp->sk;
	if(!sk)return 0;
	if (fp->readable)
		poll_wait(fp, &sk->rqueue, pt);
	if(sk->bind_port&&port_has_msg(sk->bind_port,sk->conn_port))mask|=EPOLLIN;
	mask|=EPOLLOUT;
	return mask;
}


uint32
socketnoepoll(struct file *fp, struct poll_table *pt)
{
	return 0;
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
  printf("-----msg portid:%d len:%d S-----\n",msg->port->portid,msg->len);
  printf("%s\n",msg->data);
  printf("-----msg portid:%d len:%d E-----\n",msg->port->portid,msg->len);
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
