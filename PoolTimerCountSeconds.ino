/*
  Pool Pump Timer with Internet Time
  Reads time from Internet using NTP
  14/4/18
*/
#include <TimeLib.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
//#include <SPI.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 
byte ip[] { 192, 168, 0, 21 };
char timeServer[] = "0.au.pool.ntp.org";
char site[] = "arduino.newcastleit.com.au"; 
const int timeZone = 10;     // Sydney
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
EthernetUDP Udp;
EthernetServer server(80);
String GetRequest;
unsigned int localPort = 8888;  // local port to listen for UDP packets
time_t prevDisplay = 0; // when the digital clock was displayed
time_t t;
long lastMillis = 0, loops = 0;
String PumpStatus = "Pump Off", TimeStatus = "Off";
int Hour, Minute, prevMin, MinuteAdd, MinutesRunning;
unsigned long SecondsRunning, prevMillis;


void LogInDatabase(String TextToWrite)
{
  GetRequest = "GET /collect_pool.php?Text=";GetRequest +=TextToWrite; GetRequest += " HTTP/1.0";
  Serial.println("writing to log");

  EthernetClient NS3;
 // NS3.setTimeout(1500);
  if (NS3.connect(site, 80)) 
      {
           Serial.println("connected");
           Serial.println(GetRequest);
           NS3.println(GetRequest); 
           NS3.println("Host: arduino.newcastleit.com.au"); NS3.println("Referer: Pool Timer ");
           NS3.println("User-Agent: Arduino Mt Hutton Pool Timer");NS3.println("Connection: close\r\n");
           Serial.println("Connection Closed");
           NS3.stop();
      }
}      
                              
// send an NTP request to the time server at the given address
void sendNTPpacket(char* address)
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);  // set all bytes in the buffer to 0
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) 
    {
      int size = Udp.parsePacket();
      if (size >= NTP_PACKET_SIZE) 
        {
          Serial.println("Receive NTP Response");
          LogInDatabase("Time%20updated%20NTP%20Data%20Received");
          Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
          unsigned long secsSince1900;
          secsSince1900 =  (unsigned long)packetBuffer[40] << 24; secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
          secsSince1900 |= (unsigned long)packetBuffer[42] << 8;  secsSince1900 |= (unsigned long)packetBuffer[43];
          return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
        }
    }
  Serial.println("No NTP Response :-(");
  LogInDatabase("No%20NTP%20Response");
  return 0; // return 0 if unable to get the time
}

void digitalClockDisplay(time_t tt)
{
  // digital clock display of the time
  Serial.print(hour(tt));  printDigits(minute(tt));  printDigits(second(tt));  Serial.print(" ");  Serial.print(day(tt));  Serial.print(" ");  Serial.print(month(tt));  Serial.print(" ");  Serial.print(year(tt));   Serial.println(); 
}

void printDigits(int digits)
{
  Serial.print(":");
  if(digits < 10) Serial.print('0');
  Serial.print(digits);
}


void TurnOnPump()
{
 // Serial.println("ON requested");
  if (PumpStatus == "Pump Off")
    {
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on 
      digitalWrite(2, HIGH); delay(500); digitalWrite(2, LOW);
      PumpStatus = "Pump Running";
      Serial.println("ON signal sent");
      LogInDatabase("Pump%20Turned%20On");
      prevMillis = millis();
      prevMin = Minute;
      
    } 
  else
    {
      while (millis() - prevMillis >= 1000) 
        {
          SecondsRunning++;
          prevMillis += 1000; 
        }
      if (Minute!=prevMin)
        {
          MinuteAdd = Minute - prevMin;
          if (MinuteAdd < 0)  {MinuteAdd=MinuteAdd+60;}
          MinutesRunning=MinutesRunning+MinuteAdd;
          Serial.println("Been running for:- "); Serial.println(MinutesRunning);
          prevMin = Minute;
        }
    }
}


void TurnOffPump()
{
  //Serial.println("OFF requested");
  if (PumpStatus == "Pump Running")
  {
    digitalWrite(LED_BUILTIN, LOW);   // turn the LED off
    digitalWrite(3, HIGH); delay(500); digitalWrite(3, LOW);
    PumpStatus = "Pump Off";
    Serial.println("OFF signal sent");
    LogInDatabase("Pump%20Turned%20Off");
         char mystr[40];
         sprintf(mystr,"Ran_for_sec:%lu",SecondsRunning); //l=long u=unsigned
         Serial.println(mystr);
    LogInDatabase(mystr);
         //char mystr1[40];
         sprintf(mystr,"Ran_for_min:%d",MinutesRunning); //d=integer     (%ld for long integer)
         Serial.println(mystr);
    LogInDatabase(mystr);    
  } 
}


void setup ()
{
  Serial.begin(57600);
  pinMode(2, OUTPUT);  pinMode(3, OUTPUT);  pinMode(LED_BUILTIN, OUTPUT);
  delay(250);
  Serial.println("Starting up");
  Ethernet.begin(mac, ip);  Serial.print("IP address from DHCP is ");  Serial.println(Ethernet.localIP());
  Udp.begin(localPort);  server.begin();
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(3600); // Check NTP Time server every hour
  LogInDatabase("Startup");
}

void loop () 
{
    TimeStatus = "Off";   // Default status unless time falls into an ON window
    //Serial.println("Pump Status:- " + PumpStatus);
    
    t = now();
    Hour = hour(t);
    Minute = minute(t);
    //Serial.println("Run time so far:- ");
    //Serial.println(SecondsRunning);
    
    if (Hour >= 22 && Hour <= 24)   // After 10pm 
      {
        TimeStatus = "On";
        TurnOnPump();
      }

    if (Hour >= 0 && Hour <= 4)   // Before 5am 
      {
        TimeStatus = "On";
        TurnOnPump();
      }

    if (Hour == 8 && Minute >= 0 && Minute <= 4 )   // 8am run for couple mins
      {
        TimeStatus = "On";
        TurnOnPump();
      }

    if (Hour == 11 && Minute >= 0 && Minute <= 4 )   // 11am run for couple mins
      {
        TimeStatus = "On";
        TurnOnPump();
      }

    if (Hour == 15 && Minute >= 0 && Minute <= 4 )   // 3pm run for couple mins
      {
        TimeStatus = "On";
        TurnOnPump();
      }

    if (Hour == 18 && Minute >= 0 && Minute <= 4 )   // 6pm run for couple mins
      {
        TimeStatus = "On";
        TurnOnPump();
      }
            
    if (TimeStatus == "Off")  //All other times  -  mostly the day ....
      {
        TurnOffPump();
      }
}




