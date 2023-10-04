
#include "encrypt.h"

uint8_t *xorEncrypt(uint8_t *message, uint8_t length, uint8_t key) {
    static uint8_t data[32];
    for (uint8_t i = 0; i < length; i++) {
        data[i] = (message[i] ^ key);
    }
    return data;
}
