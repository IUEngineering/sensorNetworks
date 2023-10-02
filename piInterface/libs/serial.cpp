// Serial
// 
// This object handles serial connection in a Linux/Mac environment.
//
// Author : Edwin Boer
// Version: 20200404

#include "serial.hpp"

Serial::Serial() {

  // Init stream ID
  nStream_ = 0;
};

Serial::~Serial() {

  if (nStream_ > 0) {
    close(nStream_);
  };
};

bool Serial::init(char* sName, int nBaudrate) {

  struct termios oTtyConfig;

  // Try to open the serial connection
  nStream_ = open(sName, O_RDWR | O_NONBLOCK);
	if (nStream_ <= 0) {
    perror(sName);
		return false; 
	};

  // Init minimal serial settings for noncannonical mode (no extra character processing)
  oTtyConfig.c_iflag = 0; // Input
  oTtyConfig.c_oflag = 0; // Output
  oTtyConfig.c_lflag = 0; // Line
  oTtyConfig.c_cflag = CS8 | CREAD | CLOCAL; // Character
  oTtyConfig.c_cc[VMIN] = 1; // Length
  oTtyConfig.c_cc[VTIME] = 0; // Timeout
  cfsetispeed(&oTtyConfig, nBaudrate); // Input speed
  cfsetospeed(&oTtyConfig, nBaudrate); // Output speed

  // Clear buffers 
  tcflush(nStream_, TCIFLUSH);

  // Set new settings NOW
  tcsetattr(nStream_, TCSANOW, &oTtyConfig);

  return true;
};

uint8_t Serial::get(uint8_t* nByte) {

  ssize_t nSize;

  // Check for valid open connection
  if (nStream_ <= 0) {
    return SERIAL_WARNING;
  };

  // Read one byte only
  nSize = read(nStream_, nByte, 1);
  if (nSize == 0) {
    return SERIAL_WARNING + 1;
  };
  if (nSize == -1) {
    // Fatal error 6: Device not configured or disconnected
    if (errno == 6) {
      return SERIAL_WARNING + 2;
    };

    // Warning Mac 35: Resource temporarily unavailable
    if (errno == 35) {
      return 1;
    };

    // Warning Linux 11: Resource temporarily unavailable
    if (errno == 11) {
      return 2;
    };

    // Catch all
    printf("get()-error %d: %s ", errno, strerror(errno));
    return 255;
  };

  return 0;
};

bool Serial::put(uint8_t nByte) {

  ssize_t nSize;

  // Check for valid open connection
  if (nStream_ <= 0) {
    return false;
  };

  // Write one byte only
  nSize = write(nStream_, &nByte, 1);
  if (nSize != 1) {
    return false;
  };

  return true;
};
