/***************************************************************
 * Arduino code for the UNESCO Open Hardware Cookbook
 * 
 * This is an example sketch to test the connection
 * of the Feather board with the Maxbotix Ultrasound
 * sensor using a breadboard
 *
 * Fot the breadboard setup, follow the instructions
 * in the video
 *
 * (c) Wouter Buytaert, Imperial College London and Riverlabs Ltd
 * 
 **************************************************************/

int EZread() {
  
    int result;
    char inData[6];                             //char array to read data into
    int index = 0;
    unsigned long timer;
    boolean stringComplete = false;

    Serial1.flush();                           // Clear cache ready for next reading
    timer = millis();                           // use timer to time out after 1 sec.

    while ((stringComplete == false) && ((millis() - timer) < 1000)) {
        if (Serial1.available()) {    
            char rByte = Serial1.read();       // read serial input for "R" to mark start of data
            if(rByte == 'R') {
                while (index < 5) {             // read next 4 characters for range from sensor
                    if (Serial1.available()) {
                        inData[index] = Serial1.read();               
                        index++;                // Increment where to write next
                    }
                }
                inData[index] = 0x00;           // add a padding byte at end for atoi() function
            }
            rByte = 0;                          // reset the rByte ready for next reading
            index = 0;                          // Reset index ready for next reading
            stringComplete = true;              // Set completion of read to true
            result = atoi(inData);              // Changes string data into an integer for use
        }
    }
    if(stringComplete == false) result = -2;
    return result;
}

void setup(){

    pinMode(5, OUTPUT);
    digitalWrite(5, HIGH);
    Serial.begin(9600);            // for the serial monitor
    Serial1.begin(9600);           // for the Maxbotix sensor
    while(!Serial);
    Serial.println("start");

}

void loop(){

    for(uint8_t i; i < 20; i++) {
        Serial.print(EZread());
        Serial.print(" ");
    }
    Serial.println();

}


 