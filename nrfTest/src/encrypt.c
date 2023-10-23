
#include "encrypt.h"

// xor single key encryption and decryption
uint8_t *xorEncrypt(uint8_t *message, uint8_t length, uint8_t key) {
    static uint8_t data[32];
    for (uint8_t i = 0; i < length; i++) {
        data[i] = (message[i] ^ key);
    }
    return data;
}

// xor key array encryoption and decryption
uint8_t *xorMultiEncrypt(uint8_t *message, uint8_t length, uint8_t *key, uint8_t keyLength) {
    static uint8_t data[32];
    for (uint8_t i = 0; i < length; i++) {
        int keyIndex = i % keyLength;
        data[i] = (message[i] ^ key[keyIndex]);
    }    
    return data;
}

// Encryption and Decryption using multiple key arrays with different lengths
uint8_t *keysEncrypt(uint8_t* message, uint8_t length, char* key1, uint8_t keyLength1, char * key2, uint8_t keyLength2) {
    static uint8_t data[32];
    for (uint8_t i = 0; i < length; i++) {
        if (i % 2 == 0) {  
            data[i] = (message[i] ^ key1[(i/2) % keyLength1]);
        }
        else {
            data[i] = (message[i] ^ key2[(i/2) % keyLength2]);
        }
    }
    
    return data;
}
