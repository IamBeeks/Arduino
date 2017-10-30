// include the library code:
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <Time.h>  
#include <TimeLib.h>
#include <OneWire.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

//Define Structures

struct DisplayObject {
  char line2_value[16];
  char line1_description[16];
};

//Defined Constants
OneWire  ds(8);
#define led1Pin 9
#define led2Pin 10

#define waterLevelPin A0
#define waterTempPin A1
#define roomTempPin A2
#define operatingModePin A3
#define phPin A4
#define randomPin A5

#define numSamples 5

#define EEsize 1024

#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by Unix time_t as ten ASCII digits
#define TIME_HEADER  'T'   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message

//initialize Variables

bool timeWriteDone = false;

byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];

static char buffer[80];

float waterTemperatureCalculated = 0;
float roomTemperatureCalculated = 0;
float led1DutyCycleCalc = 0;
float led2DutyCycleCalc = 0;
float shimmerValue = 0;
float CurrentMinute = 50;
float celsius, fahrenheit;

int eeAddress = 0;
int displayLoop = 0;
int displayEEPROMaddr = 0;
int led1DutyCycle = 0;
int led2DutyCycle = 0;
int hours = 0;
int minutes = 0;
int CurrentMinuteInt = 0;
int seconds = 0;
int days = 0;
int months = 0;
int years = 0;
int modeVoltage = 0;
int currentMode = 1;
int CurrentHour = 15;
int inChar = 0;
int OneWireDwell = 1000;

long shimmerRandom = 0;

String hrMinSec;
String dayMonthYear;
String tmpStr;
String inString = ""; 

unsigned long Now = 0;
unsigned long HourLength = 3600000;
unsigned long PreviousTime = 0; 
unsigned long HourDurationCounter = 0;
unsigned long HourDurationPreviousTime = 0;
unsigned long pctime = 0;
unsigned long OneWireTimeStamp = 0;

    int LedValues[24][2] = { 
                        {0,0},     //hour 0
                        {0,0},     //hour 1
                        {0,0},     //hour 2
                        {0,0},     //hour 3
                        {0,0},     //hour 4 
                        {16,0},     //hour 5 
                        {32,0},     //hour 6 
                        {32,15},    //hour 7 
                        {32,15},     //hour 8 
                        {32,15},     //hour 9 
                        {32,15},     //hour 10 
                        {32,15},     //hour 11 
                        {32,15},     //hour 12 
                        {32,15},     //hour 13 
                        {32,15},     //hour 14 
                        {32,15},     //hour 15 
                        {32,125},     //hour 16 
                        {64,150},     //hour 17 
                        {128,255},     //hour 18 
                        {128,255},     //hour 19 
                        {64,150},     //hour 20 
                        {32,0},     //hour 21
                        {16,0},     //hour 22 
                        {16,0},     //hour 23 
                        };

void setup() {
  // put your setup code here, to run once:

//  EEPROM.get(0,CurrentHour);
//  EEPROM.get(2,CurrentMinute);
  EEPROM.get(4,seconds);
  EEPROM.get(6,days);
  EEPROM.get(8,months);
  EEPROM.get(10,years);

 setTime(hours,minutes,seconds,days, months, years);


//Clear the EEPROM
for (int i = 16 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
}
  //Serial Port Configuration
  Serial.begin(9600);

  //Analog reference set to 3.3V
  analogReference(EXTERNAL);

  //Arduino Pin Configuration
  pinMode(13, OUTPUT);

  // LCD Configuration - set up the LCD's number of columns and rows:
  lcd.begin(16, 2);


  // Startup Screen
  lcd.print("v2017.10.29");
  lcd.setCursor(0, 1);
  lcd.print("EEPROM ");
  lcd.print(EEPROM.length());
  delay (5000);
  lcd.clear();

  HourDurationCounter = HourLength * CurrentMinute/60;

Serial.println ("Enter current hour");
while (readline(Serial.read(), buffer, 80) <= 0) {} 
Serial.println(buffer);
CurrentHour = atoi(buffer);
Serial.println(CurrentHour);

Serial.println ("Enter current minute");
while (readline(Serial.read(), buffer, 80) <= 0) {} 
Serial.println(buffer);
CurrentMinute = atoi(buffer);
Serial.println(CurrentMinute);

HourDurationCounter = CurrentMinute*60*1000;
}

