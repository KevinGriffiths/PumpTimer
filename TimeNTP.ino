#include <TimeLib.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 
char timeServer[] = "0.au.pool.ntp.org";
const int timeZone = 11;     // Sydney
EthernetUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
time_t prevDisplay = 0; // when the digital clock was displayed
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

// send an NTP request to the time server at the given address
void sendNTPpacket(char* address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
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
        Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
        unsigned long secsSince1900;
        unsigned long ReferenceTimeStamp; 
        unsigned long OriginateTimeStamp;  
        unsigned long ReceiveTimeStamp;  
        unsigned long ReceiveTimeStampFrac;  
        unsigned long TransmitTimeStampFrac;  
        float ReceiveTimeStampFracFloat;  
        float TransmitTimeStampFracFloat;  
        float TimeServerThought; 
        
        Serial.print("Size:- "); Serial.println(size);
        int i;
        for (i = 0; i <= 47; i= i + 1) 
             {
                Serial.print(packetBuffer[i]);Serial.print(".");
             }
        Serial.println(" ");
        for (i = 40; i <= 47; i= i + 1) 
             {
                Serial.print(packetBuffer[i]);Serial.print(".");
             }
        Serial.println(" ");

                  String IP = "";
                   IP = (unsigned long)packetBuffer[12]; IP +=".";
                   IP += (unsigned long)packetBuffer[13];IP +=".";
                   IP += (unsigned long)packetBuffer[14];IP +=".";
                   IP += (unsigned long)packetBuffer[15];
                  Serial.print("Reference IP: ");Serial.println(IP);

                  
                  ReferenceTimeStamp =  (unsigned long)packetBuffer[16] << 24;
                  ReferenceTimeStamp |= (unsigned long)packetBuffer[17] << 16;
                  ReferenceTimeStamp |= (unsigned long)packetBuffer[18] << 8;
                  ReferenceTimeStamp |= (unsigned long)packetBuffer[19];
                  Serial.print("Reference Time Stamp: ");Serial.println(ReferenceTimeStamp);
             
                  ReceiveTimeStamp =  (unsigned long)packetBuffer[32] << 24;
                  ReceiveTimeStamp |= (unsigned long)packetBuffer[33] << 16;
                  ReceiveTimeStamp |= (unsigned long)packetBuffer[34] << 8;
                  ReceiveTimeStamp |= (unsigned long)packetBuffer[35];
                  Serial.print("Receive Time Stamp: ");Serial.println(ReceiveTimeStamp);

                  ReceiveTimeStampFrac =  (unsigned long)packetBuffer[36] << 24;
                  ReceiveTimeStampFrac |= (unsigned long)packetBuffer[37] << 16;
                  ReceiveTimeStampFrac |= (unsigned long)packetBuffer[38] << 8;
                  ReceiveTimeStampFrac |= (unsigned long)packetBuffer[39];
                  ReceiveTimeStampFracFloat = (ReceiveTimeStampFrac / 4294967296.0);
                  Serial.print("Receive Time Stamp Frac: ");Serial.println(ReceiveTimeStampFracFloat,6);


        // convert four bytes starting at location 40 to a long integer
        secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
        secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
        secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
        secsSince1900 |= (unsigned long)packetBuffer[43];
        Serial.print("Transmit Time Stamp: ");Serial.println(secsSince1900);
        
                  TransmitTimeStampFrac =  (unsigned long)packetBuffer[40] << 24;
                  TransmitTimeStampFrac |= (unsigned long)packetBuffer[41] << 16;
                  TransmitTimeStampFrac |= (unsigned long)packetBuffer[42] << 8;
                  TransmitTimeStampFrac |= (unsigned long)packetBuffer[43];
                  TransmitTimeStampFracFloat = (TransmitTimeStampFrac / 4294967296.0);
                  Serial.print("Transmit Time Stamp Frac: ");Serial.println(TransmitTimeStampFracFloat,6);
                  TimeServerThought = TransmitTimeStampFracFloat - ReceiveTimeStampFracFloat;
                  Serial.print("Time Server Thought: ");Serial.println(TimeServerThought,6);
                  
        return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
      }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}



void digitalClockDisplay()
{
  // digital clock display of the time
  Serial.print(hour());  printDigits(minute());  printDigits(second());  Serial.print(" ");  Serial.print(day());  Serial.print(" ");  Serial.print(month());  Serial.print(" ");  Serial.print(year());   Serial.println(); 
}



void printDigits(int digits)
{
  Serial.print(":");
  if(digits < 10) Serial.print('0');
  Serial.print(digits);
}


void setup() 
{
  Serial.begin(9600);
  delay(250);
  Serial.println("Starting up");
  if (Ethernet.begin(mac) == 0) { while (1) {Serial.println("No Ethernet connection"); delay(10000);  }}
  Serial.print("IP address from DHCP is ");  Serial.println(Ethernet.localIP());
  Udp.begin(localPort);
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(3600); // Check NTP Time server every hour
}



void loop()
{  
  if (timeStatus() != timeNotSet) 
    {
      if (now() != prevDisplay) //update the display only if time has changed
        { 
           prevDisplay = now();
          // digitalClockDisplay();  
        }
    }
}






