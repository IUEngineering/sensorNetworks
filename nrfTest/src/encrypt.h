#ifndef _ENCRYPT_H_
#define _ENCRYPT_H_

#include <avr/io.h>

uint8_t *xorEncrypt (uint8_t *message, uint8_t length, uint8_t key);
uint8_t *xorMultiEncrypt(uint8_t *message, uint8_t length, uint8_t *key, uint8_t keyLength);
uint8_t *keysEncrypt(uint8_t* message, uint8_t length, char* key, uint8_t keyLength, char * key2, uint8_t keyLength2);

#endif // _ENCRYPT_H_
