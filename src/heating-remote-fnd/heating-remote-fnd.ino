
/*
 * L I B R A R I E S
 */
#include <TM1637Display.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <PID_v1.h>

/*
 * I O
 */
#define ONE_WIRE_BUS 2
#define DISP_DIO 3
#define DISP_CLK 4
#define TX_RX_LED 13
#define JOYSTICK_X A0
#define JOYSTICK_Y A1
#define JOYSTICK_PUSH A2

/*
 * E E P R O M
 */
 #define EEDAT_EEVALID 0x5D
 #define EEADR_EEVALID 0
 #define EEADR_CONFSET EEADR_EEVALID + sizeof(char)
 #define EEADR_BRIGHT EEADR_CONFSET + sizeof(char)
 #define EEADR_MODE EEADR_BRIGHT + sizeof(char)
 #define EEADR_SETPOINT EEADR_MODE + sizeof(char)
 #define EEADR_TOFF EEADR_SETPOINT + sizeof(char)
 #define EEADR_PERIOD EEADR_TOFF + sizeof(int)
 #define EEADR_PVALUE EEADR_PERIOD + sizeof(long)
 #define EEADR_IVALUE EEADR_PVALUE + sizeof(float)
 #define EEADR_DVALUE EEADR_IVALUE + sizeof(float)

/*
 * M E S S A G E
 */
#define MSG_INIT 0
#define MSG_CELSIUS 1
#define MSG_ERR 2
#define MSG_SET 3
#define MSG_OFF 4
#define MSG_ON 5
#define MSG_AUTO 6
#define MSG_CONFIG 7
#define MSG_MODE 8
#define MSG_SETPOINT 9
#define MSG_TOFF 10
#define MSG_BRIGHT 11
#define MSG_PERIOD 12
#define MSG_PVALUE 13
#define MSG_IVALUE 14
#define MSG_DVALUE 15

/*
 * M O D E
 */
 #define MODE_OFF 0
 #define MODE_ON 1
 #define MODE_AUTO 2

/*
 * C O M M
 */
#define CMD_HEADER '['
#define CMD_HELLO '!'
#define CMD_IS_ALIVE '?'
#define CMD_RAW '$'
#define CMD_FOOTER ']'
#define CMD_TIMEOUT 1000

/*
 * J O Y S T I C K
 */
 #define JS_NONE 0
#define JS_LEFT 1
#define JS_RIGHT 2
#define JS_UP 4
#define JS_DOWN 8
#define JS_PUSH 16

/*
 * S T A T E S
 */
#define STAT_NONE 0
#define STAT_IDLE 1
#define STAT_STANDBY 2
#define SSTAT_NONE 0
#define SSTAT_CONFIG 1
#define SSTAT_MODE 2
#define SSTAT_SETPOINT 3
#define SSTAT_TOFF 4
#define SSTAT_BRIGHT 5
#define SSTAT_PERIOD 6
#define SSTAT_PVALUE 7
#define SSTAT_IVALUE 8
#define SSTAT_DVALUE 9
#define SSTAT_INSIDE_MODE 10
#define SSTAT_INSIDE_SETPOINT 11
#define SSTAT_INSIDE_TOFF 12
#define SSTAT_INSIDE_BRIGHT 13
#define SSTAT_INSIDE_PERIOD 14
#define SSTAT_INSIDE_PVALUE 15
#define SSTAT_INSIDE_IVALUE 16
#define SSTAT_INSIDE_DVALUE 17
#define SSTAT_FIRST_CONFIG SSTAT_MODE
#define SSTAT_LAST_CONFIG SSTAT_DVALUE
/*
 * D E F A U L T S
 */
#define T_DISPLAY_BLINK 500UL
#define T_READ_SENSOR 1000UL
#define DEFAULT_BRIGHT 8
#define DEFAULT_MODE MODE_AUTO
#define DEFAULT_SETPOINT 0
#define DEFAULT_TOFF 30
#define DEFAULT_PERIOD 300L
#define DEFAULT_PVALUE 1.00
#define DEFAULT_IVALUE 3.00
#define DEFAULT_DVALUE 0.20

