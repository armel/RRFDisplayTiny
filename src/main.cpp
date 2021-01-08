// Include the correct display library

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <string.h>
#include "SSD1306Wire.h"

// Optionally include custom images

#include "font.h"

// Screen

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels
SSD1306Wire display(0x3c, 0, 2, GEOMETRY_128_32);  // ADDRESS, SDA, SCL  -  If not, they can be specified manually.

// Button

#define BUTTON_PIN 3         // the number of the pushbutton pin
int buttonState = 0;         // variable for reading the pushbutton status
int buttonRoom = 0;          // variable for room endpoint index

// Wifi

const char* ssid     = "F4HWN";
const char* password = "petitchaton";
WiFiClient client;

// JSON endpoint

String endpoint[] = {
  "http://rrf.f5nlg.ovh:8080/RRFTracker/RRF-today/rrf_tiny.json",
  "http://rrf.f5nlg.ovh:8080/RRFTracker/TECHNIQUE-today/rrf_tiny.json",
  "http://rrf.f5nlg.ovh:8080/RRFTracker/LOCAL-today/rrf_tiny.json",
  "http://rrf.f5nlg.ovh:8080/RRFTracker/BAVARDAGE-today/rrf_tiny.json",
  "http://rrf.f5nlg.ovh:8080/RRFTracker/INTERNATIONAL-today/rrf_tiny.json",
  "http://rrf.f5nlg.ovh:8080/RRFTracker/FON-today/rrf_tiny.json"
};

// Misceleanous

String tmp_str, tmp_clean;
String payload;
String payload_old;

int step = 1;
int direction = 1;
int brightness = 16;

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void setup() {

  // Initialising the UI will init the display too.
  display.init();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);

  // We start by connecting to a WiFi network

  tmp_str = ssid;
  display.drawString(64, 0, tmp_str);
  display.display();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  tmp_str =  WiFi.localIP().toString().c_str();
  display.drawString(64, 10, tmp_str);
  display.display();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  delay(1000);
}

void loop() {
  HTTPClient http;
  DynamicJsonDocument doc(8192);

  const char *salon, *date, *indicatif, *emission;
  const char *last_h_1, *last_c_1, *last_d_1;
  const char *last_h_2, *last_c_2, *last_d_2;
  const char *last_h_3, *last_c_3, *last_d_3;

  int tot = 0, link_total = 0, link_actif = 0, tx_total = 0, max_level = 0, tx[24] = {0};
  int optimize = 0;
  int tot_bis = 0;

  buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW) {
    buttonRoom += 1;
  }
  
  if (buttonRoom > 5) {
    buttonRoom = 0;
  }

  
  if ((WiFi.status() == WL_CONNECTED)) { // check the current connection status

    http.begin(endpoint[buttonRoom]); // specify the URL
    http.setTimeout(750);
    int httpCode = http.GET();  // make the request

    if (httpCode > 0) { // check for the returning code

      payload = http.getString();

      http.end(); // free the resources

      optimize = payload.compareTo(payload_old);

      if (optimize != 0) {
        payload_old = payload;
      }

      // deserialize the JSON document
      DeserializationError error = deserializeJson(doc, payload);

      if (! error) {
        display.clear();

        salon = doc["abstract"][0]["Salon"];
        date = doc["abstract"][0]["Date"];
        emission = doc["abstract"][0]["Emission cumulée"];
        link_total = doc["abstract"][0]["Links connectés"];
        link_actif = doc["abstract"][0]["Links actifs"];
        tx_total = doc["abstract"][0]["TX total"];

        indicatif = doc["transmit"][0]["Indicatif"];
        tot = doc["transmit"][0]["TOT"];

        last_h_1 = doc["last"][0]["Heure"];
        last_c_1 = doc["last"][0]["Indicatif"];
        last_d_1 = doc["last"][0]["Durée"];

        last_h_2 = doc["last"][1]["Heure"];
        last_c_2 = doc["last"][1]["Indicatif"];
        last_d_2 = doc["last"][1]["Durée"];

        last_h_3 = doc["last"][2]["Heure"];
        last_c_3 = doc["last"][2]["Indicatif"];
        last_d_3 = doc["last"][2]["Durée"];
    
        if (tot == 0) { // if no transmit
          digitalWrite(LED_BUILTIN, HIGH);
          display.setBrightness(brightness);

          /*
          display.setFont(ArialMT_Plain_10);
          display.setTextAlignment(TEXT_ALIGN_RIGHT);

          tmp_str = getValue(date, ' ', 4);
          display.drawString(128, 2, tmp_str);
          */
         
          display.setFont(Dialog_bold_14);
          display.setTextAlignment(TEXT_ALIGN_CENTER);

          tmp_str = salon;
          display.drawString(64, 0, tmp_str);

          if (step <= 5) {
            display.setFont(Dialog_bold_14);

            tmp_str = "BF ";
            tmp_str += emission;

            display.drawString(64, 15, tmp_str);
          }
          else if (step <= 10) {
            display.setFont(Dialog_bold_14);

            tmp_str = "TX ";
            tmp_str += tx_total;

            display.drawString(64, 15, tmp_str);
          }
          else if (step <= 15) {
            display.setFont(Dialog_bold_14);

            tmp_str = "Links ";
            tmp_str += link_total;

            display.drawString(64, 15, tmp_str);
          }
          
        }
        else {
          digitalWrite(LED_BUILTIN, LOW);
          display.setBrightness(255);
          display.setFont(Dialog_bold_14);
          display.setTextAlignment(TEXT_ALIGN_CENTER);

          tmp_str = last_c_1;
          tmp_clean = tmp_str;
          tmp_clean.replace("(", "");
          tmp_clean.replace(")", "");
          display.drawString(64, 0, tmp_clean);

          tmp_str = last_d_1;
          display.drawString(64, 15, tmp_str);
        }

        for(int i = 0; i < display.width(); i += 2) {
          display.drawLine(i,   16, i ,   16);
          //display.drawLine(i,  31, i ,  31);
        }

        //display.drawRect(0, 0, 128, 32);
        display.display();

        step += 1;
        brightness += (direction * 5);

        if (step > 15) {
          step = 0;
          direction = - direction;
        }
      }
    }   
  }
  delay(500);
}