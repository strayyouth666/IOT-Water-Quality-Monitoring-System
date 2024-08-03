#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

const char* ssid = "your-wifi";
const char* password = "your-wifi-pass";

// URLs
String URL_SENSOR_LOG = "your-url";
const char* apiKey = "your-api-key";

int lcdColumns = 16;
int lcdRows = 2;
const int oneWireBus = 4;
int pHSense = 35; 
int samples = 10; 
float adc_resolution = 4095.0; 

LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); 
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

unsigned long lastTime = 0;  
unsigned long timerDelay = 60000; // 1 minute

float calibration = 0.00; // Adjust this value after calibration

void connectWiFi() {
  WiFi.mode(WIFI_OFF);
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempts++;
    if (attempts > 20) { // Try connect until 20 times
      Serial.println("Failed to connect to WiFi");
      return;
    }
  }
  Serial.print("connected to : "); Serial.println(ssid);
  Serial.print("IP address: "); Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(9600);
  sensors.begin();
  lcd.init();
  lcd.backlight();
  connectWiFi();

  // Initialize LCD display
  lcd.setCursor(0, 0);
  lcd.print("Suhu Air: ");
  lcd.setCursor(0, 1);
  lcd.print("pH Air: ");
}

float ph(float voltage) {     
  // Assuming pH 7 is at 2.5V and pH decreases/increases linearly with voltage
  return 7.0 + ((2.5 - voltage) * 3.5); // Adjust slope and intercept if needed. i'm using 2.5
} 

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // Read pH value
  int measurings = 0;     
  for (int i = 0; i < samples; i++) {         
    measurings += analogRead(pHSense);         
    delay(10);    
  }    
  
  float voltage = 3.3 / adc_resolution * measurings / samples;  
  float pHValue = ph(voltage) + calibration;    
  Serial.print("pH= ");    
  Serial.println(pHValue);     

  // Read temperature value
  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);
  Serial.print(temperatureC);
  Serial.println("ºC");

  // Update LCD display
  lcd.setCursor(10, 0);
  lcd.print(temperatureC);
  lcd.print("C ");
  lcd.setCursor(8, 1);
  lcd.print(pHValue);
  lcd.print("  "); // Clear extra characters if necessary

  // Send sensor log data
  HTTPClient http;
  http.begin(URL_SENSOR_LOG);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-api-key", apiKey); // Add the API key to the header
  
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["ph"] = pHValue;
  jsonDoc["temperature"] = temperatureC;
  jsonDoc["sensorId"] = 2; // Sensor ID 
  
  String requestBody;
  serializeJson(jsonDoc, requestBody);
  
  int httpResponseCode = http.POST(requestBody);
  String response = http.getString();
  
  Serial.println(httpResponseCode);
  Serial.println(response);
  
  http.end();

  delay(timerDelay); // 
}
