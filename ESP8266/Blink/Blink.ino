/*
  Blink

  Turns an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO
  it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN is set to
  the correct LED pin independent of which board is used.
  If you want to know what pin the on-board LED is connected to on your Arduino
  model, check the Technical Specs of your board at:
  https://www.arduino.cc/en/Main/Products

  modified 8 May 2014
  by Scott Fitzgerald
  modified 2 Sep 2016
  by Arturo Guadalupi
  modified 8 Sep 2016
  by Colby Newman

  This example code is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/Blink
*/

#define analogPin A0 // Analog Pin of ESP8266 : ADC0 = A0
#define MAINSPIN 4 //GPIO on which Mains Powersupply will be checked
#define SWITCHOVER 5 //Relay for AC to Inverter switchover
#define INVERTER 12  //The "ON" switch on the inverter  
#define INVCHARGE 14  //Switch on the inverter charging.

//int pins[] = {2,4,5,12,13,14,16};   //2 is built-in LED
int pins[] = {14,12,4,5,13,16,2};
// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pins as an output.
  for (auto pin : pins) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);  //emperically its found that all outputs are low by default. turning it HIGH
  }
}

void blink_pin(int pin,int cnt = 3,int dly = 100) {
  for (int i=0; i<cnt; i++){
  digitalWrite(pin, LOW);
  delay(dly);
  digitalWrite(pin, HIGH);
  delay(dly);    
  }   
}

// the loop function runs over and over again forever
void loop() {
  for (auto pin : pins) {
    blink_pin(pin);
  }
}