/*
 * L I M I T S
 */
#define SETPOINT_UP 30
#define SETPOINT_DOWN 0
#define TOFF_UP 255
#define TOFF_DOWN 0
#define BRIGHT_UP 8
#define BRIGHT_DOWN 1
#define PERIOD_UP 1000
#define PERIOD_DOWN 0
#define KP_UP 10.00
#define KP_DOWN 0.00
#define KI_UP 10.00
#define KI_DOWN 0.00
#define KD_UP 10.00
#define KD_DOWN 0.00

/*
 * I N S T A N C E S
 */
TM1637Display display(DISP_CLK, DISP_DIO);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);
DeviceAddress onewire_address;
double pid_i, pid_o, pid_sp;
PID heatingPID(&pid_i, &pid_o, &pid_sp, 0.00, 0.00, 0.00, DIRECT);

void setup() {
    unsigned long cmdTimeout = 0;

    pinMode(JOYSTICK_PUSH, INPUT_PULLUP);
    pinMode(TX_RX_LED, OUTPUT);
    display.setBrightness(8);
    displayPrintMsg(MSG_INIT);
    sensor.begin();
    if (sensor.getAddress(onewire_address, 0)) sensor.setResolution(onewire_address, 9);    
    else {
        display.setBrightness(8);
        displayPrintMsg(MSG_ERR);
        display.showNumberDec(1,false,1,3);
        while(1);
    }
    Serial.begin(57600);
    cmdTimeout = millis();
    sendCommand(CMD_IS_ALIVE, 0);
    while ((Serial.available() <= 2) && (millis() < (cmdTimeout + CMD_TIMEOUT)));
    if (readCommand() != CMD_HELLO) {
        display.setBrightness(8);
        displayPrintMsg(MSG_ERR);
        display.showNumberDec(2,false,1,3);
        while(1);
    }
    delay(1000);
}

