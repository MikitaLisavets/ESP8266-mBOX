#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_PCF8574.h>
#include <Keypad.h>
extern "C" {
  #include "user_interface.h"
  #include "wpa2_enterprise.h"
}

LiquidCrystal_PCF8574 lcd(0x27);

bool WP2Enterprise = true;
static const char* ssid     = "ssid";
static const char* username = "username";
static const char* password = "password";

const String URL     = "http://mbox-backend.herokuapp.com/api/";

const byte ROWS = 4;
const byte COLS = 4;
byte rowPins[ROWS] = {16, 14, 12, 13};
byte colPins[COLS] = {2, 0, 3, 1};
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
Keypad kpad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

int refreshTime = 600;
int scrollTime = 300;
int timeOffset = 100;
int selectedMenuOption = 0;

void setup() {
  lcd.begin(16, 2);
  lcd.setBacklight(255);

  connect();
}

void loop() {
  String menuItem = getMenuOption();
  loadProgram(menuItem);
}

void loadProgram(String menuItem) {
  char pressedKey;
  String key;
  unsigned long startTime;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading...");

  while(1) {
    JsonObject& menu = fetch(URL + menuItem + "?key=" + key); 

    String lineOne = menu["lineOne"];
    String lineTwo = menu["lineTwo"];

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(lineOne);
    lcd.setCursor(0, 1);
    lcd.print(lineTwo);
    int lineOneLength = lineOne.length();
    int lineTwoLength = lineTwo.length();
    
    if (lineOneLength > 16 || lineTwoLength > 16) {
      int textOffset = lineOneLength > lineTwoLength ? lineOneLength : lineTwoLength;
      while(textOffset - 16 > 0) {
        startTime = millis();
        while(millis() - startTime < scrollTime) {
          pressedKey = kpad.getKey();
          if (pressedKey) {
            switch(pressedKey) {
              case '*':
                return;
            case 'A':
              refreshTime += timeOffset;
              lcd.setCursor(0, 0);
              lcd.print("[+ " + String(refreshTime) + " update]");
              break;
            case 'B':
              refreshTime -= timeOffset;
              lcd.setCursor(0, 0);
              lcd.print("[- " + String(refreshTime) + " update]");
              break;
            case 'C': 
              scrollTime += timeOffset;
              lcd.setCursor(0, 1);
              lcd.print("[+ " + String(scrollTime) + " scroll]");
              break;
             case 'D':
              scrollTime -= timeOffset;
              lcd.setCursor(0, 1);
              lcd.print("[- " + String(scrollTime) + " scroll]");
              break;
              default:
                key = pressedKey;
            }            
          }
          delay(10);
        }
        lcd.scrollDisplayLeft();
        textOffset--;
      }
      delay(scrollTime);
      lcd.home();
    }
    startTime = millis();
    while(millis() - startTime < refreshTime) {
      pressedKey = kpad.getKey();
      if (pressedKey) {
        switch(pressedKey) {
          case '*':
            return;
          case 'A':
            refreshTime += timeOffset;
            lcd.setCursor(0, 0);
            lcd.print("[+ " + String(refreshTime) + " update]");
            break;
          case 'B':
            refreshTime -= timeOffset;
            lcd.setCursor(0, 0);
            lcd.print("[- " + String(refreshTime) + " update]");
            break;
          case 'C': 
            scrollTime += timeOffset;
            lcd.setCursor(0, 1);
            lcd.print("[+ " + String(scrollTime) + " scroll]");
            break;
           case 'D':
            scrollTime -= timeOffset;
            lcd.setCursor(0, 1);
            lcd.print("[- " + String(scrollTime) + " scroll]");
            break;
          default:
            key = pressedKey;
        }                
      }
      delay(10);
    }
  }
}

String getMenuOption() {
  char pressedKey;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading...");

  JsonObject& menu = fetch(URL + "menu"); 
  const int length = menu["length"];
  
  while(pressedKey != '5') {
    pressedKey = kpad.getKey();
    
    if (pressedKey == '2') {
      selectedMenuOption -= 1;
      if (selectedMenuOption < 0) {
        selectedMenuOption = length + selectedMenuOption;
      }
    }
    if (pressedKey == '8') {
      selectedMenuOption += 1;
      selectedMenuOption %= length;
    }

   String menuItem = menu[String(selectedMenuOption)];

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("[" + String(selectedMenuOption + 1) + "] "); 
    lcd.print(menuItem); 
    lcd.setCursor(0, 1);
    lcd.print("Up / Down");
    delay(100);
  }

  return menu[String(selectedMenuOption)];
}

JsonObject& fetch(String url) {
  String data = httpRequest(url);
  return parseData(data);
}


String httpRequest(String url) {
  HTTPClient client;
  String data;
  client.begin(url);
  int httpCode = client.GET();

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      data = client.getString();
    }
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error:");
    lcd.setCursor(0, 1);
    lcd.print(client.errorToString(httpCode).c_str());
  }

  client.end();
  return data;
}

JsonObject& parseData(String data) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(data);
  if (!root.success()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error:");
    lcd.setCursor(0, 1);
    lcd.print("Parse JSON");
  }
  return root;
}


void connect() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to");
  lcd.setCursor(0, 1);
  lcd.print(ssid);
  
  if (WP2Enterprise) {
    wifi_set_opmode(STATION_MODE);
  
    struct station_config wifi_config;
  
    memset(&wifi_config, 0, sizeof(wifi_config));
    strcpy((char*)wifi_config.ssid, ssid);
  
    wifi_station_set_config(&wifi_config);
  
    wifi_station_clear_cert_key();
    wifi_station_clear_enterprise_ca_cert();
  
    wifi_station_set_wpa2_enterprise_auth(1);
    wifi_station_set_enterprise_username((uint8*)username, strlen(username));
    wifi_station_set_enterprise_password((uint8*)password, strlen(password));
  
    wifi_station_connect();
  } else {    
    WiFi.begin(ssid, password);
  }  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.print('.');
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected"); 
}

