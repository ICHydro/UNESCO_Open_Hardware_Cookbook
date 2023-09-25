/**************************************
 * Arduino code for the UNESCO Open Hardware Cookbook
 * 
 * This is an example of serial communication
 * through the serial interface (UART) using
 * serial-to-USB cable (FTDI).
 * 
 * For the hardware setup, see the documentation
 * in the UNESCO Open Hardware Cookbook
 *
 * (c) Riverlabs Ltd and Imperial College London
 */

void setup() {

  //Serial.begin(115200);
  Serial1.begin(115200);
  //while(!Serial);

}

void loop() {

  //Serial.println("Hello, World");
  Serial1.println("Hello, World via Serial1");
  delay(3000);

}
