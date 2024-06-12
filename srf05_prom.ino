#include <WiFi.h>
#include <prometheus_metrics.h>
#include <WebServer.h>

#include "SRF05.h"
#include "DHT11.h"

#define DHTPIN 17

#define SSID "Livebox-26D0"
#define PASSWORD "e5SD2jaxGNKA537hHt"
#define PORT 9111
// Set web server port number to 80
WebServer server(PORT);

// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

int lastMeasure = millis();
uint32_t delayMS = 10000;

// SRF05 pins
const unsigned int TRIG_PIN = 13;
const unsigned int ECHO_PIN = 12;

Metric distance(MetricType::gauge, "captor_distance", "Distance to the nearest object (cm)", 1);
Metric temperature(MetricType::gauge, "captor_temperature", "Temperature (celcius)", 1);
Metric humidity(MetricType::gauge, "captor_humidity", "Humidity (%)", 1);

DHT11 dht11(DHTPIN);

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);  //Optional
  WiFi.begin(SSID, PASSWORD);
  Serial.println("\nConnecting");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/metrics", handleMetrics);
  server.begin();
  Serial.printf("HTTP server started on port %d\n", PORT);

  dht11.setDelay(1000);
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();

  if (millis() - lastMeasure > delayMS) {
    measureDist();
    measureTemp();
    lastMeasure = millis();
  }
}

void measureDist() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  const unsigned long duration = pulseIn(ECHO_PIN, HIGH);
  Serial.print("Duration: ");
  Serial.print(duration);
  Serial.println(" ms");
  int dist = duration / 29 / 2;
  if (duration == 0) {
    Serial.println("Warning: no pulse from sensor");
  } else {
    Serial.print("distance to nearest object:");
    Serial.print(dist);
    Serial.println(" cm");
    distance.setValue(float(dist));
  }
}

void measureTemp() {
  int temp = 0;
  int hum = 0;

  // Attempt to read the temperature and humidity values from the DHT11 sensor.
  int result = dht11.readTemperatureHumidity(temp, hum);
  if (result == 0) {
    temperature.setValue(float(temp));
    humidity.setValue(float(hum));
    Serial.printf("Temperature : %d / Humidity %d\n", temp, hum);
  } else {
    Serial.printf("DHT11 error %s\n", dht11.getErrorString(result));
  }
}

void handleRoot() {
  server.send(200, "text/html", "<a href=\"/metrics\">metrics</a>");
}

void handleMetrics() {

  measureDist();
  measureTemp();
  lastMeasure = millis();

  String response = "";
  response += distance.getString();
  response += temperature.getString();
  response += humidity.getString();

  server.send(200, "text/plain; version=0.0.4", response);
}
