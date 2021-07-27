//################# Lights control shield ###############################


//#include <Arduino.h>

//#define BLYNK_PRINT Serial // Defines the object that is used for printing
#define BLYNK_TEMPLATE_ID "TMPLcobjXTat"
#define BLYNK_DEVICE_NAME "TTGOlights"
char auth[]= "QlAhqepp7Trb57enFlHT5LreNeXNTNkS";
#define DUMP_AT_COMMANDS
// Select your modem:
#define TINY_GSM_MODEM_SIM800
//#include <SevenSeg.h>
//#include <LiquidCrystal.h>
#include <cstring>
#include <Bounce2.h>
//#include <Button.h>
#include <EEPROM.h>
#include <Preferences.h>
#include <BlynkSimpleTinyGSM.h>
#include <TinyGsmClient.h>
//#include <SoftwareSerial.h>
#include <SimpleTimer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
//#include <LiquidCrystal.h>
#include <LiquidMenu.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
#include <TimeLib.h>

#define ENTER 34
#define UP 36
#define DOWN  39
#define ESC  35
#define OUT_pg 0
#define LED_BUILTIN 0
#define SerialAT Serial1
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26
#define I2C_SDA              21
#define I2C_SCL              22
// BME280 pins
#define I2C_SDA_2            18
#define I2C_SCL_2            19
#define IP5306_ADDR          0x75
#define IP5306_REG_SYS_CTL0  0x00
#define channel1 32
#define channel2 33
#define channel3 25
#define channel4 14
#define channel5 12
#define channel6 13
#define channel7 15
#define channel8 2

uint8_t channels[] = {channel1, channel2, channel3, channel4, channel5, channel6, channel7, channel8};

int ldr_value;

//char auth[] = "a8b998e3db1e42e888bed8797b87f108"; // (My Secret TOKEN)
//char auth1[] = "rzSKdZ2cMvECBlQN9C9PSccdpqOzpBmH";

TwoWire I2CPower = TwoWire(0);
TwoWire I2Cbuttons = TwoWire(1);


// Your GPRS credentials
// Leave empty, if missing user or pass

char apn[] = "internet"; //COSMOTE 
char user[] = "";
char pass[] = "";
uint8_t button_msg;
uint8_t lights=0x03;
char * light_stat[8] = {"off","off","off","off","off","off","off","off"};
uint8_t auto_light=0;
char * light_aut_stat[8];
char* light_disp=(char*)malloc(9);
char* auto_light_disp = (char*) malloc(9);
void LCDwrite(String msg1, String msg2 );
bool setPowerBoostKeepOn(int en);
void scan_buttons(uint8_t * buttons);
void day_night_check(int ldr_value);

Preferences prefs;

SimpleTimer connectionHandlerTimer;
SimpleTimer refreshmenuTimer;
Bounce bouncer_Enter = Bounce();
Bounce bouncer_Up = Bounce();
Bounce bouncer_Down = Bounce();
Bounce bouncer_Esc = Bounce(); 

/*const bool pullup = true;
Button enter(ENTER, pullup);
Button esc(ESC, pullup);
Button up(UP, pullup);
Button down(DOWN,pullup);
*/
String msg1;
String msg2;
int ch1_hours, ch2_hours, ch3_hours, ch4_hours, ch5_hours, ch6_hours, ch7_hours, ch8_hours, pg_hours, csq;
int ch_hours[]={ch1_hours, ch2_hours, ch3_hours, ch4_hours, ch5_hours, ch6_hours, ch7_hours, ch8_hours};
//int current_hours;
time_t start1, start2, start3;
long prev_millis, disconnect_timer;
time_t nowseconds;  
time_t started_times[8];
time_t stop_times[8];
boolean healthy=false, pg_on,start_rec_timer=false;
boolean isFirstConnect=true;
typedef enum {
  TIME_START,
  LDR,
  MANUAL
} EVENT;
void event_hanler(EVENT event, int channel);
//uncoment the following three rows in case of incorporating 7 segment display module
//SevenSeg disp(6, 5, 4, 3, 2, 14, 15); 
//const int digitNum=2;
//int digits[digitNum]={11,12};
//uncomment it in case an 16x2 lcd module is used
//LiquidCrystal lcd(6,5,4,3,2,15);//LiquidCrystal(rs, enable, d4, d5, d6, d7)
//LiquidCrystal lcd(4,3,5,6,14,15);//LiquidCrystal(rs, enable, d4, d5, d6, d7)
LiquidCrystal_I2C lcd(0x27,20,4);
//LiquidCrystal_I2C lcd(39);
//LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);
//SoftwareSerial SerialAT(16, 17); // RX, TX
//TinyGsm modem(SerialAT);
////////////////////////////
WiFiUDP ntpUDP;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);

// You can specify the time server pool and the offset, (in seconds)
// additionally you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);


#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, Serial);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif

//################Menu settings#####
LiquidLine welcome_line1(1, 0, "Main Menu ", LIQUIDMENU_VERSION);
LiquidLine welcome_line2(0, 1, "Lights:", light_disp);
LiquidLine welcome_line3(0, 2, "Ligh_auto:", auto_light_disp);
LiquidLine control(0, 3, "<Control>");
LiquidScreen welcome_screen(welcome_line1, welcome_line2, welcome_line3,control);

LiquidLine line21(0, 0, "Control");
LiquidLine line22(0, 1, "<Light Ctl>");
LiquidLine line23(0, 2, "<Light Auto ctl>");
LiquidLine line24(0, 3, "<Light assign>");
LiquidLine line25(0, 3, "<Light timer>");
LiquidScreen screen2(line21, line22, line23, line24);

LiquidLine line31(0, 0, "Light Control");
LiquidLine line32(2, 1, "Light 1:", light_stat[0]);
LiquidLine line33(2, 2, "Light 2:", light_stat[1]);
LiquidLine line34(2, 3, "Light 3:", light_stat[2]);
LiquidLine line35(2, 3, "Light 4:", light_stat[3]);
LiquidLine line36(2, 3, "Light 5:", light_stat[4]);
LiquidLine line37(2, 3, "Light 6:", light_stat[5]);
LiquidLine line38(2, 3, "Light 7:", light_stat[6]);
LiquidLine line39(2, 3, "Light 8:", light_stat[7]);
LiquidScreen screen3(line31,line32,line33,line34);
//LiquidScreen screen31(line35,line36,line37,line38);
LiquidLine line41(0, 0, "Light Auto control");
LiquidLine line42(2, 1, "Light-auto 1:", light_aut_stat[0]);
LiquidLine line43(2, 2, "Light-auto 2:", light_aut_stat[1]);
LiquidLine line44(2, 3, "Light-auto 3:", light_aut_stat[2]);
LiquidLine line45(2, 3, "Light-auto 4:", light_aut_stat[3]);
LiquidLine line46(2, 3, "Light-auto 5:", light_aut_stat[4]);
LiquidLine line47(2, 3, "Light-auto 6:", light_aut_stat[5]);
LiquidLine line48(2, 3, "Light-auto 7:", light_aut_stat[6]);
LiquidLine line49(2, 3, "Light-auto 8:", light_aut_stat[7]);
LiquidScreen screen4(line41,line42,line43,line44);

