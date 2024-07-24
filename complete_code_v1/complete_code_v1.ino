#include <WiFi.h>
#include <HTTPClient.h> 
#include <string>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ArduinoJson.h>
#define DHTTYPE    DHT11 

using namespace std;
unsigned long previousMillis = 0; // Store the last time the function was called
const long interval = 3600000; // 60 minutes in milliseconds
struct WeatherData {
  String temperature;
  String humidity;
};
const int PROGRAMDELAY = 1000;
//DEFAULT IRRIGATION TIME 20 minutes
unsigned long irrigationTime = 60000 * 20;
const int irrigationTriggeringThreshold = 60;
String relay1_name = "relay1";

String relay1_status = "";
String relay2_name = "relay2";
String relay2_status = "";
//RELAY PINS
const int relay1 = 18;
const int relay2 = 19;
//WIFI
const char* ssid = "Kua Zone Field";//"Kua Zone Field";"Interstellar";
const char* password ="kuazone@2024"; //"kuazone@2024";"11207203001";
//DHT11 sensor PINS
const int dhtsensor1 = 21;
const int dhtsensor2 = 22;
DHT_Unified dht_one(dhtsensor1, DHTTYPE);

DHT_Unified dht_two(dhtsensor2, DHTTYPE);
uint32_t delay_DHT_MS;
//soil moisture sensors
const int wet = 1000;
const int dry = 4095;
const int soil_moisture_sensor_1 = 33;
const int soil_moisture_sensor_2 = 32;
const int soil_moisture_sensor_3 = 35;
//const int soil_moisture_sensor_4 = 35;
//const int soil_moisture_sensor_5 = 32;
const int soil_mositure_array [] = {soil_moisture_sensor_1, soil_moisture_sensor_2, soil_moisture_sensor_3};
//SERVER SETTINGS
String HOST_NAME   = "https://kuazonegroup.com/"; 
unsigned long lastTime = 0;
unsigned long timeDelay = 5000;
////serial comm



void setup(){
    Serial.begin(9600);
    setupSoilMositureSensors();
    setupWIFI();
    delay(1000);
    pinMode(soil_moisture_sensor_1, INPUT);
    pinMode(soil_moisture_sensor_2, INPUT);
    pinMode(soil_moisture_sensor_3, INPUT);
    Serial.println("SETTING ");
    WeatherData weatherData = useWeb();
    String temperature1 = weatherData.temperature;
    String humidity1 = weatherData.humidity;
    if(temperature1 !="Error"){
      sendData("temperature_1" ,  temperature1);
      sendData("humidity_1" ,  humidity1);
    }
   
    
}


void loop(){
  
  if (WiFi.status() != WL_CONNECTED) {
    //Serial.println("WiFi connection lost. Reconnecting...");
    digitalWrite(relay1, LOW);
    digitalWrite(relay2, LOW);
    reconnectWIFI();
  }
  delay(8009);

  
  //upload soil moisture data
  int combined_drip_poles_moisture = 0 ;
  int combined_drip_without_poles_moisture = 0;
  for (int i = 0; i<sizeof(soil_mositure_array)/sizeof(soil_mositure_array[0]); i++){
    String moisturedata = getSoilMoisture(i);
    if (i<=2){
      combined_drip_poles_moisture += moisturedata.toInt();
    }else{
      combined_drip_without_poles_moisture += moisturedata.toInt();
    }
    //Serial.println();
    //Serial.print("soilmoisture_sensor"+String(i) +":" );
    //Serial.print(moisturedata);
    //Serial.println();
    sendData("soilmoisture_sensor"+String(i) ,  moisturedata);
    delay(2000);
  }
  ////Serial.print("COMBINED DRIP POLES..");
  ////Serial.print(combined_drip_poles_moisture);
  ////Serial.print("\n");
  ////Serial.print("COMBINED DRIP without POLES..");
  ////Serial.print(combined_drip_without_poles_moisture);
  ////Serial.print("\n");
  //Upload temperature and humidity
  unsigned long currentMillis = millis();

  // Check if 60 minutes have passed
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    WeatherData weatherData = useWeb();
    String temperature1 = weatherData.temperature;
    String humidity1 = weatherData.humidity;
    if(temperature1 !="Error"){
      sendData("temperature_1" ,  temperature1);
      sendData("humidity_1" ,  humidity1);
    }
  }
  
  

 
  //Irrigate on threshold
  String relay1_st = getRelayStatus1();
  String relay2_st = getRelayStatus2();
  if(relay1_st=="0"){
    Serial.println("L");
  }
  else if(relay1_st=="1"){
    Serial.println("H");
  }
  ////Serial.println("relay 2 is " + relay2_st);
  
  /* if (combined_drip_poles_moisture/3 <=irrigationTriggeringThreshold){
    digitalWrite(relay1, HIGH);
    relay1_status = "ON";
    sendData(relay1_name ,  relay1_status);
  }else{
    digitalWrite(relay1, LOW);
    relay1_status = "OFF";
    sendData(relay1_name ,  relay1_status);
  }
  if (combined_drip_without_poles_moisture/2 <=irrigationTriggeringThreshold){
    digitalWrite(relay2, HIGH);
    relay2_status = "ON";
    sendData(relay2_name ,  relay2_status);
  }else{
    digitalWrite(relay2, LOW);
    relay2_status = "OFF";
    sendData(relay2_name ,  relay2_status);
  } */
}

