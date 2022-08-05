#include"user.h"
//static char teststr[]="ABCD\n";
int main(int argc,char* argv[]){
  int pid = getpid();
  if(pid>1)wait4(pid-1,0);
  printf("I'm %s%d\n",argv[0],pid);
  exit(0);
  return 0;
}
