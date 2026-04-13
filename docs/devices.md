# Device Discussion

## Arduino Uno series

The Arduino Uno series is the best Arduino board to get started with, as it is the most robust and documented one.
For these reasons, it has a great market share and it makes sense to choose it as one of the possible targets.

### Arduino Uno rev3

I discarded this one, as it has no wifi capabilities.

### Arduino Uno rev4 WiFi

This one is a good fit. It features the form-factor of the Uno series, at basically the same price point of the rev3 and has additional Bluetooth and WiFi capabilities,
courtesy of the ESP32-S3 chip from Espressif.

This board features two microcontrollers. The main one is the RA4M1 from Renesas, which has a 48MHz Arm Cortex-M4.
It additionally features an ESP32-S3 MCU from Espressif which enables WiFi and Bluetooth LE capabilities on this board, making it ideal for wireless smart appliances, etc.

### Arduino Nano R3 Board with CH340 Chip

This is a board that seems to have reached its end-of-life support, but that was popular nonetheless. It similarly features WiFi capabilities,
but has a weaker ATmega328P MCU.

## Epressif Boards

### ESP8266

Very popular board back in the day, has wifi support. Superseded by ESP32.

### ESP32

Features Bluetooth and WiFi connectivity. Can perform as a standalone or as a slave to another main MCU. Very robust in various temperature environments. Ultra low power consumption.
An all-round great board, with a big market share. Very likely to be found in a variety of industrial IoT deployments.