void setupSoilMositureSensors(){
   for (int i = 0; i<sizeof(soil_mositure_array)/sizeof(soil_mositure_array[0]); i++){
      pinMode(soil_mositure_array[i], INPUT);
    }
}
String getSoilMoisture(int index){
  int cap_1 = analogRead(soil_mositure_array[index]);
  ////Serial.println("******");
  ////Serial.println(cap_1);
  ////Serial.println("******");
  int soilmoisturepercent = map(cap_1, dry, wet, 0, 100);
  return String(soilmoisturepercent);
}

void reconnectWIFI(){
    //WIFI
    WiFi.mode(WIFI_STA); 
    WiFi.begin(ssid, password);
    //Serial.println("\nConnecting");

    while(WiFi.status() != WL_CONNECTED){
        //Serial.print(".");
        delay(100);
    }
    //Serial.println("Connected");
}
void setupWIFI(){
    //WIFI
    WiFi.mode(WIFI_STA); 
    WiFi.begin(ssid, password);
    //Serial.println("\nConnecting");

    while(WiFi.status() != WL_CONNECTED){
        //Serial.print(".");
        delay(100);
    }

    //Serial.println("\nConnected to the WiFi network");
    //Serial.print("Local ESP32 IP: ");
    //Serial.println(WiFi.localIP()); 

}

void setupRelay(){
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
   

}
void setupDHTsensors(){
   //temp & humidity
    dht_one.begin();
    //dht_two.begin();
    sensor_t sensor_one;
    //sensor_t sensor_two;
    dht_one.temperature().getSensor(&sensor_one);
    //dht_two.temperature().getSensor(&sensor_two);
    dht_one.humidity().getSensor(&sensor_one);
    //dht_two.humidity().getSensor(&sensor_two);
    delay_DHT_MS = sensor_one.min_delay / 1000;

}

String get_dht_one_sensor_temp(){
  sensors_event_t event_dht_one;
  dht_one.temperature().getEvent(&event_dht_one);
  if (isnan(event_dht_one.temperature)) {
    Serial.println(F("Error reading temperature!"));
    String error = "nil";

    return error;
    
    
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(event_dht_one.temperature);
    Serial.println(F("°C"));
    return String(event_dht_one.temperature);
  }
}

String get_dht_one_sensor_humidity(){
  sensors_event_t event_dht_one;
  dht_one.humidity().getEvent(&event_dht_one);
  if (isnan(event_dht_one.relative_humidity)) {
    //Serial.println(F("Error reading humidity on dht_one! "));
    String error = "nil";
    return error;
  }
  else {
    //Serial.print(F("Humidity dht_one: "));
    //Serial.print(event_dht_one.relative_humidity);
    //Serial.println(F("%"));
    return String(event_dht_one.relative_humidity);
  }
}

String getRelayStatus1(){
  HTTPClient http;
  String  url  = HOST_NAME + "relay_status1/";
  http.begin(url);
  int httpCode = http.GET();
  String payload = "";
   // Check for a successful request
  if (httpCode > 0) {
    payload = http.getString();
  } else {
    //Serial.println("Error on HTTP request");
  }
  return payload;
}
String getRelayStatus2(){
  HTTPClient http;
  String  url  = HOST_NAME + "relay_status2/";
  http.begin(url);
  int httpCode = http.GET();
  String payload = "";
   // Check for a successful request
  if (httpCode > 0) {
    payload = http.getString();
    //Serial.println(httpCode);
    //Serial.println(payload);
  } else {
    //Serial.println("Error on HTTP request");
  }
  return payload;
}



void sendData( String sensorname, String measuredData){
  //Serial.println("Attempting to send data...");
  HTTPClient http;
  String PATH_NAME = HOST_NAME + "iot_data/";
  bool connected = http.begin(PATH_NAME);
  if (!connected){
    //Serial.println("Failed to connect to server");
    return;
  }
  http.addHeader("Content-Type", "application/json");
  String data = "{\"sensorname\":\"" + sensorname + "\",\"measured_data\":\"" + measuredData + "\"}";

  int responseCode = http.POST(data);
  //Serial.print("HTTP Response code: ");
  //Serial.println(responseCode);
  http.end();

}

// Your API key
const char* apiKey = "d97f3ae15179422bb23195826240807";

// The endpoint URL
const char* apiEndpoint = "https://api.weatherapi.com/v1/current.json";


WeatherData useWeb() {
  WeatherData data;
  
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = String(apiEndpoint) + "?key=" + apiKey + "&q=-1.20,36.78&aqi=no";
    http.begin(url);

    int httpCode = http.GET();
    Serial.print("HTTP Response code: ");
    Serial.println(httpCode);

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Received payload:");
      Serial.println(payload);

      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        float temp_c = doc["current"]["temp_c"];
        int humidity = doc["current"]["humidity"];

        // Convert to strings
        data.temperature = String(temp_c);
        data.humidity = String(humidity);
      } else {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        data.temperature = "Error";
        data.humidity = "Error";
      }
    } else {
      Serial.print("HTTP request failed, error: ");
      Serial.println(httpCode);
      data.temperature = "Error";
      data.humidity = "Error";
    }

    http.end();
  } else {
    Serial.println("WiFi not connected");
    data.temperature = "Error";
    data.humidity = "Error";
  }
  
  return data;
}