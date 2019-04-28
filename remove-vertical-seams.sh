#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "USAGE: $0 <input-filename> <num-vertical-seams-to-remove>"
    exit 1
fi

echo "removing $2 vertical seams from $1"

make

input=out/input.png
output=out/output.png

rm -rf out
mkdir out
cp $1 $input

n=$2
for i in $(seq $n); do
    ./seam-carver $input $output
    cp $output $input
done
