#include <M5StickC.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

#include <Wire.h> //The DHT12 uses I2C comunication.
#include "Adafruit_Sensor.h"
#include <Adafruit_BMP280.h>
#include "Adafruit_SHT31.h"
#include "ClosedCube_SHT31D.h"

Adafruit_BMP280 bme;
Adafruit_SHT31 sht31 = Adafruit_SHT31();

ClosedCube_SHT31D sht3xd;

// Variables declaration
const char *ssid = "TP-LINK_2.4GHz_835B49";
const char *password = "65646106";

//button related declarations
const int buttonA = 37;
const int buttonB = 39;
volatile bool buttonPressed = false;

//OWA Declarations
String jsonBuffer;
const String openWeatherMapApiKey = "fc8b187963ac056cea76092473d85799";
String city = "Hof";
double feels_like[5];
double humidityVals[5];
double pressureVals[5];
String tmp1 = " ", hum1 = " ", pressure1 = " ";
String tmp2 = " ", hum2 = " ", pressure2 = " ";

// Your Domain name with URL path or IP address with path
String iftttApiKey = "m9xDvmX7nEEuWqxBkBYwXvdmXdr5ezNlf-ybQq-41wT";
String event = "button_pressed";

//values to be passed for ifttt
String val1 = " ";
String val2 = " ";
String val3 = " ";

const int ledPin = 10;

//IFTTT Button press event
void IRAM_ATTR buttonEvent() {
  buttonPressed = true;
}

void setup() {
  Serial.begin(115200);
  //Initalise LED and Button inputs
  M5.begin();
  Wire.begin(32, 33, 100000);
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.println(F("ENV Unit(DHT12 and BMP280) test..."));
  sht3xd.begin(0x44); // I2C address: 0x44 or 0x45
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }

  M5.Lcd.setCursor(45, 0);
  M5.Lcd.println(WiFi.localIP());
  pinMode(buttonA, INPUT);
  pinMode(buttonB, INPUT);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  digitalWrite(ledPin, HIGH);

  //read weather data from OWA
  getWeatherData();
  
  //Read state of the Button A main button
  int valA = digitalRead(buttonA);

  if (valA == 0) {
    buttonPressed = false;
    sendButtonTrigger();//EMAIL Trigger
    delay(500);
  }

  float tmp = bme.readTemperature();

  SHT31D shtHum = sht3xd.readTempAndHumidity(SHT3XD_REPEATABILITY_LOW, SHT3XD_MODE_CLOCK_STRETCH, 50);
  float hum = shtHum.rh;
  float pressure = bme.readPressure();

  M5.Lcd.fillRect(0, 0, 170, 80, BLACK);

  M5.Lcd.setCursor(0, 0);
  M5.Lcd.printf("Weather readings from ENVII Unit:");

  M5.Lcd.setCursor(0, 20);
  M5.Lcd.printf("Temp: %2.1f  Humid: %2.0f%%  \r\nPressure:%2.0fPa\r\n", tmp, hum, pressure);

  tmp1 = convDoubleToStr(tmp);
  hum1 = convDoubleToStr(hum);
  pressure1 = convDoubleToStr(pressure);

  int analogValue = analogRead(36);
  String lightIntensity = " ";
  val1 = tmp1 + " " + hum1 + " " + pressure1;

  Serial.print("Analog reading = ");
  Serial.print(analogValue);   // the raw analog reading

  //conversion of actual readings to readings between 0-1000
  int minValue = 0;
  int maxValue = 1000;
  int lightValue = round(map(analogValue, 4095, 0, minValue, maxValue ));

  // We'll have a few threshholds, qualitatively determined
  if (lightValue < 10) {
    Serial.println(" - Dark");
    lightIntensity = "Dark";
  } else if (lightValue < 200) {
    lightIntensity = "Dim";
    Serial.println("Dim");
  } else if (lightValue < 500) {
    lightIntensity = "Light";
    Serial.println("Light");
  } else if (lightValue < 800) {
    lightIntensity = "Bright";
    Serial.println("Bright");
  } else {
    lightIntensity = "Very bright";
    Serial.println("Very bright");
  }
  delay(100);
  M5.Lcd.setCursor(0, 50);
  M5.Lcd.printf("Intensity of light from");

  M5.Lcd.setCursor(0, 60);
  M5.Lcd.printf("light sensor: %i", lightValue);
  val3 = String(lightValue);

  delay(500);

  int valB = digitalRead(buttonB);

  //if button is clicked
  if (valB == 0) {

    M5.Lcd.fillRect(0, 0, 170, 80, BLACK);

    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Next 5 days weather forecast!!  ");

    M5.Lcd.setCursor(0, 10);
    M5.Lcd.println("Temperatur|Humidity|Presur");

    M5.Lcd.setCursor(0, 20);
    M5.Lcd.println("-------------------------");

    int temp = 0 ;
    for (int i = 0; i < 5; i++) {

      M5.Lcd.setCursor(0, 30 + temp);
      M5.Lcd.println(feels_like[i]);

      M5.Lcd.setCursor(40, 30 + temp);
      M5.Lcd.println(humidityVals[i]);

      M5.Lcd.setCursor(100, 30 + temp);
      M5.Lcd.println(pressureVals[i]);

      temp = temp + 10;
      delay(500);
    }
    //Serial.println("------------final value---------"+val2);
    delay(5000);

  }

}

