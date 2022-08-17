kbin=src/bin.S

echo ".align 4">>$kbin
entry(){
	echo ".global $2">>$kbin
	echo ".global $3">>$kbin
	echo "$2:">>$kbin
	echo ".incbin \"$1\"">>$kbin
	echo "$2_end:">>$kbin
	echo "$3:">>$kbin
	echo ".long $2_end-$2">>$kbin
	echo ".global $2">>$kbin;
}

entry ./usrinit/initcode initcode initcodesize
entry ./usrinit/server.sh server_start server_size
entry ./usrinit/client.sh client_start client_size
entry ./bin/localtime localtime localtime_size
entry ./bin/mounts mounts mounts_size
entry ./bin/meminfo meminfo meminfo_size
entry ./bin/lat_sig lat_sig lat_sig_size
entry ./bin/hello hello hello_size
entry ./bin/sh sh sh_size


