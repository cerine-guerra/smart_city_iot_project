#define BLYNK_TEMPLATE_ID   "TMPL24vL8jmOp"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN    "ljbuc64HynZqNUZ_oZxaaXgUjak_QGH4"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>
#include "DHTesp.h"


char ssid[] = "QT";
char pass[] = "Hellowilly";


LiquidCrystal_I2C lcd(0x27, 16, 2);
const int DHT_PIN       = 15;
DHTesp   dhtSensor;

#define ServoPin        13
#define IRPin           33
#define LEDPin          4
#define MQ2_PIN         34
#define FLAME_PIN       32
#define BUZZER_PIN      26

// إشارات المرور
#define RED_LIGHT_PIN     14
#define YELLOW_LIGHT_PIN  12
#define GREEN_LIGHT_PIN   27

// عتبات الإنذار
#define MQ2_THRESHOLD      3000
#define TEMP_THRESHOLD     40.0

// نبضات PWM (دورة مستمرة)
const int SERVO_STOP_PULSE  = 1500;
const int SERVO_OPEN_PULSE  = 1300;
const int SERVO_CLOSE_PULSE = 1700;

Servo myservo;


bool    irEnabled        = false;
bool    gateOpen         = false;
bool    closingScheduled = false;
unsigned long closeTime  = 0;

bool    dhtEnabled       = false;
unsigned long lastDHTTime = 0;

bool    mq2Enabled       = false;
unsigned long lastMQ2Time = 0;

bool    flameEnabled     = false;
unsigned long lastFlameTime = 0;


bool gasAlarm   = false;
bool tempAlarm  = false;
bool flameAlarm = false;


bool gasNotified   = false;
bool tempNotified  = false;
bool flameNotified = false;


bool trafficLightOn    = false;
unsigned long lastTrafficTime = 0;
int trafficState       = 0;

void setup() {
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);


  dhtSensor.setup(DHT_PIN, DHTesp::DHT11);


  myservo.attach(ServoPin);
  myservo.writeMicroseconds(SERVO_STOP_PULSE);
  delay(500);
  myservo.detach();


  lcd.init();
  lcd.backlight();


  pinMode(IRPin, INPUT);
  pinMode(LEDPin, OUTPUT);
  pinMode(MQ2_PIN, INPUT);
  pinMode(FLAME_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(RED_LIGHT_PIN, OUTPUT);
  pinMode(YELLOW_LIGHT_PIN, OUTPUT);
  pinMode(GREEN_LIGHT_PIN, OUTPUT);
}


void openGate() {
  myservo.attach(ServoPin);
  myservo.writeMicroseconds(SERVO_OPEN_PULSE);
  delay(500);
  myservo.writeMicroseconds(SERVO_STOP_PULSE);
  myservo.detach();
  lcd.clear(); lcd.setCursor(0,0);
  lcd.print("Gate Open");
  gateOpen = true;
}


void closeGate() {
  myservo.attach(ServoPin);
  myservo.writeMicroseconds(SERVO_CLOSE_PULSE);
  delay(500);
  myservo.writeMicroseconds(SERVO_STOP_PULSE);
  myservo.detach();
  lcd.clear(); lcd.setCursor(0,0);
  lcd.print("Gate Closed");
  gateOpen = false;
}


void checkDHTSensor() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  Serial.printf("Temp: %.2f C, Hum: %.1f %%\n", data.temperature, data.humidity);

  char buf[17];
  snprintf(buf, sizeof(buf), "Temp: %.2fC", data.temperature);
  lcd.clear(); lcd.setCursor(0,0); lcd.print(buf);
  snprintf(buf, sizeof(buf), "Hum: %.1f%%", data.humidity);
  lcd.setCursor(0,1); lcd.print(buf);


  tempAlarm = (data.temperature > TEMP_THRESHOLD);


  if (tempAlarm && !tempNotified) {
    Blynk.logEvent("temp_alert", String("⚠️ High Temp: ") + String(data.temperature,1) + "C");
    tempNotified = true;
  }
  if (!tempAlarm) {
    tempNotified = false;
  }
}


