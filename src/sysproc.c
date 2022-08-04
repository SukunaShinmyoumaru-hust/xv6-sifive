#include"include/types.h"
#include"include/printf.h"

uint64
sys_execve()
{
  printf("[sys execve]\n");
  return 0;
}

uint64
sys_exit()
{
  printf("[sys exit]\n");
  while(1){
  
  }
  return 0;
}
