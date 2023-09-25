/**************************************
 * Arduino code for the UNESCO Open Hardware Cookbook
 * 
 * This is an example sketch to operate 
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
 * - For instructions on how to install the board definitions, see"
 *   https://learn.adafruit.com/adafruit-feather-m0-adalogger/setup
 * - Also don't forget to set the right upload port
 *   (if more than one shows up then it is typically the one that has
 *   "Adafruit Feather M0" in its name)
 * 
 * This script is based on the adalogger.ino example from Adafruit:
 * https://gist.github.com/ladyada/13efab4022b7358033c7
 * with adaptations by Riverlabs Ltd and Imperial College London
 *
 * All modifications are (c) Wouter Buytaert
 * 
 */

/*********** MAIN LOGGER SETTINGS *********/

#define READ_INTERVAL 5                           // Interval for sensor readings, in minutes
#define NREADINGS 9                               // number of readings taken per measurement (excluding 0 values)
#define cardSelect 4
#define DebugSerial Serial                       // This makes it easy to switch between Serial and Serial1

/************* INCLUDES *******************/

// make sure that all the required libraries are installed. You will need:
// - Time by Michael Margolis
// - Wifi101 by Arduino
// - RTCZero by Arduino
// These can be installed through the Library Manager of the Arduino IDE

#include <TimeLib.h>
#include <RTCZero.h>
#include <SPI.h>
#include <SD.h>

/******** VARIABLE DECLARATIONS ***********/

// Clock variables
RTCZero rtc;
uint32_t CompileTime;
uint32_t CurrentTime;

// variables used in the clock interrupt function should be of type volatile.
volatile bool interruptFlag = false;

// SD card variables 
char filename[] = "00000000.CSV";
int16_t distance = -9999;
File myfile;

// other variables
int16_t measuredvbat;

void setup() {

  DebugSerial.begin(9600);
  while(!DebugSerial);
  DebugSerial.println("Hello, I am a Riverlabs Ultrasonic distance logger");

  // pin 13 is used for the LED.
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  // Pin 5 is used as power supply for the ultrasound sensor
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);

  // First check, and if necessary, set the time

  CompileTime = cvt_date(__DATE__, __TIME__);
  rtc.begin();
  CurrentTime = rtc.getEpoch();
  DebugSerial.print("The clock time is ");
  PrintDateTime();
  DebugSerial.println();
  if(CurrentTime < CompileTime) {
    DebugSerial.println("This seems wrong. Setting the clock to");
    rtc.setEpoch(CompileTime);
    PrintDateTime();
  }

  // Set the alarm. We'll set it to go off every minute, but then check
  // whether it matches the measurement interval or not.
  // We should also set the interrupt function!

  rtc.setAlarmSeconds(0);
  rtc.enableAlarm(rtc.MATCH_SS);
  rtc.attachInterrupt(InterruptServiceRoutine);

  // Check if the SD card is present and can be initialized

  if (!SD.begin(cardSelect)) {
    DebugSerial.println("Card initialisation failed!");
    error(2);
  }

  getFilename(rtc);
  myday = rtc.getDay();
  myfile = SD.open(filename, FILE_WRITE);
  if( !myfile ) {
    DebugSerial.print("Could not create a file on the SD card: "); 
    DebugSerial.println(filename);
    error(3);
  }
  DebugSerial.println("SD card found and ready to write");
}

void loop() {

  if(!interruptFlag) {
    
    DebugSerial.println(F("Sleeping"));
    DebugSerial.flush();

    rtc.standbyMode();

    /* if interrupt wakes us up, then we take action: */
    DebugSerial.println(F("Waking up!"));
  }
      
  interruptFlag = false;                              // reset the flag

  if(rtc.getMinutes() % READ_INTERVAL == 0) {         // only take measurement if interval threshold is exceeded         
  
    measuredvbat = analogRead(A7) * 2 * 3.3 / 1.024;
    distance = readMaxBotix(5, 9, 1);

    DebugSerial.print("Battery voltage = ");
    DebugSerial.println(measuredvbat);
    DebugSerial.print("Distance = ");
    DebugSerial.println(distance);

    // Store the data on the SD card. Change the file every day.
    if(rtc.getDay() != myday) {
      getFilename(rtc);
      myday = rtc.getDay();
    } 
    myfile = SD.open(filename, FILE_WRITE);
    myfile.print(rtc.getYear());
    myfile.print("/");
    myfile.print(rtc.getMonth());
    myfile.print("/");
    myfile.print(rtc.getDay());
    myfile.print(" ");
    myfile.print(rtc.getHours());
    myfile.print(":");
    myfile.print(rtc.getMinutes());
    myfile.print(":");
    myfile.print(rtc.getSeconds());
    myfile.print(", ");
    myfile.print(measuredvbat);
    myfile.print(", ");
    myfile.println(distance);
    myfile.close();
    DebugSerial.println("Data written to SD card!");
  }
}


