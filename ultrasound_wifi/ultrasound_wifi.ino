/**************************************
 * Arduino code for the UNESCO Open Hardware Cookbook
 * 
 * This is an example sketch to operate 
 * the Adafruit Feather M0 based data logger with
 * Maxbotix Ultrasound distance sensor,
 * which is included in the Open Hardware Testkit
 * 
 * For this test you should use the Adafruit Feather M0 Wifi (ATWINC1500) board
 *
 * The script reads a value from the distance sensor and
 * sends it via wifi 
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

#define READ_INTERVAL 1                        // Interval for sensor readings, in minutes
#define NREADINGS 9                            // number of readings taken per measurement (excluding 0 values)
#define DEBUGSERIAL Serial                     // This makes it easy to switch between Serial and Serial1

/************* INCLUDES *******************/

// make sure that all the required libraries are installed. You will need:
// - Time by Michael Margolis
// - ArduinoMqttClient by Arduino
// - Wifi101 by Arduino
// - RTCZero by Arduino
// - NTPClient by Fabrice Weinberg
// These can be installed through the Library Manager of the Arduino IDE

#include <TimeLib.h>
#include <RTCZero.h>
#include <SPI.h>
#include <ArduinoMqttClient.h>
#include <WiFi101.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "arduino_secrets.h" 

/******** VARIABLE DECLARATIONS ***********/

// network

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int keyIndex = 0;
int status = WL_IDLE_STATUS;
WiFiClient client;
MqttClient mqttClient(client);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

const char broker[] = BROKER;
int        port     = 1883;
const char topic[]  = "v1/devices/me/telemetry";
const char token[]  = TOKEN;

int count = 0;
uint8_t i;
char myMessage[60];
uint16_t messageLength = 0;

// Clock variables
RTCZero rtc;
uint32_t CompileTime;
uint32_t CurrentTime;

// variables used in the clock interrupt function should be of type volatile.
volatile bool interruptFlag = false;

// other variables
int16_t measuredvbat;
int16_t distance = -9999;

