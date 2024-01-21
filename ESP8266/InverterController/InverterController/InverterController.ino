#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#define analogPin A0 // Analog Pin of ESP8266 : ADC0 = A0
#define MAINSPIN 4 //GPIO on which Mains Powersupply will be checked
#define PWRON 0x0  //Better to use Low as Power On
#define PWROFF 0x1 //Power "OFF" is high

/*WiFi credentials*/
#define ssid "ocean"
#define password "20010604"

int BattVolt = 0;  // Battery voltage, Variable to store Output of ADC 
int MainsState = 0; // Mains supply status, By deafult, Mains is there 

/* Generic blink function. Blink LED cnt times with delay of dly */
void blink(int cnt = 3,int dly = 100) {
  for (int i=0; i<cnt; i++){
  digitalWrite(LED_BUILTIN, PWRON);
  delay(dly);
  digitalWrite(LED_BUILTIN, PWROFF);
  delay(dly);    
  }   
}

void setup()
{
  Serial.begin(115200); /* Initialize serial communication at 115200 */
  pinMode(MAINSPIN, INPUT); 
  pinMode(LED_BUILTIN, OUTPUT); 
  blink();
  WiFi.mode(WIFI_STA); //Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,would try to act as both a client and an access-point and could cause network-issues with your other WiFi-devices on your WiFi-network. 
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected. IP address:");
  Serial.println(WiFi.localIP());
  blink();
}

void loop()
{
  BattVolt = analogRead(analogPin); /* Read the Analog Input value */
  MainsState = digitalRead(MAINSPIN);
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;
  https.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  if (https.begin(*client,"script.google.com",443,"/macros/s/AKfycbzr53VKDgBmnl4TvKvOjJX961NDY5esN6DKedwpObrTXCIjWpaZKxha0Q5CmKn37yUNpg/exec?item=10&action=R",true)){
      Serial.print("[HTTPS] GET...\n");
      int httpCode = https.GET(); // start connection and send HTTP header
      if (httpCode > 0) { // httpCode will be negative on error
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode); // HTTP header has been send and Server response header has been handled
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {  // file found at server
          String payload = https.getString();
          Serial.println(payload);
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
  } else {
      Serial.printf("[HTTPS] Unable to connect\n");
  }
  
  /* Print the output in the Serial Monitor */
  Serial.print("ADC Value = ");
  Serial.println(BattVolt);
  Serial.printf("Mains is %d ",MainsState);
  delay(1000);
  blink(3,100);
}


