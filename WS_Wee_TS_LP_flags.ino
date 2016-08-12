//Wee wifi library
#include "ESP8266.h"

#include <Wire.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <math.h>

//SODAQ Mbili libraries
#include <Sodaq_BMP085.h>
#include <Sodaq_SHT2x.h>
#include <Sodaq_DS3231.h>
//#include <GPRSbee.h>
#include <Sodaq_PcInt.h>

//The sleep length in seconds (MAX 86399)
#define SLEEP_PERIOD 60

//RTC Interrupt RTC pin 
#define RTC_PIN A7

//Data header
#define DATA_HEADER "Date Time, TempSHT21, PressureBMP, HumiditySHT21, BatVoltage, Wind direction, Wind speed, Rain Tipping"

#define SSID "MarconiLab"
#define PASSWORD   "marconi-lab"
//#define SSID "SciFabLab"
//#define PASSWORD   "makerbot"
#define HOST_NAME   "api.thingspeak.com"
#define HOST_PORT   (80)
//Add your apikey (THINGSPEAK_KEY) from your channel
// GET /update?key=[THINGSPEAK_KEY]&field1=[data 1]&field2=[data 2]...;
String GET = "GET /update?key=6Q9MXE25QLB425SG";

//These constants are used for reading the battery voltage
#define ADC_AREF 3.3
#define BATVOLTPIN A6
#define BATVOLT_R1 4.7
#define BATVOLT_R2 10

//TPH BMP sensor
Sodaq_BMP085 bmp;

ESP8266 wifi(Serial1);