LiquidLine line51(0, 0, "Light timer");
LiquidLine line52(0, 1, "ch1_hours:", ch1_hours);
LiquidLine line53(0, 2, "ch2_hours:", ch2_hours);
LiquidLine line54(0, 3, "ch3_hours:", ch3_hours);
LiquidLine line55(0, 3, "ch4_hours:", ch4_hours);
LiquidLine line56(0, 3, "ch5_hours:", ch5_hours);
LiquidLine line57(0, 3, "ch6_hours:", ch6_hours);
LiquidLine line58(0, 3, "ch7_hours:", ch7_hours);
LiquidLine line59(0, 3, "ch8_hours:", ch8_hours);
LiquidScreen screen5(line51,line52,line53,line54);

LiquidLine line61(0, 0, "Light assign");
LiquidLine line62(0,1, "Ch1: ",  channels[0] );
LiquidLine line63(0,2, "Ch2: ",  channels[1] );
LiquidLine line64(0,3, "Ch3: ",  channels[2] );
LiquidLine line65(0,3, "Ch4: ",  channels[3] );
LiquidLine line66(0,3, "Ch5: ",  channels[4] );
LiquidLine line67(0,3, "Ch6: ",  channels[5] );
LiquidLine line68(0,3, "Ch7: ",  channels[6] );
LiquidLine line69(0,3, "Ch8: ",  channels[7] );

LiquidScreen screen6(line61,line62,line63,line64);

LiquidMenu menu(lcd,welcome_screen);

typedef enum {
  CONNECT_TO_GSM,
  AWAIT_GSM_CONNECTION,
  CONNECT_TO_BLYNK,
  AWAIT_BLYNK_CONNECTION,
  MAINTAIN_CONNECTIONS,
  AWAIT_DISCONNECT
} CONNECTION_STATE;

CONNECTION_STATE connectionState;
uint8_t connectionCounter;

void ConnectionHandler(void) {
  switch (connectionState) {
  case CONNECT_TO_GSM:
    BLYNK_LOG("Connecting to GSM network.");
    msg1="Initializing";
    msg2="Wait connecting.";
    LCDwrite(msg1, msg2 );
    delay(3000);
    modem.restart();
    Serial.println();
    BLYNK_LOG("Connecting to GSM network 111.");
    if (!modem.waitForNetwork(60000L)) BLYNK_LOG("Failed to be Connected to gsm network");  // You may need lengthen this in poor service areas
    BLYNK_LOG("Registered to %s", apn);
    modem.gprsConnect(apn);
    BLYNK_LOG("Connected to %s...", apn);
    connectionState = AWAIT_GSM_CONNECTION;
    connectionCounter = 0;
    break;

  case AWAIT_GSM_CONNECTION:
    if (modem.isGprsConnected()) {
      BLYNK_LOG("Connected to %s", apn);
      connectionState = CONNECT_TO_BLYNK;
    }
    else if (++connectionCounter == 50) {
      BLYNK_LOG("Unable to connect to %s. Retry connection.", apn);
      modem.gprsDisconnect();
      connectionState = AWAIT_DISCONNECT;
      connectionCounter = 0;
    }
    break;

  case CONNECT_TO_BLYNK:
    BLYNK_LOG("Attempt to connect to Blynk server.");
    Blynk.config(modem,auth,"blynk.cloud",80);
    Blynk.connect();
    connectionState = AWAIT_BLYNK_CONNECTION;
    connectionCounter = 0;
    break;

  case AWAIT_BLYNK_CONNECTION:
    if (Blynk.connected()) {
      BLYNK_LOG("Connected to Blynk server.");
      connectionState = MAINTAIN_CONNECTIONS;
    }
    else if (++connectionCounter == 50) {
      BLYNK_LOG("Unable to connect to Blynk server. Retry connection.");
      Blynk.disconnect();
      modem.gprsDisconnect();
      connectionState = AWAIT_DISCONNECT;
      connectionCounter = 0;
    }
    break;

  case MAINTAIN_CONNECTIONS:
    if (!modem.isGprsConnected()) {
      BLYNK_LOG("gprs connection lost. Reconnect.");
      Blynk.disconnect();
      modem.gprsDisconnect();
      connectionState = AWAIT_DISCONNECT;
      connectionCounter = 0;
      healthy=false;
    }
    else  if (!Blynk.connected()) {
      BLYNK_LOG("Blynk server connection lost. Reconnect.");
      Blynk.disconnect();
      connectionState = CONNECT_TO_BLYNK;
      healthy=false;
    }
    else {
     // Blynk.run();
     healthy=true;
      csq=modem.getSignalQuality();
      BLYNK_LOG("GSM signal quality: %d ", csq);
      
    }
    break;

  case AWAIT_DISCONNECT:
    if (++connectionCounter == 10) {
      connectionState = CONNECT_TO_GSM;
      healthy=false;
    }
    break;
  }
}

void refresh_menu()
{
  light_disp[8] = '\0';
  auto_light_disp[8] = '\0';
  for (uint8_t i = 0; i<8; i++)
 {
 /* if((lights>>i)&0x01) light_disp[i]='I';
  else light_disp[i]='X';
  if((auto_light>>i)&0x01) auto_light_disp[i]='I';
  else auto_light_disp[i]='X';*/
  light_disp[i]=(((lights>>i)&0x01)?'I':'X');
  auto_light_disp[i]=(((auto_light>>i)&0x01)?'I':'X');
  light_stat[i]=(char*)(((lights>>i)&0x01)?"On ":"Off");
  light_aut_stat[i]=(char*)(((auto_light>>i)&0x01)?"On ":"Off");
 }
  menu.softUpdate();
}

    
BLYNK_CONNECTED() {
if (isFirstConnect) {
  Blynk.syncAll();
  refresh_menu();
 // Blynk.notify("TIMER STARTING!!!!");
isFirstConnect = false;
}
}

