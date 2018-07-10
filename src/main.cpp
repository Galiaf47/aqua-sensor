#define MY_RADIO_NRF24
#define MY_DEBUG

#include <SPI.h>
#include <MySensors.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Bounce2.h>

#define WATER_TEMPERATURE_PIN 2

#define WATER_FLOW_PIN 3
#define WATER_FLOW_INTERRUPT 1 // 1 = pin 3
#define WATER_FLOW_CALIBRATION 4.5

#define WATER_LEVEL_PIN 4

#define TEMPERATURE_CHILD_ID 0
#define FLOW_CHILD_ID 1
#define VOLUME_CHILD_ID 2
#define LEVEL_CHILD_ID 3

MyMessage tempMsg(TEMPERATURE_CHILD_ID, V_TEMP);
MyMessage flowMsg(FLOW_CHILD_ID, V_FLOW);
MyMessage volumeMsg(VOLUME_CHILD_ID, V_VOLUME);
MyMessage levelMsg(LEVEL_CHILD_ID, V_STATUS);

OneWire oneWire(WATER_TEMPERATURE_PIN);
DallasTemperature sensors(&oneWire);
Bounce debouncer = Bounce();

volatile byte flowPulseCount;

unsigned long volume;

bool oldLevel;
float oldTemperature;
float oldFlowRate;
unsigned long oldVolume;
unsigned long oldTime;

void pulseCounter() {
    flowPulseCount++;
}

void before() {
    sensors.begin();
}

void presentation() {
    sendSketchInfo("Aquarium sensors", "0.4");
    wait(500);
    present(TEMPERATURE_CHILD_ID, S_TEMP, "Water temperature");
    wait(500);
    present(FLOW_CHILD_ID, S_WATER, "Water flow");
    wait(500);
    present(LEVEL_CHILD_ID, S_BINARY, "Water level");
    wait(500);
}

void setup() {
    sensors.setWaitForConversion(false);

    pinMode(WATER_FLOW_PIN, INPUT_PULLUP);
    pinMode(WATER_LEVEL_PIN, INPUT_PULLUP);

    flowPulseCount = 0;
    volume = 0;

    oldTime = 0;

    debouncer.attach(WATER_LEVEL_PIN);
    debouncer.interval(5000);

    attachInterrupt(WATER_FLOW_INTERRUPT, pulseCounter, FALLING);
}

void updateFlow() {
    detachInterrupt(WATER_FLOW_INTERRUPT);

    float flowRate = ((1000.0 / (millis() - oldTime)) * flowPulseCount) / WATER_FLOW_CALIBRATION;

    oldTime = millis();

    unsigned int flowMilliLitres = (flowRate / 60) * 1000;

    volume += flowMilliLitres;

    Serial.print("Flow rate: ");
    Serial.print(flowRate);
    Serial.println("L/min");

    Serial.print("Liquid Quantity: ");
    Serial.print(volume);
    Serial.println("mL");
    Serial.print("\t"); // Print tab space
    Serial.print(volume / 1000);
    Serial.println("L");

    if (flowRate != oldFlowRate) {
        send(flowMsg.set(flowRate, 2));
        oldFlowRate = flowRate;
    }

    if (volume != oldVolume) {
        send(volumeMsg.set(volume, 3));
        oldVolume = volume;
    }

    flowPulseCount = 0;

    attachInterrupt(WATER_FLOW_INTERRUPT, pulseCounter, FALLING);
}

void updateTemperature() {
    sensors.requestTemperatures();
    float temperature = sensors.getTempCByIndex(0);
    
    Serial.print("Temperature: ");
    Serial.println(temperature);

    if (temperature != oldTemperature) {
        send(tempMsg.set(temperature, 1));
        oldTemperature = temperature;
    }
}

void updateLevel() {
    bool level = debouncer.read();

    Serial.print("Level: ");
    Serial.println(level);

    if (level != oldLevel) {
        send(levelMsg.set(level));
        oldLevel = level;
    }
}

void loop() {
    debouncer.update();

    if ((millis() - oldTime) > 1000) {
        updateFlow();
        updateTemperature();
        updateLevel();
    }
}
