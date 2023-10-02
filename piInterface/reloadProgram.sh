#!/bin/sh
pkill basisstation
gcc -Wall -o ~/sensorNetworks/piInterface/basisstation ~/sensorNetworks/piInterface/test.c ~/hva_libraries/rpitouch/*.c -I/home/piuser/hva_libraries/rpitouch -lncurses
sleep 3
exec /home/piuser/sensorNetworks/piInterface/basisstation
