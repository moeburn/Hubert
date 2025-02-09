#include <WiFi.h>
#include <WiFiManager.h> 
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
//#include <AsyncElegantOTA.h>
#include <ArduinoOTA.h>
#include <StreamLib.h>
#include "time.h"
#include <Wire.h>
//REMEMBER TO EDIT USER_SETUP.h FILE!!! =================================================================================================================
//REMEMBER TO EDIT USER_SETUP.h FILE!!! =================================================================================================================
//REMEMBER TO EDIT USER_SETUP.h FILE!!! =================================================================================================================
#include <TFT_eSPI.h> // Hardware-specific library
//REMEMBER TO EDIT USER_SETUP.h FILE!!! =================================================================================================================
//REMEMBER TO EDIT USER_SETUP.h FILE!!! =================================================================================================================
//REMEMBER TO EDIT USER_SETUP.h FILE!!! =================================================================================================================
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
WidgetTerminal terminal(V20);

// Create AsyncWebServer object on port 80
//AsyncWebServer server(80);
AsyncWebServer server2(8080);
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;  //Replace with your GMT offset (secs)
const int daylightOffset_sec = 3600;   //Replace with your daylight offset (secs)


int hours, mins, secs;
int brightVal= 0;
bool autobright = true;
bool isPM = false;

#define LED_PIN 13
#define SCREEN_WIDTH  tft.width()  // 
#define SCREEN_HEIGHT tft.height()
const uint16_t MAX_ITERATION = 300; // Nombre de couleurs



TFT_eSPI tft = TFT_eSPI();       // Invoke custom library
TFT_eSprite img = TFT_eSprite(&tft);
TFT_eSprite img2 = TFT_eSprite(&tft);
TFT_eSprite img3 = TFT_eSprite(&tft);

#define BLACK 0x0000
#define BLUE 0x001F
#define PINK 0xFC10
#define CYAN 0x841F
#define RED 0xF800
#define GREEN 0x07E0
#define LIGHTBLUE 0x07FF
#define LIGHTCYAN 0x87FF
#define LIGHTRED 0xFC18 
#define MAGENTA  0xFC1F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define GREY 0xC618
#define ORANGE 0xFE2F
#define LIGHTGREEN 0x8F71
#define LIGHTORANGE 0xFC08

float p = 3.1415926;
float lightread;



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



