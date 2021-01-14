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
int alternance = 0;

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

int interpolation(int value, int in_min, int in_max, int out_min, int out_max) {
  if ((in_max - in_min) != 0) {
    return int((value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
  }
  else {
    return 0;
  }
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

  const char *salon, *emission;
  const char *last_c_1, *last_d_1;
  const char *legende[] = {"00", "06", "12", "18", "23"};

  int tot = 0, link_total = 0, tx_total = 0, max_level = 0, tx[24] = {0};
  int optimize = 0;

  buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW) {
    buttonRoom += 1;
  }
  
  if (buttonRoom > 5) {
    buttonRoom = 0;
  }

  if ((WiFi.status() == WL_CONNECTED)) { // check the current connection status

    http.begin(client, endpoint[buttonRoom]); // specify the URL
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
        emission = doc["abstract"][0]["Emission cumulée"];

        link_total = doc["abstract"][0]["Links connectés"];
        tx_total = doc["abstract"][0]["TX total"];

        tot = doc["transmit"][0]["TOT"];

        last_c_1 = doc["last"][0]["Indicatif"];
        last_d_1 = doc["last"][0]["Durée"];
    
        max_level = 0;
        for (uint8_t i = 0; i < 24; i++) {
          int tmp = doc["activity"][i]["TX"];
          if (tmp > max_level) {
            max_level = tmp;
          }
          tx[i] = tmp;
        }

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

            display.drawString(64, 18, tmp_str);
          }
          else if (step <= 10) {
            display.setFont(Dialog_bold_14);

            tmp_str = "TX ";
            tmp_str += tx_total;

            display.drawString(64, 18, tmp_str);
          }
          else if (step <= 15) {
            display.setFont(Dialog_bold_14);

            tmp_str = "Links ";
            tmp_str += link_total;

            display.drawString(64, 18, tmp_str);
          }
          else if (step <= 20) {
            display.clear();
            int x = 4;
            int tmp = 0;
            
            for (uint8_t j = 0; j < 24; j++) {
              if (tx[j] != 0) {
                tmp = map(tx[j], 0, max_level, 0, 21);
                display.fillRect(x, 23 - tmp, 2, tmp);
              }
              x += 5;
            }
    
            for (uint8_t j = 4; j < 124; j += 5) {
              display.drawLine(j, 24, j + 3, 24);
            }

            display.setFont(TomThumb4x6);
            for (uint8_t j = 0; j < 5; j++) {
              display.drawString(j * 30, 26, legende[j]);
            }
          }
          
          if (step <= 15) {
            for(int i = 0; i < display.width(); i += 2) {
              display.drawLine(i,   16, i ,   16);
            }
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

          if (alternance == 0) {
            alternance = 1;
          }
          else {
            alternance = 0;
          }

          display.drawString(64, 19, tmp_str);

          if (strcmp ("RRF", salon) == 0) {
            tot = map(tot, 0, 150, 0, 100);
          }
          else {
            tot = map(tot, 0, 300, 0, 100);
          }

          display.drawProgressBar(0, 14, 127, 6, tot);
        }

        //display.drawRect(0, 0, 128, 32);
        display.display();

        step += 1;
        brightness += (direction * 5);

        if (step > 20) {
          step = 0;
          direction = - direction;
        }
      }
    }   
  }
  delay(500);
}