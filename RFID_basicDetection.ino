#include <SPI.h>
#include <MFRC522.h>
#define SS_PIN 53
#define RST_PIN 49

MFRC522 mfrc522(SS_PIN, RST_PIN);
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init(); //init MFRC522
  Serial.println("RFID reading UID");
}

void loop() {
// verifica presenza carta
  if( mfrc522.PICC_IsNewCardPresent() ){
//lettura del seriale
    if( mfrc522.PICC_ReadCardSerial() ) {
      Serial.print("Tag UID: ");
      for(byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
      }

      Serial.println();
      mfrc522.PICC_HaltA();
    }
  }
}