BLYNK_WRITE(V0)  // Manual selection
{
  if ((param.asInt()==1)) 
  { //turn ON the pgm
    digitalWrite(channels[0], HIGH);
    lights|=(1<<0);
    Serial.println("The CH1 set on for " + String(ch1_hours)+" hours");
    Blynk.notify("CH1 ON!");
    delay(100);
    Blynk.virtualWrite(20,1);
    //Blynk.virtualWrite(10,255);
    //msg1="The CH1 set on";
   // msg2="for "+ String(pg_hours)+" Hours";
   // LCDwrite(msg1, msg2 );
   refresh_menu();
  } 
  else if ((param.asInt()==0)) 
  {
    digitalWrite(channels[0], LOW);
    lights&=~(1<<0);
     Serial.println("The CH1 set off");
    Blynk.notify("CH1 OFF!");
    delay(100);
    Blynk.virtualWrite(20,0);
    //Blynk.virtualWrite(11,255);
    //msg1="The CH1 set off";
    //msg2="after "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
    refresh_menu();
  }
  
}

BLYNK_WRITE(V1)  // Manual selection
{
  if ((param.asInt()==1)) 
  { //turn ON the pgm
    digitalWrite(channels[1], HIGH);
    lights|=(1<<1);
    Serial.println("The CH2 set on for " + String(ch1_hours)+" hours");
    Blynk.notify("CH1 ON!");
    delay(100);
    Blynk.virtualWrite(21,1);
   // Blynk.virtualWrite(11,255);
    //msg1="The CH2 set on";
   // msg2="for "+ String(pg_hours)+" Hours";
   // LCDwrite(msg1, msg2 );
   refresh_menu();
  } 
  else if ((param.asInt()==0)) 
  {
    digitalWrite(channels[1], LOW);
    lights&=~(1<<1);
     Serial.println("The CH2 set off");
    Blynk.notify("CH2 OFF!");
    delay(100);
    Blynk.virtualWrite(21,0);
   // Blynk.virtualWrite(11,0);
   // msg1="The CH2 set off";
   // msg2="after "+ String(pg_hours)+" Hours";
   // LCDwrite(msg1, msg2 );
   refresh_menu();
  }
  
}

BLYNK_WRITE(V2)  // Manual selection
{
  if ((param.asInt()==1)) 
  { //turn ON the pgm
    digitalWrite(channels[2], HIGH);
    lights|=(1<<2);
    Serial.println("The CH3 set on for " + String(ch1_hours)+" hours");
    Blynk.notify("CH3 ON!");
    delay(100);
    Blynk.virtualWrite(22,1);
   // Blynk.virtualWrite(12,255);
    //msg1="The CH3 set on";
    //msg2="for "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
    refresh_menu();
  } 
  else if ((param.asInt()==0)) 
  {
    digitalWrite(channels[2], LOW);
    lights&=~(1<<2);
     Serial.println("The CH3 set off");
    Blynk.notify("CH3 OFF!");
    delay(100);
    Blynk.virtualWrite(22,0);
   // Blynk.virtualWrite(12,0);
    //msg1="The CH3 set off";
    //msg2="after "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
    refresh_menu();
  }
  
}

BLYNK_WRITE(V3)  // Manual selection
{
  if ((param.asInt()==1)) 
  { //turn ON the pgm
    digitalWrite(channels[3], HIGH);
    lights|=(1<<3);
    Serial.println("The CH4 set on for " + String(ch1_hours)+" hours");
    Blynk.notify("CH4 ON!");
    delay(100);
    Blynk.virtualWrite(23,1);
    //Blynk.virtualWrite(13,255);
    //msg1="The CH4 set on";
    //msg2="for "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
    refresh_menu();
  } 
  else if ((param.asInt()==0)) 
  {
    digitalWrite(channels[1], LOW);
    lights&=~(1<<3);
     Serial.println("The CH4 set off");
    Blynk.notify("CH4 OFF!");
    delay(100);
    Blynk.virtualWrite(23,0);
   // Blynk.virtualWrite(13,0);
    //msg1="The CH4 set off";
    //msg2="after "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
    refresh_menu();
  }
  
}

BLYNK_WRITE(V4)  // Manual selection
{
  if ((param.asInt()==1)) 
  { //turn ON the pgm
    digitalWrite(channels[4], HIGH);
    lights|=(1<<4);
    Serial.println("The CH5 set on for " + String(ch1_hours)+" hours");
    Blynk.notify("CH5 ON!");
    delay(100);
    Blynk.virtualWrite(24,1);
   // Blynk.virtualWrite(14,255);
    //msg1="The CH5 set on";
    //msg2="for "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
    refresh_menu();
  } 
  else if ((param.asInt()==0)) 
  {
    digitalWrite(channels[4], LOW);
    lights&=~(1<<4);
     Serial.println("The CH5 set off");
    Blynk.notify("CH5 OFF!");
    delay(100);
    Blynk.virtualWrite(24,0);
    //Blynk.virtualWrite(14,0);
    //msg1="The CH5 set off";
    //msg2="after "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
    refresh_menu();
  }
  
}

BLYNK_WRITE(V5)  // Manual selection
{
  if ((param.asInt()==1)) 
  { //turn ON the pgm
    digitalWrite(channels[5], HIGH);
    lights|=(1<<5);
    Serial.println("The CH6 set on for " + String(ch1_hours)+" hours");
    Blynk.notify("CH6 ON!");
    delay(100);
    Blynk.virtualWrite(25,1);
    //Blynk.virtualWrite(15,255);
    //msg1="The CH6 set on";
    //msg2="for "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
    refresh_menu();
  } 
  else if ((param.asInt()==0)) 
  {
    digitalWrite(channels[5], LOW);
    lights&=~(1<<5);
     Serial.println("The CH6 set off");
    Blynk.notify("CH6 OFF!");
    delay(100);
    Blynk.virtualWrite(25,0);
    //Blynk.virtualWrite(15,0);
    //msg1="The CH6 set off";
    //msg2="after "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
    refresh_menu();
  }
  
}

BLYNK_WRITE(V6)  // Manual selection
{
  if ((param.asInt()==1)) 
  { //turn ON the pgm
    digitalWrite(channels[6], HIGH);
    lights|=(1<<6);
    Serial.println("The CH7 set on for " + String(ch1_hours)+" hours");
    Blynk.notify("CH7 ON!");
    delay(100);
    Blynk.virtualWrite(26,1);
    //Blynk.virtualWrite(16,255);
    //msg1="The CH7 set on";
    //msg2="for "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
    refresh_menu();
  } 
  else if ((param.asInt()==0)) 
  {
    digitalWrite(channels[6], LOW);
    lights&=~(1<<6);
     Serial.println("The CH7 set off");
    Blynk.notify("CH7 OFF!");
    delay(100);
    Blynk.virtualWrite(26,0);
   // Blynk.virtualWrite(16,0);
    //msg1="The CH7 set off";
    //msg2="after "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
    refresh_menu();
  }
  
}

