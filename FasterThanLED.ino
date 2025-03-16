/*
   Copyright (c) 2015, Majenko Technologies
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

 * * Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

 * * Redistributions in binary form must reproduce the above copyright notice, this
     list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.

 * * Neither the name of Majenko Technologies nor the names of its
     contributors may be used to endorse or promote products derived from
     this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266mDNS.h>
#include <FastLED.h>

#define NUM_LEDS 37
#define DATA_PIN 2


CRGB leds[NUM_LEDS];
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 30; // Update every 20 milliseconds

const char *ssid = "CiriC";
const char *password = "123454321";

ESP8266WebServer server(80);

const int led = 5;

void handleRoot() {
  digitalWrite(led, 1);
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  StreamString temp;
  temp.reserve(500);  // Preallocate a large chunk to avoid memory fragmentation
  temp.printf("\
<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP8266 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP8266!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <img src=\"/test.svg\" />\
  </body>\
</html>",
              hr, min % 60, sec % 60);
  server.send(200, "text/html", temp.c_str());
  digitalWrite(led, 0);
}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) { message += " " + server.argName(i) + ": " + server.arg(i) + "\n"; }

  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void drawGraph() {
  String out;
  out.reserve(2600);
  char temp[70];
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
  out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<g stroke=\"black\">\n";
  int y = rand() % 130;
  for (int x = 10; x < 390; x += 10) {
    int y2 = rand() % 130;
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
    out += temp;
    y = y2;
  }
  out += "</g>\n</svg>\n";

  server.send(200, "image/svg+xml", out);
}
void turnLedOn(){
//  leds[1] = CRGB::Red;
//    updateLEDs(CRGB::Red);
    updateLEDs(CRGB::Red, NUM_LEDS, "waave");
//  leds[3] = CRGB::Red;
//  leds[4] = CRGB::Red;
//  digitalWrite(led, HIGH);
  server.send(200);
}
void turnLedOff(){
//  leds[1] = CRGB::Black;
    updateLEDs(CRGB::Black, NUM_LEDS, "waave");
//  leds[3] = CRGB::Black;
//  leds[4] = CRGB::Black;
//  digitalWrite(led, LOW);
  server.send(200);
}
void turnLedOrange(){
//  leds[1] = CRGB::Black;
    updateLEDs(CRGB::Blue, NUM_LEDS, "waave");
//  leds[3] = CRGB::Black;
//  leds[4] = CRGB::Black;
//  digitalWrite(led, LOW);
  server.send(200);
}
void handlePlain() {
  if (server.method() != HTTP_POST) {
//    digitalWrite(led, 1);
    server.send(405, "text/plain", "Method Not Allowed");
//    digitalWrite(led, 0);
  } else {
//    digitalWrite(led, 1);
    server.send(200, "text/plain", "POST body was:\n" + server.arg("plain"));
    Serial.println(server.arg("plain"));
    String BRIGHTNESS_String = server.arg("plain");
    int brightness = BRIGHTNESS_String.toInt();
    unsigned long startTime = millis();
    
    while (millis() - startTime < 30) {  // 30ms duration
    FastLED.setBrightness(brightness);
    FastLED.show();
    delayMicroseconds(100);
    }
//    digitalWrite(led, 0);
  }
}
void setup(void) {
  FastLED.setDither(0);
  FastLED.addLeds<WS2811, DATA_PIN, BRG>(leds, NUM_LEDS);
  FastLED.clear(); 
  FastLED.show();
  
     // Move a single white led 
     int whiteLed = 0;
   while( whiteLed < NUM_LEDS) {
    whiteLed = whiteLed + 1;
      // Turn our current led on to white, then show the leds
      leds[whiteLed] = CRGB::White;

      // Show the leds (only one of which is set to white, from above)
      FastLED.show();

      // Wait a little bit
      delay(10);

      // Turn our current led back to black for the next loop around
      leds[whiteLed] = CRGB::Black;
   }
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  //AP mode WIFI
  //____________________
  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  //____________________
  //station mode wifi
//  WiFi.mode(WIFI_STA);
//  WiFi.begin(ssid, password);
//  Serial.println("");
//
//  // Wait for connection
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(500);
//    Serial.print(".");
//  }
//
//  Serial.println("");
//  Serial.print("Connected to ");
//  Serial.println(ssid);
//  Serial.print("IP address: ");
//  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) { Serial.println("MDNS responder started"); }

  server.on("/", handleRoot);
  server.on("/test.svg", drawGraph);
  server.on("/inline", []() {
    server.send(200, "text/plain", "Dustetun Daraam");
  });
  server.on("/led/on", turnLedOn);
  server.on("/led/off", turnLedOff);
  server.on("/color/orange", turnLedOrange);
  server.on("/sensors", HTTP_POST, handlePlain);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  MDNS.update();
//  updateLEDs(CRGB);
//  delay(30);
//  noInterrupts();

//  FastLED.show();
//  FastLED.delay(10);
//  interrupts();
}
void updateLEDs(CRGB color, int ledCount, String pattern) {
    unsigned long startTime = millis();
    
    while (millis() - startTime < 30) {  // 30ms duration
        FastLED.clear();

        if (pattern == "wave") {
            // Moving wave pattern
            int pos = ((millis() - startTime) * 10) % ledCount;
            for (int i = 0; i < ledCount; i++) {
                if (abs(i - pos) < 5) leds[i] = color;
            }
        } 
        else if (pattern == "alternate") {
            // Alternating pattern
            for (int i = 0; i < ledCount; i++) {
                leds[i] = (i % 2 == 0) ? color : CRGB::Black;
            }
        }
        else {  // Solid pattern (default)
            for (int i = 0; i < ledCount; i++) {
                leds[i] = color;
            }
        }

        FastLED.show();
        delayMicroseconds(100);
    }
}
