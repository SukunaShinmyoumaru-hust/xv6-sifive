#!/bin/bash

# ./lua_testcode.sh
# ./busybox_testcode.sh
# ./lmbench_testcode.sh
# lmbench_all lat_syscall -P 1 null
# lmbench_all lat_syscall -P 1 read
# lmbench_all lat_syscall -P 1 write
busybox mkdir -p /var/tmp
busybox touch /var/tmp/lmbench
# lmbench_all lat_syscall -P 1 stat /var/tmp/lmbench
# lmbench_all lat_syscall -P 1 fstat /var/tmp/lmbench
# lmbench_all lat_syscall -P 1 open /var/tmp/lmbench
lmbench_all lat_select -n 100 -P 1 file
lmbench_all lat_sig -P 1 install
lmbench_all lat_sig -P 1 catch
lmbench_all lat_sig -P 1 prot lat_sig
lmbench_all lat_pipe -P 1
lmbench_all lat_proc -P 1 fork
lmbench_all lat_proc -P 1 exec