void loop() {
  // put your main code here, to run repeatedly:

//Define the current time and write to EEPROM every minute


Now = millis();
HourDurationCounter = Now-HourDurationPreviousTime + HourDurationCounter;
HourDurationPreviousTime = Now;

  if(HourDurationCounter > HourLength)
  {
    ++CurrentHour;
    if (CurrentHour == 24)
    {
      CurrentHour=0;
    }
    HourDurationCounter = HourDurationCounter-HourLength;
  }

CurrentMinute = (float(HourDurationCounter)/float(HourLength))*60.00000;
CurrentMinuteInt = int(CurrentMinute);

  if(second()== 0 && timeWriteDone == false) 
  {
    writeTimeToEEPROM();
    timeWriteDone = true;
  }

  if(second()>0 && timeWriteDone == true) 
  {
    timeWriteDone = false;
  }
 
//Create a string of the current time for display.
digitalClockDisplay();
dayMonthYear = String(months);
dayMonthYear += "/";
dayMonthYear += String(days);
dayMonthYear += "/";
dayMonthYear += String(years);

hrMinSec = String(hours);
hrMinSec += ":";
hrMinSec += String(minutes);
hrMinSec += ":";
hrMinSec += String(seconds);

DisplayObject currentDate; 
hrMinSec.toCharArray(currentDate.line2_value,16);
dayMonthYear.toCharArray(currentDate.line1_description,16);

EEPROM.put(112, currentDate);

 if(Serial.available() ) 
  {
    processSyncMessage();
  }
 
  if(timeStatus() == timeNotSet) 
  {
    Serial.println("waiting for sync message");
  }


  
/*Mode Selection
                                                    Display     Lights        Pump
Modes     
0-200     Mode 1 - Daylight No Scroll               No Scroll   Day Cycle     On
201-400   Mode 2 - Daylight Scroll                  Scroll      Day Cycle     On
401-600   Mode 3 - Daylight Quiet / No Scroll       No Scroll   Day Cycle     Off
601-800   Mode 4 - All On                           Scroll      Full On       On
801-1024  Mode 5 - System Off                       Off         Off           Off

*/

  modeVoltage = analogRead(operatingModePin);
  DisplayObject modeDisplay;
  tmpStr = "Current Mode";
   tmpStr.toCharArray(modeDisplay.line1_description,16);
  if ((modeVoltage > 0) && (modeVoltage < 200)) 
  {
    currentMode = 1;
      dtostrf(currentMode,2,0,modeDisplay.line2_value);
  }
  
   else if ((modeVoltage >= 200) && (modeVoltage <400)) 
   {
      currentMode = 2;
      dtostrf(currentMode,2,0,modeDisplay.line2_value);
   }
   else if ((modeVoltage >=400) && (modeVoltage <600))
   {
       currentMode = 3;
      dtostrf(currentMode,2,0,modeDisplay.line2_value);
   }
   else if ((modeVoltage >=600) && (modeVoltage <800)) 
   {
         currentMode = 4;
      dtostrf(currentMode,2,0,modeDisplay.line2_value);
   }
   else if ((modeVoltage >= 800) && (modeVoltage <= 1024)) 
   {
          currentMode = 5;
       dtostrf(currentMode,2,0,modeDisplay.line2_value);
   }
 EEPROM.put (80,modeDisplay);
  
  /* waterTemperatureCalculated = TempEvaluation(waterTempPin);
    
  */

  roomTemperatureCalculated = TMP36Evaluation(roomTempPin);
 
  DisplayObject rmTmp;
  dtostrf(roomTemperatureCalculated,2,2,rmTmp.line2_value);
  tmpStr = "Room Temp.";
  tmpStr.toCharArray(rmTmp.line1_description,16);

  EEPROM.put(16, rmTmp);

  DisplayObject tnkTmp;
  dtostrf(waterTemperatureCalculated,2,2,tnkTmp.line2_value);
  tmpStr= "Water Temp.";
  tmpStr.toCharArray(tnkTmp.line1_description,16);

  EEPROM.put(48, tnkTmp);

  DisplayObject Debug0;
  dtostrf(0,0,0,Debug0.line2_value);
  tmpStr= "Debug0 Val";
  tmpStr.toCharArray(Debug0.line1_description,16);
  EEPROM.put(208, Debug0);

  DisplayObject Debug1;
  dtostrf(0,0,0,Debug1.line2_value);
  tmpStr= "Debug1 Val";
  tmpStr.toCharArray(Debug1.line1_description,16);
  EEPROM.put(240, Debug1);

  DisplayObject Debug2;
  dtostrf(0,0,0,Debug2.line2_value);
  tmpStr= "Debug2 Val";
  tmpStr.toCharArray(Debug2.line1_description,16);
  EEPROM.put(272, Debug2);

  //LCD Display Logic
  DisplayObject Display;
   
  if ((displayLoop == 0) || (displayLoop >= 19)) displayLoop = 1;
    
  

    if (currentMode == 1 || currentMode == 3)
      {
      displayLoop = 7;
      if (Now > (PreviousTime + 999)) 
        {
        PreviousTime = Now;
        displayEEPROMaddr = displayLoop * 16;
        EEPROM.get (displayEEPROMaddr, Display);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(Display.line1_description);
        lcd.setCursor(0, 1);
        lcd.print(Display.line2_value);
      }
     }
     
    if (currentMode == 2 || currentMode == 4)
      {
       if (Now > (PreviousTime + 3000)) 
        {
        PreviousTime = Now;
        displayEEPROMaddr = displayLoop * 16;
        EEPROM.get (displayEEPROMaddr, Display);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(Display.line1_description);
        lcd.setCursor(0, 1);
        lcd.print(Display.line2_value);
        displayLoop = displayLoop + 2;
      }
    }

// LED Light Control
if (currentMode == 1 || currentMode == 2 || currentMode ==3) {       

if (hours < 21) {
    led1DutyCycle = LedValues[hours][0] + (minutes/60.00)*(LedValues[hours+1][0]-(LedValues[hours][0]));
    led2DutyCycle = LedValues[hours][1] + (minutes/60.00)*(LedValues[hours+1][1]-(LedValues[hours][1]));
    }
    else {

    led1DutyCycle = LedValues[hours][0];
    led2DutyCycle = LedValues[hours][1];
    
    }
    
}              

else if (currentMode == 4) {
    led1DutyCycle = 255;
    led2DutyCycle = 255;
}

else if (currentMode == 5) {
    led1DutyCycle = 0;
    led2DutyCycle = 0;
}
  
  analogWrite(led1Pin,led1DutyCycle);  // 0 = always off.  255 = full on.  490Hz.  this is the light brightness
  analogWrite(led2Pin,led2DutyCycle); // 0 = blue.  255 = white.  490Hz.  this is the light frequency 



  DisplayObject ledBrightness;
   dtostrf(led1DutyCycle,2,0,ledBrightness.line2_value);
  tmpStr= "Brightness #";
  tmpStr.toCharArray(ledBrightness.line1_description,16);
  
   EEPROM.put(144, ledBrightness);
  
  DisplayObject ledColor;
  dtostrf(led2DutyCycle,2,0,ledColor.line2_value);
  tmpStr= "Color #";
  tmpStr.toCharArray(ledColor.line1_description,16);
  
  EEPROM.put(176, ledColor);
  
}

