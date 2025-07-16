/*
    Go to thingspeak.com and create an account if you don't have one already.
    After logging in, click on the "New Channel" button to create a new channel for your data. This is where your data will be stored and displayed.
    Fill in the Name, Description, and other fields for your channel as desired, then click the "Save Channel" button.
    Take note of the "Write API Key" located in the "API keys" tab, this is the key you will use to send data to your channel.
    Replace the channelID from tab "Channel Settings" and privateKey with "Read API Keys" from "API Keys" tab.
    Replace the host variable with the thingspeak server hostname "api.thingspeak.com"
    Upload the sketch to your ESP32 board and make sure that the board is connected to the internet. The ESP32 should now send data to your Thingspeak channel at the intervals specified by the loop function.
    Go to the channel view page on thingspeak and check the "Field1" for the new incoming data.
    You can use the data visualization and analysis tools provided by Thingspeak to display and process your data in various ways.
    Please note, that Thingspeak accepts only integer values.

    You can later check the values at https://thingspeak.com/channels/2005329
    Please note that this public channel can be accessed by anyone and it is possible that more people will write their values.
 */

#include <SPI.h>
#include <Ethernet.h>
#include <NetworkClient.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

#define W5500_CS    14  // Chip Select pin
#define W5500_RST    9  // Reset pin
#define W5500_INT   10  // Interrupt pin
#define W5500_MISO  12  // MISO pin
#define W5500_MOSI  11  // MOSI pin
#define W5500_SCK   13  // Clock pin

EthernetClient client;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };


const char *ssid = "HIDGuest";          // Change this to your WiFi SSID
const char *password = "UKCWLinternet!";  // Change this to your WiFi password

EthernetServer server(80);



void power(const String& CMD) {
  JsonDocument json;
	DeserializationError error = deserializeJson(json, CMD);
  if (error) { Serial.println("Failed to prase"); return; }
  
  //server.send(200, "application/json", "{}");

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Connection: close");  
  //client.println("Refresh: 5");  // refresh the page automatically every 5 sec
  client.println();

  JsonArray array;
  if (json.is<JsonArray>()) {
    array = json.as<JsonArray>();
  } else if (json.is<JsonObject>()) {
    array = json.to<JsonArray>();
    array.add(json.as<JsonObject>());
  } else {
    Serial.println("Invalid JSON structure");
    return;
  }

  for (int i = 0; i < array.size(); i++) {
    JsonObject obj = array[i];
    String pinNum = obj["pin"].as<String>();
    bool state = obj["power"];

     int pin = -1; 
  if (pinNum == "boot_1") {
    pin = 48;
  }
  else if (pinNum == "boot_2") {
    pin = 47;
  }
  else if (pinNum == "boot_3") {
    pin = 46;
  }
  else if (pinNum == "boot_4") {
    pin = 42;
  }
  else if (pinNum == "reset") {
    pin = 41;
  }

  if (state) {
    digitalWrite(pin, HIGH);
  } else {
    digitalWrite(pin, LOW);
  }
}
    
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
}


void loop(void) {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    String MSG = "";

    // while (client.connected()) {
    //   if (client.available()) {
    //     char c = client.read();
    //     Serial.write(c);
    //     MSG += c;
    //   }
    // }

    Serial.println("Message is");
    Serial.println(MSG);

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        MSG+= c;
        if (c == '\n' && currentLineIsBlank) {
          Serial.println("Message is");
          Serial.println(MSG);

          String Firstline = MSG.substring(0, MSG.indexOf("\n"));
          Serial.println("Firstline is");
          Serial.println(Firstline);

          String Method = Firstline.substring(0, Firstline.indexOf(" "));
          Serial.println("Method Is");
          Serial.println(Method);

          String Path = Firstline.substring(Firstline.indexOf(" ") + 1, Firstline.indexOf(" ", Firstline.indexOf(" ")+ 1));
          Serial.println("Path Is");
          Serial.println(Path);

          String MSG_Remove_Empty_Line = MSG.substring(0, MSG.lastIndexOf("\n") - 2);
          Serial.println("MSG_Remove_Empty_Line");
          Serial.println(MSG_Remove_Empty_Line);

          String Lastline = MSG_Remove_Empty_Line.substring(MSG_Remove_Empty_Line.lastIndexOf("\n") + 1, MSG_Remove_Empty_Line.length());
          Serial.println("Lastline Is");
          Serial.println(Lastline);

          String ContentLengthSTR = Lastline.substring(Lastline.indexOf(":") + 2 , Lastline.length());
          Serial.println("ContentLengthSTR Is");
          Serial.println(ContentLengthSTR);

          int ContentLengthNUM = ContentLengthSTR.toInt();

          String body = "";
          for(int read = 0; read < ContentLengthNUM; read++) {
            char c = client.read();
            body += c;
          }
          Serial.println("body Is");
          Serial.println(body);


        
          if (Method == "POST" ) {
            if (Path == "/power") {
              power(body);
            }
          } 



          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          // output the value of each analog input pin
          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
            int sensorReading = analogRead(analogChannel);
            client.print("analog input ");
            client.print(analogChannel);
            client.print(" is ");
            client.print(sensorReading);
            client.println("<br />");
          }
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}
