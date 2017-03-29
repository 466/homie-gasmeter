# homie-gasmeter
gasmeter pulse sensor based on a ESP8266 using Homie
Uses a Hall sensor, directly connected to a Wemos D1 mini
written in Arduino

Counts pulses and keeps total, calculates hourly consumption and instant (=1min) flow.
All data is published by MQTT using the Homie framework.

## Hardware
I'm using a A3213EUA-T from Allegro Microsystems.
The IC is directly connected to a GPIO pin as defined in the firmware without any extra components (so no resistors or capacitors).
As I'm using a Wemos D1 mini, I can use the 3.3V pin to source from for the Hall sensor. GND is obviously connected to GND.

## dependencies:
- <a href="https://github.com/marvinroger/homie-esp8266">Homie by Marvin Roger</a> and subsequent dependencies
- <a href="https://github.com/bertmelis/TelnetPrinter">my simple Telnet Printer OR EQUIVALENT</a>

## To do:
- testing and debugging!
- write data directly into <a href="https://www.influxdata.com/">InfluxDB</a> (FYI: I'm currently using <a href="http://www.openhab.org/">openHAB2</a>.)
- save total counter locally for recovery purposes
