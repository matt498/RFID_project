#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <Button.h>

//PIN definitions
#define SS_PIN 53
#define RST_PIN 49
#define vibration_PIN 7
#define buzzer_PIN 11
#define closeLED 8
#define openLED 9
#define rangeLED 10

//Alarm definitions
#define tickVALUE 80  //Auto stop value in tick counts
#define toneHERTZ 500 //Frequency for the buzzer tone

//Buttons definitions
const int openPin = 2;
const int closePin = 3;

//Task definitions
void TaskReadRFID(void);
void TaskDisplay(void);
void TaskOpenButton(void);
void TaskCloseButton(void);
void TaskVibration(void);

//Display requirements
LiquidCrystal_I2C lcd(0x27, 16, 2);

//RFID reader requirements
MFRC522 mfrc522(SS_PIN, RST_PIN);

//Global variables:
SemaphoreHandle_t mutex;
bool cardAuth = false;  //false = card not within the range of the RFID reader. true = card is near the RFID reader.
byte bufferTag[4];      //buffer to store the readed CARD
int cardCount = 0;      //card in range
bool lcdPrint = true;   //true = it's ok to print NFC UID on display. false = it's not.
long tickStart = 0;     //relative counter for system tick
bool lcdImpact = false; //true = it's ok to print alarm message on display. false = it's not.

//Authorized MIFARE classic NFC card
byte arrayByteCard[4] = {
    0xA0,
    0x9F,
    0x8C,
    0x32};

/* Authorized cards
Davide: A0 9F 8C 32
Matteo: E9 BB BB B8
Mattia: 3A 59 C8 80
*/

void setup()
{
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init(); //init MFRC522 NFC reader

  Serial.println("RFID card reader");

  //Display setup:
  lcd.init();                    //initialize
  lcd.backlight();               //turn on backlight
  lcd.print("RFID card reader"); //display startup message

  //Button setup:
  pinMode(openPin, INPUT);
  pinMode(closePin, INPUT);

  //LED setup:
  pinMode(closeLED, OUTPUT);
  pinMode(openLED, OUTPUT);
  pinMode(rangeLED, OUTPUT);

  //Initial state of the car: CLOSED (red LED ON)
  digitalWrite(closeLED, HIGH);

  //Vibration sensor setup:
  pinMode(vibration_PIN, INPUT);

  //Buzzer setup
  pinMode(buzzer_PIN, OUTPUT);

  //Mutex setup:
  mutex = xSemaphoreCreateMutex();
  if (mutex != NULL)
  {
    Serial.println("Mutex successfully created");
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
      TaskVibration,
      "VibrationSensor", //Task for the vibration sensor
      128,               //stack size
      NULL,
      2, //PRIORITY
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
      TaskCloseButton,
      "CloseButton", //Task for the button that closes the car
      128,           //stack size
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
              (arrayByteCard[3] == bufferTag[3])) //Check if the card is the authorised one
          {
            cardCount++; //The card is inside the car

            if ((cardCount % 2) == 1)
            {
              cardAuth = true;              //Access granted
              digitalWrite(rangeLED, HIGH); //The rangeLED is ON

              noTone(buzzer_PIN); //Turn OFF the buzzer
              tickStart = 0;      //Reset to zero the relative counter

              Serial.println("OK: card in range");
            }
            if ((cardCount % 2) == 0)
            {
              cardAuth = false;            //Access revoked
              digitalWrite(rangeLED, LOW); //The rangeLED is OFF
              cardCount = 0;               //Reset the counter
              lcdPrint = true;             //Reset the flag for lcd printing
              lcd.clear();                 //Clear the display

              Serial.println("LOG: card out of range");
            }
          }

          Serial.println();

          mfrc522.PICC_HaltA(); //Instructs the reader in state ACTIVE to go to state HALT
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
    if (xSemaphoreTake(mutex, 10) == pdTRUE) //We need to check for mutual exclusion
    {
      if ((cardAuth == true) && (lcdPrint == true)) //If card is in range and there's need for printing UID
      {
        lcdPrint = false; //Set the flag for next lcd printing (UID)

        lcd.clear();            //Clear display
        lcd.setCursor(0, 0);    //Write on the first row
        lcd.print("Tag UID: "); //Display title
        lcd.setCursor(0, 1);    //Write on the first row

        for (byte i = 0; i < 4; i++) //Print formatted UID on external display
        {
          lcd.print((bufferTag[i] < 0x10) ? "0" : " ");
          lcd.print(bufferTag[i], HEX);
        }
      }

      if (lcdImpact == true) //If there's need for printing the impact alarm message
      {
        lcdImpact = false; //Set the flag for next lcd printing (alarm)

        lcd.clear();                //Clear display
        lcd.setCursor(0, 0);        //Write on the first row
        lcd.print("Impact alarm!"); //Print the alarm massage on external display
      }

      xSemaphoreGive(mutex); //Release mutex
    }
    vTaskDelay(1); //TODO: tweak param
  }
}

void TaskOpenButton(void)
{
  for (;;)
  {
    if (xSemaphoreTake(mutex, 10) == pdTRUE) //We need to check for mutual exclusion
    {
      if (cardAuth == true) //If card is in range
      {
        int buttonState = digitalRead(openPin); //Read the state of the button

        if (buttonState == HIGH) //If open button was pressed
        {
          digitalWrite(closeLED, LOW); //Turn off close LED
          digitalWrite(openLED, HIGH); //Turn on open LED

          Serial.println("OPEN button pressed");
        }
      }
      xSemaphoreGive(mutex); //Release mutex
    }
    vTaskDelay(1); //TODO: tweak param
  }
}

void TaskCloseButton(void)
{
  for (;;)
  {
    if (xSemaphoreTake(mutex, 10) == pdTRUE) //We need to check for mutual exclusion
    {
      if (cardAuth == true) //If card is in range
      {
        int buttonState = digitalRead(closePin); //Read the state of the button

        if (buttonState == HIGH) //If close button was pressed
        {
          digitalWrite(openLED, LOW);   //Turn off open LED
          digitalWrite(closeLED, HIGH); //Turn on close LED

          Serial.println("CLOSE button pressed");
        }
      }
      xSemaphoreGive(mutex); //Release mutex
    }
    vTaskDelay(1); //TODO: tweak param
  }
}

void TaskVibration(void)
{
  for (;;)
  {
    if (xSemaphoreTake(mutex, 10) == pdTRUE) //We need to check for mutual exclusion
    {
      if (cardAuth == false) //If card is not in range
      {
        int val = digitalRead(vibration_PIN); //Read the value from the sensor

        Serial.println(val);

        if (xTaskGetTickCount() - tickStart > tickVALUE && tickStart != 0) //After a certain number of ticks the alarm stops automatically
        {
          noTone(buzzer_PIN); //Stop the beeping
          tickStart = 0;      //Reset the relative tick counter

          Serial.println(xTaskGetTickCount());
        }

        if (val == 1 && tickStart == 0) //If there's a vibration detected
        {
          tone(buzzer_PIN, toneHERTZ);     //Start the beeping
          tickStart = xTaskGetTickCount(); //Set the relative tick counter to the current tick value

          lcdImpact = true; //Set the flag for printing on display

          Serial.println(tickStart);
        }
      }
      xSemaphoreGive(mutex); //Release mutex
    }
    vTaskDelay(1);
  }
}
