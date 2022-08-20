#include"user.h"
#include"include/elf.h"
//static char teststr[]="ABCD\n";

static longtest lmbench[];

void test_lmbench(){
  int i,status;
  printf("latency measurements\n");
  for(i = 0; lmbench[i].name[1] ; i++){
    if(!lmbench[i].valid)continue;
    int pid = fork();
    if(pid==0){
      exec(lmbench[i].name[0],lmbench[i].name);
    }
    wait4(pid,&status);
  }
}

static longtest lmbench[] = {
	{ 1 , {"lmbench_all"  ,  "lat_syscall"  ,  "-P"  ,  "1"  ,  "null"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "lat_syscall"  ,  "-P"  ,  "1"  ,  "read"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "lat_syscall"  ,  "-P"  ,  "1"  ,  "write" ,  0	}},
	{ 1 , {"busybox"  ,  "mkdir"  ,  "-p"  ,  "/var/tmp"  ,  0	}},
	{ 1 , {"busybox"  ,  "touch"  ,  "/var/tmp/lmbench"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "lat_syscall"  ,  "-P"  ,  "1"  ,  "stat"  ,  "/var/tmp/lmbench"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "lat_syscall"  ,  "-P"  ,  "1"  ,  "fstat"  ,  "/var/tmp/lmbench"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "lat_syscall"  ,  "-P"  ,  "1"  ,  "open"  ,  "/var/tmp/lmbench"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "lat_select"  ,  "-n"  ,  "100"  ,  "-P"  ,  "1"  ,  "file"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "lat_sig"  ,  "-P"  ,  "1"  ,  "install"  ,  0	}},
	{ 0 , {"lmbench_all"  ,  "lat_sig"  ,  "-P"  ,  "1"  ,  "catch"  ,  0	}},
	{ 0 , {"lmbench_all"  ,  "lat_sig"  ,  "-P"  ,  "1"  ,  "prot"  ,  "lat_sig"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "lat_pipe"  ,  "-P"  ,  "1"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "lat_proc"  ,  "-P"  ,  "1"  ,  "fork"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "lat_proc"  ,  "-P"  ,  "1"  ,  "exec"  ,  0	}},
	{ 0 , {"busybox"  ,  "cp"  ,  "hello"  ,  "/tmp"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "lat_proc"  ,  "-P"  ,  "1"  ,  "shell"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "lmdd"  ,  "label=File /var/tmp/XXX write bandwidth:"  ,  "of=/var/tmp/XXX"  ,  "move=1m"  ,  "fsync=1"  ,  "print=3"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "lat_mmap"  ,  "-P"  ,  "1"  ,  "512k"  ,  "/var/tmp/XXX"  ,  0	}},
	{ 1 , {"busybox"  ,  "echo"  ,  "file"  ,  "system"  ,  "latency"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "lat_fs"  ,  "/var/tmp"  ,  0	}},
	{ 1 , {"busybox"  ,  "echo"  ,  "Bandwidth"  ,  "measurements"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "bw_pipe"  ,  "-P"  ,  "1"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "bw_file_rd"  ,  "-P"  ,  "1"  ,  "512k"  ,  "io_only"  ,  "/var/tmp/XXX"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "bw_file_rd"  ,  "-P"  ,  "1"  ,  "512k"  ,  "open2close"  ,  "/var/tmp/XXX"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "bw_mmap_rd"  ,  "-P"  ,  "1"  ,  "512k"  ,  "mmap_only"  ,  "/var/tmp/XXX"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "bw_mmap_rd"  ,  "-P"  ,  "1"  ,  "512k"  ,  "open2close"  ,  "/var/tmp/XXX"  ,  0	}},
	{ 1 , {"busybox"  ,  "echo"  ,  "context"  ,  "switch"  ,  "overhead"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "lat_ctx"  ,  "-P"  ,  "1"  ,  "-s"  ,  "32"  ,  "2"  ,  "4"  ,  "8"  ,  "16"  ,  "24"  ,  "32"  ,  "64"  ,  "96"  ,  0	}},
	{ 1 , {"lmbench_all"  ,  "lat_pagefault"  ,  "-P"  ,  "1"  ,  "/var/tmp/XXX"  ,  0	}},
	{ 0 , { 0 , 0				}},
};
