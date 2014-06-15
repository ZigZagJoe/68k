#!/bin/bash

if [ -z "$1" ]; then
 echo "Argument required."
 exit 1
fi

IN="$1"
NAME=`basename $1 | sed 's/\..*//'`
OUT="flashimg_$NAME.bin"

echo "Creating $OUT with $IN"

cp flashimg_empty.bin $OUT
dd "if=$IN" "of=$OUT" conv=notrunc
dd "if=$IN" "of=$OUT" obs=1 seek=262144 conv=notrunc

