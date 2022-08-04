#include"include/types.h"
#include"include/printf.h"
#include"include/fat32.h"

int
exec(char *path, char **argv, char **env)
{
  printf("[exec]path:%s\n",path);
  int i;
  i = 0;
  while(argv[i]){
    printf("[exec]argv[%d] = %s\n",i,argv[i]);
    i++;
  }
  i = 0;
  while(env[i]){
    printf("[exec]env[%d] = %s\n",i,env[i]);
    i++;
  }
  
  return 0;
}
