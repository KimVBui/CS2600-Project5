#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Keypad.h>
//#define SDA 13 //Define SDA pins
//#define SCL 14 //Define SCL pins
const byte ROWS = 4; 
const byte COLS = 4; 
char keys[ROWS][COLS] = { //This is an adjustment because my own keypad's column 1 doesn't work, no matter how much I configure or try to fix it
  {'X','0','1','2'}, // Row 1 uses keys 2, 3, A
  {'X','3','4','5'}, // Row 2 uses keys 5, 6, B
  {'X','6','7','8'}, // Row 3 uses keys 8, 9, C
  {'X','X','X','X'} 
};
byte rowPins[ROWS] = {13, 12, 14, 27}; 
byte colPins[COLS] = {4, 25, 33, 32}; 

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
LiquidCrystal_I2C lcd(0x27,16,2);


const char* ssid = "KimBNetwork";
const char* password = "rgrv9998";
const char* mqtt_server = "2600cppcs.duckdns.org";

WiFiClient espClient;
PubSubClient client(espClient);

bool gameActive = false;
String currentStatus = "Waiting for Laptop...";



void setup_wifi(){
  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("Wifi connected");
  lcd.clear();
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  if (String(topic) == "tictactoe/move"){
    currentStatus = "Laptop moved: " + msg;
  }
  if (msg == "MODE:2P") {
    gameActive = true;
    currentStatus = "2P Mode Start!";
  } else if (msg == "MODE:1P") {
    gameActive = false;
    currentStatus = "Error: 2P Only";
    lcd.clear();
  } else {
    currentStatus = msg; 
  }
}
void scrollDisplay(String text) {
  static unsigned long lastScroll = 0;
  static int pos = 0;
  if (millis() - lastScroll > 350) {
    lcd.setCursor(0, 0);
    if (text.length() <= 16) {
      lcd.print(text + "                ");
    } else {
      String displayStr = text + "   " + text; // Create looping effect
      lcd.print(displayStr.substring(pos, pos + 16));
      pos++;
      if (pos > text.length() + 3) pos = 0;
    }
    lastScroll = millis();
  }
}

void reconnect() {
  while (!client.connected()) {
      if (client.connect("ESP32_TTT_Player1")) {
        client.subscribe("game/status");
        client.subscribe("game/config");
        client.subscribe("tictactoe/move");
      } else {
        delay(5000);
      }
  }
}




void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  lcd.print("Connecting...");
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}
void loop() {
  if (!client.connected()) reconnect();
  client.loop();
  
  scrollDisplay(currentStatus);
  char key = keypad.getKey();
if (gameActive && key && key != 'X') { 
    // This sends the literal index 0-8 to the server
    char move[2] = {key, '\0'};
    client.publish("tictactoe/move", move);
    Serial.print("ESP32 Sent Move: "); Serial.println(move);
}
  
}
