#!/bin/bash

sudo ./server

sleep 20

a=$(date +"%s")
file_size=`ls -alt $2 | cut -f5 -d " "`
sudo ./client $1 $2
b=$(date +"%s")
echo "start time is $a"
echo "end time is $b"
diff=$((b-a))
echo "execution time is $diff"
echo "file size is $file_size"
printf "throughput is %d" $((8*file_size/(diff*1000000)))
printf ".%d\n" $((8*file_size%diff))
