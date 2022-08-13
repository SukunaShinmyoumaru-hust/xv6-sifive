#!/bin/bash

./lua_testcode.sh
./busybox_testcode.sh
./lmbench_testcode.sh
#lmbench_all lat_syscall -P 1 null
#lmbench_all lat_syscall -P 1 read
