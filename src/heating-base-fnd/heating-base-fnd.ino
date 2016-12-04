/*
 * L I B R A R I E S
 */
#include <MsTimer2.h>

/*
 * I O
 */
#define RELAY_1 10
#define RELAY_2 11
#define VALID 13

/*
 * C O M M
 */
#define CMD_HEADER '['
#define CMD_HELLO '!'
#define CMD_IS_ALIVE '?'
#define CMD_RAW '$'
#define CMD_FOOTER ']'
#define CMD_TIMEOUT 1000
#define MAX_PERIOD 1000UL

/*
 * P W M
 */
#define PWM_STEP 125UL
#define PWM_RES 7
#define PWM_MAX_VALUE ((1 << PWM_RES) - 1)
//#define PWM_PERIOD (PWM_MAX_VALUE * PWM_STEP)

/*
 * S T A T E S 
 */
#define STAT_NONE 0
#define STAT_HEADER 1
#define STAT_CMD 2
#define STAT_DATA 3
#define STAT_FOOTER 4

/*
 * G L O B A L S
 */
boolean dataValid= false;
unsigned char rawData = 0;

void setup() {
    Serial.begin(57600);
    pinMode(RELAY_1, OUTPUT);
    pinMode(RELAY_2, OUTPUT);
    pinMode(VALID, OUTPUT);
    MsTimer2::set(PWM_STEP, slowPWM);
    MsTimer2::start();
}

void loop() {
    static boolean received = false;
    static boolean waiting = false;
    unsigned char cmd = 0;
    unsigned long maxPeriod = (MAX_PERIOD * 1000);
    static unsigned long timeout = 0;

    cmd = readCommand();
    if (cmd == CMD_RAW) {
        received = true;
        Serial.println(rawData);
        timeout = millis();
    }
    else if (cmd == CMD_IS_ALIVE) {
        sendCommand(CMD_HELLO);
        timeout = millis();
    }
    else if ((cmd == CMD_HELLO) && (waiting)) {
        timeout = millis();
        waiting = false;
    }

    if ((millis() - timeout) > maxPeriod) {
        waiting = false;
        dataValid = false;
    }
    else if ((millis() - timeout) > (unsigned long)(maxPeriod - CMD_TIMEOUT)) {
        if (!waiting) {
            sendCommand(CMD_IS_ALIVE);
        }
        waiting = true;
    }
    else if (received) dataValid = true;
}

void slowPWM() {
    boolean runSoftPWM = false;
    static unsigned char pin = 0;
    static unsigned char val = 0;
    static unsigned char currentStep = 1;
    static unsigned long t = 0;

    if (dataValid) {
        digitalWrite(VALID, LOW);
        if (rawData == 255) {
            runSoftPWM = false;
            digitalWrite(RELAY_1, HIGH);
            digitalWrite(RELAY_2, HIGH);
        }
        else if (rawData > 126) {
            runSoftPWM = true;
            digitalWrite(RELAY_1, HIGH);
            val = (rawData - 127);
            pin = RELAY_2;
        }
        else if (rawData) {
            runSoftPWM = true;
            digitalWrite(RELAY_2, LOW);
            val = rawData;
            pin = RELAY_1;
        }
        else {
            runSoftPWM = false;
            digitalWrite(RELAY_1, LOW);
            digitalWrite(RELAY_2, LOW);
        }
        if (runSoftPWM) {
            if ((millis() - t) > (unsigned long)PWM_STEP) {
                t = millis();
                currentStep++;
                if (currentStep > PWM_MAX_VALUE) currentStep = 1;
            }
            if (val < currentStep) digitalWrite(pin, LOW);
            else digitalWrite(pin, HIGH);
        }
    }
    else {
        digitalWrite(VALID, HIGH);
        digitalWrite(RELAY_1, LOW);
        digitalWrite(RELAY_2, LOW);
        currentStep = 1;
    }
}

void sendCommand(char cmd) {
    Serial.print(CMD_HEADER);
    switch (cmd) {
      case CMD_HELLO:
        Serial.print(CMD_HELLO);
        break;
      case CMD_IS_ALIVE:
        Serial.print(CMD_IS_ALIVE);
        break;
    }
    Serial.print(CMD_FOOTER);
}

char readCommand() {
    static unsigned char state = STAT_NONE;
    static char ret = 0;
    static char data[4];
    static unsigned char index = 0;
    char c = 0;

    if (Serial.available()) {
        c = Serial.read();
        switch (state) {
          case STAT_NONE:
            ret = 0;
            state = STAT_HEADER;
            //break;
          case STAT_HEADER:
            if (c == CMD_HEADER) state = STAT_CMD;
            else state = STAT_NONE;
            break;
          case STAT_CMD:
            if ((c == CMD_HELLO) || (c == CMD_IS_ALIVE)) {
                ret = c;
                state = STAT_FOOTER;
            }
            else if (c == CMD_RAW) {
                ret = c;
                index = 0;
                for (unsigned char i = 0; i < 4; i++) data[i] = 0;
                state = STAT_DATA;
            }
            else state = STAT_NONE;
            break;
          case STAT_DATA:
            if ((c >= '0') && (c <= '9')) {
                data[index] = c;
                index++;
            }
            else if (c == CMD_FOOTER) {
                rawData = atoi(data);
                state = STAT_NONE;
                return ret;
            }
            else state = STAT_NONE;
            break;
          case STAT_FOOTER:
            if (c == CMD_FOOTER) {
                state = STAT_NONE;
                return ret;
            }
            else state = STAT_NONE;
            break;
        }
    }
    else return 0;
}
