#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <LiquidCrystal_I2C.h>

//---------------------define OLED parameters-----------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3c
//---------------------------buttons--------------------------------
#define BUZZER 5
#define LED_1 15
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35
#define DHTPIN 12
#define TEMP_LED 27
#define HUMIDITY_LED 26
#define GOOD_LED 14

//---------------------time update from WIFI------------------------
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 5.5 * 60 * 60;
const int daylightOffset_sec = 0;
//---------------------------notes----------------------------------
int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C, D, E, F, G, A, B, C_H};
//------------------------declare objects---------------------------
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor;
//------------------------global variables--------------------------
int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;
unsigned long timeNow = 0;
unsigned long timeLast = 0;
bool alarm_enabled = true;
int n_alarms = 3;
int alarm_hours[] = {0, 0, 0};
int alarm_minutes[] = {0, 0, 0};
bool alarm_triggered[] = {false, false, false};
bool good_levels = false; 
int current_mode = 0;
int max_modes = 5;
String modes[] = {"1 - Set   Time Zone",
  "2 - Set   Alarm 1",
  "3 - Set   Alarm 2",
  "4 - Set   Alarm 3",
  "5-Disable all Alarms"};
int gmt_hour = 0;
float gmt_minute = 0;
float gmt_offset = 5.5 * 60 * 60;
int ButtonDebounceTime = 200;
char Temp_Array[6]; 


void setup() {
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);
  pinMode(HUMIDITY_LED, OUTPUT);
  pinMode(TEMP_LED, OUTPUT);
  pinMode(GOOD_LED, OUTPUT);
  dhtSensor.setup(DHTPIN, DHTesp::DHT22);
  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed!"));
    for (;;);
  }
  display.display();
  delay(1000);
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("WIFI \nConnecting... ", 0, 0, 2);
  }
  display.clearDisplay();
  print_line("WIFI \nConnected ", 0, 0, 2);
  delay(500);


  
  display.clearDisplay();
  print_line("Welcome to", 0, 6, 2);
  print_line("Medibox!", 20, 22, 2);
  delay(1000);
  display.clearDisplay();

}
void loop() {
  update_time_with_check_alarm();
  check_temp_humidity();

  if (alarm_enabled) {
    for (int i = 0; i < n_alarms; i++) {
      if (!alarm_triggered[i] && alarm_hours[i] == hours && alarm_minutes[i] == minutes) {
        ring_alarm();
        alarm_triggered[i] = false;
      }
    }
  }

  if (digitalRead(PB_OK) == LOW) {
    delay(ButtonDebounceTime);
    go_to_menu();
  }
}

void updateTemp() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  String(data.temperature, 2).toCharArray(Temp_Array, 6);
}


void buzzer_on(bool on) {
  if (on) {
    tone(BUZZER, 255);
  } else {
    noTone(BUZZER);
  }
}


