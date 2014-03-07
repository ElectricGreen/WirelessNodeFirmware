/* FILE:    Microduino_nRF24L01_Transceiver_Module_HCMIDU0006_Sensor_Example
 *  DATE:    03/06/13
 *  VERSION: 0.1
 *
 * This is an example of how to use the Hobby Components Microduino nRF24L01 2.4GHz
 * wireless transceiver (HCMIDU0006). This transceiver module is a very cost effective
 * way to transmit data from one microcontroller to another over short distances. This
 * simple example will configure the module using the freely available Mirf library
 * to transmit the analogue value from analogue input A0 to a remote module.
 *
 *
 * You may copy, alter and reuse this code in any way you like, but please leave
 * reference to HobbyComponents.com in your comments if you redistribute this code.
 * This software may not be used directly for the purpose of selling products that
 * directly compete with Hobby Components Ltd's own range of products.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS". HOBBY COMPONENTS MAKES NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ACCURACY OR LACK OF NEGLIGENCE.
 * HOBBY COMPONENTS SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR ANY DAMAGES,
 * INCLUDING, BUT NOT LIMITED TO, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY
 * REASON WHATSOEVER.
 *
 */

/* Include the SPI anf Mirf libraries */
#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>

#define PKT_UPDATE 0x05 
#define PKT_CMD 0x06 
#define PKT_LED 0x07

#define RFTYPE_REGISTER 1
#define RFTYPE_ACCEPTED 2
#define RFTYPE_UPDATE 3
#define RFTYPE_CMD 4
#define RFTYPE_LED 5

char RF_ADDRESS[5];
/* Stores the current analogue input value */
byte incomingByte = 0;   // for incoming serial data
byte currentColor = 0;

struct NODE{
  byte nodeNumber;
  char address[5];
};

NODE nodeInfo;

struct PACKET{
  byte type;
  union {
    struct INIT{
        NODE newNode;
      }
    INIT;
    struct UPDATE{
      byte nodeNumber;
      uint8_t soilSensor;
      uint8_t battery;
      uint8_t light;
      uint16_t temperature;
    }
    UPDATE;
    struct CMD{
      uint8_t arguments[5];
    }
    CMD;
    struct LED{
      uint8_t color;
      uint8_t value;
      uint8_t arguments[3];
    }
    LED;
    uint8_t asBytes[6];
  }pktTypes; 
};

PACKET packet;

#define SOIL_PIN   5

#define RGB_RED    3
#define RGB_GREEN  5
#define RGB_BLUE   6

#define STATE_INIT 0
#define STATE_REGISTER 1
#define STATE_RUNNING 2
#define STATE_ERROR 3

byte currentState;
uint32_t timer = 0;

void setColor(unsigned int color);
void setup(){
    Serial.begin(9600);     // opens serial port, sets data rate to 9600 bps
      while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection

      memcpy(&nodeInfo.address, &"NODE1", 5);


  /* Define the DIO used for SPI CSN & CE. Defaults are DIO
   *  10 & 9 respectively. */
  Mirf.cePin = 9;
  Mirf.csnPin = 10;

  Mirf.spi = &MirfHardwareSpi;

  /* Initialise the module */
  Mirf.init();

  /* Set the receiving address of this module. This must be a 5 byte
   *  character string */
  Mirf.setRADDR((byte *)nodeInfo.address);

  Mirf.payload = sizeof(packet);

  /* Set the channel. Channel 90 = 2.490GHz */
  Mirf.channel = 90;

  /* Configure the module */
  Mirf.config();

  pinMode(RGB_RED, OUTPUT);      // sets the digital pin as output
  pinMode(RGB_GREEN, OUTPUT);      // sets the digital pin as output
  pinMode(RGB_BLUE, OUTPUT);      // sets the digital pin as output\\\\\\\\\\

  currentState = STATE_INIT;
  Serial.println("Module initialised");
}


void setColor(unsigned int color, byte value){
  switch(color){
  case RGB_RED:
    analogWrite(RGB_RED,incomingByte);
    break;
  case RGB_GREEN:
    analogWrite(RGB_GREEN,incomingByte);
    break;
  case RGB_BLUE:
    analogWrite(RGB_BLUE,incomingByte);
    break;
  }              
}

void rf_interupt(){

}
/******* Main Program ********/
void loop()
{
  switch(currentState){
  case STATE_INIT:
    packet.type = RFTYPE_REGISTER;
    memcpy(&packet.pktTypes.INIT.newNode.address, &nodeInfo.address, 5);
      Mirf.setTADDR((byte *)"BASE1");
    Mirf.send((byte *)&packet);
    while(Mirf.isSending());                 
    Serial.println("registering to homebase");
      Mirf.setRADDR((byte *)nodeInfo.address);
    currentState = STATE_REGISTER;
    break;
  case STATE_REGISTER:
    if(!Mirf.isSending() && Mirf.dataReady()){
    Serial.println("Got packet");
    Mirf.getData((byte*)&packet);
    Serial.print("Type");
    Serial.println(packet.type);
    switch(packet.type){
    case RFTYPE_ACCEPTED:
    Serial.print("Address: ");
    Serial.println(packet.pktTypes.INIT.newNode.address);
        Serial.println(nodeInfo.address);

      if (strncmp(nodeInfo.address, (const char*)packet.pktTypes.INIT.newNode.address,5) == 0){
        nodeInfo.nodeNumber = packet.pktTypes.INIT.newNode.nodeNumber;
        currentState = STATE_RUNNING;
            Serial.println("Moving to Run State");
      }
      break;     
    }
  }
    //Wait for interupt or timeout and resend register
    break;
  case STATE_RUNNING:
      if (millis() - timer >= 2000)  {
      
      //obtain and send analog reading from soil sensor
      int sensorValue = analogRead(SOIL_PIN);
      // Convert the analog reading (which goes from 0 - 1023) to a percentage:
      float percentMoisture = 1 - (sensorValue-200)*(0.12195);//.0012195 = 1/800=range between wet and dry soil
      // print out the value you read:
      Serial.println(percentMoisture);
  
      packet.type = RFTYPE_UPDATE;
      packet.pktTypes.UPDATE.nodeNumber = nodeInfo.nodeNumber;
      packet.pktTypes.UPDATE.soilSensor = percentMoisture;
      packet.pktTypes.UPDATE.battery = 55;
            packet.pktTypes.UPDATE.light = 66;
                  packet.pktTypes.UPDATE.temperature = 77;
                    Mirf.setTADDR((byte *)"BASE1");
      Mirf.send((byte *)&packet);
        while(Mirf.isSending());                 

      Serial.println("Sending soil data");
      delay(100);
      
      timer = millis();
    }
    break;
  case STATE_ERROR:
    break;
  }

}

