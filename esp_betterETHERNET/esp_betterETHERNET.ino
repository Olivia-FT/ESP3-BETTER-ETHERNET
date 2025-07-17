#include <SPI.h>
#include <Ethernet.h>
#include <NetworkClient.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <vector>

#define W5500_CS    14  // Chip Select pin
#define W5500_RST    9  // Reset pin
#define W5500_INT   10  // Interrupt pin
#define W5500_MISO  12  // MISO pin
#define W5500_MOSI  11  // MOSI pin
#define W5500_SCK   13  // Clock pin

#define DEBUG true

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

const char *ssid = "HIDGuest";          // Change this to your WiFi SSID
const char *password = "UKCWLinternet!";  // Change this to your WiFi password

EthernetServer server(80);

// Time stuffs
struct TimedAction {
  unsigned long triggerTime;
  int pin;
  bool state;
};

unsigned long currentMillis;
const unsigned long oneSecond = 1000;
const unsigned long twoSeconds = 2000;
std::vector<TimedAction> timedActionsVec;

void command(const String& CMD) {
  JsonDocument json;
  DeserializationError error = deserializeJson(json, CMD);
  if (error) {
    Serial.println("Failed to parse");
    return;
  }

  JsonArray array = json.as<JsonArray>();
  changePowerState(array);
}

void changePowerState(JsonArray array) {
  for (int i = 0; i < array.size(); i++) {
    JsonObject obj = array[i];
    String pinNum = obj["pin"].as<String>();
    bool state = obj["power"];
    bool pulse = obj["pulse"];

    int pin = -1;
    if (pinNum == "boot_1") {
      pin = 48;
    } else if (pinNum == "boot_2") {
      pin = 47;
    } else if (pinNum == "boot_3") {
      pin = 46;
    } else if (pinNum == "boot_4") {
      pin = 42;
    } else if (pinNum == "reset") {
      pin = 41;
    }

    changePowerStateReal(pin, state);

    if (pulse) {
      unsigned long pulseTime;
      if(pinNum == "reset") pulseTime = oneSecond;
      else pulseTime = twoSeconds;

      if (DEBUG) Serial.println("pulseTime is: ");
      if (DEBUG) Serial.println(pulseTime);

      if (DEBUG) Serial.println("pin is: ");
      if (DEBUG) Serial.println(pin);

      if (DEBUG) Serial.println("!state is: ");
      if (DEBUG) Serial.println(!state);

      TimedAction newAction = {currentMillis + pulseTime, pin, !state};
      timedActionsVec.push_back(newAction);
    }
  }
}

void changePowerStateReal(int pin, bool state) {
    if (state) digitalWrite(pin, HIGH);
    else digitalWrite(pin, LOW);
}

void setup() {
  pinMode(48, OUTPUT);
  pinMode(47, OUTPUT);
  pinMode(46, OUTPUT);
  pinMode(42, OUTPUT);
  pinMode(41, OUTPUT);
  Serial.begin(115200);

  SPI.begin(W5500_SCK, W5500_MISO, W5500_MOSI, W5500_CS);

  // Initialize Ethernet using DHCP to obtain an IP address
  Ethernet.init(W5500_CS);
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    while (true); // Halt if DHCP configuration fails
  }

  // Print the assigned IP address
  Serial.print("IP Address: ");
  Serial.println(Ethernet.localIP());

  server.begin();
  Serial.println("HTTP server started");

  currentMillis = millis();
}

void loop(void) {
  currentMillis = millis();
  EthernetClient client = server.available();

  if (client) {
    Serial.println("new client");

    boolean currentLineIsBlank = true;
    String MSG = "";

    // Gather Header
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (DEBUG) Serial.write(c);
        MSG += c;
        if (c == '\n' && currentLineIsBlank) break;
        if (c == '\n') currentLineIsBlank = true;
        else if (c != '\r') currentLineIsBlank = false;
      }
    }

    // Handle Header
    if (DEBUG) Serial.println("Message is");
    if (DEBUG) Serial.println(MSG);

    String Firstline = MSG.substring(0, MSG.indexOf("\n"));
    String Method = Firstline.substring(0, Firstline.indexOf(" "));
    String Path = Firstline.substring(Firstline.indexOf(" ") + 1, Firstline.indexOf(" ", Firstline.indexOf(" ") + 1));

    if (DEBUG) {
      Serial.println("Firstline: " + Firstline);
      Serial.println("Method: " + Method);
      Serial.println("Path: " + Path);
    }

    String MSG_Remove_Empty_Line = MSG.substring(0, MSG.lastIndexOf("\n") - 2);
    if (DEBUG) Serial.println("MSG_Remove_Empty_Line");
    if (DEBUG) Serial.println(MSG_Remove_Empty_Line);

    String Lastline = MSG_Remove_Empty_Line.substring(MSG_Remove_Empty_Line.lastIndexOf("\n") + 1, MSG_Remove_Empty_Line.length());
    if (DEBUG) Serial.println("Lastline Is");
    if (DEBUG) Serial.println(Lastline);

    String ContentLengthSTR = Lastline.substring(Lastline.indexOf(":") + 2 , Lastline.length());
    if (DEBUG) Serial.println("ContentLengthSTR Is");
    if (DEBUG) Serial.println(ContentLengthSTR);

    int ContentLengthNUM = ContentLengthSTR.toInt();

    // Handle Body
    String body = "";
    while (body.length() < ContentLengthNUM && client.connected()) {
      if (client.available()) {
        char c = client.read();
        body += c;
      }
    }

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");  // the connection will be closed after completion of the response
    //client.println("Refresh: 5");  // refresh the page automatically every 5 sec
    client.println();
    client.println("{}");

    if (DEBUG) Serial.println("body Is");
    if (DEBUG) Serial.println(body);

    // call power function(s)
    if (Method == "POST") {
      if (Path == "/power") {
        command(body);
      }
    }

    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }

  for (int i = 0; i < timedActionsVec.size(); i++) {
    TimedAction action = timedActionsVec[i];

    if(action.triggerTime <= currentMillis) {
      changePowerStateReal(action.pin, action.state);
      timedActionsVec.erase(timedActionsVec.begin() + i);
      i--;
    }
  }
}
