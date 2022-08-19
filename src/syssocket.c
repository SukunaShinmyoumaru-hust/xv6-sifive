#include "include/types.h"
#include "include/printf.h"
#include "include/fcntl.h"
#include "include/dev.h"
#include "include/syscall.h"
#include "include/mmap.h"
#include "include/copy.h"
#include "include/pipe.h"
#include "include/errno.h"

uint64
sys_socket(void)
{
  int fd;
  int domain;
  int type;
  int protocol;
  argint(0, &domain);
  argint(1, &type);
  argint(2, &protocol);
  //printf("SOCKET domain:%p type:%p protocol:%p\n", domain, type, protocol);
  struct file* f = NULL;
  struct socket* sk = NULL;
  if((f=filealloc())==NULL||(fd=fdalloc(f))<0){
    goto bad;
  }
  f->type = FD_SOCKET;
  sk = socketalloc();
  f->sk = sk;
  f->readable = 1;
  f->writable = 1;
  sk->domain = domain;
  sk->type = type;
  sk->protocol = protocol;
  //printf("create socket fd:%d id:%d\n",fd,sk->id);
  return fd;
bad:
  if(!f)fileclose(f); 
  if(!sk)socketclose(sk); 
  return -1;
}

uint64
sys_setsockopt(void)
{
  int sockfd;
  int level;
  int optname;
  uint64 optvaladdr;
  socklen_t optlen;
  struct file* f = NULL;
  struct socket* sk;
  if(argsock(0, &sockfd, &f, &sk)<0){
    return -1;
  }
  argint(1, &level);
  argint(2, &optname);
  argaddr(3, &optvaladdr);
  argint(4, &optlen);
  //printf("[setsockopt]sockfd:%d level:%d optname:%d optaddr:%p optlen:%p\n",sockfd, level,optname, optvaladdr,optlen);
  return 0;
}

uint64
sys_bind(void)
{
  int sockfd;
  struct file* f;
  socklen_t addrlen;
  struct socket* sk;
  sockaddr addr;
  if(argsock(0, &sockfd, &f, &sk)<0){
    return -1;
  }
  slock(sk);
  if(sk->sk_type==SK_NONE){
    sk->sk_type = SK_BIND;
  }else{
    __debug_warn("socket type conflct\n");
    goto bad;
  }
  if(argint(2, &addrlen)<0){
    goto bad;
  }
  //printf("bind sockfd:%d addrlen:%p\n", sockfd, addrlen);
  if(argstruct(1, &addr, addrlen)==0){
    goto bad;
  }
  //print_sockaddr(&addr);
  if(bindaddr(sk, &addr)<0){
    printf("[sys bind]bind bad\n");
    goto bad;
  }
  extern struct netIP localIP[];
  if(sk->bind_port->IP==localIP+1){
    f->epollv = socketnoepoll;
  }else{
    f->epollv = acceptepoll;
  }
  sunlock(sk);
  return 0;
bad:
  sk->type =SK_NONE;
  sunlock(sk);
  return -1;
}

uint64
sys_connect(void){
  int sockfd;
  struct file* f;
  struct socket* sk;
  sockaddr addr;
  socklen_t addrlen;
  if(argsock(0, &sockfd, &f, &sk)<0){
    return -1;
  }
  slock(sk);
  if(sk->sk_type==SK_NONE){
    sk->sk_type = SK_CONNECT;
  }else{
    __debug_warn("socket type conflct\n");
    goto bad;
  }
  if(argint(2, &addrlen)<0){
    goto bad;
  }
  if(bindalloc(sk)<0){
    return -1;
  }
  seskport(sk->bind_port, sk);
  //printf("conncet sockfd:%d addrlen:%p\n", sockfd, addrlen);
  if(argstruct(1, &addr, addrlen)==0){
    goto bad;
  }
  //print_sockaddr(&addr);
  if(connect(sk,&addr)<0){
    goto bad;
  }
  sunlock(sk);
  f->epollv = socketepoll;
  return 0;
bad:
  sk->type = SK_NONE;
  sunlock(sk);
  return -1;
}

