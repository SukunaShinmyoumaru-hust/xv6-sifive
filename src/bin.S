.align 4
.global initcode
.global initcodesize
initcode:
.incbin "./usrinit/initcode"
initcode_end:
initcodesize:
.long initcode_end-initcode
.global initcode
.global sacrifice_start
.global sacrifice_size
sacrifice_start:
.incbin "./usrinit/mytest.sh"
sacrifice_start_end:
sacrifice_size:
.long sacrifice_start_end-sacrifice_start
.global sacrifice_start
.global localtime
.global localtime_size
localtime:
.incbin "./bin/localtime"
localtime_end:
localtime_size:
.long localtime_end-localtime
.global localtime
.global mounts
.global mounts_size
mounts:
.incbin "./bin/mounts"
mounts_end:
mounts_size:
.long mounts_end-mounts
.global mounts
.global meminfo
.global meminfo_size
meminfo:
.incbin "./bin/meminfo"
meminfo_end:
meminfo_size:
.long meminfo_end-meminfo
.global meminfo
.global lat_sig
.global lat_sig_size
lat_sig:
.incbin "./bin/lat_sig"
lat_sig_end:
lat_sig_size:
.long lat_sig_end-lat_sig
.global lat_sig
.global hello
.global hello_size
hello:
.incbin "./bin/hello"
hello_end:
hello_size:
.long hello_end-hello
.global hello
.global sh
.global sh_size
sh:
.incbin "./bin/sh"
sh_end:
sh_size:
.long sh_end-sh
.global sh
