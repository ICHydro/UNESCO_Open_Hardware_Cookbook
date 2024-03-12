# UNESCO_Open_Hardware_Cookbook

This repository contains the hands-on materials of the online course [Cookbook for Open Hardware Sensors for Water Resources Management](https://openlearning.unesco.org/courses/course-v1:UNESCO+OpenWater+2024/about), which is available on the [UNESCO Open Learning platform](https://openlearning.unesco.org/).

## Hardware requirements

The course uses an open hardware "test kit" to enable hands-on learning by the participants, using the materials provided here and the hands-on videos in the course. The test kit consists of the following components:

- [Adafruit Feather M0 Adalogger with SD card slot](https://www.adafruit.com/product/2796)
- [Adafruit Feather M0 WiFi - ATSAMD21 + ATWINC1500 with presoldered pins](https://www.adafruit.com/product/2598)
- [Male/Male Jumper Wires](https://www.adafruit.com/product/1957)
- [Female/Female Jumper Wires](https://www.adafruit.com/product/1950)
- [Lithium Ion Cylindrical Battery - 3.7v 2200mAh](https://www.adafruit.com/product/1781)
- [Perma-Proto Half-sized Breadboard PCB](https://www.adafruit.com/product/1609)
- [6V 1W Solar Panel](https://www.adafruit.com/product/3809)
- [FTDI Chip TTL-232R-3V3 cable](https://mou.sr/49KlDMq)
- [Riverlabs Feather Riverlogger PCB]()
- [Maxbotix MB7389 distance sensor with Riverlabs enclosure]()

Depending on the location of the participant, each of those components can be purchased individually from local or international suppliers, except for the last two items, which are only available from Imperial College London's [Riverlabs research group](https://paramo.cc.ic.ac.uk/). We can also provide the full test kit at the cost of the individual components and shipping. We are setting up an online shop to facilitate this, but for now please [get in touch](https://www.imperial.ac.uk/people/w.buytaert).

## Software requirements

The course uses the popular [Arduino](https://www.arduino.cc/) platform and ecosystem, which provides a great introduction into embedded programming. The core of this platform is the [Arduino IDE], which is available for Windows, Mac, and Linux.

We do not use the standard Arduino boards, but the [Adafruit Feather](https://www.adafruit.com/category/943) series of boards. These are fully compatible with Arduino, and come with very detailed and extensive documentation on the Adafruit website. For example, the tutorial to install and program the Adafruit Feather M0 Adalogger can be found [here](https://learn.adafruit.com/adafruit-feather-m0-adalogger).

Depending on the tutorial, some additional libraries will need to be installed. This can be done conveniently through the library manager of the Arduino IDE.

If you are using Windows and want to use the FTDI cable, then you may need to install the [FTDI driver](https://ftdichip.com/drivers/). We use the FTDI cable in the tutorial for serial communication between the logger and the PC. This can also be achieved through the USB port, and therefore the FTDI is not strictly necessary. However, serial communication with the USB port does not play very well with the sleep modes of the embedded processor. The USB port gets disconnected during sleep and may not always reconnect properly when the logger wakes up. For this reason, we find it convenient to do debugging over an FTDI cable, which does not have this issue. But if you are on a budget, then you can leave out the cable out.

## Tutorials

### HelloWorld

### SetClockM0

### ultrasound_test

### ultrasound_SD

### ultrasound_wifi




