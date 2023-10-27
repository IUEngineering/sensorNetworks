#!/bin/sh
pkill -SIGINT basisstation
gcc -Wall -o ~/sensorNetworks/piInterface/basisstation ~/sensorNetworks/piInterface/main.c ~/sensorNetworks/piInterface/libs/* ~/hva_libraries/rpitouch/*.c -I/home/piuser/hva_libraries/rpitouch -lncurses
exec /home/piuser/sensorNetworks/piInterface/basisstation 2>logging.log
