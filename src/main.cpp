#define MY_RADIO_NRF24
#define MY_DEBUG
#define MY_NODE_ID 1

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

#define RELAY_1_PIN 5

#define TEMPERATURE_CHILD_ID 1
#define WATER_CHILD_ID 2
#define RELAY_1_ID 11

MyMessage tempMsg(TEMPERATURE_CHILD_ID, V_TEMP);
MyMessage flowMsg(WATER_CHILD_ID, V_FLOW);
MyMessage volumeMsg(WATER_CHILD_ID, V_VOLUME);
MyMessage levelMsg(WATER_CHILD_ID, V_VAR1);

OneWire oneWire(WATER_TEMPERATURE_PIN);
DallasTemperature sensors(&oneWire);
Bounce debouncer = Bounce();

volatile uint32_t flowPulseCount;

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
    sendSketchInfo("Aquarium sensors", "0.5");
    wait(500);
    present(TEMPERATURE_CHILD_ID, S_TEMP, "Water temperature");
    wait(500);
    present(WATER_CHILD_ID, S_WATER, "Water flow/level");
    wait(500);
    present(RELAY_1_ID, S_BINARY, "Relay 1");
    wait(500);
}

void setup() {
    sensors.setWaitForConversion(false);

    pinMode(WATER_FLOW_PIN, INPUT_PULLUP);
    pinMode(WATER_LEVEL_PIN, INPUT_PULLUP);
    pinMode(RELAY_1_PIN, OUTPUT);

    digitalWrite(RELAY_1_PIN, HIGH);

    flowPulseCount = 0;
    volume = 0;

    oldTime = 0;

    debouncer.attach(WATER_LEVEL_PIN);
    debouncer.interval(1000);

    attachInterrupt(WATER_FLOW_INTERRUPT, pulseCounter, FALLING);
}

void updateFlow(unsigned long deltaTime) {
    detachInterrupt(WATER_FLOW_INTERRUPT);

    float flowRate = ((1000.0 / deltaTime) * flowPulseCount) / WATER_FLOW_CALIBRATION;

    // unsigned int flowMilliLitres = (flowRate / 60) * 1000;

    // volume += flowMilliLitres;

    // Serial.print("Flow rate: ");
    // Serial.print(flowRate);
    // Serial.println("L/min");

    // Serial.print("Liquid Quantity: ");
    // Serial.print(volume);
    // Serial.println("mL");
    // Serial.print("\t"); // Print tab space
    // Serial.print(volume / 1000);
    // Serial.println("L");

    if (flowRate != oldFlowRate) {
        oldFlowRate = flowRate;
        send(flowMsg.set(flowRate, 2));
    }

    // if (volume != oldVolume) {
    //     send(volumeMsg.set(volume, 3));
    //     oldVolume = volume;
    // }

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

    unsigned long now = millis();
    unsigned long deltaTime = now - oldTime;
    if (deltaTime > 1000) {
        updateFlow(deltaTime);
        // updateTemperature();
        updateLevel();
        oldTime = now;
    }
}


void receive(const MyMessage &message) {
    if (message.sensor == RELAY_1_ID && message.type == V_STATUS) {
        digitalWrite(RELAY_1_PIN, message.getBool() ? LOW : HIGH);
    }
}