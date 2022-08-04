#include"user.h"
static char teststr[]="ABCD\n";
int main(){
  write(0,teststr,5);
  exit(0);
  return 0;
}