BLYNK_WRITE(V7)  // Manual selection
{
  if ((param.asInt()==1)) 
  { //turn ON the pgm
    digitalWrite(channels[7], HIGH);
    lights|=(1<<7);
    Serial.println("The CH8 set on for " + String(ch1_hours)+" hours");
    Blynk.notify("CH8 ON!");
    delay(100);
    Blynk.virtualWrite(27,1);
    //Blynk.virtualWrite(17,255);
    //msg1="The CH8 set on";
    //msg2="for "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
    refresh_menu();
  } 
  else if ((param.asInt()==0)) 
  {
    digitalWrite(channels[7], LOW);
    lights&=~(1<<7);
     Serial.println("The CH8 set off");
    Blynk.notify("CH8 OFF!");
    delay(100);
    Blynk.virtualWrite(27,0);
    //Blynk.virtualWrite(17,0);
    //msg1="The CH8 set off";
    //msg2="after "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
    refresh_menu();
  }
  
}


BLYNK_WRITE(V8)  // Manual selection
{
  if ((param.asInt()==1))
  { //turn ON the pgm
   // digitalWrite(channels[0], HIGH);
    auto_light|=(1<<0);
    Serial.println("The CH1 set on for " + String(ch1_hours)+" hours");
    Blynk.notify("CH1 ON!");
    delay(100);
    Blynk.virtualWrite(35,1);
    //msg1="The CH1 set on";
    //msg2="for "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
  } 
  else if ((param.asInt()==0) ) 
  {
    //digitalWrite(channels[1], LOW);
    auto_light&=~(1<<0);
     Serial.println("The CH1 set off");
    Blynk.notify("CH1 OFF!");
    delay(100);
    Blynk.virtualWrite(35,0);
    //msg1="The CH1 set off";
    //msg2="after "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
  }
  
}

BLYNK_WRITE(V9)  // Manual selection
{
  if ((param.asInt()==1)) 
  { //turn ON the pgm
    //digitalWrite(channels[1], HIGH);
    auto_light|=(1<<1);
    Serial.println("The CH2 set on for " + String(ch1_hours)+" hours");
    Blynk.notify("CH1 ON!");
    delay(100);
    Blynk.virtualWrite(36,1);
    //msg1="The CH2 set on";
   // msg2="for "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
  } 
  else if ((param.asInt()==0)) 
  {
    //digitalWrite(channels[1], LOW);
    auto_light&=~(1<<1);
     Serial.println("The CH2 set off");
    Blynk.notify("CH2 OFF!");
    delay(100);
    Blynk.virtualWrite(36,0);
    //msg1="The CH2 set off";
    //msg2="after "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
  }
  
}

BLYNK_WRITE(V18)  // Manual selection
{
  if ((param.asInt()==1)) 
  { //turn ON the pgm
    //digitalWrite(channels[2], HIGH);
    lights|=(1<<2);
    Serial.println("The CH3 set on for " + String(ch1_hours)+" hours");
    Blynk.notify("CH3 ON!");
    delay(100);
    Blynk.virtualWrite(37,255);
    //msg1="The CH3 set on";
    //msg2="for "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
  } 
  else if ((param.asInt()==0)) 
  {
    //digitalWrite(channels[2], LOW);
    lights&=~(1<<2);
     Serial.println("The CH3 set off");
    Blynk.notify("CH3 OFF!");
    delay(100);
    Blynk.virtualWrite(37,0);
    //msg1="The CH3 set off";
    //msg2="after "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
  }
  
}

BLYNK_WRITE(V19)  // Manual selection
{
  if ((param.asInt()==1)) 
  { //turn ON the pgm
    //digitalWrite(channels[3], HIGH);
    lights|=(1<<3);
    Serial.println("The CH4 set on for " + String(ch1_hours)+" hours");
    Blynk.notify("CH4 ON!");
    delay(100);
    Blynk.virtualWrite(38,255);
    //msg1="The CH4 set on";
    //msg2="for "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
  } 
  else if ((param.asInt()==0)) 
  {
    //digitalWrite(channels[1], LOW);
    lights&=~(1<<3);
     Serial.println("The CH4 set off");
    Blynk.notify("CH4 OFF!");
    delay(100);
    Blynk.virtualWrite(38,0);
    //msg1="The CH4 set off";
   // msg2="after "+ String(pg_hours)+" Hours";
   // LCDwrite(msg1, msg2 );
  }
  
}

BLYNK_WRITE(V28)  // Manual selection
{
  if ((param.asInt()==1)) 
  { //turn ON the pgm
    //digitalWrite(channels[4], HIGH);
    lights|=(1<<4);
    Serial.println("The CH5 set on for " + String(ch1_hours)+" hours");
    Blynk.notify("CH5 ON!");
    delay(100);
    Blynk.virtualWrite(39,1);
    //msg1="The CH5 set on";
    //msg2="for "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
  } 
  else if ((param.asInt()==0)) 
  {
    //digitalWrite(channels[4], LOW);
    lights&=~(1<<4);
     Serial.println("The CH5 set off");
    Blynk.notify("CH5 OFF!");
    delay(100);
    Blynk.virtualWrite(39,0);
    //msg1="The CH5 set off";
    //msg2="after "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
  }
  
}

BLYNK_WRITE(V29)  // Manual selection
{
  if ((param.asInt()==1)) 
  { //turn ON the pgm
    //digitalWrite(channels[5], HIGH);
    lights|=(1<<5);
    Serial.println("The CH6 set on for " + String(ch1_hours)+" hours");
    Blynk.notify("CH6 ON!");
    delay(100);
    Blynk.virtualWrite(40,1);
    //msg1="The CH6 set on";
    //msg2="for "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
  } 
  else if ((param.asInt()==0)) 
  {
    //digitalWrite(channels[5], LOW);
    lights&=~(1<<5);
     Serial.println("The CH6 set off");
    Blynk.notify("CH6 OFF!");
    delay(100);
    Blynk.virtualWrite(40,0);
    //msg1="The CH6 set off";
    //msg2="after "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
  }
  
}

BLYNK_WRITE(V33)  // Manual selection
{
  if ((param.asInt()==1)) 
  { //turn ON the pgm
    //digitalWrite(channels[6], HIGH);
    lights|=(1<<6);
    Serial.println("The CH7 set on for " + String(ch1_hours)+" hours");
    Blynk.notify("CH7 ON!");
    delay(100);
    Blynk.virtualWrite(41,1);
    //msg1="The CH7 set on";
    //msg2="for "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
  } 
  else if ((param.asInt()==0)) 
  {
    //digitalWrite(channels[6], LOW);
    lights|=~(1<<6);
     Serial.println("The CH7 set off");
    Blynk.notify("CH7 OFF!");
    delay(100);
    Blynk.virtualWrite(41,0);
    //msg1="The CH7 set off";
    //msg2="after "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
  }
  
}

