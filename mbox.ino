#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_PCF8574.h>
#include <Keypad.h>

LiquidCrystal_PCF8574 lcd(0x27);

bool offline = false;
bool out = false;
char* ssid = "HUAWEI-G4Qp";
char* password = "d5U5eaA5Fg";

const String URL = "http://mbox-backend.herokuapp.com/api/";

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
String queryKey;

char customKey;
double first = 0;
double second = 0;
double total = 0;

void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2);
  lcd.setBacklight(255);
  lcd.setCursor(0, 0);
  lcd.print("Connect?");
  lcd.setCursor(0, 1);
  lcd.print("Yes(1) / No(2)");
  char ikey;
  int ind = 0;
  while(ikey != '1' && ikey != '2') {
    ikey = kpad.getKey();
    delay(16);
  }
  if (ikey == '1') {
    kpad.addEventListener(keypadEvent);
    connect();
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Offline (calc)");
    offline = true;
    delay(1000);
    lcd.clear();
  }
}

void loop() {
  out = false;
  queryKey = "";
  if (!offline) {
    String menuItem = getMenuOption();
    loadProgram(menuItem); 
  } else {
    calc();
  }
  delay(10);
}

void loadProgram(String menuItem) {
  char pressedKey;
  unsigned long startTime;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Fetching...");

  while(1) {
    if (out) return;
    pressedKey = kpad.getKey();
    JsonObject& menu = fetch(URL + menuItem + "?key=" + queryKey); 

    refreshTime = menu["refreshTime"] ?  menu["refreshTime"] : refreshTime;
    scrollTime = menu["scrollTime"] ? menu["scrollTime"] : scrollTime;

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
          delay(10);
        }
        lcd.scrollDisplayLeft();
        textOffset--;
      }
      lcd.home();
    }
    startTime = millis();
    while(millis() - startTime < refreshTime) {
      pressedKey = kpad.getKey();
      delay(10);
    }
  }
}

String getMenuOption() {
  char pressedKey;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading menu...");

  JsonObject& menu = fetch(URL + "menu"); 
  const int length = menu["length"];

  lcd.clear();
  while(pressedKey != '5') {
    pressedKey = kpad.getKey();
    
    if (pressedKey == '2') {
      lcd.clear();
      selectedMenuOption -= 1;
      if (selectedMenuOption < 0) {
        selectedMenuOption = length + selectedMenuOption;
      }
    }
    if (pressedKey == '8') {
      lcd.clear();
      selectedMenuOption += 1;
      selectedMenuOption %= length;
    }

    String menuItem = menu[String(selectedMenuOption)];

    lcd.setCursor(0, 0);
    lcd.print("[" + String(selectedMenuOption + 1) + "] "); 
    lcd.print(menuItem); 
    lcd.setCursor(0, 1);
    lcd.print("Up / Down");
    delay(200);
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
    
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    lcd.print('.');
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected"); 
}

void keypadEvent(KeypadEvent key){
  switch (kpad.getState()){
    case PRESSED:
      switch(key) {
        case '*':
          out = true;
          break;
        case 'A':
          refreshTime += timeOffset;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("[+ " + String(refreshTime) + " update]");
          break;
        case 'B':
          refreshTime -= timeOffset;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("[- " + String(refreshTime) + " update]");
          break;
        case 'C': 
          scrollTime += timeOffset;
          lcd.clear();
          lcd.setCursor(0, 1);
          lcd.print("[+ " + String(scrollTime) + " scroll]");
          break;
         case 'D':
          scrollTime -= timeOffset;
          lcd.clear();
          lcd.setCursor(0, 1);
          lcd.print("[- " + String(scrollTime) + " scroll]");
          break;
        default:
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("mode: [" + String(key) + "]");
          queryKey = key;
      }  
      break;
    case RELEASED:
      break;
    case HOLD:
      break;
  }
}

void calc() {
  customKey = kpad.getKey();
  switch(customKey) {
    case '0' ... '9':
      lcd.setCursor(0,0);
      first = first * 10 + (customKey - '0');
      lcd.print(first);
      break;
    case 'A':
      first = (total != 0 ? total : first);
      lcd.setCursor(0,1);
      lcd.print("+");
      second = SecondNumber(); // get the collected the second number
      total = first + second;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(total);
      first = total, second = 0; // reset values back to zero for next use
      break;
    case 'B':
      first = (total != 0 ? total : first);
      lcd.setCursor(0,1);
      lcd.print("-");
      second = SecondNumber();
      total = first - second;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(total);
      first = total, second = 0;
      break;
    case 'C':
      first = (total != 0 ? total : first);
      lcd.setCursor(0,1);
      lcd.print("*");
      second = SecondNumber();
      total = first * second;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(total);
      first = total, second = 0;
      break;
    case 'D':
      first = (total != 0 ? total : first);
      lcd.setCursor(0,1);
      lcd.print("/");
      second = SecondNumber();
      lcd.clear();
      lcd.setCursor(0,0);
      second == 0 ? lcd.print("Invalid") : total = (float)first / (float)second;
      lcd.print(total);
      first = total, second = 0;
      break;
    case '*':
      total = 0;
      first = 0,
      second = 0;
      lcd.clear();
      break;
  }
}

long SecondNumber() {
  while(1) {
    customKey = kpad.getKey();
    if(customKey >= '0' && customKey <= '9') {
      second = second * 10 + (customKey - '0');
      lcd.setCursor(0,2);
      lcd.print(second);
    }
    if(customKey == '#') break;
    delay(10);
  }
 return second; 
}