//Air Temperature Sensor Read
float TMP36Evaluation (int pinToRead) {
  int TempRaw;
  int i = 0;
  float value = 0;
  float average = 0;
  float result;

  while (i<10) {
  TempRaw = analogRead(pinToRead);

 
  value = TempRaw * (3.3/1023.00);
  value = (value - 0.5) * 100.00;
  value = ((9.00/5.00) * value) + 32.00;
  average = average + value;
  i++;
  }
 
  result = average / 10.00;
  return result; //result is in degrees F

//Water Temperature Sense using 1wire system  
  if ( !ds.search(addr)) //Start a new search for a connected device if an address is not present 
   {        
    //Serial.println("No more addresses.");
    //Serial.println();
    ds.reset_search();
    //delay(250);
    return;
   }
  
  if (ds.search(addr)) 
   {
    Serial.print("ROM =");
      for( i = 0; i < 8; i++) {
        Serial.write(' ');
        Serial.print(addr[i], HEX);
   }
   
  if (OneWire::crc8(addr, 7) != addr[7]) 
   {
      Serial.println("CRC is not valid!");
      return; 
   }
  Serial.println();
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return;
  }
  
  //communication with the selected address over the onewire system
  // begin the communication by resetting the wire
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  OneWireTimeStamp = Now;

  if (Now > (OneWireTimeStamp + OneWireDwell))
  {
     // we might do a ds.depower() here, but the reset will take care of it.
  
    present = ds.reset();
    ds.select(addr);    
    ds.write(0xBE);         // Read Scratchpad
  
    Serial.print("  Data = ");
    Serial.print(present, HEX);
    Serial.print(" ");
   for ( i = 0; i < 9; i++) {           // we need 9 bytes
      data[i] = ds.read();
      Serial.print(data[i], HEX);
      Serial.print(" ");
    }
    
    Serial.print(" CRC=");
    Serial.print(OneWire::crc8(data, 8), HEX);
    Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  
  int16_t raw = (data[1] << 8) | data[0];
  
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");
  Serial.print(fahrenheit);
  Serial.println(" Fahrenheit");
  }
  
  
  }

