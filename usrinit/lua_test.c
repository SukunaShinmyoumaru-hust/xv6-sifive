#include"user.h"
#include"include/elf.h"
//static char teststr[]="ABCD\n";

static char* testarg[5] = {"./lua_testcode.sh", 0};
/*
static mytest lua[] = {
	{ 1 , "date.lua"	},
	{ 1 , "file_io.lua"	},
	{ 1 , "max_min.lua"	},
	{ 1 , "random.lua"	},
	{ 1 , "remove.lua"	},
	{ 1 , "round_num.lua"	},
	{ 1 , "sin30.lua"	},
	{ 1 , "sort.lua"	},
	{ 1 , "strings.lua"	},
	{ 0 , 0		},
};
*/

void test_lua(){
/*
  int i,status;
  testarg[0] = "./lua";
  testarg[2] = 0;
  
  for(i = 0; lua[i].name ; i++){
    if(!lua[i].valid)continue;
    testarg[1] = lua[i].name;
    int pid = fork();
    if(pid==0){
      exec("./lua",testarg);
    }
    wait4(pid,&status);
    if(status==0){
      printf("testcase lua %s success\n",lua[i].name);
    }else{
      printf("testcase lua %s fail\n",lua[i].name);
    }
  }
  */
  int status;
  int pid = fork();
  if(pid==0){
    exec("./lua_testcode.sh",testarg);
  }
  wait4(pid,&status);
}
