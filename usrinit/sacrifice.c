#include"user.h"
//static char teststr[]="ABCD\n";
int main(int argc,char* argv[]){
  printf("I'm %s%d\n",argv[0],getpid());
  exit(0);
  return 0;
}