void setup() {

  WiFi.setPins(8,7,4,2);
  DEBUGSERIAL.begin(9600);
  while(!DEBUGSERIAL);
  DEBUGSERIAL.println("Hello, I am a Riverlabs Ultrasonic distance logger");

  // pin 13 is used for the LED.
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  // Pin 5 is used as power supply for the ultrasound sensor
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);

  // Set the alarm. We'll set it to go off every minute, but then check
  // whether it matches the measurement interval or not.
  // We should also set the interrupt function!

  rtc.begin();
  rtc.setAlarmSeconds(0);
  rtc.enableAlarm(rtc.MATCH_SS);
  rtc.attachInterrupt(InterruptServiceRoutine);

  // check for the presence of the wifi module:

  if (WiFi.status() == WL_NO_SHIELD) {
    DEBUGSERIAL.println("Cannot find the WiFi module. Are you using the right board?");
    // don't continue:
    while (true);
  } else {
    DEBUGSERIAL.println("Wifi found.");
  }

  // Make 3 attempts to connect to WiFi network.
  i = 2;

  while ((status != WL_CONNECTED) && (i > 0)) {
    DEBUGSERIAL.print("Attempting to connect to SSID: ");
    DEBUGSERIAL.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid);

    // wait 10 seconds for connection:
    delay(10000);
    i--;
  }
  if(status = WL_CONNECTED) {
    DEBUGSERIAL.println("Connected to wifi");
    printWiFiStatus();
    DEBUGSERIAL.println("Getting network clock (NTP)");
    timeClient.begin();
    if(timeClient.update()) {
      rtc.setEpoch(timeClient.getEpochTime());
      DEBUGSERIAL.print(F("NTP received. Clock is set to: "));
      PrintDateTime();
      DEBUGSERIAL.print(" UTC");
      DEBUGSERIAL.println();
    }
  } else {
    DEBUGSERIAL.println("Failed to connect to the wifi network. Check coverage and settings");
  }

  mqttClient.setCleanSession(true);
  mqttClient.setUsernamePassword(token, "");

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
  
    measuredvbat = analogRead(A7) * 2 * 3.3 / 1.024;
    distance = readMaxBotix(5, 9, 1);

    DEBUGSERIAL.print("Battery voltage = ");
    DEBUGSERIAL.println(measuredvbat);
    DEBUGSERIAL.print("Distance = ");
    DEBUGSERIAL.println(distance);

    // prepare telemetry message:

    CurrentTime = rtc.getEpoch();
    messageLength = createTelemetryMessage(myMessage);

    // attempt to connect to WiFi network:
    i = 2;
    while ((status != WL_CONNECTED) && (i < 0)) {
      DEBUGSERIAL.print("Attempting to connect to SSID: ");
      DEBUGSERIAL.println(ssid);
      // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
      status = WiFi.begin(ssid, pass);

      // wait 10 seconds for connection:
      delay(10000);
      i--;
    }

    if(status = WL_CONNECTED) {
      DEBUGSERIAL.println("Connected to wifi");
      printWiFiStatus();
      DEBUGSERIAL.print("Attempting to connect to the MQTT broker: ");
      DEBUGSERIAL.println(broker);

      mqttClient.setUsernamePassword(token, "");

      if (!mqttClient.connect(broker, port)) {
        DEBUGSERIAL.println(mqttClient.connectError());
      } else {
        DEBUGSERIAL.println("You're connected to the MQTT broker!");

        DEBUGSERIAL.print("Sending message to topic: ");
        DEBUGSERIAL.println(topic);
        DEBUGSERIAL.println(myMessage);

        // send message, the Print interface can be used to set the message contents
        mqttClient.beginMessage(topic);
        mqttClient.print(myMessage);
        mqttClient.endMessage();

        DEBUGSERIAL.println("Finished sending");
      }

    } else {
      DEBUGSERIAL.println("Failed to connect to the wifi network. Check coverage and settings");
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
  DEBUGSERIAL.print("/");
  print2digits(rtc.getMonth());
  DEBUGSERIAL.print("/");
  DEBUGSERIAL.print(rtc.getYear());
  DEBUGSERIAL.print(" ");
  // ...and time
  print2digits(rtc.getHours());
  DEBUGSERIAL.print(":");
  print2digits(rtc.getMinutes());
  DEBUGSERIAL.print(":");
  print2digits(rtc.getSeconds());
}

void print2digits(int number) {
  if (number < 10) {
    DEBUGSERIAL.print("0");
  }
  DEBUGSERIAL.print(number);
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

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  DEBUGSERIAL.print("SSID: ");
  DEBUGSERIAL.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  DEBUGSERIAL.print("IP Address: ");
  DEBUGSERIAL.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  DEBUGSERIAL.print("signal strength (RSSI):");
  DEBUGSERIAL.print(rssi);
  DEBUGSERIAL.println(" dBm");
}

uint16_t createTelemetryMessage(char *buffer) {

    uint16_t i, j, index;
    char string1[] = "{\"ts\":"; 
    char string2[] = "000,\"values\":{\"h\":";
    char string3[] = ",\"v\":";
    char string4[] = "}}";
    uint16_t bufferSize = 0;

    memcpy(buffer + bufferSize, string1, 6);
    bufferSize += 6;
    sprintf((char*) (buffer + bufferSize), "%10lu", CurrentTime);   
    bufferSize += 10;
    memcpy(buffer + bufferSize, string2, 18);
    bufferSize += 18;
    sprintf((char*) (buffer + bufferSize), "%5d", distance);
    bufferSize += 5;
    memcpy(buffer + bufferSize, string3, 5);
    bufferSize += 5;
    sprintf((char*) (buffer + bufferSize), "%5d", measuredvbat);
    bufferSize += 5;
    memcpy(buffer + bufferSize, string4, 2);
    bufferSize += 2;
    buffer[bufferSize] = '\0';
    return(bufferSize);
}
