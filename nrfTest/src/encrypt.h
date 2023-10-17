
#include <avr/io.h>

uint8_t *xorEncrypt (uint8_t *message, uint8_t length, uint8_t key);
uint8_t *xorMultiEncrypt(uint8_t *message, uint8_t length, uint8_t *key, uint8_t keyLength);
uint8_t *KeysEncrypt(uint8_t* message, uint8_t length, char* key, uint8_t keyLength, char * key2, uint8_t keyLength2);