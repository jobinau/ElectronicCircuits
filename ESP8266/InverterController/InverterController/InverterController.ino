/* Program for automating an old home inverter */
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
using namespace std;

#define analogPin A0 // Analog Pin of ESP8266 : ADC0 = A0
#define MAINSPIN 4 //GPIO on which Mains Powersupply will be checked
#define SWITCHOVER 5 //Relay for AC to Inverter switchover
#define INVERTER 12  //The "ON" switch on the inverter  
#define INVCHARGE 14  //Switch on the inverter charging.
#define PWRON 0x0  //Better to use Low as Power On
#define PWROFF 0x1 //Power "OFF" is high

/*WiFi credentials*/
#define ssid "ocean"
#define password "20010604"
//const char* apiURL = "/macros/s/AKfycbwNhKo1tLQVTMt_UgvkwDQYg8n9bMTByq5WlNAPc6QHVNFshR4gSqTRmnzACF3tjvDq8w/exec";  //--APIv3
const char* apiURL = "/macros/s/AKfycbwRcAb0-1mbDtH8b0w4-32zdNcS6t5wdM-qtMfpTkQAl-AU0uZVyHicoXirVuLp_AP1WA/exec";  //--APIV4

int loopCnt=0;
int BattVolt = 0;  // Curren tBattery voltage, Variable to store Output of ADC 
int MainsState = 0; // Mains supply status, By deafult, Mains is there "0", "1" means power went
int PrevBattVolt = 0;  //Previous Known battery voltage in ADC. So integer
int PrevMainsState = 0;  //Previous Known Mains status, 0 = Mains available 1 = Mains gone
double TrgrVolt=14;
bool invPwerOn = true;  //Switch on the inverter if power goes
bool batChrgOn = false; //Charge battery from mains.


BearSSL::WiFiClientSecure client;
HTTPClient https;

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
  pinMode(SWITCHOVER,OUTPUT);
  digitalWrite(SWITCHOVER,PWROFF);
  pinMode(INVERTER,OUTPUT);
  pinMode(INVCHARGE,OUTPUT);
  pinMode(MAINSPIN, INPUT); 
  pinMode(LED_BUILTIN, OUTPUT);
  blink();
  WiFi.mode(WIFI_STA); //Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,would try to act as both a client and an access-point and could cause network-issues with your other WiFi-devices on your WiFi-network. 
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    //delay(200);
    blink(1,200);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected. IP address:");
  Serial.println(WiFi.localIP());
  client.setInsecure();
  https.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  blink();
}

//Take Readings and decide whether to switch on / off the inverter
void checkMainsnVoltage(){
  BattVolt = analogRead(analogPin); // Read the Analog Input value for checking the battery voltage
  MainsState = digitalRead(MAINSPIN);  //Check the mains status
  if(invPwerOn && (MainsState == 1 || BattVolt > 66.7*TrgrVolt )){  //inverter can be switched on, Then power goes or battery volage increases
    //INVERTER
    digitalWrite(INVERTER,HIGH); //Switchon inverter
    blink(4,100);
    delay(3500);//Wait for  3.5 seconds for inverter to startup
    blink(4,100);
    digitalWrite(SWITCHOVER,PWRON); //Switchover to inverter
  }else if ( BattVolt < 66.7*13.2 ) { //Power is available
    digitalWrite(SWITCHOVER,PWROFF); //Switchover to mains
    blink(4,100);
    digitalWrite(INVERTER, LOW); //Switch-off Inverter
  }
}

void chargeInveter(){
  if (batChrgOn) digitalWrite(INVCHARGE, HIGH);
  else digitalWrite(INVCHARGE, LOW);
}

void loop()
{
  loopCnt++;
  checkMainsnVoltage();  //The main function to trigger the inveter
  chargeInveter(); //Decide whether to charge the battery using inverter
  Serial.printf("GLOBAL invPwerOn : %s , batChrgOn : %s , TrgrVolt : %f \n", invPwerOn ? "true" : "false",batChrgOn ? "true" : "false",TrgrVolt);
  Serial.printf("Current Batt Volt = %f, Mains is %s \n",BattVolt/66.7, MainsState == 0 ? "Available" : "Not Available");
  if ( abs(PrevBattVolt - BattVolt) > 5 ){
    updateBattVoltage(BattVolt);
    loopCnt=1;
  }
  if (PrevMainsState != MainsState) {
    updateMainsState(MainsState);
    loopCnt=1;
  }
  if (loopCnt % 10 == 0){
    Serial.println("Getting Status using API");
    getAPIupdates();
  }

  delay(2000);

  delay(500);
  //wifi_fpm_do_sleep(10000);
  

  blink(3,100);
int loopCnt=0;
}

int updateBattVoltage(int volt){
 PrevBattVolt=volt;
 //Item=2, Status=volt, Write
 char buffer[10];
 sprintf(buffer,"%.1f",volt/66.7);
 Serial.printf("updateBattVoltage is called with VoltADC:%d and calculated voltage is %s",volt,buffer);
 communicateToAPI(2,buffer,'W');
 return 0;
}

int updateMainsState(int MainsState){
  PrevMainsState = MainsState;
  if (MainsState == 1) communicateToAPI(1,"OFF",'W');
  else communicateToAPI(1,"ON",'W');
  return 0;
}

int getAPIupdates(){
  //Just pass a dummy status "x" and invoke API Read
  communicateToAPI(1,"x",'R');
  return 0;
}

int communicateToAPI(int item,const char * stat,char RW){
  char strURL[150];  //The final formatted string URL to send to the API server
  char charArr[5];
  strcpy(strURL, apiURL);
  strcat(strURL,"?item=");
  sprintf(charArr, "%d", item);
  strcat(strURL,charArr);
  strcat(strURL,"&action=");
  sprintf(charArr, "%c", RW);
  strcat(strURL,charArr);
  strcat(strURL,"&val=");
  strcat(strURL,stat);
  Serial.println(strURL);
  if (https.begin(client,"script.google.com",443, strURL,true)){
      Serial.print("[HTTPS] GET...\n");
      int httpCode = https.GET(); // start connection and send HTTP header
      if (httpCode > 0) {         // httpCode will be negative on error
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode); // HTTP header has been send and Server response header has been handled
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {  // file found at server
          String payload = https.getString();
          Serial.println(payload);
          JsonDocument jdoc;
          DeserializationError error = deserializeJson(jdoc, payload);
          if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return -1;
          }
          TrgrVolt = jdoc["TrgrVolt"];
          invPwerOn = (jdoc["inv"] == "ON") ? true : false;
          batChrgOn = (jdoc["batChrg"] == "ON") ? true : false;
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
  } else {
      Serial.printf("[HTTPS] Unable to connect\n");
  }
  return 0;
}