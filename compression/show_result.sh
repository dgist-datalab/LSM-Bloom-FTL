#!/bin/bash

list=(4 5 6 8 12)
buf_size=16

for i in $(seq 1 7) 
do
	echo $buf_size
	for j in ${list[@]} 
	do
		cat result_$j-$buf_size-result
	done
	buf_size=$((buf_size*2))
done
