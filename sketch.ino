//Libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0X3C

#define BUZZER 5
#define LED_1 15
#define LED_2 2
#define PB_CANCEL 34
#define PB_DOWN 35
#define PB_OK 32
#define PB_UP 33
#define DHTPIN 12

#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET     0
#define UTC_OFFSET_DST 0

//Declare objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor;

//Global Variables
int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;
int hours_UTC_offset = 0;
int minutes_UTC_offset = 0;


int displayDay = -1;
int displayHour = -1;
int displayMinute = -1;
int displaySecond = -1;

int tempHighFlag = 0;
int tempLowFlag = 0;
int humHighFlag = 0;
int humLowFlag = 0;


//unsigned long timeNow = 0;
//unsigned long timeLast = 0;

bool alarm_enabled = true;
int n_alarms = 3;
int alarm_hours[] = {-1, -1, -1};
int alarm_minutes[] = {-1, -1, -1};
bool alarm_triggered[] = {false, false, false};

int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int NOTES[] = {C, D, E, F, G, A, B, C_H};
int NO_NOTES = 8;

int current_mode = 0;
int max_modes = 5;

String modes[] = {"1-Set Time ", "2-Set Alarm 1", "3- Set Alarm 2", "4- Set Alarm 3", "5 - Disable Alarms"};





void setup() {

  //Declare the pin modes of ESP32 
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);

  dhtSensor.setup(DHTPIN, DHTesp::DHT22);

  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.display();
  delay(2000); //delay for 2 seconds

  //Connect to the wifi
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connection to Wifi ", 0, 0, 2);
  }

  display.clearDisplay();
  print_line("Connected to Wifi ", 0, 0, 2);

  //We get the UTC using the Internet
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);


  delay(2000);
  display.clearDisplay();



  print_line("welcome  to medibox", 10, 10, 2);
  display.clearDisplay();

}

void loop() {

  update_time_with_check_alarm();

  //check if the menu button is pressed 
  if (digitalRead(PB_OK) == LOW) {
    delay(200);
    go_to_menu();
  }

  check_temp();

}

//function to print a given String. Need to specify the position and the text size.
void print_line(String text, int column, int row, int text_size) {

  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);
  display.println(text);
  display.display();
}


//function to clear a specific string. It print the same string in Black color to erase it.
void erase_line(String text, int column, int row, int text_size) {

  display.setTextSize(text_size);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(column, row);
  display.println(text);
  display.display();
}

//function to Display the current time DD:HH:MM:SS
void print_time_now(void) {
  //Here we have used a special method to stop blinking the Displayed time in OLED display. 
  //Instead of clear the whole screen we just errase the number that apears before.

  //display.clearDisplay();
  if (displayDay != days) {
    erase_line(String(days - 1), 0, 0, 2);
    print_line(String(days), 0, 0, 2);
    print_line(":", 20, 0, 2);
    displayDay = days;
  }

  int h = 0;
  if (hours == 0)h = 23;
  else h = hours -1;
  

  if (displayHour != hours) {
    erase_line(String(h), 30, 0, 2);
    print_line(String(hours), 30, 0, 2);
    print_line(":", 50, 0, 2);
    displayHour = hours;
  }

  int m = 0;
  if (minutes == 0) m = 59;
  else m = minutes - 1;

  if (displayMinute != minutes) {
    erase_line(String(m), 60, 0, 2);
    print_line(String(minutes), 60, 0, 2);
    print_line(":", 80, 0, 2);
    displayMinute = minutes;
  }

  int sec = 0;
  if (seconds == 0) sec = 59;
  else sec = seconds - 1;

  if (displaySecond != seconds) {
    erase_line(String(sec), 90, 0, 2);
    print_line(String(seconds), 90, 0, 2);
    displaySecond = seconds;
  }

}

