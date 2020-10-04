#!/bin/bash

JG_PATH=/home/jg

files=$(ls $JG_PATH/result_*)

t=16
for i in $(seq 3 9)
do
	echo "$t KB"
	for f in $files
	do
		result_name=`basename $f`
		./compress $f $t > $result_name-$t-result &
	done
	while (true); do
		pidof compress > /dev/null
		if [ $? -eq 1 ] 
		then
			break
		fi
		sleep 1
	done
	t=$((t*2))
done