void loop() {
    static boolean confLoaded = false;
    static boolean confSet = false;
    static char mode = 0;
    static char modeTmp = 0;
    static char bright = 0;
    static char brightTmp = 0;
    static char setpoint = 0;
    static char setpointTmp = 0;
    static int timerOff = 0;
    static int timerOffTmp = 0;
    static unsigned char dispBlink = 0;
    static unsigned char state = STAT_NONE;
    static unsigned char substate = SSTAT_NONE;
    static unsigned char rTemp = 0;
    static unsigned char joystick = 0;
    static long period = 0;
    static long periodTmp = 0;
    static unsigned long prevT_Send = 0;
    static unsigned long prevT_Blink = 0;
    static unsigned long prevT_Update = -1;
    static unsigned long prevT_Joystick = 0;
    static float tempC = 0.00;
    static float kp = 0.00;
    static float kpTmp = 0.00;
    static float ki = 0.00;
    static float kiTmp = 0.00;
    static float kd = 0.00;
    static float kdTmp = 0.00;

    joystick = readJoystick(10);
    if (joystick) prevT_Joystick = millis();
    switch (state) {
      case STAT_NONE:
        if (!confLoaded) {
            if (EEPROM.read(EEADR_EEVALID) == EEDAT_EEVALID) {
                EEPROM.get(EEADR_CONFSET, confSet);
                EEPROM.get(EEADR_BRIGHT, bright);
                EEPROM.get(EEADR_MODE, mode);
                EEPROM.get(EEADR_SETPOINT, setpoint);
                EEPROM.get(EEADR_TOFF, timerOff);
                EEPROM.get(EEADR_PERIOD, period);
                EEPROM.get(EEADR_PVALUE, kp);
                EEPROM.get(EEADR_IVALUE, ki);
                EEPROM.get(EEADR_DVALUE, kd);
            }
            else {
                confSet = false;
                EEPROM.put(EEADR_CONFSET, confSet);
                bright = DEFAULT_BRIGHT;
                EEPROM.put(EEADR_BRIGHT, bright);
                mode = DEFAULT_MODE;
                EEPROM.put(EEADR_MODE, mode);
                setpoint = DEFAULT_SETPOINT;
                EEPROM.put(EEADR_SETPOINT, setpoint);
                timerOff = DEFAULT_TOFF;
                EEPROM.put(EEADR_TOFF, timerOff);
                period = DEFAULT_PERIOD;
                EEPROM.put(EEADR_PERIOD, period);
                kp = DEFAULT_PVALUE;
                EEPROM.put(EEADR_PVALUE, kp);
                ki = DEFAULT_IVALUE;
                EEPROM.put(EEADR_IVALUE, ki);
                kd = DEFAULT_DVALUE;
                EEPROM.put(EEADR_DVALUE, kd);
                EEPROM.update(EEADR_EEVALID, EEDAT_EEVALID);
            }
            brightTmp = bright;
            modeTmp = mode;
            setpointTmp = setpoint;
            timerOffTmp = timerOff;
            periodTmp = period;
            kpTmp = kp;
            kiTmp = ki;
            kdTmp = kd;
            heatingPID.SetMode(AUTOMATIC);
            heatingPID.SetTunings(kp, ki, kd);
            confLoaded = true;
        }
        state = STAT_IDLE;
        break;
      case STAT_IDLE:
        switch (substate) {
          case SSTAT_NONE:
            displayPrintTemp(rTemp);
            if (!confSet) {
                if ((millis() - prevT_Blink) > T_DISPLAY_BLINK) {
                    prevT_Blink = millis();
                    if (dispBlink) dispBlink = 0;
                    else dispBlink = bright;
                    display.setBrightness(dispBlink);
                }
            }
            if (joystick == JS_DOWN) {
                display.setBrightness(bright);
                substate = SSTAT_CONFIG;
            }
            break;
          case SSTAT_CONFIG:
            displayPrintMsg(MSG_CONFIG);
            if (joystick == JS_UP) substate = SSTAT_NONE;
            else if (joystick == JS_PUSH) substate = SSTAT_MODE;
            break;
          case SSTAT_MODE:
            displayPrintMsg(MSG_MODE);
            if (joystick == JS_UP) substate = SSTAT_CONFIG;
            else if (joystick == JS_RIGHT) substate++;
            else if (joystick == JS_LEFT) {
                substate--;
                if (substate < SSTAT_FIRST_CONFIG) substate = SSTAT_LAST_CONFIG;
            }
            else if (joystick == JS_PUSH) substate = SSTAT_INSIDE_MODE;
            break;
          case SSTAT_SETPOINT:
            displayPrintMsg(MSG_SETPOINT);
            if (joystick == JS_UP) substate = SSTAT_CONFIG;
            else if (joystick == JS_RIGHT) substate++;
            else if (joystick == JS_LEFT) substate--;
            else if (joystick == JS_PUSH) substate = SSTAT_INSIDE_SETPOINT;
            break;
          case SSTAT_TOFF:
            displayPrintMsg(MSG_TOFF);
            if (joystick == JS_UP) substate = SSTAT_CONFIG;
            else if (joystick == JS_RIGHT) substate++;
            else if (joystick == JS_LEFT) substate--;
            else if (joystick == JS_PUSH) substate = SSTAT_INSIDE_TOFF;
            break;
          case SSTAT_BRIGHT:
            displayPrintMsg(MSG_BRIGHT);
            if (joystick == JS_UP) substate = SSTAT_CONFIG;
            else if (joystick == JS_RIGHT) substate++;
            else if (joystick == JS_LEFT) substate--;
            else if (joystick == JS_PUSH) substate = SSTAT_INSIDE_BRIGHT;
            break;
          case SSTAT_PERIOD:
            displayPrintMsg(MSG_PERIOD);
            if (joystick == JS_UP) substate = SSTAT_CONFIG;
            else if (joystick == JS_RIGHT) substate++;
            else if (joystick == JS_LEFT) substate--;
            else if (joystick == JS_PUSH) substate = SSTAT_INSIDE_PERIOD;
            break;
          case SSTAT_PVALUE:
            displayPrintMsg(MSG_PVALUE);
            if (joystick == JS_UP) substate = SSTAT_CONFIG;
            else if (joystick == JS_RIGHT) substate++;
            else if (joystick == JS_LEFT) substate--;
            else if (joystick == JS_PUSH) substate = SSTAT_INSIDE_PVALUE;
            break;
          case SSTAT_IVALUE:
            displayPrintMsg(MSG_IVALUE);
            if (joystick == JS_UP) substate = SSTAT_CONFIG;
            else if (joystick == JS_RIGHT) substate++;
            else if (joystick == JS_LEFT) substate--;
            else if (joystick == JS_PUSH) substate = SSTAT_INSIDE_IVALUE;
            break;
          case SSTAT_DVALUE:
            displayPrintMsg(MSG_DVALUE);
            if (joystick == JS_UP) substate = SSTAT_CONFIG;
            else if (joystick == JS_RIGHT) {
                substate++;
                if (substate > SSTAT_LAST_CONFIG) substate = SSTAT_FIRST_CONFIG;
            }
            else if (joystick == JS_LEFT) substate--;
            else if (joystick == JS_PUSH) substate = SSTAT_INSIDE_DVALUE;
            break;
          case SSTAT_INSIDE_MODE:
            displayPrintMsg(modeTmp + MSG_OFF);
            if (joystick == JS_UP) modeTmp++;
            else if (joystick == JS_DOWN) modeTmp--;
            else if (joystick == JS_PUSH) {
                mode = modeTmp;
                EEPROM.put(EEADR_MODE, mode);
                substate = SSTAT_MODE;
            }
            if (modeTmp > MODE_AUTO) modeTmp = MODE_OFF;
            else if (modeTmp < MODE_OFF) modeTmp = MODE_AUTO;
            break;
          case SSTAT_INSIDE_SETPOINT:
            display.showNumberDec(setpointTmp, false, 4, 0);
            if (joystick == JS_UP) setpointTmp++;
            else if (joystick == JS_DOWN) setpointTmp--;
            else if (joystick == JS_PUSH) {
                setpoint = setpointTmp;
                EEPROM.put(EEADR_SETPOINT, setpoint);
                confSet = true;
                EEPROM.put(EEADR_CONFSET, confSet);
                substate = SSTAT_SETPOINT;
            }
            if (setpointTmp > SETPOINT_UP) setpointTmp = SETPOINT_DOWN;
            else if (setpointTmp < SETPOINT_DOWN) setpointTmp = SETPOINT_UP;
            break;
          case SSTAT_INSIDE_TOFF:
            display.showNumberDec(timerOffTmp, false, 4, 0);
            if (joystick == JS_UP) timerOffTmp++;
            else if (joystick == JS_DOWN) timerOffTmp--;
            else if (joystick == JS_PUSH) {
                timerOff = timerOffTmp;
                EEPROM.put(EEADR_TOFF, timerOff);
                substate = SSTAT_TOFF;
            }
            if (timerOffTmp > TOFF_UP) timerOffTmp = TOFF_DOWN;
            else if (timerOffTmp < TOFF_DOWN) timerOffTmp = TOFF_UP;
            break;
          case SSTAT_INSIDE_BRIGHT:
            display.showNumberDec(brightTmp, false, 4, 0);
            if (joystick == JS_UP) brightTmp++;
            else if (joystick == JS_DOWN) brightTmp--;
            else if (joystick == JS_PUSH) {
                bright = brightTmp;
                EEPROM.put(EEADR_BRIGHT, bright);
                substate = SSTAT_BRIGHT;
            }
            if (brightTmp > BRIGHT_UP) brightTmp = BRIGHT_DOWN;
            else if (brightTmp < BRIGHT_DOWN) brightTmp = BRIGHT_UP;
            display.setBrightness(brightTmp);
            break;
          case SSTAT_INSIDE_PERIOD:
            display.showNumberDec(periodTmp, false, 4, 0);
            if (joystick == JS_UP) periodTmp += 10;
            else if (joystick == JS_DOWN) periodTmp -= 10;
            else if (joystick == JS_PUSH) {
                period = periodTmp;
                EEPROM.put(EEADR_PERIOD, period);
                substate = SSTAT_PERIOD;
            }
            if (periodTmp > PERIOD_UP) periodTmp = PERIOD_DOWN;
            else if (periodTmp < PERIOD_DOWN) periodTmp = PERIOD_UP;
            break;
          case SSTAT_INSIDE_PVALUE:
            display.showNumberDecEx(kpTmp * 100, 0xFF, false, 4, 0);
            if (joystick == JS_UP) kpTmp += 0.10;
            else if (joystick == JS_DOWN) kpTmp -= 0.10;
            else if (joystick == JS_PUSH) {
                kp = kpTmp;
                EEPROM.put(EEADR_PVALUE, kp);
                heatingPID.SetTunings(kp, ki, kd);
                substate = SSTAT_PVALUE;
            }
            if (kpTmp > KP_UP) kpTmp = KP_DOWN;
            else if (kpTmp < KP_DOWN) kpTmp = KP_UP;
            break;
          case SSTAT_INSIDE_IVALUE:
            display.showNumberDecEx(kiTmp * 100, 0xFF, false, 4, 0);
            if (joystick == JS_UP) kiTmp += 0.10;
            else if (joystick == JS_DOWN) kiTmp -= 0.10;
            else if (joystick == JS_PUSH) {
                ki = kiTmp;
                EEPROM.put(EEADR_IVALUE, ki);
                heatingPID.SetTunings(kp, ki, kd);
                substate = SSTAT_IVALUE;
            }
            if (kiTmp > KI_UP) kiTmp = KI_DOWN;
            else if (kiTmp < KI_DOWN) kiTmp = KI_UP;
            break;
          case SSTAT_INSIDE_DVALUE:
            display.showNumberDecEx(kdTmp * 100, 0xFF, false, 4, 0);
            if (joystick == JS_UP) kdTmp += 0.10;
            else if (joystick == JS_DOWN) kdTmp -= 0.10;
            else if (joystick == JS_PUSH) {
                kd = kdTmp;
                EEPROM.put(EEADR_DVALUE, kd);
                heatingPID.SetTunings(kp, ki, kd);
                substate = SSTAT_DVALUE;
            }
            if (kdTmp > KD_UP) kdTmp = KD_DOWN;
            else if (kdTmp < KD_DOWN) kdTmp = KD_UP;
            break;
        }
        if ((millis() - prevT_Joystick) > (unsigned long)(timerOff * 1000)) {
            prevT_Joystick = millis();
            state = STAT_STANDBY;
            substate = SSTAT_NONE;
            brightTmp = bright;
            modeTmp = mode;
            setpointTmp = setpoint;
            timerOffTmp = timerOff;
            periodTmp = period;
            kpTmp = kp;
            kiTmp = ki;
            kdTmp = kd;
            displayFadeOff(bright);
        }
        break;
      case STAT_STANDBY:
        if (joystick == JS_PUSH) {
            displayPrintTemp(rTemp);
            state = STAT_IDLE;
            displayFadeOn(bright);
        }
        break;
    }
    // Cada T_READ_SENSOR ms
    if ((millis() - prevT_Update) > T_READ_SENSOR) {
        prevT_Update = millis();
        sensor.requestTemperatures();
        tempC = sensor.getTempC(onewire_address);
        rTemp = (unsigned char)tempC;
        if (mode == MODE_AUTO) {
            pid_sp = setpoint;
            pid_i = tempC;
            heatingPID.Compute();
        }
    }
    // Cada $period s
    if ((millis() - prevT_Send) > (unsigned long)(period * 1000)) {
        prevT_Send = millis();
        if (mode == MODE_AUTO) sendCommand(CMD_RAW, (unsigned char)pid_o);
        else if (mode == MODE_ON) sendCommand(CMD_RAW, 255);
        else if (mode == MODE_OFF) sendCommand(CMD_RAW, 0);
    }
    if (readCommand() == CMD_IS_ALIVE) sendCommand(CMD_HELLO, 0);
}