//function to automatically update the current time
void update_time() {
  //We get the time of UTC over the wifi. It needs to corret the time according to the user's timezone.
  //Here we use the offset from UTC which we get from the user and set the time according to that.

  struct tm timeinfo;
  getLocalTime(&timeinfo);

  int dayFlag = 0;
  int hourFlag = 0;

  char timeMinute[3];
  strftime(timeMinute, 3, "%M", &timeinfo);
  minutes = atoi(timeMinute) + minutes_UTC_offset;
  if (minutes > 59) {
    minutes -= 60;
    hourFlag = 1;
  }
  else if (minutes < 0) {
    minutes += 60;
    hourFlag = -1;
  }

  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  hours = atoi(timeHour) + hours_UTC_offset + hourFlag;
  if (hours > 23) {
    hours -= 24;
    dayFlag = 1;
  }
  else if (hours < 0) {
    hours += 24;
    dayFlag = -1;
  }

  char timeSecond[3];
  strftime(timeSecond, 3, "%S", &timeinfo);
  seconds = atoi(timeSecond);

  char timeDay[3];
  strftime(timeDay, 3, "%d", &timeinfo);
  days = atoi(timeDay) + dayFlag ;


}

//funtion to ring the alarm .
//When the alarm time has reached it show a message on display and light up the LED 1 While ring the buzzer.
void ring_alarm() {
  display.clearDisplay();
  print_line("MEDICINE TIME!", 0, 0, 2);

  digitalWrite(LED_1, HIGH);

  bool break_happened = false;

  //Ring the buzzer
  while (break_happened == false && digitalRead(PB_CANCEL) == HIGH) {
    for (int i = 0; i < NO_NOTES; i++) {
      if (digitalRead(PB_CANCEL) == LOW) {
        delay(200);
        break_happened = true;  // alarm stops when the cancel button pressed.
        break;
      }
      tone(BUZZER, NOTES[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);
    }
  }

  digitalWrite(LED_1, LOW);
  display.clearDisplay();
  displayHour = -1;
  displayMinute = -1;
  displayDay = -1;
  displaySecond = -1;

}


//function to automatically update the currnet time while checking for alarms
void update_time_with_check_alarm() {
  update_time();
  print_time_now();

  if (alarm_enabled == true) {
    for (int i = 0; i < n_alarms; i++) {
      if (alarm_triggered[i] == false && alarm_hours[i] == hours && alarm_minutes[i] == minutes) {
        ring_alarm();
        alarm_triggered[i] = true;
      }
    }
  }


}


//function to wait for button press in the menu
int wait_for_button_press() {
  while (true) {
    if (digitalRead(PB_UP) == LOW) {
      delay(200);
      return PB_UP;
    }
    else if (digitalRead(PB_DOWN) == LOW) {
      delay(200);
      return PB_DOWN;
    }
    else if (digitalRead(PB_OK) == LOW) {
      delay(200);
      return PB_OK;
    }
    else if (digitalRead(PB_CANCEL) == LOW) {
      delay(200);
      return PB_CANCEL;
    }
    update_time();

  }
}

//function to navigate through the menu
void go_to_menu() {
  while (digitalRead(PB_CANCEL) == HIGH) {
    display.clearDisplay();
    displayHour = -1;
    displayMinute = -1;
    displayDay = -1;
    displaySecond = -1;

    print_line(modes[current_mode], 0, 0, 2);


    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      current_mode += 1;
      current_mode = current_mode % max_modes;
    }
    else if (pressed == PB_DOWN) {
      delay(200);
      current_mode -= 1;
      if (current_mode < 0) {
        current_mode = max_modes - 1 ;
      }
    }
    else if (pressed == PB_OK) {
      delay(200);
      run_mode(current_mode);
    }
    else if (pressed == PB_CANCEL) {
      display.clearDisplay();
      delay(200);
      break;
    }

  }
}

//Get the UTC offset from user to correct the timezone
void set_time() {
  int temp_hour = hours;

  while (true) {
    display.clearDisplay();
    print_line("Enter UTC offset of hours : " + String(temp_hour), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      if (temp_hour > 23) {
        temp_hour = -23 ;
      }
    }
    else if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < -23) {
        temp_hour = 23 ;
      }
    }
    else if (pressed == PB_OK) {
      delay(200);
      hours_UTC_offset = temp_hour;
      break;
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }


  int temp_minute = minutes;

  while (true) {
    display.clearDisplay();
    print_line("Enter UTC offset of minutes : " + String(temp_minute), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 1;
      if (temp_minute > 59) {
        temp_minute = -59 ;
      }

    }
    else if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 1;
      if (temp_minute < -59) {
        temp_minute = 59 ;
      }
    }
    else if (pressed == PB_OK) {
      delay(200);
      minutes_UTC_offset = temp_minute;
      break;
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Time is set", 0, 0, 2);
  delay(1000);

}