/*************** Additional functions *****************/

// interrupt function. Should be as short as possible

void InterruptServiceRoutine() {
    interruptFlag = true;
}



int16_t readMaxBotix(uint8_t powerPin, uint8_t nreadings, bool debug) {

    int16_t readings[nreadings];

    for (int i = 0; i < nreadings; i++){
      readings[i] = -1;
    }

    //Serial1.end();                              // in the case that we also use the Serial1 port for debugging
    Serial1.begin(9600);

    digitalWrite(13, HIGH);                     // LED
    digitalWrite(powerPin, HIGH);
    delay(160);   // wait 160ms for startup and boot message to pass
    
    uint32_t readstart = millis();
    uint8_t n = 0;
         
    while ((millis() - readstart) <= 1800) {
        readings[n] = EZread(Serial1);
        if(readings[n] > -2) {                   // Returning -2 indicates an error
            n++;
            if(n >= nreadings) {n = 0; }             //avoid overflow
        }
    }

    //Serial1.end();

    digitalWrite(powerPin, LOW);
    digitalWrite(13, LOW);
            
    int16_t distance = median(readings, nreadings);

    return distance;
}

/* function adapted from the TTL_ArduinoCode_Parsing example on the Arduino forum */

int EZread(Stream &stream) {
  
    int result;
    char inData[6];                             //char array to read data into
    int index = 0;
    unsigned long timer;
    boolean stringComplete = false;

    stream.flush();                           // Clear cache ready for next reading
    timer = millis();                           // use timer to time out after 1 sec.

    while ((stringComplete == false) && ((millis() - timer) < 1000)) {
        if (stream.available()) {    
            char rByte = stream.read();       //read serial input for "R" to mark start of data
            if(rByte == 'R') {
                while (index < 5) {             //read next 4 characters for range from sensor
                    if (stream.available()) {
                        inData[index] = stream.read();               
                        index++;                // Increment where to write next
                    }
                }
                inData[index] = 0x00;           //add a padding byte at end for atoi() function
            }
            rByte = 0;                          //reset the rByte ready for next reading
            index = 0;                          // Reset index ready for next reading
            stringComplete = true;              // Set completion of read to true
            result = atoi(inData);              // Changes string data into an integer for use
        }
    }
    if(stringComplete == false) result = -2;
    return result;
}


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
  print2digits(rtc.getDay());
  DebugSerial.print("/");
  print2digits(rtc.getMonth());
  DebugSerial.print("/");
  DebugSerial.print(rtc.getYear());
  DebugSerial.print(" ");
  // ...and time
  print2digits(rtc.getHours());
  DebugSerial.print(":");
  print2digits(rtc.getMinutes());
  DebugSerial.print(":");
  print2digits(rtc.getSeconds());
}

void print2digits(int number) {
  if (number < 10) {
    DebugSerial.print("0");
  }
  DebugSerial.print(number);
}

void bubbleSort(int16_t A[],int len)
{
  unsigned long newn;
  unsigned long n=len;
  int16_t temp=0;
  do {
    newn=1;
    for(int p=1;p<len;p++){
      if(A[p-1]>A[p]){
        temp=A[p];           //swap places in array
        A[p]=A[p-1];
        A[p-1]=temp;
        newn=p;
      } //end if
    } //end for
    n=newn;
  } while(n>1);
}

int16_t median(int16_t samples[],int m) //calculate the median
{
  //First bubble sort the values: https://en.wikipedia.org/wiki/Bubble_sort
  int16_t sorted[m];   //Define and initialize sorted array.
  int16_t temp=0.0;      //Temporary float for swapping elements

  for(int i=0;i<m;i++){
    sorted[i]=samples[i];
  }
  bubbleSort(sorted,m);  // Sort the values

  if (bitRead(m,0)==1) {  //If the last bit of a number is 1, it's odd. This is equivalent to "TRUE". Also use if m%2!=0.
    return sorted[m/2]; //If the number of data points is odd, return middle number.
  } else {    
    return (sorted[(m/2)-1]+sorted[m/2])/2; //If the number of data points is even, return avg of the middle two numbers.
  }
}

