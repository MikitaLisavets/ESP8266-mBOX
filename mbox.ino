#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_PCF8574.h>
#include <Keypad.h>

LiquidCrystal_PCF8574 lcd(0x27);

const char* SSID     = "ssid";
const char* PASSWORD = "password";
const String URL     = "URL";

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

unsigned long lastConnectionTime = 0;
unsigned long postingInterval = 0;
struct Weather {
  unsigned int test;
};
String httpData;
Weather weather;

void setup() {
  Serial.begin(115200);
  Serial.print("\nConnecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected\nIP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  lcd.begin(16, 2);
  lcd.setBacklight(255);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (millis() < lastConnectionTime) lastConnectionTime = 0;
    if (millis() - lastConnectionTime > postingInterval or lastConnectionTime == 0) {
      if (httpRequest() and parseData()) {
        Serial.println("\nWeather");
        Serial.printf("test: %d\n", weather.test);
        Serial.println();
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


bool httpRequest() {
  HTTPClient client;
  bool find = false;
  //client.setTimeout(1000);
  Serial.print("Connecting ");
  client.begin(url);
  int httpCode = client.GET();

  if (httpCode > 0) {
    Serial.printf("successfully, code: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK) {
      httpData = client.getString();
      lastConnectionTime = millis();
      find = true;
    }
  }
  else Serial.printf("failed, error: %s\n", client.errorToString(httpCode).c_str());

  postingInterval = find ? 600L * 1000L : 60L * 1000L;
  client.end();

  return find;
}

bool parseData() {
  Serial.println(httpData);

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(httpData);

  if (!root.success()) {
    Serial.println("Json parsing failed!");
    return false;
  }

  weather.test = root["test"];

  httpData = "";
  return true;
}
