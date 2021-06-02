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
void TaskReadRFID(void);
void TaskDisplay(void);
void TaskOpenButton(void);
void TaskCloseButton(void);

//Display requirements
LiquidCrystal_I2C lcd(0x27, 16, 2);

//RFID reader requirements
MFRC522 mfrc522(SS_PIN, RST_PIN);

//Global variables:
SemaphoreHandle_t mutex;
bool cardAuth = false; //false = card not within the range of the RFID reader. true = card is near the RFID reader.
byte bufferTag[4];     //buffer to store the readed CARD
int cardCount = 0;

//Authorized card
byte arrayByteCard[4] = {
    0x3A,
    0x59,
    0xC8,
    0x80};

/* Authorized cards
Davide: A0 9F 8C 32
Matteo: E9 BB BB B8
Mattia: 3A 59 C8 80
*/

void setup()
{
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init(); //init MFRC522

  Serial.println("RFID card reader");

  //Display setup:
  lcd.init();                    //initialize
  lcd.backlight();               //turn on backlight
  lcd.print("RFID card reader"); //display startup message
  delay(3000);                   //TODO
  lcd.clear();                   //clear display

  //Button setup:
  pinMode(openPin, INPUT);
  pinMode(closePin, INPUT);

  //LED setup:
  pinMode(closeLED, OUTPUT);
  pinMode(openLED, OUTPUT);
  pinMode(rangeLED, OUTPUT);

  //Mutex setup:
  mutex = xSemaphoreCreateMutex();
  if (mutex != NULL)
  {
    Serial.println("Mutex created");
  }

  //Task creation:
  xTaskCreate(
      TaskReadRFID,
      "RFID", //Task for managing RFID reader
      128,    //stack size
      NULL,
      2, //PRIORITY, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
      NULL);

  xTaskCreate(
      TaskDisplay,
      "DisplayLCD", //Task for the display
      128,          //stack size
      NULL,
      1, //PRIORITY
      NULL);

  xTaskCreate(
      TaskOpenButton,
      "OpenButton", //Task for the button that opens the car
      128,          //stack size
      NULL,
      1, //PRIORITY
      NULL);

  xTaskCreate(
      TaskCloseButton, "CloseButton", //Task for the button that closes the car
      128,                            //stack size
      NULL,
      1, //PRIORITY
      NULL);
}

//Not required since we're using FreeRTOS
void loop()
{
}

//Task for managing RFID reader:
void TaskReadRFID(void)
{
  for (;;)
  {
    if (xSemaphoreTake(mutex, 10) == pdTRUE) //We need to check for mutual exclusion
    {
      if (mfrc522.PICC_IsNewCardPresent() == true) //Vendor-specific function that tells if there's a card within the range
      {
        if (mfrc522.PICC_ReadCardSerial() == true) //Vendor-specific function that tells if the reading part was successful
        {
          Serial.print("Tag UID: ");
          for (byte i = 0; i < mfrc522.uid.size; i++)
          {
            bufferTag[i] = mfrc522.uid.uidByte[i]; //Save the readed UID on buffer

            Serial.print((mfrc522.uid.uidByte[i] < 0x10) ? "0" : " "); //Print formatted UID on serial
            Serial.print(mfrc522.uid.uidByte[i], HEX);
          }

          Serial.println();

          if ((arrayByteCard[0] == bufferTag[0]) &&
              (arrayByteCard[1] == bufferTag[1]) &&
              (arrayByteCard[2] == bufferTag[2]) &&
              (arrayByteCard[3] == bufferTag[3])) //Check if the card is an authorised one
          {
            cardCount++; //A card is inside the car

            if ((cardCount % 2) == 1)
            {
              cardAuth = true;              //Access granted
              digitalWrite(rangeLED, HIGH); //The rangeLED is ON

              Serial.println("OK: card in range");
            }
            if ((cardCount % 2) == 0)
            {
              cardAuth = false;            //Access revoked
              digitalWrite(rangeLED, LOW); //The rangeLED is OFF
              cardCount = 0;               //Reset the counter

              Serial.println("ERR: card out of range");
            }
          }

          Serial.println();

          mfrc522.PICC_HaltA(); //Instructs the reader in state ACTIVE to go to state HALT
          delay(1000); //TODO: tweak param not to limit two readings in a row -> display print
        }
      }
      xSemaphoreGive(mutex); //Release mutex
    }
    vTaskDelay(1); //TODO: tweak param
  }
}

void TaskDisplay(void)
{
  for (;;)
  {
    if (xSemaphoreTake(mutex, 10) == pdTRUE)
    {
      if (cardAuth == true)
      {
        lcd.print("Tag UID: ");
        for (byte i = 0; i < 4; i++) //Print formatted UID on external display
        {
          lcd.print((bufferTag[i] < 0x10) ? "0" : " ");
          lcd.print(bufferTag[i], HEX);
        }
      }
      xSemaphoreGive(mutex);
    }
    vTaskDelay(1); //TODO: tweak param
  }
}

void TaskOpenButton(void)
{
  for (;;)
  {
    if (xSemaphoreTake(mutex, 10) == pdTRUE)
    {
      if (cardAuth == true) //If card is in range
      {
        int buttonState = digitalRead(openPin);

        if (buttonState == HIGH) //If open button was pressed
        {
          digitalWrite(closeLED, LOW); //Turn off close LED
          digitalWrite(openLED, HIGH); //Turn on open LED

          Serial.println("OPEN button pressed");
        }
      }
      xSemaphoreGive(mutex);
    }
    vTaskDelay(1);
  }
}

void TaskCloseButton(void)
{
  for (;;)
  {
    if (xSemaphoreTake(mutex, 10) == pdTRUE)
    {
      if (cardAuth == true) //If card is in range
      {
        int buttonState = digitalRead(closePin);

        if (buttonState == HIGH) //If close button was pressed
        {
          digitalWrite(openLED, LOW);   //Turn off open LED
          digitalWrite(closeLED, HIGH); //Turn on close LED

          Serial.println("CLOSE button pressed");
        }
      }
      xSemaphoreGive(mutex);
    }
    vTaskDelay(1);
  }
}
