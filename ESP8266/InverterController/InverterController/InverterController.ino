/* Program for automating an old home inverter */
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
using namespace std;

#define analogPin A0 // Analog Pin of ESP8266 : ADC0 = A0
#define MAINSPIN 5 //GPIO on which Mains Powersupply will be checked (for older board PIN 4 & 5 was on reverse roles)
#define SWITCHOVER 4 //Switch load to Inverter
#define INVERTER 12  //Power "ON" the inverter (Swith on the inverter)
#define INVCHARGE 14  //Switch on the inverter charging.
#define INDICATOR 16  //Default is LED_BUILTIN (2)
#define PWRON 0x0  //Better to use Low as Power On
#define PWROFF 0x1 //Power "OFF" is high
#define VOLTREADING 72.14  //Reading corresponds to 1 volt. This may change based on the circuit assembly. 
//How to calculate : Supply 14v to the setepdown converter and divide the reading by 14

/*WiFi credentials*/
#define ssid "ocean"
#define password "20010604"
//const char* apiURL = "/macros/s/AKfycbwNhKo1tLQVTMt_UgvkwDQYg8n9bMTByq5WlNAPc6QHVNFshR4gSqTRmnzACF3tjvDq8w/exec";  //--APIv3
const char* apiURL = "/macros/s/AKfycbwRcAb0-1mbDtH8b0w4-32zdNcS6t5wdM-qtMfpTkQAl-AU0uZVyHicoXirVuLp_AP1WA/exec";  //--APIV4

int loopCnt=0;
int strtLoop = -200;
int BattVolt = 0;  // Curren tBattery voltage, Variable to store Output of ADC 
int MainsState = 0; // Mains supply status, By deafult, Mains is there "0", "1" means power went
int PrevBattVolt = 0;  //Previous Known battery voltage in ADC. So integer
int PrevMainsState = 0;  //Previous Known Mains status, 0 = Mains available 1 = Mains gone
double TrgrVolt=14;
bool invPwerOn = true;  //Switch on the inverter if power goes
bool batChrgOn = false; //Charge battery from mains.
int outpins[]={SWITCHOVER,INVERTER,INVCHARGE,INDICATOR};

BearSSL::WiFiClientSecure client;
HTTPClient https;

/* Generic blink function. Blink LED cnt times with delay of dly */
void blink(int cnt = 3,int dly = 100) {
  for (int i=0; i<cnt; i++){
  digitalWrite(INDICATOR, PWRON);
  delay(dly);
  digitalWrite(INDICATOR, PWROFF);
  delay(dly);    
  }   
}

void setup()
{
  Serial.begin(115200); /* Initialize serial communication at 115200 */
  for (auto pin : outpins) {  //Initialize all output pins and turn it off by default
    pinMode(pin, OUTPUT);
    digitalWrite(pin, PWROFF);
  }
  pinMode(MAINSPIN, INPUT); //The only digital input pin for checking whether AC is available.
  blink();
  WiFi.mode(WIFI_STA); //Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,would try to act as both a client and an access-point and could cause network-issues with your other WiFi-devices on your WiFi-network. 
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    blink(1,300);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected. IP address:");
  Serial.println(WiFi.localIP());
  client.setInsecure();
  https.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  blink();  //Three blinks to indicate that the setup is over.
}

//Take Readings and decide whether to switch on / off the inverter
void checkMainsnVoltage(){
  BattVolt = analogRead(analogPin); // Read the Analog Input value for checking the battery voltage
  MainsState = digitalRead(MAINSPIN);  //Check the mains status
  if(invPwerOn && BattVolt > VOLTREADING*11.5 && (MainsState == 1 || BattVolt > VOLTREADING*TrgrVolt )){  //inverter can be switched on, if the power goes or battery volage increases
    if (BattVolt > VOLTREADING*TrgrVolt) strtLoop = loopCnt;
    //Serial.println("Switch on Inverter");
    digitalWrite(INVERTER,PWRON); //Switchon inverter
    blink(4,100);
    delay(3500);//Wait for 3.5 seconds for inverter to startup
    digitalWrite(SWITCHOVER,PWRON); //Switchover the load to the inverter
  }else if ( loopCnt-strtLoop > 200 && BattVolt < VOLTREADING*13.2 ) { //Power is available
    digitalWrite(SWITCHOVER,PWROFF); //Switchover to mains
    //blink(1,100);
    digitalWrite(INVERTER, PWROFF); //Switch-off Inverter
  }else{
    printf("Inverter goes off in another %d loop\n", 200 - (loopCnt-strtLoop));
  }
}

void chargeInveter(){
  if (batChrgOn) digitalWrite(INVCHARGE, PWRON);
  else digitalWrite(INVCHARGE, PWROFF);
}

void loop()
{
  loopCnt++;
  checkMainsnVoltage();  //The main function to trigger the inveter
  chargeInveter(); //Decide whether to charge the battery using inverter
  Serial.printf("GLOBAL invPwerOn : %s , batChrgOn : %s , TrgrVolt : %f \n", invPwerOn ? "true" : "false",batChrgOn ? "true" : "false",TrgrVolt);
  Serial.printf("Cur.Batt.Volt = (%d) %fv, Mains is %s \n",BattVolt,BattVolt/VOLTREADING, MainsState == 0 ? "Available" : "Not Available");
  if ( abs(PrevBattVolt - BattVolt) > 10 ){
    updateBattVoltage(BattVolt);
  }
  if (PrevMainsState != MainsState) {
    updateMainsState(MainsState);
  }
  if (loopCnt % 10 == 0){
    Serial.println("Getting Status using API");
    getAPIupdates();
  }

  delay(2700);
  //wifi_fpm_do_sleep(10000);
  blink(1,50);
}

int updateBattVoltage(int volt){
 PrevBattVolt=volt;
 //Item=2, Status=volt, Write
 char buffer[10];
 sprintf(buffer,"%.1f",volt/VOLTREADING);
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