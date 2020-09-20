#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

// WiFi Parameters
const char* ssid = "XXX";
const char* password = "XXX"

//LED Parameters
#define PIN 0
#define NUMPIXELS 3
float brightness; //= 1.0;
uint32_t currentColor; //= pixels.Color(0,0,0);
uint32_t AQI_Increasing_Color;
uint32_t AQI_Decreasing_Color;

//AQI Parameters
float PM_now = 0.0;
float PM_10min = 0.0;

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);


void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }
  Serial.println("Connected!");

  pixels.begin();
  pixels.clear();

  brightness = 0.5; //change to time of day
  currentColor = pixels.Color(0, 0, 0);
  AQI_Increasing_Color = pixels.Color(255, 0, 0);
  AQI_Decreasing_Color = pixels.Color(0, 255, 0);

  pixels.fill(pixels.Color(0,0,0), 0, 256);
  pixels.show();
}

void getPmValues(float& _PM_now, float& _PM_10min)
{
  
  // Check WiFi Status
  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;  //Object of class HTTPClient
    http.begin("http://www.purpleair.com/json?show=15091");
    //http.begin("http://www.purpleair.com/json?key=OZUQH57IOWW840PA&show=60015");
    int httpCode = http.GET();

    //Check the returning code
    if (httpCode > 0) {

      const size_t capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(28) + JSON_OBJECT_SIZE(40) + 1340;
      DynamicJsonDocument doc(capacity);
      const char* json = http.getString().c_str();
      deserializeJson(doc, json);

      JsonObject results_0 = doc["results"][0];
      const char* results_0_PM2_5Value = results_0["PM2_5Value"]; // "14.89"
      const char* results_0_Stats = results_0["Stats"];

      //unpack the horribly ugly stats.....
      const size_t _capacity = JSON_OBJECT_SIZE(10) + 60;
      DynamicJsonDocument _doc(_capacity);

      const char* _json = results_0_Stats;

      deserializeJson(_doc, _json);

      float v = _doc["v"]; // 22.61
      float v1 = _doc["v1"]; // 21.19
      float v2 = _doc["v2"]; // 20.23
      float v3 = _doc["v3"]; // 19.79
      float v4 = _doc["v4"]; // 30.02
      float v5 = _doc["v5"]; // 27.03
      float v6 = _doc["v6"]; // 11.33
      float pm = _doc["pm"]; // 22.61

      _PM_now = v;
      _PM_10min = v1;

      Serial.print("PM_now: ");
      Serial.println(_PM_now);
      Serial.print("PM_10min: ");
      Serial.println(_PM_10min);

      http.end();

      //return atof(results_0_PM2_5Value);
    }
    else {
      Serial.print("Error: HTTP Code ");
      Serial.println(httpCode);
      http.end();

    }
  }
}


uint32_t getColorFromPM(float pm, float _brightness)
{
  uint8_t r, g, b = 0;

  if (pm <= 12)       //good
  {
    r = int(pm * 21 * _brightness);
    g = int(255 * _brightness);
    b = 0;

  }
  else if (pm <= 35)  //moderate
  {
    r = int(255 * _brightness);
    g = int((255 - (pm - 12) * 5.65) * _brightness);
    b = 0;
  }
  else if (pm <= 55)  //unhealthy for sensitive individuals
  {
    r = int(255 * _brightness);
    g = int((127 - (pm - 35) * 6.35) * _brightness);
    b = 0;
  }
  else if (pm <= 150) //unhealthy
  {
    r = int(255 * _brightness);
    g = 0;
    b = int((pm - 55) * 2.68 * _brightness);
  }
  else                //very unhealthy
  {
    r = int(255 * _brightness);
    g = 0;
    b = int(255 * _brightness);
  }

  Serial.print("RGB: ");
  Serial.print(r);
  Serial.print(",");
  Serial.print(g);
  Serial.print(",");
  Serial.println(b);

  return pixels.Color(r, g, b);

  /*
    pixels.fill(pixels.Color(r,g,b), 0, NUMPIXELS);
    pixels.setBrightness(brightness);
    pixels.show();
  */

  /*
    for(int i=0; i<NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(r, g, b));
      pixels.show();
    }
  */

}

// Returns the Red component of a 32-bit color
uint8_t Red(uint32_t color)
{
  return (color >> 16) & 0xFF;
}

// Returns the Green component of a 32-bit color
uint8_t Green(uint32_t color)
{
  return (color >> 8) & 0xFF;
}

// Returns the Blue component of a 32-bit color
uint8_t Blue(uint32_t color)
{
  return color & 0xFF;
}

//TODO: improve fade
void fadeToColor(uint32_t _oldColor, uint32_t _newColor, int steps = 30, boolean bidrectional = true)
{
  uint8_t r, g, b = 0;

  for (int i = 0; i < steps; i++)
  {
    r = ((Red(_oldColor) * (steps - i)) + (Red(_newColor) * i)) / steps;
    g = ((Green(_oldColor) * (steps - i)) + (Green(_newColor) * i)) / steps;
    b = ((Blue(_oldColor) * (steps - i)) + (Blue(_newColor) * i)) / steps;

    pixels.setPixelColor(1, pixels.gamma32(pixels.Color(r, g, b))); //assuming 3 LEDs total

    pixels.show();

    delay(66);
  }

  if (bidrectional)
  {
    for (int i = 0; i < steps; i++)
    {
      r = ((Red(_newColor) * (steps - i)) + (Red(_oldColor) * i)) / steps;
      g = ((Green(_newColor) * (steps - i)) + (Green(_oldColor) * i)) / steps;
      b = ((Blue(_newColor) * (steps - i)) + (Blue(_oldColor) * i)) / steps;


      pixels.setPixelColor(1, pixels.gamma32(pixels.Color(r, g, b))); //assuming 3 LEDs total

      pixels.show();

      delay(66);
    }
  }

}

void displayDirectionOfChange()
{
  Serial.println(abs(PM_now - PM_10min));

  if (abs(PM_now - PM_10min) > 1)
  {
    if (PM_now < PM_10min)
    {
      uint32_t decreasingColor = pixels.Color(int(Red(AQI_Decreasing_Color) * brightness), int(Green(AQI_Decreasing_Color) * brightness), int(Blue(AQI_Decreasing_Color) * brightness));
      
      Serial.println("display: AQI getting better");    
      fadeToColor(currentColor, decreasingColor);
    }
    else
    {
      uint32_t increasingColor = pixels.Color(int(Red(AQI_Increasing_Color) * brightness), int(Green(AQI_Increasing_Color) * brightness), int(Blue(AQI_Increasing_Color ) * brightness));
      
      Serial.println("display: AQI getting worse");
      fadeToColor(currentColor, AQI_Increasing_Color);
    }
  }
}

void loop() {

  pixels.clear();

  //TODO: Check time of day to affect brightness

  getPmValues(PM_now, PM_10min);

  currentColor = getColorFromPM(PM_now, brightness);
  pixels.fill(pixels.gamma32(currentColor), 0, NUMPIXELS);
  pixels.show();

  //oscillate a few LEDs to show direction of change
  //otherwise just wait 120 (15*8) seconds
  for (int i = 0; i < 8; i++)
  {
    displayDirectionOfChange();
    delay(15000);
  }

  //setLedColorFromPM(getPmVal(), 255);

}
