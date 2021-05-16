#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#define SS_PIN 53
#define RST_PIN 49

LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init(); //init MFRC522
  Serial.println("RFID reading UID");

  //Display
  lcd.init(); //inizializza
  lcd.backlight(); //accende retroilluminazione
  lcd.print("RFID reading UID");
  delay(5000); //---- solo come prova!
  lcd.clear(); //pulisce display
}

void loop() {
  // put your main code here, to run repeatedly:
  if( mfrc522.PICC_IsNewCardPresent() ){
    if( mfrc522.PICC_ReadCardSerial() ) {
      Serial.print("Tag UID: ");
      for(byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : " ");
        lcd.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : " ");
        
        Serial.print(mfrc522.uid.uidByte[i], HEX);
        lcd.print(mfrc522.uid.uidByte[i], HEX);
      }

      Serial.println();
      mfrc522.PICC_HaltA();
      delay(5000); //TODO: limita due letture consecutive. Serve riorganizzare lettura RFID -> display print
      lcd.clear();
    }
  }
}
