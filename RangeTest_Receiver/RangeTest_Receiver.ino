/**
 * @file RangeTest_Sender.ino
 * @author Sergio Solorzano
 * @date 23 Abr 2021
 * @par Institution:
 * DELTALab. Instituto Tecnologico de Costa Rica.
 * @par Git repository:
 */

/**
 * @brief This is program helps in measuring the LoRa signal budget and signal strength indicators.
 * WORKS ONLY TOGETHER WITH RangeTest_Receiver! 
*/               
//IMPORTANT > > Data packets sent or received by LoRa nodes, to be uploaded to adafruit IO, must be converted to integer, floting or double numbers. 

#include "heltec.h"

#define BAND      915E6     //you can set band here directly,e.g. 868E6 (Europe),915E6 (North America),433E6 (Asia)
#define AIRTIME   400       //LoRa message estimated time in air in miliseconds
#define TXPOWER   20        /*txPower -- 0 ~ 20 dBm*/
/** LoRa.setTxPower(txPower,RFOUT_pin);
  * txPower -- 0 ~ 20
  * RFOUT_pin could be RF_PACONFIG_PASELECT_PABOOST or RF_PACONFIG_PASELECT_RFO
  *   - RF_PACONFIG_PASELECT_PABOOST -- LoRa single output via PABOOST, maximum output 20dBm
  *   - RF_PACONFIG_PASELECT_RFO     -- LoRa single output via RFO_HF / RFO_LF, maximum output 14dBm
----------------------------------------------------------------------------------------------*/ 
/*-------------------------------------LoRa variables----------------------------------------*/
byte localAddress = 0xB;          // address of this device
byte senderAddress = 0xA;       // address of sender

String incoming; int packetSize; 

char *strtok(char *str1, const char *str2); char *part; 
char charpacket [99]; char delimiter [2] = "e"; char ppack[8] = {'\0'};

bool message_flag;

void setup() {
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  Heltec.display->init();
  Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "Heltec.LoRa Initial success!");
  Heltec.display->drawString(0, 20, "Beginning sending process!");
  Heltec.display->display();

  delay(2000);
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);Heltec.display->setFont(ArialMT_Plain_10);
  Serial.println("time;SenderRSSI;SenderSnr;ReceiverRSSI;ReceiverSnr;");
}

/**@brief This is the main loop of the program. ALWAYS LISTENING*/
void loop() {
  Heltec.display->clear();
  //Listening...
  LoRa.receive();                   //back into receive mode
  for(int i = 0; i < 10; i++){      // listen for 2 seconds in total)
    packetSize = LoRa.parsePacket();
    if (packetSize){
      break;
    }
    delay(200);
  }
  onReceive(packetSize, senderAddress);
  if (message_flag){
    delay(2000);
    
    // send packet
    LoRaSend(String(LoRa.packetRssi())+";"+String(LoRa.packetSnr())+";", senderAddress);
  }
}

/*----------------------------LoRa Functions----------------------------------------------*/

void LoRaSend(String outgoing, byte destination) {
  Heltec.display->drawString(0, 0, "Sending: ");
  Heltec.display->drawString(0, 10, outgoing);
  Heltec.display->display();
  delay(AIRTIME);                   
  for(int i = 0; i < 4; i++){ // send same packet 4 times (1 second in total)
    LoRa.beginPacket();
    LoRa.setTxPower(TXPOWER,RF_PACONFIG_PASELECT_PABOOST);
    // Packet header bytes:
    LoRa.write(destination);              // add destination address
    LoRa.write(localAddress);             // add sender address
    LoRa.write(outgoing.length());        // add payload length
    // add payload
    LoRa.print(outgoing);                 
    LoRa.endPacket();
    //Serial.print("Sent"); Serial.println(outgoing);
    delay(250);
  }
  delay(AIRTIME);                      
}

void onReceive(int packetSize, byte expected_sender){
  if (!packetSize) {
    Serial.println("No order");
    Heltec.display->drawString(0, 40, "No Data received"); Heltec.display->display();
    return;                             // if there's no packet, return
  }

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingLength = LoRa.read();    // incoming msg length

  incoming ="";
  while (LoRa.available()){
    incoming += (char)LoRa.read();
  }
  
  if (incomingLength != incoming.length()){   // check length for error
      Heltec.display->drawString(0, 40,"error: length does not match"); Heltec.display->display();
      Serial.println("error: message length does not match length");
      return;                             // skip rest of function
    }
  
  // if the recipient isn't this device or sender isn't gateway,
  if ((recipient != localAddress) || (sender != expected_sender)) {
      Heltec.display->drawString(0, 40,"error: message not for me"); Heltec.display->display();
      Serial.println("Message not for me");
      return;                             // skip rest of function
    }
 
  Heltec.display->drawString(0, 30, "RSSI: " + String(LoRa.packetRssi())); Heltec.display->drawString(0, 40, incoming); Heltec.display->display();

  //Received Signal Strength Indicator (RSSI) - closer the value is to 0, the stronger the received signal has been. 
  // if message is for this device, or broadcast, print details:
  //printing > > time;SenderRSSI;SenderSnr;ReceiverRSSI;ReceiverSnr
  Serial.print(";"); Serial.print(incoming); 
  Serial.print(String(LoRa.packetRssi()));Serial.print(";"); Serial.print(String(LoRa.packetSnr()));
}
/*-------------------------------------------------------------------------------------------*/