uint64
sys_sendto(void)
{
  int sockfd;
  struct file* f;
  sockaddr* addr;
  struct socket* sk;
  uint64 bufaddr;
  char* buf;
  uint64 len;
  int flags;
  socklen_t addrlen;
  int addrfree = 0;
  if(argsock(0, &sockfd, &f, &sk)<0){
    return -1;
  }
  argaddr(1, &bufaddr);
  argaddr(2, &len);
  argint(3, &flags);
  buf = kmalloc(len);
  if(either_copyin(1, buf, bufaddr,len)<0){
    return -1;
  }
  struct msg* msg = createmsg(buf, len);
  if(argint(5, &addrlen)<0){
    return -1;
  }
  slock(sk);
  if(sk->sk_type == SK_CONNECT){
    sendmsg(sk, NULL, msg);
  }else{
    addr = kmalloc(sizeof(sockaddr));
    addrfree = 1;
    if(argstruct(4, addr, addrlen)==0){
      return -1;
    }
    sendmsg(sk, addr, msg);
  }
  //printf(" sendto sockfd:%d addrlen:%p\n", sockfd, addrlen);
  destroymsg(msg);
  msg = nullmsg();
  sendmsg(sk,NULL,msg);
  destroymsg(msg);
  sunlock(sk);
  if(addrfree)kfree(addr);
  /*
  sockaddr tmpaddr = (sockaddr){
    .addr4 = (struct sockaddr_in){
      .sin_family = AF_INET,
      .sin_addr = 0x100007f,
      .sin_port = 0xeb18,
    }
  };
  print_port_info(findport(&tmpaddr));
  printf("sendto leave\n");
  */
  return len;
}

uint64
sys_recvfrom(void)
{
  int sockfd;
  struct file* f;
  sockaddr* addr = NULL;
  struct socket* sk;
  uint64 bufaddr;
  int addrfree = 0;
  //char buf[100];
  uint64 len;
  int flags;
  socklen_t addrlen;
  if(argsock(0, &sockfd, &f, &sk)<0){
    return -1;
  }
  argaddr(1, &bufaddr);
  argaddr(2, &len);
  argint(3, &flags);
  //printf("recvfrom bufaddr:%p\n",bufaddr);
  if(argint(5, &addrlen)<0){
    return -1;
  }
  slock(sk);
  if(sk->sk_type == SK_CONNECT){
    
  }else{
    addr = kmalloc(sizeof(sockaddr));
    addrfree = 1;
    if(argstruct(4, addr, addrlen)==0){
      return -1;
    }
    //print_sockaddr(addr);
  }
  //printf(" recvfrom sockfd:%d id:%d\n", sockfd,sk->id);
  len = socketread(sk,1,bufaddr,len);
  sunlock(sk);
  if(addrfree)kfree(addr);
  return len;
}

uint64
sys_listen(void)
{
  int sockfd;
  int backlog;
  struct file* f;
  struct socket* sk;
  if(argsock(0, &sockfd, &f, &sk)<0){
    return -1;
  }
  argint(1, &backlog);
  //printf("listen sockfd:%d backlog:%d\n",sockfd,backlog);
  return 0;
}

uint64
sys_accept4(void)
{
  int sockfd;
  int fd;
  int flags;
  struct file* f;
  struct socket* sk;
  struct socket* sessionsk = NULL;
  struct proc* p = myproc();
  sockaddr addr;
  uint64 addr_addr;
  socklen_t addrlen;
  uint64 addrlen_addr;
  if(argsock(0, &sockfd, &f, &sk)<0){
    printf("[sys accept4]argsock bad\n");
    return -1;
  }
  argaddr(1,&addr_addr);
  argaddr(2,&addrlen_addr);
  argint(3,&flags);
  //printf("[sys accept4] accept fd:%d id:%d flags:%p\n",sockfd,sk->id,flags);
  slock(sk);
  //printf("addrlen = %p addr=%p\n",addrlen_addr,addr_addr);
  struct netport* pport = getconn(sk, sk->bind_port);
  sunlock(sk);
  if(!pport){
    return -EAGAIN;
  }
  portaddr(pport, &addr);
  //print_sockaddr(&addr);
  if(either_copyout(1, addr_addr, &addr,sizeof(sockaddr))<0){
    return -1;
  }
  int family = ADDR_FAMILY(&addr);
  if(family == 0x2){
    addrlen = 0x10;
  }else if(family == 0xa){
    addrlen = 0x1c;
  }
  if(either_copyout(1, addrlen_addr, &addrlen,sizeof(socklen_t))<0){
    return -1;
  }
  
  struct file* newf = NULL;
  if((newf=filealloc())==NULL||(fd=fdalloc(newf))<0){
    goto bad;
  }
  
  newf->type = FD_SOCKET;
  sessionsk = socketalloc();
  sessionsk->sk_type = SK_CONNECT;
  sessionsk->conn_port = pport;
  sessionsk->bind_port = sk->bind_port;
  seskport(sk->bind_port, sessionsk);
  newf->sk = sessionsk;
  newf->epollv = socketepoll;
  newf->readable = 1;
  newf->writable = 1;
  sessionsk->nonblock =  flags&SOCK_NONBLOCK;
  p->exec_close[fd] = flags&SOCK_CLOEXEC;
  //printf("[accept4]create socket fd:%d id:%d\n",fd,sessionsk->id);
  return fd;
bad:
  if(!newf)fileclose(newf); 
  if(!sessionsk)socketclose(sessionsk); 
  return -1;
}

uint64
sys_getpeername(void)
{
  return 0;
}
