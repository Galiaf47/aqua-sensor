#define MY_RADIO_NRF24
#define MY_DEBUG
#define MY_NODE_ID 3

#include <SPI.h>
#include <MySensors.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Bounce2.h>

#define WATER_TEMPERATURE_PIN 2

#define WATER_FLOW_PIN 3
#define WATER_FLOW_INTERRUPT 1 // 1 = pin 3

#define WATER_LEVEL_PIN 4

#define TEMPERATURE_CHILD_ID 1
#define FLOW_CHILD_ID 2
#define LEVEL_CHILD_ID 3

#define SEND_INTERVAL 30000

MyMessage temperatureMsg(TEMPERATURE_CHILD_ID, V_TEMP);
MyMessage flowMsg(FLOW_CHILD_ID, V_FLOW);
MyMessage levelMsg(LEVEL_CHILD_ID, V_STATUS);

OneWire oneWire(WATER_TEMPERATURE_PIN);
DallasTemperature sensors(&oneWire);
Bounce debouncer = Bounce();

volatile uint32_t flowPulseCount;
float flowRate;
uint8_t flowRateSamples;

bool level;

float temperature;
uint8_t temperatureSamples;

unsigned long updateTime;
unsigned long sendTime;

void pulseCounter() {
    flowPulseCount++;
}

void before() {
    sensors.begin();
}

void presentation() {
    sendSketchInfo("Aquarium sensors", "1.2");
    wait(500);
    present(TEMPERATURE_CHILD_ID, S_TEMP, "Water temperature");
    wait(500);
    present(FLOW_CHILD_ID, S_WATER, "Water flow rate");
    wait(500);
    present(LEVEL_CHILD_ID, S_BINARY, "Water level");
    wait(500);
}

void setup() {
    pinMode(WATER_TEMPERATURE_PIN, INPUT_PULLUP);
    pinMode(WATER_FLOW_PIN, INPUT_PULLUP);
    pinMode(WATER_LEVEL_PIN, INPUT_PULLUP);

    sensors.setWaitForConversion(false);
    sensors.requestTemperatures();
    
    flowPulseCount = 0;
    flowRate = 0;
    flowRateSamples = 0;

    level = 0;

    temperature = 0;
    temperatureSamples = 0;

    updateTime = 0;
    sendTime = 0;

    debouncer.attach(WATER_LEVEL_PIN);
    debouncer.interval(500);

    attachInterrupt(WATER_FLOW_INTERRUPT, pulseCounter, FALLING);
}

void updateFlow(unsigned long deltaTime) {
    flowRate += (1000.0 / deltaTime * flowPulseCount);
    flowRateSamples++;
    flowPulseCount = 0;
}

void updateTemperature() {
    float currentTemperature = sensors.getTempCByIndex(0);
    
    if (currentTemperature != -127 && currentTemperature != 85) {
        temperature += currentTemperature;
        temperatureSamples++;
    }
    
    sensors.requestTemperatures();
}

void updateLevel() {
    level = debouncer.read();
}

void loop() {
    debouncer.update();

    unsigned long now = millis();
    unsigned long deltaTime = now - updateTime;
    if (deltaTime > 1000) {
        updateTime = now;

        updateFlow(deltaTime);
        updateTemperature();
        updateLevel();
    }

    if (now - sendTime > SEND_INTERVAL) {
        sendTime = now;

        if (temperatureSamples != 0) {
            temperature = temperature / temperatureSamples;
            temperatureSamples = 1;
        }

        if (flowRateSamples != 0) {
            flowRate = flowRate / flowRateSamples;
            flowRateSamples = 1;
        }

        send(temperatureMsg.set(temperature, 1));
        wait(500);
        send(flowMsg.set(flowRate, 1));
        wait(500);
        send(levelMsg.set(level));
        wait(500);
    }
}
