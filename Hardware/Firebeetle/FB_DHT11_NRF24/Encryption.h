#ifndef Encryption_h
#define Encription_h

#include "Arduino.h"
#include "AESLib.h"

#define MAX_KEY_LENGTH 16 // Enter the largest value for the key length

class Encryption{
  public:
    // AES Encryption Key
    byte aes_key[MAX_KEY_LENGTH] = { 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30 };

    Encryption(byte *Key, uint8_t KeyLength){
      byte itemsLength;
      if (KeyLength > MAX_KEY_LENGTH)
        itemsLength = MAX_KEY_LENGTH;
      else
          itemsLength = KeyLength;
      byte i;
      for (i = 0; i < itemsLength; i++){
          aes_key[i] = Key[i];
      }
      aes_init();
    }
    
    // Generate IV (once)
    void aes_init() {
      Serial.println("gen_iv()");
      aesLib.gen_iv(aes_iv);
      // workaround for incorrect B64 functionality on first run...
      Serial.println("encrypt()");
      String plaintext = "init";
      Serial.println(encrypt(strdup(plaintext.c_str()), aes_iv));
    }

    String encrypt(char * msg) {
      return encrypt(msg,aes_iv); 
    }
    
    String encrypt(char * msg, byte iv[]) {
      int msgLen = strlen(msg);
      Serial.print("msglen = "); Serial.println(msgLen);
      char encrypted[4 * msgLen]; // AHA! needs to be large, 2x is not enough
      aesLib.encrypt64(msg, encrypted, aes_key, iv);
      Serial.print("encrypted = "); Serial.println(encrypted);
      return String(encrypted);
    }
    
    String decrypt(char * msg, byte iv[]) {
      unsigned long ms = micros();
      int msgLen = strlen(msg);
      char decrypted[msgLen]; // half may be enough
      aesLib.decrypt64(msg, decrypted, aes_key, iv);
      return String(decrypted);
    }
  private:
    // General initialization vector (you must use your own IV's in production for full security!!!)
    byte aes_iv[N_BLOCK] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    
    AESLib aesLib;
};
#endif