//end of loop
}



void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}


void processSyncMessage() {
  // if time sync available from serial port, update time and return true
  while(Serial.available() >=  TIME_MSG_LEN ){  // time message consists of header & 10 ASCII digits
    char c = Serial.read() ; 
    Serial.print(c);  
    if( c == TIME_HEADER ) {       
      time_t pctime = 0;
      for(int i=0; i < TIME_MSG_LEN -1; i++){   
        c = Serial.read();          
        if( c >= '0' && c <= '9'){   
          pctime = (10 * pctime) + (c - '0') ; // convert digits to a number    
        }
      }   
      setTime(pctime);   // Sync Arduino clock to the time received on the serial port
      adjustTime(-18000); // sets time to EST
      writeTimeToEEPROM();
    }  
  }
}

void digitalClockDisplay(){
  // digital clock display of the time
  
 hours = CurrentHour; // hours = hour();
 minutes = CurrentMinuteInt; // minutes = minute();
  seconds = second();

   days = day();
   months = month();
   years = year(); 


}

void writeTimeToEEPROM() {
      EEPROM.put(0,CurrentHour);
      EEPROM.put(2,CurrentMinute);
      EEPROM.put (4,seconds);
      EEPROM.put (6,days);
      EEPROM.put (8,months);
      EEPROM.put (10,years);

Serial.println(CurrentHour && " " && CurrentMinute);

}

int readline(int readch, char *buffer, int len)
{
  static int pos = 0;
  int rpos;

  if (readch > 0) {
    switch (readch) {
      case '\n': // Ignore new-lines
        break;
      case '\r': // Return on CR
        rpos = pos;
        pos = 0;  // Reset position index ready for next time
        return rpos;
      default:
        if (pos < len-1) {
          buffer[pos++] = readch;
          buffer[pos] = 0;
        }
    }
  }
  // No end of line has been found, so return -1.
  return -1;
}
