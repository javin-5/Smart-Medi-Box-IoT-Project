#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Display settings
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_RESET_PIN -1
#define DISPLAY_I2C_ADDRESS 0x3C

// User input devices
#define BUZZER_PIN 5
#define STATUS_LED_PIN 15
#define BUTTON_CANCEL 34
#define BUTTON_OK 32
#define BUTTON_UP 33
#define BUTTON_DOWN 35

// Sensor pins
#define DHT_SENSOR_PIN 12
#define LIGHT_SENSOR_PIN_LEFT A0
#define LIGHT_SENSOR_PIN_RIGHT A3
#define SERVO_PIN 16
#define TEMP_WARNING_LED_PIN 27
#define HUMIDITY_WARNING_LED_PIN 26
#define OPERATION_INDICATOR_LED 14

char temperatureBuffer[6];

#define NTP_POOL_SERVER "pool.ntp.org"
#define TIME_ZONE_OFFSET 0
#define DST_OFFSET 0

// Networking
WiFiClient networkClient;
PubSubClient clientMQTT(networkClient);
WiFiUDP udpNTP;
NTPClient clientNTP(udpNTP);

float timeZone = TIME_ZONE_OFFSET;

float correctionFactor = 0.75;
float calculatedLux = 0;
float loadResistor = 50;
float minimumServoAngle = 30;

char temperatureData[10];
char lightIntensityData[10];
char servoPositionData[10];

// Time variables
int currentDay = 0;
int currentHour = 0;
int currentMinute = 0;
int currentSecond = 0;
int currentMonth = 0;
int currentYear = 0;

int lightLevel = 0;
int servoAngle = 0;
bool isAlarmActive = true;
int numberOfAlarms = 3;
int alarmHours[] = {0, 0, 1};
int alarmMinutes[] = {1, 2, 10};
bool alarmsTriggered[] = {false, false, false};

bool isTimerSet = false;
unsigned long scheduledOnTime;

int toneC = 262;
int toneD = 296;
int toneE = 330;
int toneF = 350;
int toneG = 392;
int toneA = 440;
int toneB = 492;
int toneHighC = 532;

int melodyNotes[] = {toneC, toneD, toneE, toneF, toneG, toneA, toneB, toneHighC};

int totalNotes = 8;

int currentMenuOption = 0;
int totalMenuOptions = 5;
String menuOptions[] = {"Set Time Zone", "Set Alarm 1", "Set Alarm 2", "Set Alarm 3", "Disable Alarms", "Set Time Zone"};

Adafruit_SSD1306 oledDisplay(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, DISPLAY_RESET_PIN);
DHTesp dhtSensor;

Servo servoMotor;

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(BUTTON_CANCEL, INPUT);
  pinMode(BUTTON_OK, INPUT);
  pinMode(BUTTON_UP, INPUT);
  pinMode(BUTTON_DOWN, INPUT);
  pinMode(LIGHT_SENSOR_PIN_LEFT, INPUT);
  pinMode(LIGHT_SENSOR_PIN_RIGHT, INPUT);
  pinMode(HUMIDITY_WARNING_LED_PIN, OUTPUT);
  pinMode(TEMP_WARNING_LED_PIN, OUTPUT);
  pinMode(OPERATION_INDICATOR_LED, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  dhtSensor.setup(DHT_SENSOR_PIN, DHTesp::DHT22);
  servoMotor.attach(SERVO_PIN, 500, 2400);

  Serial.begin(115200);
  if (!oledDisplay.begin(SSD1306_SWITCHCAPVCC, DISPLAY_I2C_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) ;
  }
  oledDisplay.display();
  delay(2000);

  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    oledDisplay.clearDisplay();
    displayText("WIFI Connecting... ", 0, 0, 2);
  }
  oledDisplay.clearDisplay();
  Serial.println("IP Address");
  Serial.println(String(WiFi.localIP()));
  displayText("WIFI Connected ", 0, 0, 2);
  Serial.println("WIFI Connected");
  delay(1000);

  oledDisplay.clearDisplay();
  setupMQTT();
  clientNTP.begin();
  clientNTP.setTimeOffset(5.5 * 3600);
  displayText("Welcome to", 0, 6, 2);
  displayText("Medibox", 20, 22, 2);
  delay(2000);
  oledDisplay.clearDisplay();

  configTime(timeZone * 3600, DST_OFFSET * 3600, NTP_POOL_SERVER);
}

