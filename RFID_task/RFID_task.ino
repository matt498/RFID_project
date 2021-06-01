#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Button.h>

//PIN definitions
#define SS_PIN 53
#define RST_PIN 49
#define closeLED 8
#define openLED 9
#define rangeLED 10

//Buttons definitions
const int openPin = 2;
const int closePin = 3;

//Task definitions
void TaskReadRFID( void *pvParameters );
void TaskDisplay( void *pvParameters );
void TaskOpenButton( void *pvParameters );
void TaskCloseButton( void *pvParameters );

//Display requirements
LiquidCrystal_I2C lcd(0x27, 16, 2);

//RFID reader requirements
MFRC522 mfrc522(SS_PIN, RST_PIN);

//Global variables
SemaphoreHandle_t mutex;
bool autorizzazioneCarta = false; //false = card not within the range of the RFID reader // true = card is near the RFID reader

//Authorized card 
byte arrayByteCard[4] = {
  0x3A,
  0x59,
  0xC8,
  0x80
  };

//Buffer to store the readed CARD
byte bufferTag[4];

int contatoreCarta = 0;

void setup(){
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init(); //init MFRC522
  
  Serial.println("RFID card reader");

  //Display setup:
  lcd.init(); //initialize
  lcd.backlight(); //turn on backlight
  lcd.print("RFID card reader");
  delay(3000); //TODO 
  lcd.clear(); //clear display

  //Button setup:
  pinMode(openPin, INPUT);
  pinMode(closePin, INPUT);

  //LED setup:
  pinMode(closeLED, OUTPUT);
  pinMode(openLED, OUTPUT);
  pinMode(rangeLED, OUTPUT);

  //Mutex setup:
  mutex = xSemaphoreCreateMutex();
  if (mutex != NULL) {
    Serial.println("Mutex created");
  }

  //Task creation:
  xTaskCreate(
  TaskReadRFID
  ,  "RFID"   //Task for managing RFID reader
  ,  128  //stack size
  ,  NULL
  ,  2  //PRIORITY, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
  ,  NULL );

  xTaskCreate(
  TaskDisplay
  ,  "DisplayLCD" //Task for the display
  ,  128  //stack size
  ,  NULL
  ,  1  //PRIORITY
  ,  NULL );

    xTaskCreate(
  TaskOpenButton
  ,  "OpenButton" //Task for the button that opens the car
  ,  128  //stack size
  ,  NULL
  ,  1  //PRIORITY
  ,  NULL );

  xTaskCreate(
  TaskCloseButton
  ,  "CloseButton" //Task for the button that closes the car
  ,  128  //stack size
  ,  NULL
  ,  1  //PRIORITY
  ,  NULL );

}

//Not required since we're using FreeRTOS
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
