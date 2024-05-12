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

#define DEBUGSERIAL Serial1

void setup() {

  DEBUGSERIAL.begin(9600);
  while(!DEBUGSERIAL);

}

void loop() {

  DEBUGSERIAL.println("Hello, World!");
  delay(3000);

}
