// Serial
// This object handles serial connection in a Linux/Mac environment.
// Name options:      /dev/ttyACM0                Linux
//                    /dev/tty.usbmodem14101      Mac OS
// Baudrate options:  B0  B50  B75  B110  B134  B150  B200
//                    B300  B600  B1200  B1800  B2400  B4800
//                    B9600  B19200  B38400  B57600  B115200
//                    B230400  B460800

#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <termios.h>
#include <stdint.h>


#define SERIAL_WARNING 10

uint8_t initUartStream(char *name, uint32_t baudrate);
uint8_t serialPutChar(uint8_t byte);
uint8_t serialGetChar(uint8_t *byte);
uint8_t sendBuffer(uint8_t *buffer, uint8_t length);
void exitUartStream(void);

#endif // _SERIAL_H_
