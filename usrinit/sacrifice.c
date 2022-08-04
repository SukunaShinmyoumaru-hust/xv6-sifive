#include"user.h"
static char teststr[]="ABCD\n";
int main(int argc,char* argv[]){
  printf("I'm %s\n",argv[0]);
  printf("teststr:%d %d %d %d %d\n",teststr[0]
                                   ,teststr[1]
                                   ,teststr[2]
                                   ,teststr[3]
                                   ,teststr[4]);
  exit(0);
  return 0;
}
