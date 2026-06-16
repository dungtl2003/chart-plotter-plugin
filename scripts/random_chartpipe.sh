#!/usr/bin/env bash

# x=0
#
# while true; do
#     y=$(( RANDOM % 100 ))
#     z=$(( RANDOM % 100 ))
#     t=$(( RANDOM % 100 ))
#     printf "%d,%d,%d,%d\n" "$x" "$y" "$z" "$t" > /tmp/chartplotter.csvpipe
#     x=$(( x + 1 ))
#     sleep 0.5
# done
#

x=0
y=0
z=0
t=0

while true; do
    printf "%d,%d,%d,%d\n" "$x" "$y" "$z" "$t" > /tmp/chartplotter.csvpipe

    y=$(( y + (RANDOM % 11 - 5) ))
    z=$(( z + (RANDOM % 11 - 5) ))
    t=$(( t + (RANDOM % 11 - 5) ))

    x=$(( x + 1 ))
    sleep 0.25
done
