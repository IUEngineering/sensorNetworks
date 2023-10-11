#include "mcuComm.h"
#include "serial.h"
#include "buttons.h"

uint8_t commBuffer[255 * 5 + 2];

//!! make a exit function so if program stops or restarts it releases /dev/ACM*

void initMcuComm(void){
    //TODO: show connecting
    
    initUartStream("/dev/ttyACM1", B115200);

}


void sendC(button_t *button){
    serialPutChar('c');
}


void interpretBullshit(){

    //serialGetChar();

 
}