BLYNK_WRITE(V34)  // Manual selection
{
  if ((param.asInt()==1)) 
  { //turn ON the pgm
    //digitalWrite(channels[7], HIGH);
    lights|=(1<<7);
    Serial.println("The CH8 set on for " + String(ch1_hours)+" hours");
    Blynk.notify("CH8 ON!");
    delay(100);
    Blynk.virtualWrite(42,1);
    //msg1="The CH8 set on";
    //msg2="for "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
  } 
  else if ((param.asInt()==1)) 
  {
    //digitalWrite(channels[7], LOW);
    lights&=~(1<<7);
     Serial.println("The CH8 set off");
    Blynk.notify("CH8 OFF!");
    delay(100);
    Blynk.virtualWrite(42,0);
    //msg1="The CH8 set off";
    //msg2="after "+ String(pg_hours)+" Hours";
    //LCDwrite(msg1, msg2 );
  }
  
}

BLYNK_WRITE(V10)
{
ch1_hours=param.asInt();
}
BLYNK_WRITE(V11)
{
ch2_hours=param.asInt();
}
BLYNK_WRITE(V12)
{
ch3_hours=param.asInt();
}
BLYNK_WRITE(V13)
{
ch4_hours=param.asInt();
}
BLYNK_WRITE(V14)
{
ch5_hours=param.asInt();
}
BLYNK_WRITE(V15)
{
ch6_hours=param.asInt();
}
BLYNK_WRITE(V16)
{
ch7_hours=param.asInt();
}
BLYNK_WRITE(V17)
{
ch8_hours=param.asInt();
}

BLYNK_WRITE(V30)// lights sceduler  
{
   TimeInputParam t(param);
    int dayadjustment = -1;  
    if(weekday() == 1)
    {
      dayadjustment =  6; // needed for Sunday, Time library is day 1 and Blynk is day 7
    }
    if(t.isWeekdaySelected(weekday() + dayadjustment))
    {
       if (t.hasStartTime()) // Process start time
      {
          start1=(t.getStartHour()*3600)+(t.getStartMinute()*60);
          nowseconds=(timeClient.getHours())*3600+(timeClient.getMinutes())*60+(timeClient.getSeconds());
          if((nowseconds>=start1)&&(nowseconds<=start1+30)) 
           { for(int i=0; i<=7; i++)
            {
              if((auto_light>>i)&0x01)
              event_hanler(TIME_START, channels[i]);
            }
           } 
      }
    }   
}

void reconnectBlynk() {
  if (!Blynk.connected()) {
    if(Blynk.connect()) {
     BLYNK_LOG("Reconnected");
     msg1="Reconnected...";
     msg2=".......";
     LCDwrite(msg1, msg2 );
     delay(2000);
      msg1="Timer time";
      msg2="set to :"+ String(pg_hours)+" Hours";
      LCDwrite(msg1, msg2 );
    } else {
      BLYNK_LOG("Not reconnected");
      if (!start_rec_timer)
      {
        disconnect_timer=millis();
        start_rec_timer=true;
      }
      if ((millis()- disconnect_timer) > 15000)
      {
      modem.restart();
      start_rec_timer=false;
      msg1="Initializing gsm";
      msg2="Wait connecting.";
      LCDwrite(msg1, msg2 );
      delay(2000);
       msg1="Timer time";
       msg2="set to :"+ String(pg_hours)+" Hours";
       LCDwrite(msg1, msg2 );
      }
    }
  }
}



//Buttons_menu function
void buttonsCheck() {
	bouncer_Up.update();
  if (bouncer_Up.fell())
	 {
		// Calls the function identified with
		// increase or 1 for the focused line.
		menu.call_function(1);
    //menu.next_screen();
    menu.update();
	}
  bouncer_Down.update();
  if (bouncer_Down.fell())
  {
		menu.call_function(2);
    //menu.previous_screen();
    menu.update();
	}
  bouncer_Enter.update();
	if (bouncer_Enter.fell()) {
		// Switches focus to the next line.
		//menu.call_function(3);
    menu.switch_focus();
    menu.update();
	}
  bouncer_Esc.update();
  if (bouncer_Esc.fell()) {
    //menu.call_function(4);
    //LCDwrite("Button ESC", "Pressed" );
    menu.previous_screen();
    menu.update();
  } 
  /*if (up.check() == LOW) {
		menu.next_screen();
	}
	if (down.check() == LOW) {
		menu.previous_screen();
	}
	if (enter.check() == LOW) {
		// Switches focus to the next line.
		menu.switch_focus();
	}*/
}



// Callback functions

void idle_function(){}
void nextScreen()
{
  Serial.printf("current screen = %d \n",menu.get_currentScreen());
  Serial.printf("focused line = %d \n",menu.get_focusedLine());
  if((menu.get_currentScreen()==&welcome_screen)&&(menu.get_focusedLine()==3))
  menu.change_screen(&screen2);
  else if((menu.get_currentScreen()==&screen2)&&(menu.get_focusedLine()==1))
  menu.change_screen(&screen3);
  else if((menu.get_currentScreen()==&screen2)&&(menu.get_focusedLine()==2))
  menu.change_screen(&screen4);
  else if((menu.get_currentScreen()==&screen2)&&(menu.get_focusedLine()==3))
  {
    menu.change_screen(&screen6);
    Serial.printf("current screen = %d \n",menu.get_currentScreen());
  }
  else if((menu.get_currentScreen()==&screen2)&&(menu.get_focusedLine()==4))
  menu.change_screen(&screen5);
  else if(menu.get_currentScreen()!=&welcome_screen) 
  menu.change_screen(&welcome_screen);
   
}

void nextLine()
{
  menu.switch_focus(true);
}

void prevLine()
{
  menu.switch_focus(false);
}

void toggle_lights()
{
  uint8_t channel;
  channel=menu.get_focusedLine()-1;
  Serial.printf("channel=%d \n", channel);
  Serial.printf("lights=%d \n", lights);
  //if(menu.get_currentScreen()==&screen3)
  //{
    
    lights^=(1<<channel);
    digitalWrite(channels[channel], (0x01&(lights>>channel)));
    Blynk.virtualWrite(channel,(0x01&(lights>>channel)));
    Blynk.virtualWrite(channel+20,(0x01&(lights>>channel)));
   // Blynk.virtualWrite(channel+9,(0x01&(lights>>channel))?255:0);
  //}
  light_stat[channel]=(char*)(((lights>>channel)&0x01)?"On ":"Off");
  for (uint8_t i = 0; i<8; i++)
 {
    if((lights>>i)&0x01) light_disp[i]='I';
    else light_disp[i]='X';
 }
  menu.softUpdate();
   Serial.printf("lights=%d \n", lights);
}