void loop() {
  if (!clientMQTT.connected()) {
    connectToMQTTBroker();
  }
  clientMQTT.loop();
  updateAndCheckAlarms();
  checkTemperatureAndHumidity();
  if (digitalRead(BUTTON_OK) == LOW) {
    delay(500);
    navigateMenu();
  }

  clientMQTT.publish("Luminous Intensity", lightIntensityData);
  clientMQTT.publish("Temperature", temperatureData);
  clientMQTT.publish("Position", servoPositionData);
  displayDateTimeAndSensorData();

  readFromLightSensor();
  adjustServo();
  checkScheduledTimer();
}

void displayText(String text, int column, int row, int size) {
  oledDisplay.setTextSize(size);
  oledDisplay.setTextColor(SSD1306_WHITE);
  oledDisplay.setCursor(column, row);
  oledDisplay.println(text);
  oledDisplay.display();
}

void displayDateTimeAndSensorData() {
  displayText("Time: " + String(currentHour) + ":" + String(currentMinute) + ":" + String(currentSecond), 0, 0, 1);
  displayText("Date: " + String(currentMonth) + "/" + String(currentDay) + "/" + String(currentYear), 0, 10, 1);

  // Check and display temperature and humidity data
  TempAndHumidity sensorData = dhtSensor.getTempAndHumidity();
  if (sensorData.temperature > 32) {
    digitalWrite(TEMP_WARNING_LED_PIN, HIGH);
    displayText("TEMP HIGH: " + String(sensorData.temperature) + " C", 0, 20, 1);
  } else if (sensorData.temperature < 26) {
    digitalWrite(TEMP_WARNING_LED_PIN, HIGH);
    displayText("TEMP LOW: " + String(sensorData.temperature) + " C", 0, 20, 1);
  } else {
    digitalWrite(TEMP_WARNING_LED_PIN, LOW);
  }

  if (sensorData.humidity > 80) {
    digitalWrite(HUMIDITY_WARNING_LED_PIN, HIGH);
    displayText("HUMIDITY HIGH: " + String(sensorData.humidity) + "%", 0, 30, 1);
  } else if (sensorData.humidity < 60) {
    digitalWrite(HUMIDITY_WARNING_LED_PIN, HIGH);
    displayText("HUMIDITY LOW: " + String(sensorData.humidity) + "%", 0, 30, 1);
  } else {
    digitalWrite(HUMIDITY_WARNING_LED_PIN, LOW);
  }

  String(sensorData.temperature, 2).toCharArray(temperatureData, 10);
}

void updateTime() {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  currentHour = timeInfo.tm_hour;
  currentMinute = timeInfo.tm_min;
  currentSecond = timeInfo.tm_sec;
  currentDay = timeInfo.tm_mday; // tm_mday is the day of the month
  currentMonth = timeInfo.tm_mon + 1; // tm_mon is months since January, so add 1
  currentYear = timeInfo.tm_year + 1900; // tm_year is years since 1900
}

void updateAndCheckAlarms() {
  updateTime();
  displayDateTimeAndSensorData();
  if (isAlarmActive) {
    for (int i = 0; i < numberOfAlarms; i++) {
      if (!alarmsTriggered[i] && alarmHours[i] == currentHour && alarmMinutes[i] == currentMinute) {
        triggerAlarm();
        alarmsTriggered[i] = true;
      }
    }
  }
}

void triggerAlarm() {
  displayText("Alarm Triggered!", 0, 0, 2);
  digitalWrite(STATUS_LED_PIN, HIGH);
  for (int i = 0; i < totalNotes; i++) {
    tone(BUZZER_PIN, melodyNotes[i], 500); // Play tone for 500 ms
    delay(500); // Wait 500 ms
  }
  digitalWrite(STATUS_LED_PIN, LOW);
}

