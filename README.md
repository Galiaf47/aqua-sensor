# AquaSensor
AquaSensor - MySensors based controller for aquarium.
My goal is to make hardware as dumb as possible with external control (openHAB in my case), so one can use it with any smart home controller.

## Basic functions
Sensors:
* water flow sensor
* water temperature sensor
* water level sensor

Relays:
* relay for peristaltic pump
* relay for light
* relay for CO2/O2
* relay for cooler fans

Sensors and relays are not coupled, sensors send information to controller, relays listen for controller's comands, all rules should be implemented as controller rules or scripts.

## Compile/Upload

**Arduino uno:** platformio run -e uno

**Arduino pro mini:** platformio run -e pro -t upload