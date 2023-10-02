// Linux serial
//
// g++ -Wall -o linux_serial linux_serial.cpp serial.cpp
// ./linux_serial
//
// Author : Edwin Boer
// Version: 20200404

#include "linux_serial.hpp"

int main(int nArgc, char* aArgv[]) {

  char aDevicename[100];
  Serial oSerial;

  // Initialiseren
  printf("Linux serial\n\n");

  // Geen parameter? Toon dan de mogelijke Linux en Mac devices
  if (nArgc == 1) {

    DIR *pDir;
    struct dirent *pItem;

    printf("> Fout: er is geen serial device gekozen!\n");
    printf("> Actieve opties Linux:\n");
    pDir = opendir("/dev/");
    if (pDir != NULL) {
      while ((pItem = readdir(pDir)) != NULL) {
        if (strncmp(pItem->d_name, "ttyACM", 6) == 0) {
          printf("%s ", pItem->d_name);
        };
      };
      closedir(pDir);
    };

    printf("\n> Actieve opties Mac OS:\n");
    pDir = opendir("/dev/");
    if (pDir != NULL) {
      while ((pItem = readdir(pDir)) != NULL) {
        if (strncmp(pItem->d_name, "tty.usb", 7) == 0) {
          printf("%s ", pItem->d_name);
        };
      };
      closedir(pDir);
    };

    printf("\n\n");
    return 1;
  };

  // Initialiseer de device-naam
  strcpy(aDevicename, "/dev/");
  strncpy(aDevicename + 5, aArgv[1], 100 - 5);
  aDevicename[100 - 1] = 0;

  // Initialiseer de connectie (B38400 or B57600 or B115200)
  printf("> Verbinden: ");
  if (!oSerial.init(aDevicename, B115200)) {
    printf(" fout: serial device kon niet geopend worden!\n");
    return 1;
  }
  else {
    printf(" OK\n");
  };

  // Wachten 2sec om de Xmega op te laten starten
  // (Alternatief: wacht op een 'klaar' van de Xmega en vervolg dan.)
  printf("Wacht 2sec: "); fflush(stdout);
  sleep(2);
  printf("OK\n"); fflush(stdout);

  // Stuur de test bytes
  uint8_t cw = 0, cr = 0;
  uint8_t nRet;
  do {
    printf("> put "); fflush(stdout);
    oSerial.put(cw);
    printf("%d =?= ", cw); fflush(stdout);

    printf("get "); fflush(stdout);
    do {
      nRet = oSerial.get(&cr);
      if (nRet > SERIAL_WARNING) {
        // Fatale fout
        printf("\nFout %d: device niet meer verbonden!\n", nRet);
        return 2;
      };

    } while (nRet > 0);
    printf("%d ", cr); fflush(stdout);

    printf("\n"); fflush(stdout);
    cw++;
  }
  while (cw != 0);

  // Afsluiten
  printf("\nAfgesloten :-) \n");

  return 0;
}


