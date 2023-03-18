/*
    This sketch establishes a TCP connection to a "quote of the day" service.
    It sends a "hello" message, and then prints received data.
*/

#include <ESP8266WiFi.h>

#define ssid "ocean"
#define password "20010604"

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  digitalWrite(LED_BUILTIN, LOW);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  unsigned long timeout = millis();
  //Serial.println(timeout);
  Serial.printf("Time : %ld : ", timeout);
  Serial.println("Its working");
  digitalWrite(LED_BUILTIN, LOW); 
  delay(100); 
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000) ;
}
