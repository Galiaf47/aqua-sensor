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

#define WATER_LEVEL_PIN 4

#define RELAY_1_PIN 5
#define RELAY_2_PIN 6
#define RELAY_3_PIN 7
#define RELAY_4_PIN 8

#define SENSORS_CHILD_ID 1

#define SEND_INTERVAL 30000

MyMessage sensorsMsg(SENSORS_CHILD_ID, V_TEXT);

OneWire oneWire(WATER_TEMPERATURE_PIN);
DallasTemperature sensors(&oneWire);
Bounce debouncer = Bounce();

volatile uint32_t flowPulseCount;
float flowRate;
uint8_t flowRateSamples;

bool level;

float temperature;
uint8_t temperatureSamples;

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
    sendSketchInfo("Aquarium sensors", "1.1");
    wait(500);
    present(SENSORS_CHILD_ID, S_INFO, "Sensors");
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
    flowRate = 0;
    flowRateSamples = 0;

    level = 0;

    temperature = 0;
    temperatureSamples = 0;

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
    flowRate += (1000.0 / deltaTime * flowPulseCount);
    flowRateSamples++;
    flowPulseCount = 0;
}

void updateTemperature() {
    float currentTemperature = sensors.getTempCByIndex(0);
    
    if (currentTemperature != -127) {
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

        String result = "";
        result = result
            + temperature + ":"
            + flowRate + ":"
            + level + ":"
            + relay1Status + ":"
            + relay2Status + ":"
            + relay3Status + ":"
            + relay4Status;

        send(sensorsMsg.set(result.c_str()));
    }
}

void receive(const MyMessage &message) {
    if (message.type == V_TEXT) {
        const char * command = message.getString();

        if (command[0] == 'r') {
            uint8_t pin;
            uint8_t state = command[3] - '0';

            switch (command[1]) {
                case '1':
                    pin = RELAY_1_PIN;
                    relay1Status = state;
                    break;
                case '2':
                    pin = RELAY_2_PIN;
                    relay2Status = state;
                    break;
                case '3':
                    pin = RELAY_3_PIN;
                    relay3Status = state;
                    break;
                case '4':
                    pin = RELAY_4_PIN;
                    relay4Status = state;
                    break;
                default:
                    return;
            }

            digitalWrite(pin, state ? HIGH : LOW);
        }
    }
}