#define every(interval) \
    static uint32_t __every__##interval = millis(); \
    if (millis() - __every__##interval >= interval && (__every__##interval = millis()))



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

unsigned long millisBlynk, millisAuto;

float pm25in, pm25out, bridgetemp, bridgehum, bridgepres, iaq, windspeed, brtemp, brhum, bridgeco2, bridgeIrms, watts, kw, neotemp, jojutemp, temptodraw;

float randnum, randnum2, zooom;

float cr, ci;

BLYNK_WRITE(V20) {
  if (String("help") == param.asStr()) {
    terminal.println("==List of available commands:==");
    terminal.println("wifi");
    terminal.println("julia");
    terminal.println("cube");
    terminal.println("==End of list.==");
  }
  if (String("wifi") == param.asStr()) {
    terminal.println("> Connected to: ");
    terminal.println(ssid);
    terminal.print("IP address:");
    terminal.println(WiFi.localIP());
    terminal.print("Signal strength: ");
    printLocalTime();
    terminal.println(WiFi.RSSI());
  }
  if (String("julia") == param.asStr()) {
    randnum = random(0, 1000);
    cr = randnum/1000;
    terminal.println("> ");
    terminal.print(cr);
    terminal.print(",");
    randnum = random(0, 500);
    ci = randnum/1000;
    terminal.print(ci);
    terminal.print(",");
    randnum = random(0, 1000);
    zooom = randnum/10;
    terminal.println(zooom);
    terminal.flush();
    draw_Julia(-0.8,+0.156,zooom);
    delay(5000);
    tft.fillScreen(TFT_BLACK);
    prepdisplay();
  }
  if (String("cube") == param.asStr()) {
    tft.fillScreen(TFT_BLACK);
    doCube();
    tft.fillScreen(TFT_BLACK);
    prepdisplay();
  }
  terminal.flush();
}

void printLocalTime() {
  time_t rawtime;
  struct tm* timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  terminal.print("-");
  terminal.print(asctime(timeinfo));
  terminal.print(" - ");
  terminal.flush();
}

BLYNK_WRITE(V41) {
  neotemp = param.asFloat();
}

BLYNK_WRITE(V42) {
  jojutemp = param.asFloat();
}

BLYNK_WRITE(V71) {
  pm25in = param.asFloat();
}
/*BLYNK_WRITE(V51) {
  pm25out = param.asFloat();
}*/

BLYNK_WRITE(V62) {
  bridgetemp = param.asFloat();
}
BLYNK_WRITE(V63) {
  bridgehum = param.asFloat();
}

BLYNK_WRITE(V66) {
  pm25out = param.asFloat();
}

BLYNK_WRITE(V75) {
  iaq = param.asFloat();
}
BLYNK_WRITE(V78) {
  float ws = param.asFloat();
  if (!isnan(ws)){
    windspeed = ws;
  }
}

BLYNK_WRITE(V79) {
}

BLYNK_WRITE(V72) {
  brtemp = param.asFloat();
}
BLYNK_WRITE(V74) {
  brhum = param.asFloat();
}
BLYNK_WRITE(V76) {
  bridgepres = param.asFloat();
}
BLYNK_WRITE(V77) {
  bridgeco2 = param.asFloat();
}

BLYNK_WRITE(V81) {
  bridgeIrms = param.asFloat();
  watts = bridgeIrms;
  kw = watts / 1000.0;
}

BLYNK_WRITE(V82) {
}

BLYNK_WRITE(V83) {
  lightread  = param.asFloat();
}

uint16_t RGBto565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
}

float pmR, pmG, pmB;
float pmR2, pmG2, pmB2;
float  pmR3, pmG3, pmB3;
float pmR4, pmG4, pmB4;

void prepdisplay() {
        tft.drawFastHLine(0, 24, 128, TFT_WHITE);
          tft.setTextSize(1);
  tft.setTextColor(YELLOW);
  tft.setCursor(0,121);
  //////////"/////////////////////"
  tft.print("PMin/PMout/KW/IAQ/CO2");
}

void getVocColor(int voc_index, uint8_t &r, uint8_t &g, uint8_t &b) {
    float bright = 0.75;
    if (voc_index < 150) {
        r = 0; g = 255; b = 0;
    } else if (voc_index < 250) {
        r = map(voc_index, 150, 250, 0, 255);
        g = 255; b = 0;
    } else if (voc_index < 400) {
        r = 255;
        g = map(voc_index, 250, 400, 255, 128);
        b = 0;
    } else {
        r = 255;
        g = map(voc_index, 400, 500, 128, 0);
        b = 0;
    }
  b = voc_index - 155;
  if (b < 0) {b = 0;}
  b *= (255.0/155.0);

  r *= bright;
  g *= bright;
  b *= bright;
  if (r > 255) {r = 255;}
  if (g > 255) {g = 255;}
  if (b > 255) {b = 255;}
}