void navigateMenu() {
  int buttonState = digitalRead(BUTTON_CANCEL);
  while (buttonState == HIGH) {
    displayText(menuOptions[currentMenuOption], 0, 0, 2);
    if (digitalRead(BUTTON_UP) == LOW) {
      currentMenuOption = (currentMenuOption + 1) % totalMenuOptions;
      delay(200); // Debounce
    } else if (digitalRead(BUTTON_DOWN) == LOW) {
      currentMenuOption = (currentMenuOption == 0) ? totalMenuOptions - 1 : currentMenuOption - 1;
      delay(200); // Debounce
    } else if (digitalRead(BUTTON_OK) == LOW) {
      operateCurrentMode(currentMenuOption);
      delay(200); // Debounce
    }
    buttonState = digitalRead(BUTTON_CANCEL);
  }
}

void operateCurrentMode(int mode) {
  switch (mode) {
  case 0:
    setAlarm(1);
    break;
  case 1:
    setAlarm(2);
    break;
  case 2:
    setAlarm(3);
    break;
  case 3:
    isAlarmActive = !isAlarmActive;
    displayText(isAlarmActive ? "Alarm Enabled" : "Alarm Disabled", 0, 0, 2);
    delay(1000);
    break;
  case 4:
    setTime();
    break;
  default:
    // Do nothing
    break;
  }
}

void setTime() {
  float tempTimeZone = timeZone;
  while (true) {
    displayText("Set Time Zone: " + String(tempTimeZone), 0, 0, 2);
    if (digitalRead(BUTTON_UP) == LOW) {
      tempTimeZone += 0.5;
      if (tempTimeZone > 14) tempTimeZone = 0;
      delay(200); // Debounce
    } else if (digitalRead(BUTTON_DOWN) == LOW) {
      tempTimeZone -= 0.5;
      if (tempTimeZone < 0) tempTimeZone = 14;
      delay(200); // Debounce
    } else if (digitalRead(BUTTON_OK) == LOW) {
      timeZone = tempTimeZone;
      configTime(timeZone * 3600, DST_OFFSET * 3600, NTP_POOL_SERVER);
      break;
    } else if (digitalRead(BUTTON_CANCEL) == LOW) {
      break;
    }
  }
}

void setAlarm(int alarm) {
  int tempHour = alarmHours[alarm];
  while (true) {
    displayText("Set Hour: " + String(tempHour), 0, 0, 2);
    if (digitalRead(BUTTON_UP) == LOW) {
      tempHour = (tempHour + 1) % 24;
      delay(200); // Debounce
    } else if (digitalRead(BUTTON_DOWN) == LOW) {
      tempHour = (tempHour == 0) ? 23 : tempHour - 1;
      delay(200); // Debounce
    } else if (digitalRead(BUTTON_OK) == LOW) {
      alarmHours[alarm] = tempHour;
      break;
    }
  }
  int tempMinute = alarmMinutes[alarm];
  while (true) {
    displayText("Set Minute: " + String(tempMinute), 0, 0, 2);
    if (digitalRead(BUTTON_UP) == LOW) {
      tempMinute = (tempMinute + 1) % 60;
      delay(200); // Debounce
    } else if (digitalRead(BUTTON_DOWN) == LOW) {
      tempMinute = (tempMinute == 0) ? 59 : tempMinute - 1;
      delay(200); // Debounce
    } else if (digitalRead(BUTTON_OK) == LOW) {
      alarmMinutes[alarm] = tempMinute;
      break;
    }
  }
}

