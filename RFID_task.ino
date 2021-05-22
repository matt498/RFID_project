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
  
}