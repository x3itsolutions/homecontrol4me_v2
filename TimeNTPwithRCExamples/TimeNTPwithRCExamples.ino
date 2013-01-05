#include <Time.h> 
#include <TimeAlarms.h>
#include <SPI.h>    
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 
byte ip[] = { 192, 168, 121, 101 }; // set the IP address to an unused address on your network

IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov NTP server
//byte SNTP_server_IP[] = { 130,149,17,21};    // ntps1-0.cs.tu-berlin.de
//byte SNTP_server_IP[] = { 192,53,103,108};   // ptbtime1.ptb.de

unsigned int localPort = 8888;      // local port to listen for UDP packets

const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 

// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

time_t prevDisplay = 0; // when the digital clock was displayed
const  long timeZoneOffset = -3600L; // set this to the offset in seconds to your local time;
AlarmID_t timers[10];

void setup() 
{
  
  // 433MHz Transmitter is connected to Arduino Pin #10  
  mySwitch.enableTransmit(10);
  
  
  Serial.begin(9600);
  Ethernet.begin(mac,ip);  
  Serial.println("waiting for sync");
  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
  while(timeStatus()== timeNotSet)   
  
  
     ; // wait until the time is set by the sync provider
  timers[0] = Alarm.alarmRepeat(20,10,0, jAlarm);  // 8:10pm every day - calls function jAlarm();
  timers[0] = Alarm.alarmRepeat(20,15,0, jAlarm2);  // 8:15pm every day - calls function jAlarm2();
  
}
int i;
void loop()
{  
  if( now() != prevDisplay) //update the display only if the time has changed
  {
    prevDisplay = now();
    digitalClockDisplay();  
  }
  Alarm.delay(1000); 
  
}

void jAlarm(){
  mySwitch.switchOn("11001", "01010"); //Example for wireless sockets with DIP switches
  delay(300);
  mySwitch.switchOn('b', 3, 2); //Example for Intertechno family 'b', group #3, device #2  or if you have only 2 switches family 'b' device #10
  Alarm.free(timers[0]); //disable Timer 0 & make free for reuse
  timers[0] = Alarm.alarmRepeat(20,30,0, jAlarm);  // reuse timer 0 at 8:30pm every day 
}

void jAlarm2(){
  mySwitch.switchOff("11001", "01010"); //Example for wireless sockets with DIP switches
  delay(300);
  mySwitch.switchOff('b', 3, 2); //Example for Intertechno family 'b', group #3, device #2  or if you have only 2 switches family 'b' device #10
  Alarm.free(timers[0]); //disable Timer 0 & make free for reuse
  timers[0] = Alarm.alarmRepeat(20,35,0, jAlarm);  // reuse timer 0 at 8:35pm every day 
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

/*-------- NTP code ----------*/

unsigned long getNtpTime()
{
  sendNTPpacket(timeServer);
  delay(1000);
  
  if ( Udp.parsePacket() ) {  
    // We've received a packet, read the data from it
    Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;  
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL + timeZoneOffset;     
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;  
    return epoch;
  }
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address 
unsigned long sendNTPpacket(IPAddress& address)
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
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket(); 
}