void dodisplay() {

  float CO2center = 1000.0;
  float brightness = 0.75;
  
  pmG = 55 - pm25in;
  if (pmG < 0) { pmG = 0; }
  pmG *= (200.0 / 55.0);
  if (pmG > 255) { pmG = 255; }
  pmG *= brightness;

  pmR = pm25in;
  if (pmR < 0) { pmR = 0; }
  pmR *= (255.0 / 55.0);
  if (pmR > 255) { pmR = 255; }
  pmR *= brightness;

  pmB = pm25in - 100;
  if (pmB < 0) { pmB = 0; }
  pmB *= (255.0 / 55.0);
  if (pmB > 255) { pmB = 255; }
  pmB *= brightness;
//===============================
  pmG2 = 55 - pm25out;
  if (pmG2 < 0) { pmG2 = 0; }
  pmG2 *= (200.0 / 55.0);
  if (pmG2 > 255) { pmG2 = 255; }
  pmG2 *= brightness;

  pmR2 = pm25out;
  if (pmR2 < 0) { pmR2 = 0; }
  pmR2 *= (255.0 / 55.0);
  if (pmR2 > 255) { pmR2 = 255; }
  pmR2 *= brightness;

  pmB2 = pm25out - 100;
  if (pmB2 < 0) { pmB2 = 0; }
  pmB2 *= (255.0 / 55.0);
  if (pmB2 > 255) { pmB2 = 255; }
  pmB2 *= brightness;
//================================
  pmG3 = 250 - iaq;
  if (pmG3 < 0) {pmG3 = 0;}
  pmG3 *= (255.0/250.0);
  if (pmG3 > 255) {pmG3 = 255;}
  pmG3 *= brightness;
  
  pmR3 = iaq;
  if (pmR3 < 0) {pmR3 = 0;}
  pmR3 *= (255.0/250.0);
  if (pmR3 > 255) {pmR3 = 255;}
  pmR3 *= brightness;
  
  pmB3 = iaq - 250;
  if (pmB3 < 0) {pmB3 = 0;}
  pmB3 *= (255.0/250.0);
  if (pmB3 > 255) {pmB3 = 255;}
  pmB3 *= brightness;
  //getVocColor(iaq, pmR3, pmG3, pmB3);
//================================
  pmG4 = CO2center - (bridgeco2-400);
  if (pmG4 < 0) {pmG4 = 0;}
  pmG4 *= (255.0/CO2center);
  if (pmG4 > 255) {pmG4 = 255;}
  pmG4 *= brightness;
  
  pmR4 = (bridgeco2-400);
  if (pmR4 < 0) {pmR4 = 0;}
  pmR4 *= (255.0/CO2center);
  if (pmR4 > 255) {pmR4 = 255;}
  pmR4 *= brightness;
  
  pmB4 = (bridgeco2-400) - CO2center;
  if (pmB4 < 0) {pmB4 = 0;}
  pmB4 *= (255.0/CO2center);
  if (pmB4 > 255) {pmB4 = 255;}
  pmB4 *= brightness;

    struct tm timeinfo;
  getLocalTime(&timeinfo);
  hours = timeinfo.tm_hour;
  mins = timeinfo.tm_min;
  secs = timeinfo.tm_sec;

  if ((hours < 8) && (!isSleeping)){
    for(int i=brightVal; i<255; i++)
    {
      analogWrite(LED_PIN, i);
      delay(40);
    }
    analogWrite(LED_PIN, 255);
    isSleeping = true;
  }
    if ((hours >= 8) && (isSleeping)){
    for (int i=255; i>brightVal; i--)
      {
        analogWrite(LED_PIN, i);
        delay(40);
      }
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
  if (bridgetemp > 15) {
    img2.setTextColor(LIGHTCYAN);
    img2.println("oH:");
  }
  else
  {
    img2.setTextColor(LIGHTORANGE);
    img2.println("P:");    
  }
  img2.setTextColor(LIGHTGREEN);
  img2.println("Ws:");
  img2.pushSprite(0,28);

  img.fillSprite(TFT_BLACK);
  img.setCursor(0,0);
  img.setTextSize(2);
  img.setTextColor(TFT_BLUE, TFT_BLACK);
  img.setTextDatum(TR_DATUM);
  int ypos;
  String stringtodraw = String(brtemp, 2) + " C";
  img.drawCircle(72, 3, 2, LIGHTRED);
  img.drawCircle(72, 3, 3, LIGHTRED);
  img.setTextColor(LIGHTRED);
  img.drawString(stringtodraw, 90,0);
  ypos += img.fontHeight();
  stringtodraw = String(brhum, 2) + "g";
  img.setTextColor(LIGHTBLUE);
  img.drawString(stringtodraw, 90,ypos);
  ypos += img.fontHeight();
  float min1 = min(neotemp, jojutemp);
  temptodraw = min(bridgetemp, min1);
  stringtodraw = String(temptodraw, 2) + " C";
  img.drawCircle(72, 35, 2, MAGENTA);
  img.drawCircle(72, 35, 3, MAGENTA);
  img.setTextColor(MAGENTA);
  img.drawString(stringtodraw, 90,ypos);
  ypos += img.fontHeight();
  if (bridgetemp > 15) {
    stringtodraw = String(bridgehum, 2) + "g";
    img.setTextColor(LIGHTCYAN);
    img.drawString(stringtodraw, 90,ypos);
  }
  else {
    stringtodraw = String(bridgepres, 1) + "mb";
    img.setTextColor(LIGHTORANGE);
    img.drawString(stringtodraw, 90,ypos);    
  }
  ypos += img.fontHeight();
  stringtodraw = String(windspeed, 1) + "kph";
  img.setTextColor(LIGHTGREEN);
  img.drawString(stringtodraw, 90,ypos);
  img.pushSprite(38, 28);

  img3.fillSprite(TFT_BLACK);
  img3.setCursor(0,0);
  img3.setTextFont(2);
  img3.setTextSize(1);
  img3.setTextDatum(TL_DATUM);
 img3.setTextColor(WHITE, RGBto565(pmR, pmG, pmB));
  img3.drawFloat(pm25in, 0, 0, 0);
  //img3.print(" ");
  img3.setTextDatum(TC_DATUM);
  //img3.setCursor(64,0);
  img3.setTextColor(WHITE, RGBto565(pmR2, pmG2, pmB2));
  //if (pm25out < 10) {img3.print(" ");}
  //if (pm25in >= 10) {img3.print(pm25out, 1);} else {img3.print(pm25out, 2);}
  img3.drawFloat(pm25out, 0, 26, 0);
//  img3.setTextDatum(TC_DATUM);
  //img3.setCursor(128,0);
  img3.setTextColor(WHITE);
  int watts_x = 52; //because I cite it twice in the if/else statement:
  if (watts < 1000) {img3.drawFloat(watts, 0, watts_x, 0);}
  else {img3.drawFloat(kw, 1, watts_x, 0);}
  img3.setTextColor(WHITE, RGBto565(pmR3, pmG3, pmB3));
  img3.drawFloat(iaq, 0, 80, 0);
  //img3.print(iaq);
  img3.setTextDatum(TR_DATUM);
  img3.setTextColor(WHITE, RGBto565(pmR4, pmG4, pmB4));
  img3.drawFloat(bridgeco2, 0, 128, 0);
  img3.pushSprite(0, 107);
}


void draw_Julia(float c_r, float c_i, float zoom) {

  tft.setCursor(0,0);
  float new_r = 0.0, new_i = 0.0, old_r = 0.0, old_i = 0.0;

  /* Pour chaque pixel en X */

  for(int16_t x = SCREEN_WIDTH/2 - 1; x >= 0; x--) { // Rely on inverted symettry
    /* Pour chaque pixel en Y */
    for(uint16_t y = 0; y < SCREEN_HEIGHT; y++) {      
      old_r = 1.5 * (x - SCREEN_WIDTH / 2) / (0.5 * zoom * SCREEN_WIDTH);
      old_i = (y - SCREEN_HEIGHT / 2) / (0.5 * zoom * SCREEN_HEIGHT);
      uint16_t i = 0;

      while ((old_r * old_r + old_i * old_i) < 4.0 && i < MAX_ITERATION) {
        new_r = old_r * old_r - old_i * old_i ;
        new_i = 2.0 * old_r * old_i;

        old_r = new_r+c_r;
        old_i = new_i+c_i;
        
        i++;
      }
      /* Affiche le pixel */
      if (i < 100){
        tft.drawPixel(x,y,tft.color565(255,255,map(i,0,100,255,0)));
        tft.drawPixel(SCREEN_WIDTH - x - 1,SCREEN_HEIGHT - y - 1,tft.color565(255,255,map(i,0,100,255,0)));
      }if(i<200){
        tft.drawPixel(x,y,tft.color565(255,map(i,100,200,255,0),0));
        tft.drawPixel(SCREEN_WIDTH - x - 1,SCREEN_HEIGHT - y - 1,tft.color565(255,map(i,100,200,255,0),0));
      }else{
        tft.drawPixel(x,y,tft.color565(map(i,200,300,255,0),0,0));
        tft.drawPixel(SCREEN_WIDTH - x - 1,SCREEN_HEIGHT - y - 1,tft.color565(map(i,200,300,255,0),0,0));
      }
    }
  }
}

const float sin_d[] = {
  0, 0.17, 0.34, 0.5, 0.64, 0.77, 0.87, 0.94, 0.98, 1, 0.98, 0.94,
  0.87, 0.77, 0.64, 0.5, 0.34, 0.17, 0, -0.17, -0.34, -0.5, -0.64,
  -0.77, -0.87, -0.94, -0.98, -1, -0.98, -0.94, -0.87, -0.77,
  -0.64, -0.5, -0.34, -0.17
};
const float cos_d[] = {
  1, 0.98, 0.94, 0.87, 0.77, 0.64, 0.5, 0.34, 0.17, 0, -0.17, -0.34,
  -0.5, -0.64, -0.77, -0.87, -0.94, -0.98, -1, -0.98, -0.94, -0.87,
  -0.77, -0.64, -0.5, -0.34, -0.17, 0, 0.17, 0.34, 0.5, 0.64, 0.77,
  0.87, 0.94, 0.98
};
const float d = 10;
float px[] = {
  -d, d, d, -d, -d, d, d, -d
};
float py[] = {
  -d, -d, d, d, -d, -d, d, d
};
float pz[] = {
  -d, -d, -d, -d, d, d, d, d
};

float p2x[] = {
  0, 0, 0, 0, 0, 0, 0, 0
};
float p2y[] = {
  0, 0, 0, 0, 0, 0, 0, 0
};

int r[] = {
  0, 0, 0
};

void doCube() {
delay(16);
  for (int k = 0; k < 400; k++) {
    //tft.fillScreen(TFT_BLACK);
    r[0] = r[0] + 1;
    r[1] = r[1] + 1;
    if (r[0] == 36) r[0] = 0;
    if (r[1] == 36) r[1] = 0;
    if (r[2] == 36) r[2] = 0;
    for (int i = 0; i < 8; i++) {

      float px2 = px[i];
      float py2 = cos_d[r[0]] * py[i] - sin_d[r[0]] * pz[i];
      float pz2 = sin_d[r[0]] * py[i] + cos_d[r[0]] * pz[i];

      float px3 = cos_d[r[1]] * px2 + sin_d[r[1]] * pz2;
      float py3 = py2;
      float pz3 = -sin_d[r[1]] * px2 + cos_d[r[1]] * pz2;

      float ax = cos_d[r[2]] * px3 - sin_d[r[2]] * py3;
      float ay = sin_d[r[2]] * px3 + cos_d[r[2]] * py3;
      float az = pz3 - 190;

      p2x[i] = ((tft.width()) / 2) + ax * 500 / az;
      p2y[i] = ((tft.height()) / 2) + ay * 500 / az;
    }
    for (int i = 0; i < 3; i++) {
      tft.drawLine(p2x[i], p2y[i], p2x[i + 1], p2y[i + 1], WHITE);
      tft.drawLine(p2x[i + 4], p2y[i + 4], p2x[i + 5], p2y[i + 5], RED);
      tft.drawLine(p2x[i], p2y[i], p2x[i + 4], p2y[i + 4], BLUE);
    }
    tft.drawLine(p2x[3], p2y[3], p2x[0], p2y[0], MAGENTA);
    tft.drawLine(p2x[7], p2y[7], p2x[4], p2y[4], GREEN);
    tft.drawLine(p2x[3], p2y[3], p2x[7], p2y[7], YELLOW);
    //delay(20);
    for (int i = 0; i < 3; i++) {
      tft.drawLine(p2x[i], p2y[i], p2x[i + 1], p2y[i + 1], BLACK);
      tft.drawLine(p2x[i + 4], p2y[i + 4], p2x[i + 5], p2y[i + 5], BLACK);
      tft.drawLine(p2x[i], p2y[i], p2x[i + 4], p2y[i + 4], BLACK);
    }
    tft.drawLine(p2x[3], p2y[3], p2x[0], p2y[0], BLACK);
    tft.drawLine(p2x[7], p2y[7], p2x[4], p2y[4], BLACK);
    tft.drawLine(p2x[3], p2y[3], p2x[7], p2y[7], BLACK);
  }
}



 
void setup()
{
  pinMode(LED_PIN, OUTPUT);
  pm25in = pm25out = bridgetemp = bridgehum = iaq = windspeed = brtemp = brhum = 1.0;
    analogWrite(LED_PIN, 128);
      Serial.begin(115200);
    tft.init();
    tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_WHITE, TFT_BLACK, true);
      tft.setTextWrap(true); // Wrap on width
          tft.setCursor(0, 0);
          tft.println("Connecting to wifi...");
      WiFi.mode(WIFI_STA);
      WiFiManager wm;
      bool res;
      // res = wm.autoConnect(); // auto generated AP name from chipid
       res = wm.autoConnect("HubertSetup"); // anonymous ap


      if(!res) {
          tft.fillScreen(TFT_RED);
          tft.setCursor(0, 0);
          tft.println("Failed to connect, restarting");
          delay(3000);
          ESP.restart();
      } 
    else {
        //if you get here you have connected to the WiFi    
        tft.fillScreen(TFT_GREEN);
        tft.setCursor(0, 0);
        tft.println("Connected!");
        tft.println(WiFi.localIP());
        delay(3000);
      }
      //WiFi.begin(ssid, password);
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0);

      tft.print("Connecting to Blynk...");
      /*while (WiFi.status() != WL_CONNECTED) {
        delay(250);
        tft.print(".");
      }*/
      Blynk.config(auth, IPAddress(216,110,224,105), 8080);
      Blynk.connect();
      //display.display();

      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      struct tm timeinfo;
      getLocalTime(&timeinfo);
      hours = timeinfo.tm_hour;
      mins = timeinfo.tm_min;
      secs = timeinfo.tm_sec;
        terminal.println("***Hubert the Clock v1.4***");

  terminal.print("Connected to ");
  terminal.println(ssid);
  terminal.print("IP address: ");
  terminal.println(WiFi.localIP());

  printLocalTime();
  terminal.flush();


      //AsyncElegantOTA.begin(&server);    // Start ElegantOTA

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
          brightVal = 255 - inputMessage.toInt();
          analogWrite(LED_PIN, brightVal);
          Blynk.virtualWrite(V2, (255-brightVal));
          autobright = false;
          millisAuto = millis();
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

      ArduinoOTA.setHostname("Hubert");
      ArduinoOTA.begin();
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
prepdisplay();
  dodisplay();
  for (int i=255; i>0; i--)
  {
    analogWrite(LED_PIN, i);
    delay(10);
  }
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
ArduinoOTA.handle();
Blynk.run();
      

    if (millis() - millisBlynk >= 10000)  //if it's been X milliseconds
  {
    millisBlynk = millis();
    dodisplay();
    if (autobright){
      int lightmap = map(lightread, 0, 25000, 255, 0);
      if (lightmap > 255) {lightmap = 255;}
      if (lightmap < 1) {lightmap = 1;}
      analogWrite(LED_PIN, lightmap);
      Blynk.virtualWrite(V2, (255-lightmap));
    }
    else {
      if (millis() - millisAuto >= 3600000) { //every hour
        //millisAuto = millis();
        autobright = true;
      }
    }
  }

  if (!autobright) {

  }

}