void toggle_lights_auto()
{
  uint8_t channel;
  channel=menu.get_focusedLine()-1;
  //if(menu.get_currentScreen()==&screen4)
  auto_light^=(1<<channel);
  Blynk.virtualWrite(channel+20,(0x01&(auto_light>>channel)));
  //Blynk.virtualWrite(channel+19,(0x01&(auto_light>>channel))?255:0);
  //EEPROM.put(1,auto_light);
  prefs.putUChar("auto_light", auto_light);
  light_aut_stat[channel]=(char*)(((auto_light>>channel)&0x01)?"On ":"Off");
 for (uint8_t i = 0; i<8; i++)
 {
    if((auto_light>>i)&0x01) auto_light_disp[i]='I';
    else auto_light_disp[i]='X';
 }
  menu.softUpdate();
}

void time_increment()
{
  //if(menu.get_currentScreen()==&screen5)
  //{
  switch(menu.get_focusedLine())
  {
  case 1:
  ch1_hours++;
  if (ch1_hours==12)ch1_hours=0;
  prefs.putUChar("ch1_time",ch1_hours);
  Blynk.virtualWrite(10,ch1_hours);
  break;
  case 2:
  ch2_hours++;
  if (ch2_hours==12)ch2_hours=0;
  prefs.putUChar("ch2_time",ch2_hours);
  Blynk.virtualWrite(11,ch2_hours);
  break;
  case 3:
  ch3_hours++;
  if (ch3_hours==12)ch3_hours=0;
  prefs.putUChar("ch3_time",ch3_hours);
  Blynk.virtualWrite(12,ch3_hours);
  break;
  case 4:
  ch4_hours++;
  if (ch4_hours==12)ch4_hours=0;
  prefs.putUChar("ch4_time",ch4_hours);
  Blynk.virtualWrite(13,ch4_hours);
  break;
  case 5:
  ch5_hours++;
  if (ch5_hours==12)ch5_hours=0;
  prefs.putUChar("ch5_time",ch5_hours);
  Blynk.virtualWrite(14,ch5_hours);
  break;
  case 6:
  ch6_hours++;
  if (ch6_hours==12)ch6_hours=0;
  prefs.putUChar("ch6_time",ch6_hours);
  Blynk.virtualWrite(15,ch6_hours);
  break; 
  case 7:
  ch7_hours++;
  if (ch7_hours==12)ch7_hours=0;
  prefs.putUChar("ch7_time",ch7_hours);
  Blynk.virtualWrite(16,ch7_hours);
  break;
  case 8:
  ch8_hours++;
  if (ch8_hours==12)ch8_hours=0;
  prefs.putUChar("ch8_time",ch8_hours);
  Blynk.virtualWrite(17,ch8_hours);
  break;

  }
  //}
  menu.update();
}

void time_decr()
{
//if(menu.get_currentScreen()==&screen5)
  //{
  switch(menu.get_focusedLine())
  {
  case 1:
  ch1_hours--;
  if (ch1_hours==0)ch1_hours=12;
  prefs.putUChar("ch1_time",ch1_hours);
  break;
  case 2:
  ch2_hours--;
  if (ch2_hours==0)ch2_hours=12;
  prefs.putUChar("ch2_time",ch2_hours);
  break;
  case 3:
  ch3_hours--;
  if (ch3_hours==0)ch3_hours=12;
  prefs.putUChar("ch3_time",ch3_hours);
  case 4:
  ch4_hours--;
  if (ch4_hours==0)ch4_hours=12;
  prefs.putUChar("ch4_time",ch4_hours);
  break;
  case 5:
  ch5_hours--;
  if (ch5_hours==0)ch5_hours=12;
  prefs.putUChar("ch5_time",ch5_hours);
  break;
  case 6:
  ch6_hours--;
  if (ch6_hours==0)ch6_hours=12;
  prefs.putUChar("ch6_time",ch6_hours);
  break; 
  case 7:
  ch5_hours--;
  if (ch7_hours==0)ch7_hours=12;
  prefs.putUChar("ch7_time",ch7_hours);
  break;
  case 8:
  ch5_hours--;
  if (ch8_hours==0)ch8_hours=12;
  prefs.putUChar("ch8_time",ch8_hours);
  break;

  }
  //}
  menu.update();
}

void assign_channel()
{
   static uint8_t temp=0;
  uint8_t channels_temp[8]={channel1, channel2, channel3, channel4, channel5, channel6, channel7, channel8};
  uint8_t line;
  if(menu.get_currentScreen()==&screen6)
  {
  line=menu.get_focusedLine();
  temp++;
  if (temp>=8) temp=0;
 // switch(line)
  //{
   // case 1:
    channels[line-1]=channels_temp[temp];
   /* break;
    case 2:
    channels[line]=channels_temp[temp];
    break;
    case 3:
    channels[temp]=channel3;
    break;
    case 4:
    channels[temp]=channel4;
    break;
    case 5:
    channels[temp]=channel5;
    break;
    case 6:
    channels[temp]=channel6;
    break;
    case 7:
    channels[temp]=channel7;
    break;
    case 8:
    channels[temp]=channel8;
    break;

  }*/
  
  }
  menu.update();
}

void assign_channel_()
{
  static uint8_t temp=0;
  uint8_t channels_temp[8]={channel1, channel2, channel3, channel4, channel5, channel6, channel7, channel8};
  uint8_t line;
  if(menu.get_currentScreen()==&screen6)
  {
  line=menu.get_focusedLine();
  temp--;
  if (temp<=0) temp=7;
 // switch(line)
  //{
   // case 1:
    channels[line-1]=channels_temp[temp];
   /* break;
    case 2:
    channels[line]=channels_temp[temp];
    break;
    case 3:
    channels[temp]=channel3;
    break;
    case 4:
    channels[temp]=channel4;
    break;
    case 5:
    channels[temp]=channel5;
    break;
    case 6:
    channels[temp]=channel6;
    break;
    case 7:
    channels[temp]=channel7;
    break;
    case 8:
    channels[temp]=channel8;
    break;

  }*/
  
  }
  menu.update();
}

