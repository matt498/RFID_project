#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>

//definizione pin
#define SS_PIN 53
#define RST_PIN 49

//definizione task
void TaskReadRFID( void *pvParameters );
void TaskDisplay( void *pvParameters );

LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);

SemaphoreHandle_t mutex;
bool letturaRFID = false;

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


}

void loop() {
}

void TaskReadRFID( void *pvParameters ){
  for(;;){
    if (xSemaphoreTake(mutex, 10) == pdTRUE){
      if( mfrc522.PICC_IsNewCardPresent() ){
        if( mfrc522.PICC_ReadCardSerial() ) {
          letturaRFID = true;
          Serial.print("Tag UID: ");
          for(byte i = 0; i < mfrc522.uid.size; i++) {
            Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : " ");
            
            Serial.print(mfrc522.uid.uidByte[i], HEX);
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
      if(letturaRFID == true){
        Serial.println("visualizzazione su schermo OK");
        letturaRFID = false;
      }
    xSemaphoreGive(mutex);
    }
    vTaskDelay(1);
  }
}