void displayPrintMsg(unsigned char msg) {
    unsigned char data[4];
    const unsigned char PROGMEM disp_msg[] = {
        0x10, 0x54, 0x10, 0x78,     //init
        0x00, 0x00, 0x63, 0x39,     //  ºC
        0x79, 0x50, 0x50, 0x00,     //Err
        0x00, 0x6D, 0x79, 0x78,     // SEt
        0x00, 0x5C, 0x71, 0x71,     // oFF
        0x00, 0x00, 0x5C, 0x54,     //  on
        0x77, 0x3E, 0x78, 0x5C,     //AUto
        0x39, 0x5C, 0x54, 0x71,     //ConF
        0x55, 0x5C, 0x5E, 0x79,     //modE
        0x6D, 0x79, 0x78, 0x73,     //SEtP
        0x78, 0x5C, 0x71, 0x71,     //toFF
        0x7C, 0x50, 0x10, 0x6F,     //brig
        0x73, 0x79, 0x50, 0x10,     //PEri
        0x00, 0x00, 0x00, 0x73,     //   P
        0x00, 0x00, 0x00, 0x30,     //   I
        0x00, 0x00, 0x00, 0x5E      //   d
    };

    for (unsigned char i = 0; i < 4; i++) {
        data[i] = disp_msg[i + (msg * 4)];
    }
    display.setSegments(data, 4, 0);
}