//Event handler ################################
//####################################################
void event_hanler(EVENT event, int channel)
{
  started_times[channel]=timeClient.getEpochTime();
  stop_times[channel]=started_times[channel]+ch_hours[channel]*3600;

}

//###### AutomationHandler*************
void automation_handler()
{
  uint8_t i;
  for (i=0;i<=7;i++)
  {
    if((auto_light>>i)&0x01)
      if(stop_times[i]<=timeClient.getEpochTime())
        { digitalWrite(channels[i],0);
          Blynk.virtualWrite(i,0);
        }
  }

}
//############################################
void setup() {
  // put your setup code here, to run once:
  //pinMode(ENTER, INPUT_PULLUP);
 // pinMode(UP, INPUT_PULLUP);
  //pinMode(DOWN, INPUT_PULLUP);
  //power configuration
  bool isOk = setPowerBoostKeepOn(1);
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);
  //pinMode(OUT_pg, OUTPUT);
 // digitalWrite(OUT_pg, LOW);

  //output channels configuration
 for (uint8_t i = 0; i<8; i++)
 {
   pinMode(channels[i], OUTPUT);
   digitalWrite(channels[i], LOW);
 }

  pg_on=LOW;
  
  pinMode(ENTER, INPUT);
  pinMode(ESC, INPUT_PULLUP);
  pinMode(UP, INPUT);
  pinMode(DOWN, INPUT);

  bouncer_Enter.attach(ENTER);
  bouncer_Enter.interval(5);
  bouncer_Up.attach(UP);
  bouncer_Up.interval(5);
  bouncer_Down.attach(DOWN);
  bouncer_Down.interval(5);
  bouncer_Esc.attach(ESC);
  bouncer_Esc.interval(5);
  
 
  Serial.begin(115200);
  I2CPower.begin(I2C_SDA, I2C_SCL, 400000);
  I2Cbuttons.begin(I2C_SDA_2, I2C_SCL_2, 400000);
  delay(10);
  // Set GSM module baud rate
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  //lcd.begin(20, 4);
   // initialize LCD
  lcd.init(I2C_SDA_2,I2C_SCL_2);                      // initialize the lcd 
 //lcd.begin(20,4);
  lcd.backlight();
  /////////
  ConnectionHandler();
  prev_millis=millis();
  EEPROM.get(0, pg_hours);
  prefs.begin("values_store",false);
  ch1_hours=prefs.getUChar("ch1_hours",1);
  ch2_hours=prefs.getUChar("ch2_hours",1);
  ch3_hours=prefs.getUChar("ch3_hours",1);
  ch4_hours=prefs.getUChar("ch4_hours",1);
  ch5_hours=prefs.getUChar("ch5_hours",1);
  ch6_hours=prefs.getUChar("ch6_hours",1);
  ch7_hours=prefs.getUChar("ch7_hours",1);
  ch8_hours=prefs.getUChar("ch8_hours",1);
  auto_light=prefs.getUChar("auto_light", auto_light);
 /* free(light_disp);
  free(auto_light_disp);
  light_disp=(char *)malloc(10*sizeof(char));
  auto_light_disp=(char *)malloc(10*sizeof(char));*/
  lights=0;
  light_disp[8] = '\0';
  auto_light_disp[8] = '\0';
  for (uint8_t i = 0; i<8; i++)
 {
 /* if((lights>>i)&0x01) light_disp[i]='I';
  else light_disp[i]='X';
  if((auto_light>>i)&0x01) auto_light_disp[i]='I';
  else auto_light_disp[i]='X';*/
  light_disp[i]=(((lights>>i)&0x01)?'I':'X');
  auto_light_disp[i]=(((auto_light>>i)&0x01)?'I':'X');
  light_stat[i]=(char*)(((lights>>i)&0x01)?"On ":"Off");
  light_aut_stat[i]=(char*)(((auto_light>>i)&0x01)?"On ":"Off");
 }
 // light_disp[8]='\0';
 // auto_light_disp[8]='\0';
  pinMode(LED_BUILTIN, OUTPUT);
 //uncoment following 2 rows if using 7 segment display
  //disp.setDigitPins(digitNum,digits);
  //disp.setCommonCathode();
  Blynk.virtualWrite(V4, pg_hours);
  Blynk.virtualWrite(0,0);
  //uncomment next rows if using 16x2 LCD
  msg1="Timer time";
  msg2="set to :"+ String(pg_hours)+" Hours";
  LCDwrite(msg1, msg2 );
  //@@@@@@@@@@@@@@@@@@@

    control.attach_function(1, nextScreen);
    control.attach_function(2, nextScreen);
  line22.attach_function(1, nextScreen);
  //line22.attach_function(1, nextLine);
  line22.attach_function(2, nextScreen);
  line23.attach_function(1, nextScreen);
 // line23.attach_function(1, nextLine);
  line23.attach_function(2, nextScreen);  
  line24.attach_function(1, nextScreen);
  //line24.attach_function(1, nextLine);
  line24.attach_function(2, nextScreen);
  line25.attach_function(1, nextScreen);
  //line25.attach_function(1, nextLine);
  line25.attach_function(2, nextScreen);
  line32.attach_function(1, toggle_lights);
  line32.attach_function(2, toggle_lights);
  //line32.attach_function(3, nextLine);
  line33.attach_function(1, toggle_lights);
  line33.attach_function(2, toggle_lights);
  //line33.attach_function(3, nextLine);
  line34.attach_function(1, toggle_lights);
  line34.attach_function(2, toggle_lights);
  //line34.attach_function(3, nextLine);
  line35.attach_function(1, toggle_lights);
  line35.attach_function(2, toggle_lights); 
  //line35.attach_function(3, nextLine);  
  line36.attach_function(1, toggle_lights);
  line36.attach_function(2, toggle_lights);
  //line36.attach_function(3, nextLine);
  line37.attach_function(1, toggle_lights);
  line37.attach_function(2, toggle_lights);
  //line37.attach_function(3, nextLine);
  line38.attach_function(1, toggle_lights);
  line38.attach_function(2, toggle_lights); 
  //line38.attach_function(3, nextLine);
  line39.attach_function(1, toggle_lights);
  line39.attach_function(2, toggle_lights); 
  //line39.attach_function(3, nextLine);  

  line42.attach_function(1, toggle_lights_auto);
  line42.attach_function(2, toggle_lights_auto);
  //line42.attach_function(3, nextLine);
  line43.attach_function(1, toggle_lights_auto);
  line43.attach_function(2, toggle_lights_auto);
  //line43.attach_function(3, nextLine);
  line44.attach_function(1, toggle_lights_auto);
  line44.attach_function(2, toggle_lights_auto);
  //line44.attach_function(3, nextLine);
  line45.attach_function(1, toggle_lights_auto);
  line45.attach_function(2, toggle_lights_auto);
  //line45.attach_function(3, nextLine);
  line46.attach_function(1, toggle_lights_auto);
  line46.attach_function(2, toggle_lights_auto);
  //line46.attach_function(3, nextLine);
  line47.attach_function(1, toggle_lights_auto);
  line47.attach_function(2, toggle_lights_auto);
  //line47.attach_function(3, nextLine);
  line48.attach_function(1, toggle_lights_auto);
  line48.attach_function(2, toggle_lights_auto);
  //line48.attach_function(3, nextLine);
  line49.attach_function(1, toggle_lights_auto);
  line49.attach_function(2, toggle_lights_auto);
  //line49.attach_function(3, nextLine);

  line52.attach_function(1,time_increment);
  line52.attach_function(2,time_decr);
  //line52.attach_function(3, nextLine);
  line53.attach_function(1,time_increment);
  line53.attach_function(2,time_decr);
  //line53.attach_function(3, nextLine);
  line54.attach_function(1,time_increment);
  line54.attach_function(2,time_decr);
  //line54.attach_function(3, nextLine);
  line55.attach_function(1,time_increment);
  line55.attach_function(2,time_decr);
  //line55.attach_function(3, nextLine);
  line56.attach_function(1,time_increment);
  line56.attach_function(2,time_decr);
  //line56.attach_function(3, nextLine);
  line57.attach_function(1,time_increment);
  line57.attach_function(2,time_decr);
  //line57.attach_function(3, nextLine);
  line58.attach_function(1,time_increment);
  line58.attach_function(2,time_decr);
  //line58.attach_function(3, nextLine);
  line59.attach_function(1,time_increment);
  line59.attach_function(2,time_decr);
  //line59.attach_function(3, nextLine);
  
  line62.attach_function(1,assign_channel);
  line62.attach_function(2,assign_channel_);
  //line62.attach_function(3, nextLine);
  line63.attach_function(1,assign_channel);
  line63.attach_function(2,assign_channel_);
  //line63.attach_function(3, nextLine);
  line64.attach_function(1,assign_channel);
  line64.attach_function(2,assign_channel_);
  //line64.attach_function(3, nextLine);
  line65.attach_function(1,assign_channel);
  line65.attach_function(2,assign_channel_);
  //line65.attach_function(3, nextLine);
  line66.attach_function(1,assign_channel);
  line66.attach_function(2,assign_channel_);
  //line66.attach_function(3, nextLine);
  line67.attach_function(1,assign_channel);
  line67.attach_function(2,assign_channel_);
  //line67.attach_function(3, nextLine);
  line68.attach_function(1,assign_channel);
  line68.attach_function(2,assign_channel_);
  //line68.attach_function(3, nextLine);
  line69.attach_function(1,assign_channel);
  line69.attach_function(2,assign_channel_);
  //line69.attach_function(3, nextLine);

   menu.init();
	menu.add_screen(screen2);
	menu.add_screen(screen3);
	menu.add_screen(screen4);
  menu.add_screen(screen5);
  menu.add_screen(screen6);
  screen2.add_line(line25);
  screen3.add_line(line35);
  screen3.add_line(line36);
  screen3.add_line(line37);
  screen3.add_line(line38);
  screen3.add_line(line39);  
  screen4.add_line(line45);
  screen4.add_line(line46);
  screen4.add_line(line47);
  screen4.add_line(line48);
  screen4.add_line(line49);
  screen5.add_line(line55);
  screen5.add_line(line56);
  screen5.add_line(line57);
  screen5.add_line(line58);
  screen5.add_line(line59);
  screen6.add_line(line65);
  screen6.add_line(line66);
  screen6.add_line(line67);
  screen6.add_line(line68);
  screen6.add_line(line69);

  screen2.set_displayLineCount(4);
  screen3.set_displayLineCount(4);
  screen4.set_displayLineCount(4);
  screen5.set_displayLineCount(4);
  screen6.set_displayLineCount(4);


  menu.update();
  //@@@@@@@@@@@@@@@@@@@@@@@@@
 //%%%%%%%%%%%%%%%%%%%%
 timeClient.begin();
  timeClient.setTimeOffset(10800);
  timeClient.update();
  setTime(timeClient.getEpochTime());
  connectionHandlerTimer.setInterval(100, ConnectionHandler);
  refreshmenuTimer.setInterval(200,refresh_menu);
  connectionState = AWAIT_GSM_CONNECTION;
}

