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
        if (chosenDir != NULL) {
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

    printf("Wait 2 sec: "); fflush(stdout);
    sleep(2);
    printf("OK"); fflush(stdout);

    // Stuur de test bytes
    uint8_t cw = 0, cr = 0;
    uint8_t retAmount;
    do {
        printf("> put "); fflush(stdout);
        putChar(cw);
        printf("%d =?= ", cw); fflush(stdout);

    printf("get "); fflush(stdout);
    do {
      retAmount = getChar(&cr);
      if (retAmount > SERIAL_WARNING) {
        // Fatale fout
        printf("\nFAULT! %d: device can't be connected\n", retAmount);
        return 2;
      };

    } while (retAmount > 0);
    printf("%d ", cr); fflush(stdout);

    printf("\n"); fflush(stdout);
    cw++;
  }
  while (cw != 0);

  // Afsluiten
  printf("\nClosed :D \n");

  return 0;


  }