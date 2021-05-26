#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Button.h>

//definizione pin
#define SS_PIN 53
#define RST_PIN 49
#define closeLED 8
#define openLED 9
#define rangeLED 10

//definizione task
void TaskReadRFID( void *pvParameters );
void TaskDisplay( void *pvParameters );
void TaskOpenButton( void *pvParameters );
void TaskCloseButton( void *pvParameters );

LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);

SemaphoreHandle_t mutex;
bool autorizzazioneCarta = false;
//********
//false = carta NON nella portata del lettore RFID
//true = carta nella portata del lettore RFID
//bool cardState = false

//Button def
const int openPin = 2;
const int closePin = 3;

//codice carta registrata
byte arrayByteCard[4] = {
  0xE9,
  0xBB,
  0xBB,
  0xB8
  };

//buffer TAG CARD
byte bufferTag[4];

int contatoreCarta = 0;

void setup(){
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

  //Button
  pinMode(openPin, INPUT);
  pinMode(closePin, INPUT);

  //LED
  pinMode(closeLED, OUTPUT);
  pinMode(openLED, OUTPUT);
  pinMode(rangeLED, OUTPUT);

  mutex = xSemaphoreCreateMutex();
  if (mutex != NULL) {
    Serial.println("Mutex created");
  }

  xTaskCreate(
  TaskReadRFID
  ,  "RFID"   // A name just for humans
  ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
  ,  NULL
  ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
  ,  NULL );

  xTaskCreate(
  TaskDisplay
  ,  "DisplayLCD"
  ,  128  // Stack size
  ,  NULL
  ,  1  // Priority
  ,  NULL );

    xTaskCreate(
  TaskOpenButton
  ,  "OpenButton"
  ,  128  // Stack size
  ,  NULL
  ,  1  // Priority
  ,  NULL );

  xTaskCreate(
  TaskCloseButton
  ,  "CloseButton"
  ,  128  // Stack size
  ,  NULL
  ,  1  // Priority
  ,  NULL );

}

void loop() {
}

void TaskReadRFID( void *pvParameters ){
  for(;;){
    if (xSemaphoreTake(mutex, 10) == pdTRUE){
      if( mfrc522.PICC_IsNewCardPresent() ){
        if( mfrc522.PICC_ReadCardSerial() ) {
          Serial.print("Tag UID: ");
          for(byte i = 0; i < mfrc522.uid.size; i++) {
            Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : " ");
            Serial.print(mfrc522.uid.uidByte[i], HEX);
            bufferTag[i] = mfrc522.uid.uidByte[i];
          }
          Serial.println();
          if(arrayByteCard[0] == bufferTag[0] &&
             arrayByteCard[1] == bufferTag[1] &&
             arrayByteCard[2] == bufferTag[2] &&
             arrayByteCard[3] == bufferTag[3] ){
              contatoreCarta++;
              if(contatoreCarta % 2 == 1){
                autorizzazioneCarta = true;
                Serial.println("CARTA IN PORTATA");
                digitalWrite(rangeLED, HIGH);
              }
              if(contatoreCarta % 2 == 0){
                autorizzazioneCarta = false;
                Serial.println("CARTA FUORI PORTATA");
                contatoreCarta = 0;
                digitalWrite(rangeLED, LOW);
              }
          }
           
          Serial.println();
          mfrc522.PICC_HaltA();
          delay(1500); //TODO: limita due letture consecutive. Serve riorganizzare lettura RFID -> display print
        }
      }
    xSemaphoreGive(mutex);
    }
    vTaskDelay(1);
  }
}


void TaskDisplay( void *pvParameters ){
  for(;;){
    if (xSemaphoreTake(mutex, 10) == pdTRUE){
      if(autorizzazioneCarta == true){
        for(byte i = 0; i < 4; i++) {
            lcd.print(bufferTag[i] < 0x10 ? "0" : " ");
            lcd.print(bufferTag[i], HEX);
        }
      }
    xSemaphoreGive(mutex);
    }
    vTaskDelay(1);
  }
}

void TaskOpenButton( void *pvParameters ){
  for(;;){
    if (xSemaphoreTake(mutex, 10) == pdTRUE){
      if(autorizzazioneCarta == true){
        int buttonState = digitalRead(openPin);

        if (buttonState == HIGH) {
        // turn LED on:
          Serial.println("Pressione di APERTURA");
          digitalWrite(openLED, HIGH);
          digitalWrite(closeLED, LOW);
        }
      }
      xSemaphoreGive(mutex);
    }
    vTaskDelay(1);
  }
}

void TaskCloseButton( void *pvParameters ){
  for(;;){
    if (xSemaphoreTake(mutex, 10) == pdTRUE){
      if(autorizzazioneCarta == true){
        int buttonState = digitalRead(closePin);

        if (buttonState == HIGH) {
        // turn LED on:
          Serial.println("Pressione di CHIUSURA");
          digitalWrite(openLED, LOW);
          digitalWrite(closeLED, HIGH);   
        }
      }
      xSemaphoreGive(mutex);
    }
    vTaskDelay(1);
  }
}
