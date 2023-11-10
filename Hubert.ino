#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <StreamLib.h>
#include "time.h"
#include <Wire.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
#include <BlynkSimpleEsp32.h>

char auth[] = "8_-CN2rm4ki9P3i_NkPhxIbCiKd5RXhK";  //BLYNK

// Include custom images
//#include "images.h"
const char* ssid = "mikesnet";
const char* password = "springchicken";
const char* PARAM_INPUT_1 = "state";
const char* PARAM_INPUT_2 = "value";

const int output = 2;

String timerSliderValue = "50";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebServer server2(8080);
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;  //Replace with your GMT offset (secs)
const int daylightOffset_sec = 0;   //Replace with your daylight offset (secs)


int hours, mins, secs;
int brightVal= 0;
bool isPM = false;

#define LED_PIN 13




TFT_eSPI tft = TFT_eSPI();       // Invoke custom library
TFT_eSprite img = TFT_eSprite(&tft);
TFT_eSprite img2 = TFT_eSprite(&tft);
TFT_eSprite img3 = TFT_eSprite(&tft);

#define BLACK 0x0000
#define BLUE 0x001F
#define LIGHTRED 0xFC10
#define LIGHTBLUE 0x841F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define LIGHTCYAN 0x87FF
#define MAGENTA 0xFC3B
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define GREY 0xC618
#define ORANGE 0xFE2F
#define LIGHTGREEN 0x8F71

float p = 3.1415926;



int screenW = 128;
int screenH = 64;
int clockCenterX = screenW / 2;
int clockCenterY = ((screenH - 16) / 2) + 20; // top yellow part is 16 px height
int clockRadius = 18;

// utility function for digital clock display: prints leading 0
String twoDigits(int digits) {
  if (digits < 10) {
    String i = '0' + String(digits);
    return i;
  }
  else {
    return String(digits);
  }
}







const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP Web Server</title>
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.4rem;}
    p {font-size: 2.2rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
    .slider2 { -webkit-appearance: none; margin: 14px; width: 300px; height: 20px; background: #ccc;
      outline: none; -webkit-transition: .2s; transition: opacity .2s;}
    .slider2::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 30px; height: 30px; background: #2f4468; cursor: pointer;}
    .slider2::-moz-range-thumb { width: 30px; height: 30px; background: #2f4468; cursor: pointer; } 
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  <p><span id="timerValue">%TIMERVALUE%</span> s</p>
  <p><input type="range" onchange="updateSliderTimer(this)" id="timerSlider" min="0" max="255" value="%TIMERVALUE%" step="1" class="slider2"></p>
  %BUTTONPLACEHOLDER%
<script>
function toggleCheckbox(element) {
  var sliderValue = document.getElementById("timerSlider").value;
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?state=1", true); xhr.send();
    var count = sliderValue, timer = setInterval(function() {
      count--; document.getElementById("timerValue").innerHTML = count;
      if(count == 0){ clearInterval(timer); document.getElementById("timerValue").innerHTML = document.getElementById("timerSlider").value; }
    }, 1000);
    sliderValue = sliderValue*1000;
    setTimeout(function(){ xhr.open("GET", "/update?state=0", true); 
    document.getElementById(element.id).checked = false; xhr.send(); }, sliderValue);
  }
}
function updateSliderTimer(element) {
  var sliderValue = document.getElementById("timerSlider").value;
  document.getElementById("timerValue").innerHTML = sliderValue;
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/slider?value="+sliderValue, true);
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    String outputStateValue = outputState();
    buttons+= "<p><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label></p>";
    return buttons;
  }
  else if(var == "TIMERVALUE"){
    return timerSliderValue;
  }
  return String();
}

String outputState(){
  return "";
}

bool isSleeping = false;

unsigned long millisBlynk;

float pm25in, pm25out, bridgetemp, bridgehum, iaq, windspeed, brtemp, brhum;


BLYNK_WRITE(V71) {
  pm25in = param.asFloat();
}
BLYNK_WRITE(V51) {
  pm25out = param.asFloat();
}

BLYNK_WRITE(V62) {
  bridgetemp = param.asFloat();
}
BLYNK_WRITE(V63) {
  bridgehum = param.asFloat();
}

BLYNK_WRITE(V75) {
  iaq = param.asFloat();
}
BLYNK_WRITE(V56) {
  windspeed = param.asFloat();
}
BLYNK_WRITE(V72) {
  brtemp = param.asFloat();
}
BLYNK_WRITE(V74) {
  brhum = param.asFloat();
}

uint16_t RGBto565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
}

float pmR, pmG, pmB;
float pmR2, pmG2, pmB2;
float pmR3, pmG3, pmB3;