void checkTemperatureAndHumidity() {
  TempAndHumidity sensorData = dhtSensor.getTempAndHumidity();
  String(sensorData.temperature, 2).toCharArray(temperatureData, 10);

  if (sensorData.temperature >= 32) {
    displayText("TEMP HIGH", 0, 20, 1);
    digitalWrite(TEMP_WARNING_LED_PIN, HIGH);
  } else if (sensorData.temperature <= 26) {
    displayText("TEMP LOW", 0, 20, 1);
    digitalWrite(TEMP_WARNING_LED_PIN, HIGH);
  } else {
    digitalWrite(TEMP_WARNING_LED_PIN, LOW);
  }

  if (sensorData.humidity >= 80) {
    displayText("HUMIDITY HIGH", 0, 30, 1);
    digitalWrite(HUMIDITY_WARNING_LED_PIN, HIGH);
  } else if (sensorData.humidity <= 60) {
    displayText("HUMIDITY LOW", 0, 30, 1);
    digitalWrite(HUMIDITY_WARNING_LED_PIN, HIGH);
  } else {
    digitalWrite(HUMIDITY_WARNING_LED_PIN, LOW);
  }
}

void readFromLightSensor() {
  int leftValue = analogRead(LIGHT_SENSOR_PIN_LEFT);
  int rightValue = analogRead(LIGHT_SENSOR_PIN_RIGHT);
  int chosenValue = (leftValue < rightValue) ? leftValue : rightValue;
  lightLevel = (chosenValue == leftValue) ? 10 : 100;
  String(lightLevel).toCharArray(servoPositionData, 10);

  float voltage = chosenValue / 1024.0 * 5.0;
  float resistance = 10000 * voltage / (5 - voltage);
  float maximumLux = pow(loadResistor * 1000 * pow(10, correctionFactor) / 500.0, 1 / correctionFactor);
  calculatedLux = pow(loadResistor * 1000 * pow(10, correctionFactor) / resistance, 1 / correctionFactor) / maximumLux;
  String(calculatedLux, 2).toCharArray(lightIntensityData, 10);
}

void adjustServo() {
  float targetAngle = minimumServoAngle + (180 - minimumServoAngle) * calculatedLux * correctionFactor;
  servoAngle = (targetAngle < 180) ? targetAngle : 180;
  Serial.println(servoAngle);
  servoMotor.write(servoAngle);
}

void checkScheduledTimer() {
  if (isTimerSet) {
    unsigned long currentEpoch = clientNTP.getEpochTime();
    if (currentEpoch > scheduledOnTime) {
      triggerAlarm();
      isTimerSet = false;
      clientMQTT.publish("Scheduled Alarm", "1");
      clientMQTT.publish("Scheduled Alarm", "0");
    }
  }
}

void setupMQTT() {
  clientMQTT.setServer("test.mosquitto.org", 1883);
  clientMQTT.setCallback(mqttCallback);
}

void connectToMQTTBroker() {
  while (!clientMQTT.connected()) {
    Serial.println("Attempting to connect to MQTT broker...");
    if (clientMQTT.connect("ESP32-24244555444")) {
      Serial.println("Connected to MQTT broker");
      clientMQTT.subscribe("Slider");
      clientMQTT.subscribe("Controlling factor");
      clientMQTT.subscribe("Alarm Time");
      clientMQTT.subscribe("Scheduled Alarm");
    } else {
      Serial.print("MQTT connection failed, state: ");
      Serial.println(clientMQTT.state());
      delay(5000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  char msg[length + 1];
  for (int i = 0; i < length; i++) {
    msg[i] = (char)payload[i];
  }
  msg[length] = '\0'; // Null-terminate the string
  Serial.println(msg);

  if (strcmp(topic, "Slider") == 0) {
    minimumServoAngle = atoi(msg);
  } else if (strcmp(topic, "Controlling factor") == 0) {
    correctionFactor = atof(msg);
  } else if (strcmp(topic, "Alarm Time") == 0) {
    processAlarmTime(msg);
  } else if (strcmp(topic, "Scheduled Alarm") == 0) {
    processScheduledAlarm(msg);
  }
}

void processAlarmTime(char* data) {
  if (data[0] == 't') {
    triggerAlarm();
  }
}

void processScheduledAlarm(char* data) {
  if (data[0] == 'N') {
    isTimerSet = false;
  } else {
    isTimerSet = true;
    scheduledOnTime = atol(data + 1);
  }
}
