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
 * - Set the board definition to "Adafruit Feather M0 (SAMD21)"
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
#define CARDSELECT 4
#define DEBUGSERIAL Serial                       // This makes it easy to switch between Serial and Serial1

/************* INCLUDES *******************/

// make sure that all the required libraries are installed. You will need:
// - Time by Michael Margolis
// - SD by Arduino, SparkFun
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
uint8_t myday = 40;
char datestring[20];

// variables used in the clock interrupt function should be of type volatile.
volatile bool interruptFlag = false;

// SD card variables 
char filename[] = "00000000.CSV";
int16_t distance = -9999;
File myfile;

// other variables
int16_t measuredvbat;

void setup() {

  DEBUGSERIAL.begin(9600);
  while(!DEBUGSERIAL);
  Serial1.begin(9600);
  DEBUGSERIAL.println("Hello, I am a Riverlabs Ultrasonic distance logger");

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
  DEBUGSERIAL.print("The clock time is ");
  formatDateTime();
  DEBUGSERIAL.print(datestring);
  DEBUGSERIAL.println();
  if(CurrentTime < CompileTime) {
    DEBUGSERIAL.println("This seems wrong. Setting the clock to");
    rtc.setEpoch(CompileTime);
    formatDateTime();
    DEBUGSERIAL.print(datestring);
    DEBUGSERIAL.println();
  }

  // Set the alarm. We'll set it to go off every minute, but then check
  // whether it matches the measurement interval or not.
  // We should also set the interrupt function!

  rtc.setAlarmSeconds(0);
  rtc.enableAlarm(rtc.MATCH_SS);
  rtc.attachInterrupt(InterruptServiceRoutine);

  // Check if the SD card is present and can be initialized

  if (!SD.begin(CARDSELECT)) {
    DEBUGSERIAL.println("Card initialisation failed!");
    error(2);
  } else {
    getFilename(rtc);
    myday = rtc.getDay();
    myfile = SD.open(filename, FILE_WRITE);
    if( !myfile ) {
      DEBUGSERIAL.print("Could not create a file on the SD card: "); 
      DEBUGSERIAL.println(filename);
      error(3);
    } else {
      DEBUGSERIAL.println("SD card found and ready to write");
      myfile.close();
      digitalWrite(13, HIGH);
      delay(1000);
      digitalWrite(13, LOW);
    }
  }
  delay(5000); // needed to let the Arduino IDE verify that uploading has finished properly
}

void loop() {

  if(!interruptFlag) {
    
    DEBUGSERIAL.println(F("Sleeping"));
    DEBUGSERIAL.flush();

    while(!interruptFlag);
    //rtc.standbyMode();

    /* if interrupt wakes us up, then we take action: */
    DEBUGSERIAL.println(F("Waking up!"));
  }
      
  interruptFlag = false;                              // reset the flag

  if(rtc.getMinutes() % READ_INTERVAL == 0) {         // only take measurement if interval threshold is exceeded         

    digitalWrite(13, HIGH);
    measuredvbat = analogRead(A7) * 2 * 3.3 / 1.024;
    distance = readMaxBotix(5, 9, 1);
    digitalWrite(13, LOW);

    DEBUGSERIAL.print("Battery voltage = ");
    DEBUGSERIAL.println(measuredvbat);
    DEBUGSERIAL.print("Distance = ");
    DEBUGSERIAL.println(distance);

    // Store the data on the SD card. Change the file every day.
    if(rtc.getDay() != myday) {
      getFilename(rtc);
      myday = rtc.getDay();
    }
    if (!SD.begin(CARDSELECT)) {
      DEBUGSERIAL.println("Card initialisation failed!");
      error(2);
    } else {
      myfile = SD.open(filename, FILE_WRITE);
      if( !myfile ) {
        DEBUGSERIAL.print("Could not open a file on the SD card: "); 
        DEBUGSERIAL.println(filename);
        error(3);
      } else {
        formatDateTime();
        myfile.print(datestring);
        myfile.print(", ");
        myfile.print(measuredvbat);
        myfile.print(", ");
        myfile.println(distance);
        myfile.close();
        DEBUGSERIAL.println("Data written to SD card!");
        delay(100);         // give the SD card time to write and close the file
      }
    }
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

// Create file name 

void getFilename(RTCZero rtc) {
  uint8_t year = rtc.getYear();
  uint8_t month = rtc.getMonth();
  uint8_t day = rtc.getDay();
  filename[0] = '2';
  filename[1] = '0';
  filename[2] = year/10 + '0';
  filename[3] = year%10 + '0';
  filename[4] = month/10 + '0';
  filename[5] = month%10 + '0';
  filename[6] = day/10 + '0';
  filename[7] = day%10 + '0';
  filename[8] = '.';
  filename[9] = 'C';
  filename[10] = 'S';
  filename[11] = 'V';
  return;
}

#define countof(a) (sizeof(a) / sizeof(a[0]))

void formatDateTime() {
  snprintf_P(datestring, 
      countof(datestring),
      PSTR("%04u/%02u/%02u %02u:%02u:%02u"),
      rtc.getYear() + 2000,
      rtc.getMonth(),
      rtc.getDay(),
      rtc.getHours(),
      rtc.getMinutes(),
      rtc.getSeconds() );
}