void dodisplay() {
  
  pmG = 55 - pm25in;
  if (pmG < 0) { pmG = 0; }
  pmG *= (200.0 / 55.0);
  if (pmG > 255) { pmG = 255; }

  pmR = pm25in;
  if (pmR < 0) { pmR = 0; }
  pmR *= (255.0 / 55.0);
  if (pmR > 255) { pmR = 255; }

  pmB = pm25in - 100;
  if (pmB < 0) { pmB = 0; }
  pmB *= (255.0 / 55.0);
  if (pmB > 255) { pmB = 255; }
//===============================
  pmG2 = 55 - pm25out;
  if (pmG2 < 0) { pmG2 = 0; }
  pmG2 *= (200.0 / 55.0);
  if (pmG2 > 255) { pmG2 = 255; }

  pmR2 = pm25out;
  if (pmR2 < 0) { pmR2 = 0; }
  pmR2 *= (255.0 / 55.0);
  if (pmR2 > 255) { pmR2 = 255; }

  pmB2 = pm25out - 100;
  if (pmB2 < 0) { pmB2 = 0; }
  pmB2 *= (255.0 / 55.0);
  if (pmB2 > 255) { pmB2 = 255; }
//================================
  pmG3 = 155 - iaq;
  if (pmG3 < 0) {pmG3 = 0;}
  pmG3 *= (255.0/155.0);
  if (pmG3 > 255) {pmG3 = 255;}
  
  pmR3 = iaq;
  if (pmR3 < 0) {pmR3 = 0;}
  pmR3 *= (255.0/155.0);
  if (pmR3 > 255) {pmR3 = 255;}
  
  pmB3 = iaq - 155;
  if (pmB3 < 0) {pmB3 = 0;}
  pmB3 *= (255.0/155.0);
  if (pmB3 > 255) {pmB3 = 255;}

    struct tm timeinfo;
  getLocalTime(&timeinfo);
  hours = timeinfo.tm_hour;
  mins = timeinfo.tm_min;
  secs = timeinfo.tm_sec;

  if ((hours < 8) && (!isSleeping)){
    brightVal = 255;
    analogWrite(LED_PIN, brightVal);
    isSleeping = true;
  }
    if ((hours >= 8) && (isSleeping)){
    brightVal = 1;
    analogWrite(LED_PIN, brightVal);
    isSleeping = false;
  }

    if (hours > 12) {
    hours -= 12;
    isPM = true;
  } else {
    isPM = false;
  }
  if (hours == 12) {isPM = true;}
  if (hours == 0) {hours = 12;}

  tft.setCursor(64, 1);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK, true);
  tft.setTextSize(3);

//String timenow = String(hours) + ":" + twoDigits(mins);
if (hours < 10) {
  if (isPM) {
      String timenow = String(hours) + ":" + twoDigits(mins) + " PM";
      tft.drawString(timenow, 64, 0);
    } 
    else {
      String timenow = String(hours) + ":" + twoDigits(mins) +  " AM";
      tft.drawString(timenow, 64, 0);
    }
  }
  else {
        if (isPM) {
      String timenow = String(hours) + ":" + twoDigits(mins) + "PM";
      tft.drawString(timenow, 64, 0);
    } 
    else {
      String timenow = String(hours) + ":" + twoDigits(mins) +  "AM";
      tft.drawString(timenow, 64, 0);
    }
  }
  img2.setCursor(0, 0);
  img2.setTextSize(2);
  img2.fillScreen(TFT_BLACK);
  img2.setTextColor(LIGHTRED);
  img2.println("iT:");
  img2.setTextColor(LIGHTBLUE);
  img2.println("iH:");
  img2.setTextColor(MAGENTA);
  img2.println("oT:");
  img2.setTextColor(LIGHTCYAN);
  img2.println("oH:");
  img2.setTextColor(LIGHTGREEN);
  img2.println("Ws:");
  img2.pushSprite(0,27);

  img.fillSprite(TFT_BLACK);
  img.setCursor(0,0);
  img.setTextSize(2);
  img.setTextColor(TFT_BLUE, TFT_BLACK);
  img.setTextDatum(TR_DATUM);
  int ypos;
  String stringtodraw = String(brtemp, 2) + "\367""C";
  img.setTextColor(LIGHTRED);
  img.drawString(stringtodraw, 90,0);
  ypos += img.fontHeight();
  stringtodraw = String(brhum, 2) + "g";
  img.setTextColor(LIGHTBLUE);
  img.drawString(stringtodraw, 90,ypos);
  ypos += img.fontHeight();
  stringtodraw = String(bridgetemp, 2) + "\367""C";
  img.setTextColor(MAGENTA);
  img.drawString(stringtodraw, 90,ypos);
  ypos += img.fontHeight();
  stringtodraw = String(bridgehum, 2) + "g";
  img.setTextColor(LIGHTCYAN);
  img.drawString(stringtodraw, 90,ypos);
  ypos += img.fontHeight();
  stringtodraw = String(windspeed, 1) + "kph";
  img.setTextColor(LIGHTGREEN);
  img.drawString(stringtodraw, 90,ypos);
  img.pushSprite(38, 27);

  img3.fillSprite(TFT_BLACK);
  img3.setCursor(0,0);
  img3.setTextFont(2);
  img3.setTextSize(1);
  img3.setTextDatum(TL_DATUM);
 img3.setTextColor(YELLOW, RGBto565(pmR, pmG, pmB));
  img3.drawFloat(pm25in, 1, 0, 0);
  //img3.print(" ");
  img3.setTextDatum(TC_DATUM);
  //img3.setCursor(64,0);
  img3.setTextColor(YELLOW, RGBto565(pmR2, pmG2, pmB2));
  //if (pm25out < 10) {img3.print(" ");}
  //if (pm25in >= 10) {img3.print(pm25out, 1);} else {img3.print(pm25out, 2);}
  img3.drawFloat(pm25out, 1, 64, 0);
  img3.setTextDatum(TR_DATUM);
  //img3.setCursor(128,0);
  img3.setTextColor(YELLOW, RGBto565(pmR3, pmG3, pmB3));
  img3.drawFloat(iaq, 1, 128, 0);
  //img3.print(iaq);
  img3.pushSprite(0, 107);
}
 
