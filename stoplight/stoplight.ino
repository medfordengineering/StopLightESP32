/* References: 
https://randomnerdtutorials.com/stepper-motor-esp32-web-server/
Add EDT
*/

//#include <ESP8266WiFi.h>
#include <WiFi.h>
//#include <ESPAsyncTCP.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SPIFFS.h>

#define WIFI_SSID "EngineeringSubNet"
#define WIFI_PASS "password"

//#define WIFI_SSID "timesink2"
//#define WIFI_PASS "sweetpotato"

#define HOURS 0
#define MINUTES 1

#define WARNING_TIME 5
#define CLEANING_TIME 13

#define GRN_LIGHT 3  // 4 8266
#define YEL_LIGHT 10 // 15 8266
#define RED_LIGHT 4  // 5 8266
#define NOT_LIGHT 0

#define MAR 3
#define NOV 11
#define SUN 0
#define WEEK 7
#define NOTSET 3
#define MAGIC_NUMBER 8

#define ON 1
#define OFF 0

#define NOSTATE 0
#define INCLASS 1
#define WARNING 2
#define PASSING 3
#define LAST_PERIOD 4
#define CLEANUP 5
#define AFTERSCHOOL 6
#define PERIOD_RESET 7
#define BEFORESCHOOL 8


#define EST -18000
#define EDT -14400

#define MIDNIGHT 0

#define RS 0
#define ER 1
#define AA 2
#define EA 3

//bool dst_state;

uint8_t est_state = NOTSET;

const long utcOffsetInSeconds = EST;

String hostname = "Stoplight V105";

AsyncWebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

const char* HTML_FORM_1 = "sch";
//const char* HTML_FORM_2 = "dst";

extern const char* scheduleNames[];
//extern const char* dstState[];
extern const char* stateNames[];

const char* stateNames[] = { "Not Set", "In Class", "Warning", "Passing", "Last Period", "Clean Up", "After School", "Period Reset", "Before School" };

extern const char* schRS[];
extern const char* schER[];
extern const char* schAA[];

String schedule;
//String daylightsavings;

bool newRequest = false;

uint32_t timeThis, timeLast;

uint8_t state = NOSTATE;

//Schedule tracking variables
int8_t sch_index;
uint8_t sch_str = 0;  //Defaults to regular schedule
uint8_t sch_end = 15;

uint16_t day_str;
uint16_t day_end;
uint16_t day_cln;

uint16_t period_str;
uint16_t period_end;
uint16_t period_wrn;
uint16_t period_nxt;

uint8_t sch_limits[4][2]{
  { 0, 15 },
  { 16, 29 },
  { 30, 47 },
  { 48, 63 }
};

uint8_t hrs, mns, scs;

