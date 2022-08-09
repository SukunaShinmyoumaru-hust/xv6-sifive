#!/bin/bash
./busybox mkdir test_dir
./busybox mv test_dir test
./busybox rmdir test
./busybox grep hello busybox_cmd.txt
./busybox cp busybox_cmd.txt busybox_cmd.bak
./busybox rm busybox_cmd.bak
./busybox find -name "busybox_cmd.txt"
