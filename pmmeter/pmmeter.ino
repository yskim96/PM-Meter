#include <SPI.h>
#include <Ethernet.h>
#include <SoftwareSerial.h>
#include "U8glib.h"

int pmcf10=0;
int pmcf25=0;
int pmcf100=0;
int pmat10=0;
int pmat25=0;
int pmat100=0;

String pm10;
String pm25;

int LED_L = 8;
int LED_M = 7;
int LED_H = 6;

int range = 0;
int x;
int ok = 0;

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);

SoftwareSerial mySerial(4, 3); //232_TX,232_RX

// Local Network Settings
byte mac[] = { 0xD4, 0x28, 0xB2, 0xFF, 0xA0, 0xA1 };

// ThingSpeak Settings
char thingSpeakAddress[] = "api.thingspeak.com";
String writeAPIKey = "B86DSH6BYS7XAA30"; //API 키를 넣으세요
const int updateThingSpeakInterval = 20 * 1000; // Time interval in milliseconds to update ThingSpeak (number of seconds * 1000 = interval)

// Variable Setup
long lastConnectionTime = 0; 
boolean lastConnected = false;
int failedCounter = 0;

// Initialize Arduino Ethernet Client
EthernetClient client;

char buf[50];

void setup() {
  
  //Serial.begin(9600);
  mySerial.begin(9600);

    // assign default color value
  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);     // white
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255,255,255);
  }
  pinMode(LED_L, OUTPUT);
  pinMode(LED_M, OUTPUT);
  pinMode(LED_H, OUTPUT);
  digitalWrite(LED_L, HIGH);
  digitalWrite(LED_M, HIGH);
  digitalWrite(LED_H, HIGH);
  
  delay(2000);
  
  startEthernet();

}


void updateThingSpeak(String tsData)
{
  if (client.connect(thingSpeakAddress, 80))
  {         
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: "+writeAPIKey+"\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(tsData.length());
    client.print("\n\n");

    Serial.println(tsData);
    client.print(tsData);
    
    lastConnectionTime = millis();
    
    if (client.connected())
    {
      //Serial.println("Connecting to ThingSpeak...");
      //Serial.println();
      
      failedCounter = 0;
    }
    else
    {
      failedCounter++;
  
      //Serial.println("Connection to ThingSpeak failed ("+String(failedCounter, DEC)+")");   
      //Serial.println();
    }
    
  }
  else
  {
    failedCounter++;
    
    //Serial.println("Connection to ThingSpeak Failed ("+String(failedCounter, DEC)+")");   
    //Serial.println();
    
    lastConnectionTime = millis(); 
  }
  
}

void startEthernet()
{
  
  client.stop();

  //Serial.println("Connecting Arduino to network...");
  //Serial.println();  

  delay(1000);
  
  // Connect to network amd obtain an IP address using DHCP
  if (Ethernet.begin(mac) == 0)
  {
    //Serial.println("DHCP Failed, reset Arduino to try again");
    startEthernet();
    //Serial.println();
  }
  else
  {
    //Serial.println("Arduino connected to network using DHCP");
    //Serial.println();
  }
  
  delay(1000);
}

uint8_t draw_state = 0;

void loop() {
  
 if (ok==0) {
 current_sensor();

 u8g.firstPage();  
  do {
    draw();
  } while( u8g.nextPage() );
  
  // increase the state
  draw_state++;
  if ( draw_state >= 9*8 )
    draw_state = 0; 
 }

 else if (ok==1){
  
 send_data();
 
 }


}

void current_sensor(){

int count = 0;
unsigned char c;
unsigned char high;

  while (mySerial.available()) {
    //digitalWrite(LED_S, HIGH);
    c = mySerial.read();
    if((count==0 && c!=0x42) || (count==1 && c!=0x4d)){
      //Serial.println("check failed");
      break;
    }
    if(count > 15){
      //Serial.println("complete");
      pm10=String(pmcf100);
      pm25=String(pmcf25);
      
      status_led(pmcf25,pmcf100);
      ok=1;
      break;
    }
    else if(count == 4 || count == 6 || count == 8 || count == 10 || count == 12 || count == 14) {
      high = c;
    }
    else if(count == 5){
      pmcf10 = 256*high + c;
      //Serial.print("CF=1, PM1.0=");
      //Serial.println(pmcf10);
    }
    else if(count == 7){
      pmcf25 = 256*high + c;
      //Serial.print("CF=1, PM2.5=");
      //Serial.println(pmcf25);
    }
    else if(count == 9){
      pmcf100 = 256*high + c;
      //Serial.print("CF=1, PM10=");
      //Serial.println(pmcf100);
    }
    else if(count == 11){
      pmat10 = 256*high + c;
      //Serial.print("atmosphere, PM1.0=");
      //Serial.println(pmat10);
    }
    else if(count == 13){
      pmat25 = 256*high + c;
      //Serial.print("atmosphere, PM2.5=");
      //Serial.println(pmat25);
    }
    else if(count == 15){
      pmat100 = 256*high + c;
      //Serial.print("atmosphere, PM10=");
      //Serial.println(pmat100);
    }
    count++;
  }
  while(mySerial.available()) mySerial.read();
  
  //Serial.println();


  
  delay(5000);
  
}

void send_data(){
  // Update ThingSpeak
   if (client.available())
  {
    char c = client.read();
    //Serial.print(c);
  }

  // Disconnect from ThingSpeak
  if (!client.connected() && lastConnected)
  {
    //Serial.println("...disconnected");
    //Serial.println();
    ok=0;
    client.stop();
  }
  
  // Update ThingSpeak
  if(!client.connected() && (millis() - lastConnectionTime > updateThingSpeakInterval))
  {
    updateThingSpeak("field1="+pm10+"&"+"field2="+pm25);

  }
  
  // Check if Arduino Ethernet needs to be restarted
  if (failedCounter > 3 ) {startEthernet();}
  
  lastConnected = client.connected();
}

void status_led(int pm25,int pm100) {

  if(pm25 <= 50 || pm100 <= 80){
      digitalWrite(LED_L, HIGH);
      digitalWrite(LED_M, LOW);
      digitalWrite(LED_H, LOW);
    
      Serial.println("LOW");
  }
  else if(pm25 > 50 || pm25 <= 100 || pm100 > 80 || pm100 <= 150){
      digitalWrite(LED_L, LOW);
      digitalWrite(LED_M, HIGH);
      digitalWrite(LED_H, LOW);

      Serial.println("MID");
    }
     else if(pm25 > 101 || pm100 > 150){
      digitalWrite(LED_L, LOW);
      digitalWrite(LED_M, LOW);
      digitalWrite(LED_H, HIGH);

      Serial.println("HIGH");
    }
    else{
    } 
}

void draw(void) {

  //u8g.setDefaultBackgroundColor();
  u8g.setDefaultForegroundColor();

  //u8g.drawBox(-1,-1,130,65);
 
 
 u8g.setFont(u8g_font_profont12);

    u8g.drawStr( 99, 12, "ug/m3");
    

    u8g.drawStr( 1, 63, "PM2.5");
    u8g.drawStr( 49, 63, "PM 10");
    u8g.drawStr( 96, 63, "PM1.0");

  u8g.setPrintPos(1, 40); 
    u8g.print(pmcf25);
  u8g.setPrintPos(48, 40); 
    u8g.print(pmcf100);
  u8g.setPrintPos(96, 40); 
    u8g.print(pmcf10);

  //run icon
  if (draw_state % 2) { u8g.drawDisc(5,10,3);}
   u8g.drawCircle(5,10,3);
}




  