void checkMQ2Sensor() {
  int val = analogRead(MQ2_PIN);
  Serial.printf("MQ2: %d\n", val);

  lcd.clear(); lcd.setCursor(0,0);
  lcd.print("Gas:"); lcd.setCursor(5,0); lcd.print(val);

  gasAlarm = mq2Enabled && (val > MQ2_THRESHOLD);


  if (gasAlarm && !gasNotified) {
    Blynk.logEvent("gas_alert", String("⚠️ Gas level: ") + String(val));
    gasNotified = true;
  }
  if (!gasAlarm) {
    gasNotified = false;
  }
}


void checkFlameSensor() {
  int state = digitalRead(FLAME_PIN);

  flameAlarm = flameEnabled && (state == HIGH);

  if (flameAlarm) {
    lcd.clear(); lcd.setCursor(0,0);
    lcd.print("Flame Detected!");
  }


  if (flameAlarm && !flameNotified) {
    Blynk.logEvent("flame_alert", "🔥 Flame detected!");
    flameNotified = true;
  }
  if (!flameAlarm) {
    flameNotified = false;
  }
}


void updateBuzzer() {
  if (gasAlarm || tempAlarm || flameAlarm) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}


BLYNK_WRITE(V0) {
  dhtEnabled = param.asInt();
  if (dhtEnabled) {
    checkDHTSensor();
    lastDHTTime = millis();
  } else {
    lcd.clear();
    tempAlarm = false;
    tempNotified = false;
  }
}

BLYNK_WRITE(V1) {
  irEnabled = param.asInt();
  if (!irEnabled) {
    closingScheduled = false;
    gateOpen = false;
    myservo.attach(ServoPin);
    myservo.writeMicroseconds(SERVO_STOP_PULSE);
    delay(50);
    myservo.detach();
    lcd.clear();
  }
}

BLYNK_WRITE(V2) {
  mq2Enabled = param.asInt();
  if (!mq2Enabled) {
    lcd.clear();
    gasAlarm = false;
    gasNotified = false;
  }
}

BLYNK_WRITE(V3) {
  flameEnabled = param.asInt();
  if (!flameEnabled) {
    flameAlarm = false;
    flameNotified = false;
    lcd.clear();
  }
}

BLYNK_WRITE(V4) {
  trafficLightOn = param.asInt();
  if (!trafficLightOn) {
    digitalWrite(RED_LIGHT_PIN, LOW);
    digitalWrite(YELLOW_LIGHT_PIN, LOW);
    digitalWrite(GREEN_LIGHT_PIN, LOW);
  }
}

BLYNK_WRITE(V5) {
  int ledState = param.asInt();
  digitalWrite(LEDPin, ledState);
}

void loop() {
  Blynk.run();
  unsigned long now = millis();


  if (dhtEnabled && now - lastDHTTime >= 3000) {
    lastDHTTime = now;
    checkDHTSensor();
  }

  if (irEnabled) {
    int state = digitalRead(IRPin);
    if (state == LOW && !gateOpen) {
      openGate();
      closingScheduled = false;
    } else if (state == HIGH && gateOpen && !closingScheduled) {
      closingScheduled = true;
      closeTime = now;
      lcd.clear(); lcd.setCursor(0,0); lcd.print("Closing in 3s");
    }
    if (closingScheduled && now - closeTime >= 3000) {
      closeGate();
      closingScheduled = false;
    }
  }

  if (mq2Enabled && now - lastMQ2Time >= 1000) {
    lastMQ2Time = now;
    checkMQ2Sensor();
  }

  if (flameEnabled && now - lastFlameTime >= 1000) {
    lastFlameTime = now;
    checkFlameSensor();
  }

  updateBuzzer();

  if (trafficLightOn && now - lastTrafficTime >= 2000) {
    lastTrafficTime = now;
    switch (trafficState) {
      case 0:
        digitalWrite(RED_LIGHT_PIN, HIGH);
        digitalWrite(YELLOW_LIGHT_PIN, LOW);
        digitalWrite(GREEN_LIGHT_PIN, LOW);
        trafficState = 1;
        break;
      case 1:
        digitalWrite(RED_LIGHT_PIN, LOW);
        digitalWrite(YELLOW_LIGHT_PIN, HIGH);
        digitalWrite(GREEN_LIGHT_PIN, LOW);
        trafficState = 2;
        break;
      case 2:
        digitalWrite(RED_LIGHT_PIN, LOW);
        digitalWrite(YELLOW_LIGHT_PIN, LOW);
        digitalWrite(GREEN_LIGHT_PIN, HIGH);
        trafficState = 0;
        break;
    }
  }
}