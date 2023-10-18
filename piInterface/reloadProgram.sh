#!/bin/sh
pkill basisstation
gcc -Wall -o ~/sensorNetworks/piInterface/basisstation ~/sensorNetworks/piInterface/main.c ~/sensorNetworks/piInterface/libs/* ~/hva_libraries/rpitouch/*.c -I/home/piuser/hva_libraries/rpitouch -lncurses -g
sleep 3
exec /home/piuser/sensorNetworks/piInterface/basisstation