void loop() {
  // put your main code here, to run repeatedly:
  
  
  //uncomment the next row if using 7 segnment display
  //disp.write(pg_hours);
  buttonsCheck();
  //scan_buttons(&button_msg);
  connectionHandlerTimer.run();
  refreshmenuTimer.run();
  if(healthy) Blynk.run();
 delay(20);
 
}

//LCD routine
void LCDwrite(String msg1, String msg2 )
{
 
  //LCD_progress_bar (0, WaterDepth1, 0, MAX_DISTANCE);
  //lcd.clear();
  //lpg.draw(WaterDepth1);
    
    // go to row 1 column 0, note that this is indexed at 0
   lcd.setCursor(0,0);
   lcd.clear();
   //for(int i=0;i<16;i++) lcd.write(1022);
   lcd.setCursor(0,1);
   lcd.clear();
  // for(int i=0;i<16;i++) lcd.write(1022);
   lcd.setCursor(0,0);
   delay(50);
   lcd.print(msg1);
   delay(50);
   lcd.setCursor(0,1);
   lcd.setCursor(0,1);
   delay(50);
   lcd.print(msg2);
   
}

//Power boost
bool setPowerBoostKeepOn(int en){
  I2CPower.beginTransmission(IP5306_ADDR);
  I2CPower.write(IP5306_REG_SYS_CTL0);
  if (en) {
    I2CPower.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
  } else {
    I2CPower.write(0x35); // 0x37 is default reg value
  }
  return I2CPower.endTransmission() == 0;
}

void scan_buttons(uint8_t * buttons)
{
  //I2Cbuttons.beginTransmission(0x01);
  I2Cbuttons.requestFrom(0x08,1);
  while(I2Cbuttons.available()) 
  I2Cbuttons.readBytes(buttons,1);
  if ((*buttons!=0xff)||(*buttons!=0x00))
  {
   lights^=(1<<*buttons);
    digitalWrite(channels[*buttons], (0x01&(lights>>*buttons)));
    Blynk.virtualWrite(*buttons,(0x01&(lights>>*buttons)));
  }
  I2Cbuttons.endTransmission();
}
//######################Menu#########


void day_night_check(int ldr_value)
{

}




