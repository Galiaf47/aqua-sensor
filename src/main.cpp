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
#define WATER_FLOW_CALIBRATION 7.5

#define WATER_LEVEL_PIN 4

#define RELAY_1_PIN 5
#define RELAY_2_PIN 6
#define RELAY_3_PIN 7
#define RELAY_4_PIN 8

#define TEMPERATURE_CHILD_ID 1
#define WATER_CHILD_ID 2
#define RELAY_1_ID 11
#define RELAY_2_ID 12
#define RELAY_3_ID 13
#define RELAY_4_ID 14

#define SEND_INTERVAL 10000

MyMessage tempMsg(TEMPERATURE_CHILD_ID, V_TEMP);
MyMessage flowMsg(WATER_CHILD_ID, V_FLOW);
MyMessage volumeMsg(WATER_CHILD_ID, V_VOLUME);
MyMessage levelMsg(WATER_CHILD_ID, V_VAR1);
MyMessage relay1Msg(RELAY_1_ID, V_STATUS);
MyMessage relay2Msg(RELAY_2_ID, V_STATUS);
MyMessage relay3Msg(RELAY_3_ID, V_STATUS);
MyMessage relay4Msg(RELAY_4_ID, V_STATUS);

OneWire oneWire(WATER_TEMPERATURE_PIN);
DallasTemperature sensors(&oneWire);
Bounce debouncer = Bounce();

volatile uint32_t flowPulseCount;

bool level;
float temperature;
float flowRate;

bool relay1Status;
bool relay2Status;
bool relay3Status;
bool relay4Status;

unsigned long updateTime;
unsigned long sendTime;

void pulseCounter() {
    flowPulseCount++;
}

void before() {
}

void presentation() {
    sendSketchInfo("Aquarium sensors", "1.0");
    wait(500);
    present(TEMPERATURE_CHILD_ID, S_TEMP, "Water temperature");
    wait(500);
    present(WATER_CHILD_ID, S_WATER, "Water flow/level");
    wait(500);
    present(RELAY_1_ID, S_BINARY, "Relay 1");
    wait(500);
    present(RELAY_2_ID, S_BINARY, "Relay 2");
    wait(500);
    present(RELAY_3_ID, S_BINARY, "Relay 3");
    wait(500);
    present(RELAY_4_ID, S_BINARY, "Relay 4");
    wait(500);
}

void setup() {
    pinMode(WATER_FLOW_PIN, INPUT_PULLUP);
    pinMode(WATER_LEVEL_PIN, INPUT_PULLUP);

    pinMode(RELAY_1_PIN, OUTPUT);
    pinMode(RELAY_2_PIN, OUTPUT);
    pinMode(RELAY_3_PIN, OUTPUT);
    pinMode(RELAY_4_PIN, OUTPUT);

    digitalWrite(RELAY_1_PIN, LOW);
    digitalWrite(RELAY_2_PIN, LOW);
    digitalWrite(RELAY_3_PIN, LOW);
    digitalWrite(RELAY_4_PIN, LOW);

    sensors.begin();
    sensors.setWaitForConversion(false);
    sensors.requestTemperatures();
    
    flowPulseCount = 0;

    level = 0;
    temperature = 0;
    flowRate = 0;

    updateTime = 0;
    sendTime = 0;

    relay1Status = false;
    relay2Status = false;
    relay3Status = false;
    relay4Status = false;

    debouncer.attach(WATER_LEVEL_PIN);
    debouncer.interval(500);

    attachInterrupt(WATER_FLOW_INTERRUPT, pulseCounter, FALLING);
}

void updateFlow(unsigned long deltaTime) {
    // flowRate = ((1000.0 / deltaTime) * flowPulseCount) / WATER_FLOW_CALIBRATION;
    flowRate = (((1000.0 / deltaTime) * flowPulseCount) * 60 / WATER_FLOW_CALIBRATION);
    flowPulseCount = 0;

    Serial.print("Flow rate: ");
    Serial.print(flowRate);
    Serial.println("L/hour");
}

void updateTemperature() {
    temperature = sensors.getTempCByIndex(0);
    sensors.requestTemperatures();
    
    Serial.print("Temperature: ");
    Serial.println(temperature);
}

void updateLevel() {
    level = debouncer.read();

    Serial.print("Level: ");
    Serial.println(level);
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

        wait(500);
        send(flowMsg.set(flowRate, 2));
        wait(500);
        send(tempMsg.set(temperature, 1));
        wait(500);
        send(levelMsg.set(level));

        wait(500);
        send(relay1Msg.set(relay1Status));
        wait(500);
        send(relay2Msg.set(relay2Status));
        wait(500);
        send(relay3Msg.set(relay3Status));
        wait(500);
        send(relay4Msg.set(relay4Status));
    }
}

void receive(const MyMessage &message) {
    if (message.type == V_STATUS) {
        if (message.sensor == RELAY_1_ID) {
            relay1Status = message.getBool();
            digitalWrite(RELAY_1_PIN, relay1Status ? HIGH : LOW);
        }

        if (message.sensor == RELAY_2_ID) {
            relay2Status = message.getBool();
            digitalWrite(RELAY_2_PIN, relay2Status ? HIGH : LOW);
        }

        if (message.sensor == RELAY_3_ID) {
            relay3Status = message.getBool();
            digitalWrite(RELAY_3_PIN, relay3Status ? HIGH : LOW);
        }
        
        if (message.sensor == RELAY_4_ID) {
            relay4Status = message.getBool();
            digitalWrite(RELAY_4_PIN, relay4Status ? HIGH : LOW);
        }
    }
}