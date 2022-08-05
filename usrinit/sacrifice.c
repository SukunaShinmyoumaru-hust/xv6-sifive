#include"user.h"
//static char teststr[]="ABCD\n";
int main(int argc,char* argv[]){
  for(int i=0;i<12;i++){
    int pid = fork();
    if(pid){
    	wait(pid);
    	printf("I'm %s%d\n",argv[0],pid);
    }
    else{
    	exit(0);
    }
    
    
  }
  exit(0);
  return 0;
}