uint8_t schedules[65][2]{

  //REGULAR TIME SCHEDULE:
  //Index (0-15)
  //(ADVISORY)
  { 7, 45 },  // #0: 465 <- Index # and the conversion of hours and minutes to total minutes since 12:00 am
  { 7, 49 },  // #1: 469
  //(PERIOD 1)
  { 7, 52 },  // #2: 472
  { 8, 48 },  // #3: 528
  //(PERIOD 2)
  { 8, 51 },  // #4: 531
  { 9, 47 },  // #5: 587
  //(PERIOD 3)
  { 9, 50 },   // #6: 590
  { 10, 46 },  // #7: 640
  //(PERIOD 4)
  { 10, 51 },  // #8: 651
  { 11, 47 },  // #9: 707
  //(LUNCH)
  { 11, 50 },  // #10: 710
  { 12, 20 },  // #11: 738
  //(PERIOD 5)
  { 12, 23 },  // #12: 743
  { 13, 19 },  // #13: 799
  //(PERIOD 6)
  { 13, 22 },  // #14: 802
  { 14, 18 },  // #15: 858

  //EARLY RELEASE SCHEDULE:
  //Index (16-29)
  //(ADVISORY)
  { 7, 45 },  // #16: 465
  { 7, 50 },  // #17: 470
  //(PERIOD 1)
  { 7, 53 },  // #18: 473
  { 8, 35 },  // #19: 515
  //(PERIOD 2)
  { 8, 38 },  // #20: 518
  { 9, 16 },  // #21: 556
  //(PERIOD 3)
  { 9, 19 },  // #22: 559
  { 9, 57 },  // #23: 597
  //(PERIOD 4)
  { 10, 00 },  // #24: 600
  { 10, 38 },  // #25: 638
  //(PERIOD 5)
  { 10, 41 },  // #26: 641
  { 11, 19 },  // #27: 679
  //(PERIOD 6)
  { 11, 22 },  //#28: 682
  { 12, 00 },  //#29: 720

  //ADVISORY ACTIVITY SCHEDULE:
  //Index (30-47)
  //(ADVISORY)
  { 7, 45 },  // #30: 465
  { 7, 50 },  // #31: 470
  //(PERIOD 1)
  { 7, 53 },  // #32: 515
  { 8, 41 },  // #33: 521
  //(PERIOD 2)
  { 8, 44 },  // #34: 524
  { 9, 32 },  // #35: 572
  //(ADVISORY ACTIVITY)
  { 9, 35 },   // #36: 575
  { 10, 19 },  //#37: 619
  //(PERIOD 3)
  { 10, 22 },  // #38: 622
  { 11, 10 },  // #39: 670
  //(PERIOD 4)
  { 11, 15 },  // #40: 675
  { 12, 03 },  // #41: 723
  //(LUNCH)
  { 12, 06 },  // #42: 726
  { 12, 36 },  // #43: 756
  //(PERIOD 5)
  { 12, 39 },  // #44: 759
  { 13, 27 },  // #45: 807
  //(PERIOD 6)
  { 13, 30 },  // #46: 810
  { 14, 18 },  // #47: 858

  //EXTENDED ADVISORY SCHEDULE:
  //Index (48-63)
  //(ADVISORY)
  { 7, 45 },  // #48: 465
  { 8, 15 },  // #49: 495
  //(PERIOD 1)
  { 8, 18 },  // #50: 498
  { 9, 10 },  // #51: 550
  //(PERIOD 2)
  { 9, 13 },   // #52: 553
  { 10, 05 },  // #53: 605
  //(PERIOD 3)
  { 10, 8 },   // #54: 608
  { 11, 00 },  // #55: 660
  //(PERIOD 4)
  { 11, 03 },  // #56: 663
  { 11, 55 },  // #57: 715
  //(LUNCH)
  { 11, 58 },  // #58: 718
  { 12, 28 },  // #59: 748
  //(PERIOD 5)
  { 12, 31 },  // #60: 751
  { 13, 23 },  // #61: 803
  //(PERIOD 6)
  { 13, 26 },  // #62: 806
  { 14, 18 },  // #63: 858
};

void turn_on(uint8_t color) {
  digitalWrite(GRN_LIGHT, OFF);
  digitalWrite(YEL_LIGHT, OFF);
  digitalWrite(RED_LIGHT, OFF);
  if (color)
    digitalWrite(color, ON);
}

