#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_PCF8574.h>
#include <Keypad.h>

LiquidCrystal_PCF8574 lcd(0x27);

const char* SSID     = "ssid";
const char* PASSWORD = "password";
const string URL     = "URL";

const byte ROWS = 4;
const byte COLS = 4;
byte rowPins[ROWS] = {2, 0, 3, 1};
byte colPins[COLS] = {16, 14, 12, 13};
char hexaKeys[ROWS][COLS] = {
  {'0','4','8','C'},
  {'1','5','9','D'},
  {'2','6','A','E'},
  {'3','7','B','F'}
};
Keypad kpad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

void setup() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  lcd.begin(16, 2);
  lcd.setBacklight(255);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (millis() < lastConnectionTime) lastConnectionTime = 0;
    if (millis() - lastConnectionTime > postingInterval or lastConnectionTime == 0) {
      if (httpRequest() and parseData()) {

      }
    }
  }
  
//  char customKey = kpad.getKey();
//  if (customKey){
//    lcd.home();
//    lcd.clear();
//    lcd.print(customKey);
//  }
}


int menu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading...");
  
}

String httpRequest(string url) {
  HTTPClient client;
  string data;
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

JsonObject& parseData(string data) {
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
