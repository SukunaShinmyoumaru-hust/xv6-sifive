#include"user.h"
#include"include/elf.h"
//static char teststr[]="ABCD\n";

//static char *args[] = {"/mytest.sh",0};

int main(int argc, char* argv[]){
  test_lua();
  test_busybox();
  test_lmbench();
  exit(0);
  return 0;
}
