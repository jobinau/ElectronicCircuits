/*  Open the Door after late night. Jobin Augustine */
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>  //UDP communication with NTP servers
#include <ESP8266WebServer.h>  //Start a webserver

#define LEDON 0x0 // D1 mini clone has LED connected to +ve, define according to behaviour
#define LEDOFF 0x1
#define LOCKPIN 5 //GPIO number to which Solinoid lock is connected

/*WiFi credentials*/
#define ssid "ocean"
#define password "20010604"

/*Basic Infra for communicating with NTP server */
unsigned int localPort = 2390;  // local port to listen for UDP packets
const char* ntpServerName = "in.pool.ntp.org";   // NTP Server for India.
IPAddress timeServerIP;  // Resolved IP address of the NTP server
const int NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE];  // buffer to hold incoming and outgoing packets
WiFiUDP udp; // A UDP instance to let us send and receive packets over UDP
unsigned long secsSince1900;  //Response from NTP server
unsigned long epoch, startepoch ;  //Converted UNIX epoch
unsigned long millisec; //Milliseconds since the startup
bool doorClosed = true;  //When the program starts, door must be closed.

ESP8266WebServer server(80);   //Webserver

void setup() {
  Serial.begin(9600);     //Baud rate for Serial monitor
  pinMode(LED_BUILTIN, OUTPUT);  //LED as Output pin which is GPIO2 (D4)
  pinMode(LOCKPIN, OUTPUT);  //GPIO output for door lock
  digitalWrite(LOCKPIN, LOW);  //Make sure that solinoid is not active from the begining

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  /* Start Wifi-Client */
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
  
  server.on("/", handleRoot);               // Call the 'handleRoot' function when a client requests URI "/"
  server.on("/noblink", noblink);
  server.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
  server.begin();                           // Actually start the server
  Serial.println("HTTP server started");

  /*Initialize the UPD */
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());

  WiFi.hostByName(ntpServerName, timeServerIP);   //resolve to a random NTP server ip
  Serial.print("Time Server IP =");
  Serial.println(timeServerIP);
  sendNTPpacket(timeServerIP);   //Send a request to NTP server
  delay(2000);  //Wait for 2 seconds for NTP server response.
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("No response from NTP Server :( ");
  } else {
    Serial.printf("packet received, length=%d \n",cb);
    //Serial.println(cb);
    udp.read(packetBuffer, NTP_PACKET_SIZE);  // read the packet into the buffer

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);  // the timestamp starts at byte 40 of the received packet and is four bytes,
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  //  or two words, long. First, esxtract the two words:
    secsSince1900 = highWord << 16 | lowWord;   // combine the four bytes (two words) into a long integer
    Serial.print("NTP SUCCESS!. Seconds since Jan 1 1900 = ");  // this is NTP time (seconds since Jan 1 1900):
    Serial.println(secsSince1900);
    startepoch = secsSince1900 - 2208988800UL; //Substract 70 years for converting it to unix epoch
  }  
}

void loop() {
  unsigned long millisec = millis();
  server.handleClient();                    // Listen for HTTP requests from clients  
  epoch = startepoch + (millisec/1000);
  Serial.printf("%lu ",millisec);
  digitalWrite(LED_BUILTIN, LEDON);
  if ( doorClosed &&  ( millisec > 14400000UL || ( (epoch % 86400L) / 3600 > 20 && (epoch % 3600) / 60 > 30)))   //Trigger the Door unlock if conditions met 7200000UL=2hrs  10800000UL=3hrs 14400000UL=4hrs
    {
      releaseDoor();
    }
  if (doorClosed)
    delay(1000); 
  else
    blink();      //notification of dooropening attempt
  digitalWrite(LED_BUILTIN, LEDOFF);
  digitalWrite(LOCKPIN, LOW); //Make sure that lock is not active
  delay(2000) ;
}

void releaseDoor(){
  Serial.println("\n Releasing the Door at:");
  Serial.printf("Time out %lu \n",millisec);
  Serial.printf("Hour %lu \n",(epoch % 86400L) / 3600);
  Serial.printf("Minute %lu \n",(epoch % 3600) / 60);
  digitalWrite(LOCKPIN, HIGH);  //Trigger the solinoid
  doorClosed = false;   //Status of the door is open
  delay(1000);  //Hold it for a second
  digitalWrite(LOCKPIN, LOW); //Explicitly release it back  
}

/* blink LED 3 TIMES */
void blink() {   
  digitalWrite(LED_BUILTIN, LEDON);
  delay(100);
  digitalWrite(LED_BUILTIN, LEDOFF);
  delay(100);
  digitalWrite(LED_BUILTIN, LEDON);
  delay(100);
  digitalWrite(LED_BUILTIN, LEDOFF);
  delay(100);
  digitalWrite(LED_BUILTIN, LEDON);
  delay(100);
  digitalWrite(LED_BUILTIN, LEDOFF);
  delay(100);
}

/* send NTP packet to the address of the NTP server */
void sendNTPpacket(IPAddress& address) {
  Serial.println("sending NTP packet...");
  memset(packetBuffer, 0, NTP_PACKET_SIZE);   // set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum, or type of clock
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  udp.beginPacket(address, 123);  // NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE); // all NTP fields have been given values, now we can send a packet requesting a timestamp:
  udp.endPacket();
}

void handleRoot() {
  server.send(200, "text/plain", "Open Sesame!");   // Send HTTP status 200 (Ok) and send some text to the browser/client
  releaseDoor();
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

void noblink(){  //Stop the notification of door opening by setting it as doorClosed
  doorClosed = true;
}