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
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define WLAN_SSID       "Alfaro "          /*"Alfaro "*/           /*"Asada Samara "*/     /*HUAWEI P20*/      /*DESKTOP-88UTQES 8386*/                      
#define WLAN_PASS       "wimase2019"        /*"wimase2019"*/        /*"SamaraAzul19"*/      /*927c8dfc3fe2*/    /*+h02102L*/
#define AIO_SERVER      "io.adafruit.com" 
#define AIO_SERVERPORT  1883                  
#define AIO_USERNAME    "ssolorzano_cr"
#define AIO_KEY         "aio_roXH51gaTjTz5F1JbdmSiaRW4naH"



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
byte localAddress = 0xA;          // address of this device
byte receiverAddress = 0xB;       // address of receiver

String incoming; 
int8_t attempt = 0; int packetSize; 

char *strtok(char *str1, const char *str2); char *part; 
char charpacket [99]; char delimiter [2] = "e"; char ppack[8] = {'\0'};

/* create a hardware timer */
hw_timer_t * timer = NULL; bool AIO_flag, WiFi_flag;

void IRAM_ATTR timer_isr(){
  AIO_flag = 1;
}
/*----------------------------------------------------------------------------------------------*/ 

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);       
Adafruit_MQTT_Publish receiverrssi = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/signal.receiver-rssi");
Adafruit_MQTT_Publish receiversnr = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/signal.receiver-snr");
Adafruit_MQTT_Publish senderrssi = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/signal.sender-rssi");
Adafruit_MQTT_Publish sendersnr = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/signal.sender-snr");


void setup() {
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  Heltec.display->init();
  Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "Heltec.LoRa Initial success!");
  Heltec.display->drawString(0, 20, "Beginning sending process!");
  Heltec.display->display();
  
  /* 1 tick take 1/(80MHZ/80) = 1us so we set divider 80 and count up */
  timer = timerBegin(0, 80, true);
  /* Attach timer_isr function to our timer */
  timerAttachInterrupt(timer, &timer_isr, true);
  /* Set alarm to call onTimer function every second 1 tick is 1us
  => 1 second is 1000000us */
  /* Repeat the alarm (third parameter) */
  timerAlarmWrite(timer, 300000000, true);          //5 minutes timer for data upload 
  /* Start an alarm */
  timerAlarmEnable(timer);

  delay(2000);
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);Heltec.display->setFont(ArialMT_Plain_10);
  Serial.println("time;SenderRSSI;SenderSnr;ReceiverRSSI;ReceiverSnr;");
}

/**@brief This is the main loop of the program.*/
void loop() {
  if (AIO_flag){
    AIO_flag=0;
    Heltec.display->clear();
    // send packet
    LoRaSend("HI THERE e2947e2197H9/4e1ssW", receiverAddress);
    
    //Listening now...
    LoRa.receive();                   //back into receive mode
    for(int i = 0; i < 10; i++){      // listen for 3 seconds in total)
      packetSize = LoRa.parsePacket();
      if (packetSize){
        break;
      }
      delay(300);
    }
    onReceive(packetSize, receiverAddress);

    WiFi_connect();
    if (WiFi_flag){
      MQTT_connect();
      Heltec.display->clear();
      for(int i = 0; i < 3; i++){ 
        if (!receiverrssi.publish(LoRa.packetRssi())) {                    //write feedname.publish 
          Heltec.display->drawString(0 , 0 , "Failed"); Heltec.display->display();
        }else{
          Heltec.display->drawString(0 , 0 , "OK!"); Heltec.display->display();
          break;
        }
      }
      for(int i = 0; i < 3; i++){ 
        if (!receiversnr.publish(LoRa.packetSnr())) {                    //write feedname.publish 
          Heltec.display->drawString(0 , 0 , "Failed"); Heltec.display->display();
        }else{
          Heltec.display->drawString(0 , 0 , "OK!"); Heltec.display->display();
          break;
        }
      }
      /*
      for(int i = 0; i < 3; i++){ 
        if (!sender-rssi.publish(sendrssi)) {                    //write feedname.publish 
          Heltec.display->drawString(0 , 0 , "Failed"); Heltec.display->display();
        }else{
          Heltec.display->drawString(0 , 0 , "OK!"); Heltec.display->display();
          break;
        }
      }
      for(int i = 0; i < 3; i++){ 
        if (!sender-snr.publish(sendsnr)) {                    //write feedname.publish 
          Heltec.display->drawString(0 , 0 , "Failed"); Heltec.display->display();
        }else{
          Heltec.display->drawString(0 , 0 , "OK!"); Heltec.display->display();
          break;
        }
      }
      */
    }
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
    attempt++;
    if ((attempt > 4) && (expected_sender == receiverAddress)){
      attempt=0;
      Serial.println("ALERTA: ERROR EN COMUNICACION LoRa"); 
    }
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

/*---------------------------------WiFi & MQTT Conection.-------------------------------------*/
void WiFi_connect(){
  if (WiFi.status() == WL_CONNECTED){
    return;
  }
  Heltec.display->clear();
  Heltec.display->drawString(0, 20, "Connecting to ");  Heltec.display->drawString(0, 30, WLAN_SSID);
  Heltec.display->display();
  //Serial.print("Connecting to ");  Serial.println(WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  for(int i = 0; i < 5; i++){
    WiFi.status();
    delay(1000);
  }
  if (WiFi.status() == WL_CONNECTED){
    WiFi_flag=1;
    //Serial.println("WiFi connected");
    Heltec.display->drawString(0, 40,"WiFi connected");
    Heltec.display->display();
    //Serial.println("IP address: "); Serial.println(WiFi.localIP());
  }else{
    WiFi_flag=0;
  }
}
void MQTT_connect(){
  int8_t ret;
  if (mqtt.connected()){
    return;
  }
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "Connecting to MQTT...");
  Heltec.display->display();
  //Serial.print("Connecting to MQTT...");
  uint8_t retries = 5;
  while ((ret = mqtt.connect()) != 0){ // connect will return 0 for connected
    Heltec.display->drawString(0, 20, mqtt.connectErrorString(ret));Serial.println(mqtt.connectErrorString(ret));
    Heltec.display->drawString(0, 30, "Retrying MQTT connection"); Serial.println("Retrying MQTT connection in 5 seconds...");
    Heltec.display->display();
    mqtt.disconnect();
    delay(1000);
    retries--;
    if (retries == 0){
      Heltec.display->drawString(0, 40, "MQTT connection failed"); Serial.println("MQTT connection failed");
      Heltec.display->drawString(70, 40, "Please reboot"); Serial.println("Please reboot");
      Heltec.display->display();
      delay (2000);
      return;
    }    
  }
  Heltec.display->drawString(0, 40, "MQTT Connected!"); Serial.println("MQTT Connected!");
  Heltec.display->display();
}
/*--------------------------------------------------------------------------------------------*/