//function to set the alarm. It takes hour and minutes from the user.
void set_alarm(int alarm) {
  int temp_hour = alarm_hours[alarm];

  while (true) {
    display.clearDisplay();
    print_line("Enter hour : " + String(temp_hour), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    }
    else if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < 0) {
        temp_hour = 23 ;
      }
    }
    else if (pressed == PB_OK) {
      delay(200);
      alarm_hours[alarm] = temp_hour;
      break;
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }


  int temp_minute = alarm_minutes[alarm];

  while (true) {
    display.clearDisplay();
    print_line("Enter minute : " + String(temp_minute), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    }
    else if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0) {
        temp_minute = 59 ;
      }
    }
    else if (pressed == PB_OK) {
      delay(200);
      alarm_minutes[alarm] = temp_minute;
      break;
    }
    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Alarm is set", 0, 0, 2);
  delay(1000);

}


void run_mode(int mode) {
  if (mode == 0) {
    set_time();
  }
  else if (mode  == 1 || mode == 2 || mode == 3) {
    set_alarm(mode - 1);
  }
  else if (mode == 4) {
    if (alarm_enabled == true) {
      alarm_enabled = false;
      modes[4] = "5 - Enable Alarms";
      display.clearDisplay();
      print_line("Alarm disabled", 0, 0, 2);
    }
    else if (alarm_enabled == false) {
      alarm_enabled = true;
      modes[4] = "5 - disable Alarms";
      display.clearDisplay();
      print_line("Alarm Enabled", 0, 0, 2);
    }
    delay(2000);
  }

}

//function to check whether the temperature and humidity have exceeded healthy limits or not.
//if exceeded it show a warning msg and light up the corresponding LED
void check_temp() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  if (data.temperature > 32 && tempHighFlag == 0) {
    //display.clearDisplay();
    tempHighFlag = 1;
    tempLowFlag = 0;
    erase_line("TEMP LOW", 0, 40, 2);
    print_line("TEMP HIGH", 0, 40, 2);
    digitalWrite(LED_1, HIGH);
  }
  else if (data.temperature < 26 && tempLowFlag == 0) {
    //display.clearDisplay();
    tempLowFlag = 1;
    tempHighFlag = 0;
    erase_line("TEMP HIGH", 0, 40, 2);
    print_line("TEMP LOW", 0, 40, 2);
    digitalWrite(LED_1, HIGH);
  }
  else if (data.temperature >= 26 && data.temperature <= 32 ) {
    tempLowFlag = 0;
    tempHighFlag = 0;
    erase_line("TEMP HIGH", 0, 40, 2);
    erase_line("TEMP LOW", 0, 40, 2);
    digitalWrite(LED_1, LOW);
  }
  if (data.humidity > 80 && humHighFlag == 0) {
    //display.clearDisplay();
    humLowFlag = 0;
    humHighFlag = 1;
    erase_line("HUMD LOW", 0, 20, 2);
    print_line("HUMD HIGH", 0, 20, 2);
    digitalWrite(LED_1, HIGH);
  }
  else if (data.humidity < 60 && humLowFlag == 0) {
    //display.clearDisplay();
    humLowFlag = 1;
    humHighFlag = 0;
    erase_line("HUMD HIGH", 0, 20, 2);
    print_line("HUMD LOW", 0, 20, 2);
    digitalWrite(LED_2, HIGH);
  }
  else if (data.humidity <= 80 && data.humidity >= 60) {
    humLowFlag = 0;
    humHighFlag = 0;
    erase_line("HUMD LOW", 0, 20, 2);
    erase_line("HUMD HIGH", 0, 20, 2);
    digitalWrite(LED_2, LOW);
  }

}