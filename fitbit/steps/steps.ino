/*
 * @author Trent Andraka
 * @author Austin Stover
 * Rudimentary Fitbit with SparkFun
 * Arduino, tracks steps, amount of sleep,
 * and temperature. On Arduino side,
 * gathers and takes in data. Also
 * determines whether subject is
 * in sleep mode based on given threshold,
 * whih can be adjusted
 */

#include <Wire.h>
#include <SparkFun_MMA8452Q.h>

const int SLEEP_PIN = A0;
const int PEDO_PIN = A1;
const int THERMO_PIN = A2;
const int LED_PIN = 13;

const int MAGIC_NUM = '!';
const int DEBUG_STRING = 0x30;
const int ERROR_STRING = 0x31;
const int TIMESTAMP = 0x32;
const int ACCEL_X = 0x33;
const int ACCEL_Y = 0x34;
const int ACCEL_Z = 0x35;
const int NUM_STEPS = 0x36;
const int CONVERTED_TEMP = 0x37;
const int SLEEP_TIME = 0x38;
const int RESET_STEPS = 0x39;

const int SLEEP_THRESHOLD = 2.0;  //Derived from testing of acceleration values
                                  //Represents magnitude, simulates slight tossing/turning
long count = 0;
bool ledOn = false;
unsigned long amountOfSleep = 0;
unsigned long lastTime = 0;

float accelx;
float accely;
float accelz;

MMA8452Q accel;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  analogReference(DEFAULT);
  pinMode(SLEEP_PIN, INPUT_PULLUP);
  pinMode(PEDO_PIN, INPUT_PULLUP);
  pinMode(THERMO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); //pedometer mode by default
  accel.init();
}

void loop() {
  // put your main code here, to run repeatedly:
  
  if (accel.available()) {
    
    accel.read();
    accelx = accel.cx;
    accely = accel.cy;
    accelz = accel.cz;
    float currentConvertedTemp = convertedTemp();
    
    if (accelMag() > 10.0) {
      sendError("YOU ARE OUT OF CONTROL");
    }
    else {
      sendAccelData(accelx, 'x');
      sendAccelData(accely, 'y');
      sendAccelData(accelz, 'z');
      sendTimestamp(millis());
      sendSteps();
    }
    while (!digitalRead(PEDO_PIN)) {
      sendReset();
    }
    
    sendConvertedTemp(currentConvertedTemp);
    if (!digitalRead(SLEEP_PIN)) {
      unsigned long startTime = millis();
      while (digitalRead(PEDO_PIN)) {
        
        digitalWrite(LED_PIN, LOW);
        accel.read();
        accelx = accel.cx;
        accely = accel.cy;
        accelz = accel.cz;
        sendAccelData(accelx, 'x');
        sendAccelData(accely, 'y');
        sendAccelData(accelz, 'z');
        sendTimestamp(millis());
        while (accelMag() > SLEEP_THRESHOLD) {
          accel.read();
          accelx = accel.cx;
          accely = accel.cy;
          accelz = accel.cz;
          sendAccelData(accelx, 'x');
          sendAccelData(accely, 'y');
          sendAccelData(accelz, 'z');
          sendTimestamp(millis());
          sendError("YOU ARE NOT ASLEEP");
          startTime = millis();
        }
        amountOfSleep += millis() - startTime;
        startTime = millis();
        sendSleepTime(amountOfSleep);
        }
      
      sendSleepTime(amountOfSleep);
    }
  }
}

float convertedTemp() {
  float voltage = (5.0/1023) * analogRead(THERMO_PIN);
  float temp = 25 + (voltage - 0.75) * 100;
  return temp;
}

void sendDebug(String message) {
  int len = message.length();
  Serial.write(MAGIC_NUM);
  Serial.write(DEBUG_STRING);
  Serial.write(len >> 8);
  Serial.write(len);
  for (int n = 0; n < len; n++) {
    Serial.write(message.charAt(n));
  }
}

void sendError(String message) {
  int len = message.length();
  Serial.write(MAGIC_NUM);
  Serial.write(ERROR_STRING);
  Serial.write(len >> 8);
  Serial.write(len);
  for (int n = 0; n < len; n++) {
    Serial.write(message.charAt(n));
  }
}

void sendSteps() {
  Serial.write(MAGIC_NUM);
  Serial.write(NUM_STEPS);
}

void sendTimestamp(unsigned long timestamp) {
  Serial.write(MAGIC_NUM);
  Serial.write(TIMESTAMP);
  Serial.write(timestamp >> 24);
  Serial.write(timestamp >> 16);
  Serial.write(timestamp >> 8);
  Serial.write(timestamp);
}

void sendSleepTime(unsigned long timestamp) {
  Serial.write(MAGIC_NUM);
  Serial.write(SLEEP_TIME);
  Serial.write(timestamp >> 24);
  Serial.write(timestamp >> 16);
  Serial.write(timestamp >> 8);
  Serial.write(timestamp);
}

void sendConvertedTemp(float celsius) {
  Serial.write(MAGIC_NUM);
  Serial.write(CONVERTED_TEMP);

  union data {
    float a;
    byte datas[4];
  }
  data;

  data.a = celsius;
  for (int i = 0; i < 4; i++) {
    Serial.write(data.datas[i]);
  }
}

void sendAccelData(float accel, char c) {
  Serial.write(MAGIC_NUM);
  if (c == 'x') {
    Serial.write(ACCEL_X);
  }
  else if (c == 'y') {
    Serial.write(ACCEL_Y);
  }
  else {
    Serial.write(ACCEL_Z);
  }

  union data {
    float a;
    byte datas[4];
  }
  data;

  data.a = accel;
  for (int i = 0; i < 4; i++) {
    Serial.write(data.datas[i]);
  }
}

float accelMag() {
  return sqrt(sq(accelx) + sq(accely) + sq(accelz));
}

void sendReset() {
  Serial.write(MAGIC_NUM);
  Serial.write(RESET_STEPS);
}

