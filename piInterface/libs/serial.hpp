// Serial
// 
// This object handles serial connection in a Linux/Mac environment.
//
// Name options:      /dev/ttyACM0                Linux
//                    /dev/tty.usbmodem14101      Mac OS
// Baudrate options:  B0  B50  B75  B110  B134  B150  B200
//                    B300  B600  B1200  B1800  B2400  B4800
//                    B9600  B19200  B38400  B57600  B115200
//                    B230400  B460800
//
// Author : Edwin Boer
// Version: 20200404

#ifndef _SERIAL_HPP_
#define _SERIAL_HPP_

  #include <string.h>
  #include <fcntl.h>
  #include <errno.h>
  #include <termios.h>
  #include <unistd.h>
  #include <stdbool.h>
  #include <stdint.h>
  #include <stdlib.h>	
  #include <stdio.h>

  #define SERIAL_WARNING 10

  class Serial {
    public:
      Serial();
      ~Serial();
      bool init(char* sName, int nBaudrate);
      uint8_t get(uint8_t* nByte);
      bool put(uint8_t nByte);

    private:
      int nStream_;
  };

#endif // _SERIAL_HPP_