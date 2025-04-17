#!/bin/sh

count=150

for ((i=0; i<count; i++))
do
    random=$((RANDOM % 256))
    echo "$random" >> numbers.txt
done