void setup()
{
  pinMode(LED_PIN, OUTPUT);
  pm25in = pm25out = bridgetemp = bridgehum = iaq = windspeed = brtemp = brhum = 1.0;
    analogWrite(LED_PIN, 0);
      Serial.begin(115200);
    tft.init();
    tft.fillScreen(TFT_BLACK);
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);
      tft.setCursor(0, 0);
      tft.setTextColor(TFT_WHITE, TFT_BLACK, true);
      tft.setTextWrap(true); // Wrap on width
      tft.print("Connecting...");
      while (WiFi.status() != WL_CONNECTED) {
        delay(250);
        tft.print(".");
      }
      Blynk.config(auth, IPAddress(192, 168, 50, 197), 8080);
      Blynk.connect();
      //display.display();

      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      struct tm timeinfo;
      getLocalTime(&timeinfo);
      hours = timeinfo.tm_hour;
      mins = timeinfo.tm_min;
      secs = timeinfo.tm_sec;
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Hi! I am Hubert.");
      });

      AsyncElegantOTA.begin(&server);    // Start ElegantOTA

      server2.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html, processor);
      });

      // Send a GET request to <ESP_IP>/update?state=<inputMessage>
      server2.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String inputMessage;
        // GET input1 value on <ESP_IP>/update?state=<inputMessage>
        if (request->hasParam(PARAM_INPUT_1)) {
          inputMessage = request->getParam(PARAM_INPUT_1)->value();
          digitalWrite(output, inputMessage.toInt());
        }
        else {
          inputMessage = "No message sent";
        }
        Serial.println(inputMessage);
        request->send(200, "text/plain", "OK");
      });
      
      // Send a GET request to <ESP_IP>/slider?value=<inputMessage>
      server2.on("/slider", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String inputMessage;
        // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
        if (request->hasParam(PARAM_INPUT_2)) {
          inputMessage = request->getParam(PARAM_INPUT_2)->value();
          brightVal = 256 - inputMessage.toInt();
          analogWrite(LED_PIN, brightVal);
          timerSliderValue = inputMessage;
        }
        else {
          inputMessage = "No message sent";
        }
        Serial.println(inputMessage);
        
        request->send(200, "text/plain", "OK");
      });
      
      // Start server
      server2.begin();

      server.begin();
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(ssid);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      analogWrite(LED_PIN, 1);
    tft.fillScreen(TFT_BLACK);
    //tft.drawRect(0,0,128,128, TFT_WHITE);
      img.createSprite(90, 79);
      img.fillSprite(TFT_BLACK);
      img2.createSprite(38, 79);
      img2.fillSprite(TFT_BLACK);
      img3.createSprite(128, 14);
      img3.fillSprite(TFT_BLACK);
      tft.drawFastHLine(0, 24, 128, TFT_WHITE);
          tft.setTextSize(1);
  tft.setTextColor(YELLOW);
  tft.setCursor(0,121);
  tft.print("PM2.5 in/PM2.5out/IAQ");
}



/*BLYNK BRIDGES
================
floortemp:
  bridge2.virtualWrite(V51, pmavgholder);
  bridge2.virtualWrite(V55, inetTemp);
  bridge2.virtualWrite(V56, inetWindspeed);
  bridge2.virtualWrite(V57, inetWindgust);
  bridge2.virtualWrite(V58, inetWinddeg);
bedroom:
  bridge2.virtualWrite(V71, pm25Avg.mean());
  bridge2.virtualWrite(V72, tempSHT);
  bridge2.virtualWrite(V73, humSHT);
  bridge2.virtualWrite(V74, abshumSHT);
  bridge2.virtualWrite(V75, bmeiaq);
outdoors:
  bridge2.virtualWrite(V62, tempBME);
  bridge2.virtualWrite(V63, abshumBME);
  bridge2.virtualWrite(V64, windchill);
  bridge2.virtualWrite(V65, humidex);
===============*/

void loop() {

Blynk.run();
      

    if (millis() - millisBlynk >= 10000)  //if it's been X milliseconds
  {
    millisBlynk = millis();
    dodisplay();
  }
}