String convDoubleToStr(double doubleVal) {
  char charVal[10];               //temporarily holds data from vals
  String stringVal = "";     //data on buff is copied to this string

  dtostrf(doubleVal, 4, 2, charVal);  //4 is mininum width, 4 is precision; float value is copied onto buff
  //display character array
  Serial.print("charVal: ");
  for (int i = 0; i < sizeof(charVal); i++)
  {
    Serial.print(charVal[i]);
  }
  Serial.println();
  //convert chararray to string
  for (int i = 0; i < sizeof(charVal); i++)
  {
    stringVal += charVal[i];
  }
  Serial.print("stringVal: "); Serial.println(stringVal); //display string
  return stringVal;
}

void getWeatherData() {

  // Call first api to get coordinates
  String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "&APPID=" + openWeatherMapApiKey;
  jsonBuffer = httpGETRequest(serverPath.c_str());
  JSONVar myObject = JSON.parse(jsonBuffer);
  double latitude = myObject["coord"]["lat"];
  double longitude = myObject["coord"]["lon"];

  //Call second api with the coordinates to get the feels like data
  serverPath = "http://api.openweathermap.org/data/2.5/onecall?lat=" + String(latitude) + "&lon=" + String(longitude) + "&APPID=" + openWeatherMapApiKey;
  jsonBuffer = httpGETRequest(serverPath.c_str());
  myObject = JSON.parse(jsonBuffer);


  //Store the temp, Humdity, pressure
  for (int i = 0; i < 5; i++) {
    feels_like[i] = myObject["daily"][i]["feels_like"]["day"];
    feels_like[i] = feels_like[i] - 273.15;

    humidityVals[i] = myObject["daily"][i]["pressure"];
    pressureVals[i] = myObject["daily"][i]["humidity"];

    tmp2 = convDoubleToStr(feels_like[i]);
    hum2 = convDoubleToStr(humidityVals[i]);
    pressure2 = convDoubleToStr(pressureVals[i]);
    String dayTemp = String(i + 1);
    val2 = val2 + "Day" + dayTemp + " " + tmp2 + " " + hum2 + " " + pressure2 + " ";
    delay(500);
  }
}

void sendButtonTrigger() {
  
  // Send an HTTP GET request
  // Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED) {

    String serverPath = "https://maker.ifttt.com/trigger/" + event + "/with/key/" + iftttApiKey + "?value1=" + val1 + "&value2=" + val2 + "&value3=" + val3;

    String jsonBuffer = httpGETRequest(serverPath.c_str());
    Serial.println(jsonBuffer);
    JSONVar myObject = JSON.parse(jsonBuffer);

    digitalWrite(ledPin, LOW);
    delay(2000);

    // JSON.typeof(jsonVar) can be used to get the type of the var
    if (JSON.typeof(myObject) == "undefined") {
      Serial.println("Parsing input failed!");
      return;
    }
    Serial.print("JSON object = ");
    Serial.println(myObject);
  }
  else {
    Serial.println("WiFi Disconnected");
  }
}

String httpGETRequest(const char* serverName) {
  HTTPClient http;
  http.begin(serverName);
  int httpResponseCode = http.GET();

  String payload = "{}";

  if (httpResponseCode > 0) {
    payload = http.getString();
  }

  http.end();
  return payload;
}
