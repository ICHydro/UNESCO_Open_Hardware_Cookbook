/**************************************
 * Arduino code for the UNESCO Open Hardware Cookbook
 * 
 * This is an example implements a simple script to operate 
 * the Adafruit Feather M0 based data logger with
 * Maxbotix Ultrasound distance sensor,
 * which is included in the Open Hardware Testkit
 *
 * The script reads a value from the distance sensor and
 * writes it to the SD card at a predefined interval
 * (see READ_INTERVAL definition below)
 *
 * NOTES:
 * - Set the board definition to "Adafruit Feather M0 (SAMD21"
 * - For instructions on how to install the board definnitions, see"
 *   https://learn.adafruit.com/adafruit-feather-m0-adalogger/setup
 * - Also don't forget to set the right upload port
 *   (if more than one shows up then it is typically the one that has
 *   "Adafruit Feather M0" in its name)
 * 
 * This script is based on the adalogger.ino example from Adafruit:
 * https://gist.github.com/ladyada/13efab4022b7358033c7
 * with adaptations by Riverlabs Ltd,
 * 
 */

/*********** MAIN LOGGER SETTINGS *********/

#define READ_INTERVAL 5                           // Interval for sensor readings, in minutes
#define NREADINGS 9                               // number of readings taken per measurement (excluding 0 values)

/************* INCLUDES *******************/

// make sure that all the required libraries are installed!

#include <TimeLib.h>
#include <RTCZero.h>
#include <SPI.h>
#include <SD.h>

/******** VARIABLE DECLARATIONS ***********/

// Clock stuff
RTCZero rtc;
uint32_t CompileTime;
uint32_t CurrentTime;

// variables needed in interrupt should be of type volatile.

volatile uint16_t interruptCount = 0;
volatile bool interruptFlag = false;

// SD card stuff

char filename[15];


// Set the pins used
#define cardSelect 4

File logfile;

// This line is not needed if you have Adafruit SAMD board package 1.6.2+
//   #define Serial SerialUSB

void setup() {

  Serial.begin(115200);
  while(!Serial);
  Serial.println("Hello, I am a Riverlabs Utrasonic distance logger");

  // First check, and if necessary, set the time

  CompileTime = cvt_date(__DATE__, __TIME__);

  rtc.begin();
  CurrentTime = rtc.getEpoch();
  Serial.println("The clock time is ");
  PrintDateTime();
  Serial.println();
  if(CurrentTime < CompileTime) {
      Serial.println("This seems wrong. Setting the clock to");
      rtc.setEpoch(CompileTime);
      PrintDateTime();
  }

  // see if the card is present and can be initialized:
  if (!SD.begin(cardSelect)) {
    Serial.println("Card init. failed!");
    error(2);
  }




  logfile = SD.open(filename, FILE_WRITE);
  if( ! logfile ) {
    Serial.print("Couldnt create "); 
    Serial.println(filename);
    error(3);
  }
  Serial.print("Writing to "); 
  Serial.println(filename);

  pinMode(13, OUTPUT);
  pinMode(8, OUTPUT);
  Serial.println("Ready!");
}

uint8_t i=0;
void loop() {
  digitalWrite(8, HIGH);
  logfile.print("A0 = "); logfile.println(analogRead(0));
  Serial.print("A0 = "); Serial.println(analogRead(0));
  digitalWrite(8, LOW);
  
  delay(100);
}


// Various helper functions below


time_t cvt_date(char const *date, char const *time)
{
    char s_month[5];
    int year, day;
    int hour, minute, second;
    tmElements_t t;
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    sscanf(date, "%s %d %d", s_month, &day, &year);
    //sscanf(time, "%2hhd %*c %2hhd %*c %2hhd", &t.Hour, &t.Minute, &t.Second);
    sscanf(time, "%2d %*c %2d %*c %2d", &hour, &minute, &second);
    // Find where is s_month in month_names. Deduce month value.
    t.Month = (strstr(month_names, s_month) - month_names) / 3 + 1;
    t.Day = day;
    // year can be given as '2010' or '10'. It is converted to years since 1970
    if (year > 99) t.Year = year - 1970;
    else t.Year = year + 50;
    t.Hour = hour;
    t.Minute = minute;
    t.Second = second;
    return makeTime(t);
}

// blink out an error code
void error(uint8_t errno) {
  while(1) {
    uint8_t i;
    for (i=0; i<errno; i++) {
      digitalWrite(13, HIGH);
      delay(100);
      digitalWrite(13, LOW);
      delay(100);
    }
    for (i=errno; i<10; i++) {
      delay(200);
    }
  }
}

void PrintDateTime() {
    // Print date...
  Serial.print(rtc.getDay());
  Serial.print("/");
  Serial.print(rtc.getMonth());
  Serial.print("/");
  Serial.print(rtc.getYear());
  Serial.print("\t");

  // ...and time
  print2digits(rtc.getHours());
  Serial.print(":");
  print2digits(rtc.getMinutes());
  Serial.print(":");
  print2digits(rtc.getSeconds());
}

void print2digits(int number) {
  if (number < 10) {
    Serial.print("0");
  }
  Serial.print(number);
}

