#!/bin/bash

a=$(date +"%s")
sudo ./client $1 $2
b=$(date +"%s")
echo "start time is $a"
echo "end time is $b"
file_size=`ls -alt $2 | cut -f5 -d " "`
echo "file size is $file_size"
diff=$((b-a))
echo "execution time is $diff"
printf "throughput is %d" $((8*file_size/(diff*1000000)))
printf ".%d\n" $((8*file_size%diff))
sleep 5
sudo ./server
