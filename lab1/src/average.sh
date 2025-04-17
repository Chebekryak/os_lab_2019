#!/bin/sh

sum=0
count=$#

for num in "$@"; do
    sum=$(echo "$sum + $num" | bc)
done

average=$(echo "$sum / $count" | bc)

echo "Количество аргументов: $count"
echo "Среднее арифметическое: $average"