#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino_FreeRTOS.h>

//definizione pin
#define SS_PIN 53
#define RST_PIN 49

//definizione task
void TaskReadRFID( void *pvParameters );
void TaskDisplay( void *pvParameters );

LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);

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
}


void TaskDisplay( void *pvParameters ){
}