void displayPrintTemp(unsigned char value) {
    const unsigned char PROGMEM celsius_symbol[] = {
        0x63, 0x39                  //ºC
    };

    display.showNumberDec(value, false, 2, 0);
    display.setSegments(celsius_symbol, 2, 2);
}

void displayFadeOn(unsigned char b) {
    for (unsigned char i = 0; i < (b + 1); i++) {
        display.setBrightness(i);
        delay(50);
    }
}

void displayFadeOff(unsigned char b) {
    for (unsigned char i = b; i < 9; i--) {
        display.setBrightness(i);
        delay(50);
    }
}

void sendCommand(char cmd, unsigned char val) {
    digitalWrite(TX_RX_LED, HIGH);
    Serial.print(CMD_HEADER);
    switch (cmd) {
      case CMD_HELLO:
        Serial.print(CMD_HELLO);
        break;
      case CMD_IS_ALIVE:
        Serial.print(CMD_IS_ALIVE);
        break;
      case CMD_RAW:
        Serial.print(CMD_RAW);
        Serial.print(val, DEC);
        break;
    }
    Serial.println(CMD_FOOTER);
    digitalWrite(TX_RX_LED, LOW);
}

char readCommand() {
    char ret = 0;

    if (Serial.available() > 2) {
        digitalWrite(TX_RX_LED, HIGH);
        if (Serial.read() == CMD_HEADER) {
            ret = Serial.read();
            if (Serial.read() != CMD_FOOTER) ret = 0;
        }
    }
    digitalWrite(TX_RX_LED, LOW);
    return ret;
}

unsigned char readJoystick(unsigned int timeout) {
    boolean quit = false;
    unsigned char currentPush = 0;
    unsigned char ret = JS_NONE;
    static unsigned char prev = 0;
    unsigned int currentX = 0;
    unsigned int currentY = 0;
    unsigned long t = 0;

    t = millis() + timeout;
    while((millis() < t) && (!quit)) {
        currentPush = digitalRead(JOYSTICK_PUSH);
        currentX = analogRead(JOYSTICK_X);
        currentY = analogRead(JOYSTICK_Y);
        if (prev) {
            if ((currentPush) && ( (currentX > 503) && (currentX < 523) ) && ( (currentY > 530)  && (currentY < 550) ) ) {
                ret = prev;
                prev = 0;
                quit = true;
            }
        }
        else {
            if (currentX < 503) prev = JS_LEFT;
            else if (currentX > 523) prev = JS_RIGHT;
            if (currentY < 530) prev = JS_UP;
            else if (currentY > 550) prev = JS_DOWN;
            if (!currentPush) prev = JS_PUSH;
        }
    }
    return ret;
}