//Sodaq_DS3231 RTC; //Create RTC object for DS3231 RTC come Temp Sensor 
char weekDay[][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
//year, month, date, hour, min, sec and week-day(starts from 0 and goes to 6)
//writing any non-existent time-data may interfere with normal operation of the RTC.
//Take care of week-day also.*/
DateTime dt(2016, 07, 21, 11, 54, 00,4 );


//wind and rain sensors
volatile int rpmcount = 0;//see http://arduino.cc/en/Reference/Volatile
volatile int rpmtipcount=0;
int rpm = 0;

#define DirPin  A3  //Wind direction
//#define SPEEDPIN 8 //not in use right now
#define TIPPPIN 5  //Rain tipping soft int
                   //Wind speed in pin 10 as physical interruption 2

//RG11 variables
float Lluvia = -666;
int time = 500;
 
//variables used for interruptions
volatile boolean rtcFlag = true;
//volatile uint8_t * pcmsk = digitalPinToPCMSK(SPEEDPIN);
volatile uint8_t * pcmsk2 = digitalPinToPCMSK(TIPPPIN);

void setup() 
{
  //Initialise the serial connection
  Serial.begin(9600);
  Serial.println("Starting Weather Station by RJC");
    
  //Initialise sensors
  setupSensors();
    
  //Setup Wifi connection
  setupWifi();
  
  //Setup sleep mode
  setupSleep();
  
  //Echo the data header to the serial connection
  Serial.println(DATA_HEADER);
  delay(1000);
}

void loop() 
{
   if (rtcFlag) {
    //noInterrupts(); 
    //cli();
    
    detachInterrupt(2);
    //See mail Gabriel 19/10/15 about interruptions
    detachInterruptMbili();//detach interruption from RPM and Tipp
    
    //Take readings
    takeReading(getNow());
    
    RTCsleep();
           
    //interrupts();
    //sei();
    attachInterrupt(2,rpm_fan,RISING);
    attachInterruptMbili();
    delay(100);
    rtcFlag = false;
  }
  //Sleep
  systemSleep();  
}

void setupSensors()
{
  //Initialise the wire protocol for the TPH sensors
  Wire.begin();
  
  //Initialise the TPH BMP sensor
  bmp.begin();

  //Initialise the DS3231 RTC
  rtc.begin();
  //rtc.setDateTime(dt); //Adjust date-time as defined 'dt' above

  //Power sensor column
  pinMode(22, OUTPUT);
  digitalWrite(22, LOW);
  
  //WindSpeed Analog Input
  //pinMode(DirPin, INPUT);
  
  //Interruption wind and rain
  pinMode(10, INPUT_PULLUP);
  attachInterrupt(2,rpm_fan,RISING);
  pinMode(TIPPPIN, INPUT_PULLUP);
  PcInt::attachInterrupt(TIPPPIN,rpm_tip);
  //attachInterruptMbili();
}

void setupWifi(){      
    pinMode(23, OUTPUT);//D23 power WIFI on GPRSBee power
    digitalWrite(23, HIGH);
    
    Serial.print("Wee WiFi FW Version:");
    Serial.print(wifi.getVersion().c_str());
      
    //only for Station setOprToStation for Station +AP setOprToStationSoftAP
     if (wifi.setOprToStation()) { 
        Serial.print(", set as station ok, ");
    } else {
        Serial.print(", set as station err, ");
    }
 
    if (wifi.disableMUX()) {
        Serial.print("single ok!\r\n");
    } else {
        Serial.print("single err!\r\n");
    }
    digitalWrite(23, LOW);//off wifi
 }

 void setupSleep()
{
  pinMode(RTC_PIN, INPUT_PULLUP);
  PcInt::attachInterrupt(RTC_PIN, wakeISR);
  
  //Set the sleep mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

void detachInterruptMbili()
{
  //*pcmsk &= ~_BV(digitalPinToPCMSKbit(SPEEDPIN));
  *pcmsk2 &= ~_BV(digitalPinToPCMSKbit(TIPPPIN));
}

void attachInterruptMbili()
{
  //*pcmsk |= _BV(digitalPinToPCMSKbit(SPEEDPIN));
  *pcmsk2 |= _BV(digitalPinToPCMSKbit(TIPPPIN));
}

void RTCsleep()
{
  Serial.print("Sleeping in low-power mode for ");
  Serial.print(SLEEP_PERIOD/60.0);
  Serial.println(" minutes");
  //This method handles any sensor specific sleep setup
  sensorsSleep();
  
  //Wait until the serial ports have finished transmitting
  Serial.flush();
  Serial1.flush();
  
  //Schedule the next wake up pulse timeStamp + SLEEP_PERIOD
  DateTime wakeTime(getNow() + SLEEP_PERIOD);
  rtc.enableInterrupts(wakeTime.hour(), wakeTime.minute(), wakeTime.second());
  
  //The next timed interrupt will not be sent until this is cleared
  rtc.clearINTStatus();
}

void systemSleep()
{
  sleep_enable();
  //Disable pheriferiques
  disableper();
  //interrupts();
  sleep_cpu();
  
  sleep_disable();
  //Enbale pheriferiques
  enableper();

  //This method handles any sensor specific wake setup
  sensorsWake();
}

void disableper(){
  ADCSRA &= ~(1<<ADEN);            //Disable ADC
  DIDR0 = 0x3F;                    //Disable digital input buffers on all ADC0-ADC5 pins
  DIDR1 = (1<<AIN1D)|(1<<AIN0D);   //Disable digital input buffer on AIN1/0
  ACSR = (1<<ACD);                 //Disable the analog comparator
  power_twi_disable();
  power_spi_disable();
  power_usart0_disable();
  power_timer0_disable();     //Needed for delay_ms
  power_timer1_disable();
  }
  
void enableper(){
  ADCSRA |= _BV(ADEN);
  DIDR0 = 0x00; 
  DIDR1 = 0x00;
  ACSR = 0x00;                 
  power_twi_enable();
  power_spi_enable();
  power_usart0_enable();
  power_timer0_enable();     //Needed for delay_ms
  power_timer1_enable();
  }
  
void sensorsSleep()
{
  //Add any code which your sensors require before sleep
}

void sensorsWake()
{
  //Add any code which your sensors require after waking
}

void takeReading(uint32_t ts)
{
  digitalWrite(22, HIGH);//D22 power on Column
  //delay(100); 
  
  //Read sensor values
  String Temp1=String(SHT2x.GetTemperature());
  String Temp2=String(bmp.readTemperature());
  String Press=String(bmp.readPressure() / 100);
  String Hum=String(SHT2x.GetHumidity());
  
  //Read the voltage
  int mv = getRealBatteryVoltage() * 1000.0;
  String Smv=String(mv);

  //Convert RPM
  rpm = rpmcount /2;  /* Con  , note: this works for one interruption per full rotation. For two interrups per full rotation use rpmcount * 30.*/
  String rpmS= String(rpm);
  
  //wind direction
  int dir=analogRead(DirPin);
  String dirS=String(dir);

  //Rain tipping bucket
  String rpmtipcountS=String(rpmtipcount);
  
  //Lluvia = RG.Medir_Lluvia(); //no anda
  Lluvia = 10;
  String LluviaS=String(Lluvia);

  //digitalWrite(22, LOW);//D22 power off Column
  
  Serial.print(getDateTime());Serial.print(",");
  Serial.print(Temp1);Serial.print(",");
  Serial.print(Temp2);Serial.print(",");
  Serial.print(Press);Serial.print(",");
  Serial.print(Hum);Serial.print(",");
  Serial.print(Smv);Serial.print(",");
  Serial.print(dir);Serial.print(",");
  Serial.print(rpm);Serial.print(",");
  Serial.println(rpmtipcount/2);//Serial.print(",");
  //Serial.println(Lluvia);

  //wifi on
  digitalWrite(23, HIGH);
  delay(100);
  
  //Create TCP connection
  createTCP();
  
  //post to TS
  updateTS( Temp1, Press, Hum, Smv, rpmS, dirS, rpmtipcountS, String(Lluvia) );
  
  //Wifi off
  digitalWrite(23, LOW);
  rpmtipcount=0;
  rpmcount = 0; // Restart the RPM counter 
}


void createTCP(){
    delay(1000);
    if (wifi.joinAP(SSID, PASSWORD)) {
        Serial.print("Join AP success\r\n");
        Serial.print("IP:");
        Serial.println( wifi.getLocalIP().c_str());       
    } else {
        Serial.print("Join AP failure\r\n");
    }
    
    uint8_t buffer[128] = {0};
    digitalWrite(23, HIGH);
    if (wifi.createTCP(HOST_NAME, HOST_PORT)) {
        Serial.print("create tcp ok\r\n");
    } else {
        Serial.print("create tcp err\r\n");
    }
}

//----- update the  Thingspeak string with 3 values
void updateTS( String T1 , String P, String H, String V, String RPM, String DIR, String TIP, String RAIN)
{
  //create GET message
  String cmd = GET + "&field1=" + T1 +"&field2="+ P +"&field3=" + H +"&field4=" + V+"&field5=" + RPM+"&field6=" + DIR+"&field7=" + TIP+"&field8=" + RAIN+"\r\n";
   
   char buf[150]; //make buffer large enough for 7 digits
   cmd.toCharArray(buf, sizeof(buf));
    
    wifi.send((const uint8_t*)buf, strlen(buf));
    
    uint8_t buffer[128] = {0};
    uint32_t len = wifi.recv(buffer, sizeof(buffer), 10000);
    if (len > 0) {
        Serial.print("Received:[");
        for(uint32_t i = 0; i < len; i++) {
            Serial.print((char)buffer[i]);
        }
        Serial.print("]\r\n");
    }
    
    if (wifi.releaseTCP()) {
        Serial.print("release tcp ok\r\n");
    } else {
        Serial.print("release tcp err\r\n");
    }
    
}

String getDateTime()
{
  String dateTimeStr;
  
  //Create a DateTime object from the current time
  DateTime dt(rtc.makeDateTime(rtc.now().getEpoch()));

  //Convert it to a String
  dt.addToString(dateTimeStr);
  
  return dateTimeStr;  
}

uint32_t getNow()
{
  return rtc.now().getEpoch();
}

float getRealBatteryVoltage()
{
  uint16_t batteryVoltage = analogRead(BATVOLTPIN);
  return (ADC_AREF / 1023.0) * (BATVOLT_R1 + BATVOLT_R2) / BATVOLT_R2 * batteryVoltage;
}

// Interruptions RTC, WindSpeed and Tipping Bucket
void wakeISR()
{
  rtcFlag = true;
  //Serial.println("it");
}

void rpm_fan(){ /* this code will be executed every time the interrupt 0 (pin2) gets low.*/
  rpmcount++;
  //Serial.println("i1");
  //rtcFlag = false;
}

void rpm_tip(){ /* this code will be executed every time the interrupt 0 (pin2) gets low.*/
  rpmtipcount++;
  //Serial.println("i2");
  //rtcFlag =false;
}