//-----------------------display and alarms--------------------------
void print_line(String text, int column, int row, int text_size) {
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row); //(column, row)
  display.println(text);
  display.display();
}
void print_time_now(void) {
  display.clearDisplay();

  String minutesStr = (minutes < 10) ? "0" + String(minutes) : String(minutes);

  print_line(String(days), 0, 0, 2);
  print_line(":", 20, 0, 2);
  print_line(String(hours), 30, 0, 2);
  print_line(":", 50, 0, 2);
  print_line(minutesStr, 60, 0, 2);
  print_line(":", 80, 0, 2);
  print_line(String(seconds), 90, 0, 2);
}
//----------------------------update time---------------------------------
void update_time() {
  configTime(gmt_offset, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  hours = atoi(timeHour);
  char timeMinute[3];
  strftime(timeMinute, 3, "%M", &timeinfo);
  minutes = atoi(timeMinute);
  char timeSecond[3];
  strftime(timeSecond, 3, "%S", &timeinfo);
  seconds = atoi(timeSecond);
  char timeDay[3];
  strftime(timeDay, 3, "%d", &timeinfo);
  days = atoi(timeDay);
}
bool break_happened = false;
//-----------------------------ring alarm---------------------------------
void ring_alarm() {
  display.clearDisplay();
  print_line("Medicine Time", 0, 0, 2);
  digitalWrite(LED_1, HIGH);
  
  while (break_happened == false  && digitalRead(PB_CANCEL) == HIGH) {
    for (int i = 0; i < n_notes; i++) {
      if (digitalRead(PB_CANCEL) == LOW) {
        delay(ButtonDebounceTime);
        break_happened = true;
        break;
      }
      tone(BUZZER, notes[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);
    }
  }
  display.clearDisplay();
  print_line("ALARM OFF", 25, 25, 1);
  digitalWrite(LED_1, LOW);
  display.clearDisplay();
}
void update_time_with_check_alarm(void) {
  update_time();
  print_time_now();
  if (alarm_enabled == true) {
    for (int i = 0; i < n_alarms; i++) {
      if (alarm_triggered[i] == false && alarm_hours[i] == hours && alarm_minutes[i] == minutes) {
        ring_alarm();
        alarm_triggered[i] = false;
      }
    }
  }
}
//------------------------------button press-------------------------------
int wait_for_button_press() {
  while (true) {
    if (digitalRead(PB_UP) == LOW) {
      delay(ButtonDebounceTime);
      return PB_UP;
    }
    else if (digitalRead(PB_DOWN) == LOW) {
      delay(ButtonDebounceTime);
      return PB_DOWN;
    }
    else if (digitalRead(PB_OK) == LOW) {
      delay(ButtonDebounceTime);
      return PB_OK;
    }
    else if (digitalRead(PB_CANCEL) == LOW) {
      delay(ButtonDebounceTime);
      return PB_CANCEL;
    }
    update_time();
  }
}
//--------------------------------menu------------------------------------
void go_to_menu() {
  display.clearDisplay();
  print_line("MENU", 40, 20, 2);
  delay(1000);

  while (digitalRead(PB_CANCEL) == HIGH) {
    display.clearDisplay();
    print_line(modes[current_mode], 0, 0, 2);
    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      current_mode += 1;
      current_mode %= max_modes;
    }
    else if (pressed == PB_DOWN) {
      current_mode -= 1;
      if (current_mode < 0) {
        current_mode = max_modes - 1;
      }
    }
    else if (pressed == PB_OK) {
      run_mode(current_mode);
    }
    else if (pressed == PB_CANCEL) {
      break;
    }
  }
}
//------------------------------set time zone-----------------------------
void set_time_zone() {
  int temp_hour = gmt_hour;
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);
    int pressed = wait_for_button_press();

    if (pressed == PB_UP) {
      temp_hour += 1;

      if (temp_hour > 12) {
        temp_hour = -12;
      }
    }
    else if (pressed == PB_DOWN) {
      temp_hour -= 1;
      if (temp_hour < -12) {
        temp_hour = 12;
      }
    }
    else if (pressed == PB_OK) {
      hours = temp_hour;
      break;
    }
    else if (pressed == PB_CANCEL) {
      break;
    }
  }
  float temp_minute = gmt_minute;
  while (true) {
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minute), 0, 0, 2);
    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      temp_minute += 30;
      if (temp_minute > 30) {
        temp_minute = 0;
      }
    }
    else if (pressed == PB_DOWN) {
      temp_minute += 30;
      if (temp_minute > 30) {
        temp_minute = 0;
      }
    }
    else if (pressed == PB_OK) {
      minutes = temp_minute;
      display.clearDisplay();
      print_line("Time zone is set", 0, 0, 2);
      delay(1000);
      break;
    }
    else if (pressed == PB_CANCEL) {
      display.clearDisplay();
      print_line("Cancelled", 0, 0, 2);
      delay(1000);
      break;
    }
  }
  gmt_hour = temp_hour;
  gmt_minute = gmt_minute;
  gmt_offset = (gmt_hour + (gmt_minute / 60)) * 60 * 60;
}
//-----------------------------set alarm----------------------------------
void set_alarm(int alarm) {
  alarm_enabled = true;
  int temp_hour = alarm_hours[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);
    int pressed = wait_for_button_press();

    if (pressed == PB_UP) {
      temp_hour += 1;
      temp_hour %= 24;
    }
    else if (pressed == PB_DOWN) {
      temp_hour -= 1;
      if (temp_hour < 0) {
        temp_hour = 23;
      }
    }
    else if (pressed == PB_OK) {
      alarm_hours[alarm] = temp_hour;
      break;
    }
    else if (pressed == PB_CANCEL) {
      break;
    }
  }
  int temp_minute = alarm_minutes[alarm];
  while (true) {
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minute), 0, 0, 2);
    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      temp_minute += 1;
      temp_minute %= 60;
    }
    else if (pressed == PB_DOWN) {
      temp_minute -= 1;
      if (temp_minute < 0) {
        temp_minute = 59;
      }
    }
    else if (pressed == PB_OK) {
      alarm_minutes[alarm] = temp_minute;
      display.clearDisplay();
      print_line("Alarm is set", 0, 0, 2);
      delay(1000);
      break;
    }
    else if (pressed == PB_CANCEL) {
      display.clearDisplay();
      print_line("Cancelled", 0, 0, 2);
      delay(1000);
      break;
    }
  }
  alarm_triggered[alarm] = false;
}
//-------------------------------run mode----------------------------
void run_mode(int mode) {
  if (mode == 0) {
    set_time_zone();
  }
  else if (mode == 1 || mode == 2 || mode == 3) {
    set_alarm(mode - 1);
  }
  else if (mode == 4) {
    alarm_enabled = false;
    display.clearDisplay();
    print_line("All Alarms Disabled!", 0, 0, 2);
    delay(1000);
  }
}
//-----------------check temperature and humidity-------------------
void check_temp_humidity() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  String(data.temperature, 2).toCharArray(Temp_Array, 6);

  if (data.temperature >= 32) {
    display.clearDisplay();
    print_line("TEMP HIGH", 0, 40, 1);
    digitalWrite(TEMP_LED, HIGH);
    digitalWrite(GOOD_LED, LOW); 
    delay(200); 
  } else if (data.temperature <= 26) {
    display.clearDisplay();
    print_line("TEMP LOW", 0, 40, 1);
    digitalWrite(TEMP_LED, HIGH);
    digitalWrite(GOOD_LED, LOW);
    delay(200); 
  } else {
    digitalWrite(TEMP_LED, LOW);
  }

  // Humidity limits
  if (data.humidity >= 80) {
    display.clearDisplay();
    print_line("HUMIDITY HIGH", 0, 50, 1);
    digitalWrite(HUMIDITY_LED, HIGH);
    digitalWrite(GOOD_LED, LOW); 
    delay(200); 
  } else if (data.humidity <= 60) {
    display.clearDisplay();
    print_line("HUMIDITY LOW", 0, 50, 1);
    digitalWrite(HUMIDITY_LED, HIGH);
    digitalWrite(GOOD_LED, LOW);
    delay(200); 
  } else {
    digitalWrite(HUMIDITY_LED, LOW);
  }

  if (digitalRead(TEMP_LED) == LOW && digitalRead(HUMIDITY_LED) == LOW) {
    print_line("GOOD LEVELS", 0, 50, 1);
    digitalWrite(GOOD_LED, HIGH); 
    delay(200);
  } else {
    digitalWrite(GOOD_LED, LOW); 
  }
}