// Converts hours and minutes into total minutes
uint16_t total_minutes(uint8_t hrs, uint8_t mns) {
  return (hrs * 60) + mns;
};

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  //TRIED TO SET HOSTNMAME BUT FAILED
  // WiFi.mode(WIFI_STA);
  // WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  // WiFi.setHostname(hostname.c_str());

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  if (!SPIFFS.begin()) {
    Serial.println("Failed to initialize SPIFFS");
    return;
  }

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    Serial.println("html");
    //request->send(SPIFFS, "/index.html", "text/html");
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Route to load style.css file
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest* request) {
    Serial.println("css");
    request->send(SPIFFS, "/styles.css", "text/css");
  });

  // Load on form submission
  server.on("/", HTTP_POST, [](AsyncWebServerRequest* request) {
    Serial.print("request");
    int params = request->params();
    for (int i = 0; i < params; i++) {
      const AsyncWebParameter* p = request->getParam(i);
      if (p->isPost()) {
        if (p->name() == HTML_FORM_1) {
          schedule = p->value().c_str();
          sch_str = sch_limits[schedule.toInt()][0];
          sch_end = sch_limits[schedule.toInt()][1];
          //Serial.println(schedule);
        }
        // if (p->name() == HTML_FORM_2) {
        //   daylightsavings = p->value().c_str();
        //dst_state = daylightsavings.toInt();
        //Serial.println(daylightsavings);
        //  if (daylightsavings == "1")
        //timeClient.setTimeOffset(EDT);
        // else
        //timeClient.setTimeOffset(EST);
        //}
        newRequest = true;
      }
    }
    state = NOSTATE;
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  //Start server
  server.begin();
  Serial.println("HTTP server started");

  pinMode(GRN_LIGHT, OUTPUT);
  pinMode(YEL_LIGHT, OUTPUT);
  pinMode(RED_LIGHT, OUTPUT);

  timeClient.begin();

  // Sets time in minutes for start of school, end of school, and start of clean up.
  // day_str = total_minutes(schedules[sch_str][HOURS], schedules[sch_str][MINUTES]);
  // day_end = total_minutes(schedules[sch_end][HOURS], schedules[sch_end][MINUTES]);
  //day_cln = day_end - CLEANING_TIME;
}

String processor(const String& var) {
  const char* scheduleNames[] = { "Regular Schedule", "Early Release", "Advisory Activity", "Extended Advisory" };
  //const char* dstState[] = { "EST", "EDT" };
  const char* estState[] = { "EDT", "EST" };
  const char* schRS[] = { "Advisory", "Period 1", "Period 2", "Period 3", "Period 4", "Lunch", "Period 5", "Period 6" };
  const char* schER[] = { "Advisory", "Period 1", "Period 2", "Period 3", "Period 4", "Period 5", "Period 6" };
  const char* schAA[] = { "Advisory", "Period 1", "Period 2", "Advisory Activity", "Period 3", "Period 4", "Lunch", "Period 5", "Period 6" };
  // const char* stateNames[] = {"Not Set",  "In Class", "Warning", "Passing", "Last Period", "Clean Up", "After School", "Period Reset", "Before School" };

  if (var == "SET_SCHEDULE") {
    return scheduleNames[schedule.toInt()];
  }
  if (var == "SET_DST") {
    // return dstState[daylightsavings.toInt()];
    return estState[est_state];
  }
  if (var == "TIME") {
    char buf[12];
    sprintf(buf, "%02d:%02d:%02d", hrs, mns, scs);
    return buf;
  }
  if (var == "PERIOD") {
    uint8_t period = (sch_index - sch_str) / 2;
    uint8_t schInt = schedule.toInt();
    if ((state != BEFORESCHOOL) && (state != AFTERSCHOOL)) {
      if ((schInt == RS) || (schInt == EA))
        return (schRS[period]);
      else if (schInt == ER)
        return (schER[period]);
      else if (schInt == AA)
        return (schAA[period]);
    }
  }
  if (var == "STATE")
    return stateNames[state];

  return String();
}

void loop() {

  // Get time
  timeClient.update();
  hrs = timeClient.getHours();
  mns = timeClient.getMinutes();
  scs = timeClient.getSeconds();

  // Day of week, month and day
  uint8_t dayofweek = timeClient.getDay();
  String formattedDate = timeClient.getFormattedDate();
  uint8_t splitDash = formattedDate.indexOf("-") + 1;
  String mnt = formattedDate.substring(splitDash, splitDash + 2);
  splitDash += 3;
  String dte = formattedDate.substring(splitDash, splitDash + 2);

  uint8_t month = mnt.toInt();
  uint8_t day = dte.toInt();

  // if (((month > MAR) && (month < NOV)) ||
  // ((month == MAR) && (day > WEEK) && (dayofweek == SUN)) ||
  // ((month == MAR) && (day > WEEK * 2)) ||
  // ((month == NOV) && (day < WEEK) && (dayofweek < day)))
  // {

  uint8_t previousSunday = day - dayofweek;

  if (((month > MAR) && (month < NOV)) || ((month == MAR) && (previousSunday >= MAGIC_NUMBER)) || ((month == MAR) && (day > WEEK * 2)) || ((month == NOV) && (previousSunday < 1))) {
    if ((est_state == true) || (est_state == NOTSET)) {
      timeClient.setTimeOffset(EDT);
      est_state = false;
    }
}
else {
  if ((est_state == false) || (est_state == NOTSET)) {
    timeClient.setTimeOffset(EST);
    est_state = true;
  }
}

uint16_t minuteTime = total_minutes(hrs, mns);

delay(500);

Serial.printf("%02d:%02d:%02d:%02d:%02d:%s:%02d:%02d\n", hrs, mns, scs, minuteTime, sch_index, stateNames[state], sch_str, sch_end);

switch (state) {
  case NOSTATE:
    // Sets time in minutes for start of school, end of school, and start of clean up.
    day_str = total_minutes(schedules[sch_str][HOURS], schedules[sch_str][MINUTES]);
    day_end = total_minutes(schedules[sch_end][HOURS], schedules[sch_end][MINUTES]);
    day_cln = day_end - CLEANING_TIME;

    // Determines which block of the day based on a given time and schedule
    for (sch_index = sch_str; sch_index <= sch_end; sch_index += 2) {
      if (minuteTime <= total_minutes(schedules[sch_index][HOURS], schedules[sch_index][MINUTES])) break;
    }
    sch_index -= 2;

    // Checks to see if we are outside of school time on in last period (special case)
    if (minuteTime < day_str)
      state = BEFORESCHOOL;
    else if (minuteTime > day_end)
      state = AFTERSCHOOL;
    else if (sch_index == (sch_end - 1))  //USE LESS THAN INSTEAD?
      state = LAST_PERIOD;
    else
      state = PERIOD_RESET;
    break;
  // Based on block calculate start, end, warning and next period times in minutes
  case PERIOD_RESET:
    period_str = total_minutes(schedules[sch_index + 0][HOURS], schedules[sch_index + 0][MINUTES]);
    period_end = total_minutes(schedules[sch_index + 1][HOURS], schedules[sch_index + 1][MINUTES]);
    period_wrn = period_end - WARNING_TIME;
    period_nxt = total_minutes(schedules[sch_index + 2][HOURS], schedules[sch_index + 2][MINUTES]);
    state = INCLASS;
    turn_on(GRN_LIGHT);  //Assume green as default. This will get corrected below.
    break;
  case INCLASS:
    if (minuteTime >= period_wrn) {
      state = WARNING;
      turn_on(YEL_LIGHT);
    }
    break;
  case WARNING:
    if (minuteTime >= period_end) {
      state = PASSING;
      turn_on(RED_LIGHT);
    }
    break;
  case PASSING:
    if (minuteTime >= period_nxt) {  // Move to next block and reset all block values
      state = PERIOD_RESET;
      sch_index += 2;
      if (sch_index == (sch_end - 1)) {  // Last period is a special case, since it has no next period time value. //USE LESS THAN INSTEAD?
        state = LAST_PERIOD;
        turn_on(GRN_LIGHT);
      }
    }
    break;
  case LAST_PERIOD:
    if (minuteTime >= day_cln) {
      state = CLEANUP;
      turn_on(YEL_LIGHT);
    }
    break;
  case CLEANUP:
    if (minuteTime >= day_end) {
      state = AFTERSCHOOL;
      turn_on(NOT_LIGHT);
    }
    break;
  case AFTERSCHOOL:
    if (minuteTime == MIDNIGHT) {
      state = BEFORESCHOOL;
    }
    break;
  case BEFORESCHOOL:
    if (minuteTime >= day_str) {
      state = PERIOD_RESET;
      sch_index = sch_str;
    }
    break;
}
}
