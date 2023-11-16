  #include <stdbool.h>
  #include <stdio.h>
  #include <stdint.h>
  #include <dirent.h>
  #include <string.h>
  #include <unistd.h>


  // Include the serial library
  #include "serial.h"

  int main(int argc, char *argv[]) {

    char deviceName[100];

    printf("Linux serial start\n\n");

    // no input parameter? Show options for Linux and Mac
    if (argc == 1){

        DIR *chosenDir;
        struct dirent *item;

        printf("> Fault: there are no active serial devices chosen\n");
        printf("> The active options are:\n");
        chosenDir = opendir("/dev/");
        if(chosenDir != NULL) {
            while((item = readdir(chosenDir)) != NULL) {
                if (strncmp(item->d_name, "ttyACM", 6) ==0 ){
                    printf("%s ", item->d_name);
                }
            }
            closedir(chosenDir);
        }

        printf("\n\n");
        return 1;
    }

    // Init device name
    strcpy(deviceName, "/dev/");
    strncpy(deviceName + 5, argv[1], 100 - 5);
    deviceName[100 - 1] = 0;

    // Init connection (B38400 or B57600 or B115200)
    printf("> Connecting: ");
    if(!initUartStream(deviceName, B115200)){
        printf(" Fault: Serial device could not be opened!\n");
        return 1;
    }
    else{
        printf(" OK\n");
    }

    // Start char
    serialPutChar('c');

    while(1) {
        uint8_t inByte;
        uint8_t err = serialGetChar(&inByte);
        if(err) {
            //printf("fuck off %d\n", err);
        }
        else {
            printf("%c ", inByte); 
        }
        //printf("%02x ", inByte); 
    }
}