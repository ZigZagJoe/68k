#!/bin/bash

for ((a=56000; a <= 58000 ; a+=5))  # Double parentheses, and naked "LIMIT"
do
 echo $a
 upload-loader -a 0x4000 bootloader.s68 -r $a 
 echo
done
