# homie-gasmeter
gasmeter pulse sensor based on a ESP8266 using Homie
Uses a A3213* Hall sensor, directly connected to a Wemos D1 mini
written in Arduino

*Allegro A3213EUA-T
Vdd connected to 3.3V (from the same power source as ESP8266)
GND connected to ground
Vout connected to the pin defined in firmware (but not the special puprose pins).
works for me without any extra components like resistors or capacitors.

Counts pulses and keeps total, calculates hourly consumption and flow.
All data is published by MQTT, using Homie.

dependencies:
- <a href="https://github.com/marvinroger/homie-esp8266">Homie by Marvin Roger</a> and subsequent dependencies
- <a href="https://github.com/JoaoLopesF/ESP8266-RemoteDebug-Telnet">RemoteDebug by JoaoLopesF</a> and subsequent dependencies

To do:
- testing and debugging!
- write data directly into <a href="https://www.influxdata.com/">InfluxDB</a> (FYI: I'm currently using <a href="http://www.openhab.org/">openHAB2</a>.)
- in the long run: add webpage displaying current total with possibility to set value (to set starting point as on fysical utility meter)
