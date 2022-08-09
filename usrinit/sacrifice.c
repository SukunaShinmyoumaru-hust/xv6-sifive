#include"user.h"
#include"include/elf.h"
//static char teststr[]="ABCD\n";

static char *args[] = {"/mytest.sh",0};

int main(int argc, char* argv[]){
  exec("/mytest.sh", args);
  exit(0);
  return 0;
}
