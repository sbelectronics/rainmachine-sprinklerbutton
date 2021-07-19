# Rainmachine Sprinklerbutton using an ESP8266
### Scott Baker, http://www.smbaker.com/

## Purpose

I have a yard hydrant (i.e. a fancy faucet) that is on my irrigation system behind
a master valve. In order to use the hydrant the master valve has to be activated. I
do have a "smart" sprinkler controller, called a rainmachine, that is accessible
via a REST API. 

While it's possible to turn on the master valve from my phone, it's also somewhat
inconvenient to grab a phone and launch the app, so I decied to implement a simple
ESP8266-based button. The button when pressed will reset the ESP8266 which will then
send a few REST API requests to the sprinkler timer. Once complete, the ESP8266 will
enter deep sleep mode to conserve battery power.
