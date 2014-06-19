#!/bin/bash

if [ -z "$1" ]; then
 echo "Argument required."
 exit 1
fi

if [ -z "$2" ]; then
 echo "Argument required."
 exit 1
fi

IN1="$1"
IN2="$2"
NAME1=`basename $1 | sed 's/\..*//'`
NAME2=`basename $2 | sed 's/\..*//'`
OUT="flashimg_$NAME1-$NAME2.bin"

echo "Creating $OUT with $IN1, $IN2"

cp flashimg_empty.bin $OUT
dd "if=$IN1" "of=$OUT" conv=notrunc
dd "if=$IN2" "of=$OUT" obs=1 seek=262144 conv=notrunc

