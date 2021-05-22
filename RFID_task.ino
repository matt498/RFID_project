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
