#include"user.h"
static char teststr[]="ABCD\n";
static char hh[] = "\n";
int main(int argc,char* argv[]){
  write(0,argv[0],10);
  write(0,hh,1);
  write(0,teststr,5);
  exit(0);
  return 0;
}
