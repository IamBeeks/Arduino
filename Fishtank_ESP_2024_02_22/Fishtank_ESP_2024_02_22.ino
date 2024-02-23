
//Included Libraries:
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <ESP8266WiFi.h>  // Include the Wi-Fi library
#include <ThreeWire.h>  // Include the 3 wire interface
#include <RtcDS1302.h>  // Include the real time clock libary
#include <SPI.h>
#include <OneWire.h>            // this library controls the one wire
#include <DallasTemperature.h>  // this is the library for reading temperature sensor on one-wire network
#include <LiquidCrystal_I2C.h>
#include <PCF8574.h>  // this is the library for accessing the remote I/O modules
#include <ArduinoOTA.h>

int i = 1;

String Version = "2024.02.18.1";

bool Enable_DataCollection = false;
bool Enable_Wifi = false;
bool Enable_OneWire = true;
bool Enable_ThreeWire = true;
bool Enable_CAN = true;


// CONNECTIONS for the DS1302 real time clock using 3-wire interface :
// DS1302 CLK/SCLK --> 5
// DS1302 DAT/IO --> 4
// DS1302 RST/CE --> 2
// DS1302 VCC --> 3.3v - 5v
// DS1302 GND --> GND


//Description: 

union MY_DATA {
  uint8_t allBits;
  struct {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t bit5 : 1;
    uint8_t bit6 : 1;
    uint8_t bit7 : 1;
  };
};

union MY_DATA myData;



// ESP8266 Node MCU 12-E Pin Setup***********************************************************************************

#define PIN_WIRE_SDA (4) // This is D2 on the board
#define PIN_WIRE_SCL (5) // This is D1 on the board


//I2C bus system setup***********************************************************************************************
//Specify the node numbers for the network I/O boards
PCF8574 n20(0x20);  // HMI inputs
PCF8574 n21(0x21);  // HMI outputs
PCF8574 n22(0x22);
PCF8574 n23(0x23);
PCF8574 n24(0x24);
PCF8574 n25(0x25);  //Box1 control
PCF8574 n26(0x26);
PCF8574 n27(0x27);

// LCD display address is 0x3c

// Information for network and server interaction ********************************************************************
  
int HTTP_PORT = 80;
String HTTP_METHOD = "GET";
char HOST_NAME[] = "10.216.1.38";  //this is the IP address of the Linux web server for data collection
String PATH_NAME = "/insert_temp.php";
String queryString = "?temperature=29.1";

const char* ssid = "beekerhouse"; // SSID of the server's network
const char* password = "%mw4shx5bm9w"; // Password for the server's network
const char* serverIP = "10.216.1.233"; // IP address of the server (ESP8266 in AP mode)
const int serverPort = 12345; // Port on which the server listens


// Initialize the client library for use when the Nano is acting as a server for a webpage that users can access.*****


// ThreeWire bus system configuration ********************************************************************************
//  the ThreeWire bus system is used for the realtime clock

ThreeWire myWire(12, 14, 13);  // IO, SCLK, CE     12-IO  13-CE  14-SCLK
RtcDS1302<ThreeWire> Rtc(myWire);
RtcDateTime compiled;


// OneWire bus system **************************************************************************************************
// The OneWire bus system is used for the analog inputs into the processor.  Temperature, salinity, etc

#define ONE_WIRE_BUS 2  // This is D4 on the board 

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature onewireNET(&oneWire);

int deviceCount = 0;
DeviceAddress sensorAddress;

uint8_t sensor1[8] = { 0x28, 0x0A, 0xB0, 0x95, 0xF0, 0x01, 0x3C, 0xAA };  //end of network ds18b20 ambient sensor
uint8_t sensor2[8] = { 0x28, 0xC5, 0x2F, 0x20, 0x0B, 0x00, 0x00, 0x4C };  //node 1 ds18b20 waterproof sensor
uint8_t sensor3[8] = { 0x28, 0x37, 0x7A, 0x20, 0x0B, 0x00, 0x00, 0x17 };  //node 2 ds18b20 waterproof sensor

float tempSensor1, tempSensor2, tempSensor3;
int sensor1Addr, sensor2Addr, sensor3Addr;

// Information for fishtank operation *********************************************************************************

int internalLEDValue = LOW, refugiumLight = LOW;
int PumpON = 0;
int PumpStop = 23;
int PumpReset = 25;

// Data collection ******************************************************************************************************

int hour = 0;
int minute = 0;
int uptime_total = 0;  // total number of minutes since last restart
int uptime_days = 0;   //number of days since last restart
int uptime_hours = 0; 
int uptime_minutes = 0;
// temperature sensor initialization ************************************************************************************

const int analogInPin = 0;  //ESP8266 Analog Pin ADC0 = A0
int sensorValue = 0;        // initialize value read from the temperature sensor

//lcd display *************************************************************************************************************
// 128 characters per row
// (x,y) (0,0) is top left
// y 0 - 15 is yellow
// y 16-63 is blue

int lcdPageSelect = 1;
int displayUpdateRequest = 0;

//Set up the structure for the lcd display pages and initialize them
struct  page {
  int textSize = 1;
  char textColor[6] = "WHITE";
  char Line1[50] = "                                           ";
  char Line2[50] = "                                           ";
  char Line3[50] = "                                           ";
  char Line4[50] = "                                           ";
  char Line5[50] = "                                           ";
  char Line6[50] = "                                           ";
 };

//Set up the x and y coordinates for the lines of text on the LCD Screen.
int lcdX[6] = {1,1,1,1,1,1};
int lcdY[6] = {5,16,26,36,46,56};

// Line 1 (1,5): Yellow Text
// Line 2 (1,16): Blue Text
// Line 3 (1,26): Blue Text 
// Line 4 (1,36): Blue Text 
// Line 5 (1,46): Blue Text
// Line 6 (1,56): Blue Text 

//COMMAND REFERENCE FOR THE LCD SCREEN
// display.clearDisplay() – all pixels are off
// display.drawPixel(x,y, color) – plot a pixel in the x,y coordinates
// display.setTextSize(n) – set the font size, supports sizes from 1 to 8
// display.setCursor(x,y) – set the coordinates to start writing text
// display.print(“message”) – print the characters at location x,y
// display.display() – call this method for the changes to make effect

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

#define OLED_RESET -1        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C  ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//#if (SSD1306_LCDHEIGHT != 64)
//#error("Height incorrect, please fix Adafruit_SSD1306.h");
//#endif

//Initialize the structured page variables for each screen and the write locations.
  struct page Page1;      //Main Screen 
  struct page Page2; 
  struct page Page3; 
  struct page Page4; 
  struct page Page5;
  struct page lcdWrite;
  struct page lcdLastWrite;

//End of Configuration and Initialization code ****************************************************************************
//


void setup() {

  // Display the start up screen while the setup is running.

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();
  display.display();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 30);
  display.print("BeekerTank");
  display.setCursor(10, 40);
  display.print(Version);
  display.display();
  delay(4000);

  // Start up the serial port *******************************************************************************************

  pinMode(LED_BUILTIN, OUTPUT);  // Initialize the LED_BUILTIN pin as an output

  Wire.begin();

  Serial.begin(57600);

  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);

  display.clearDisplay();
  display.setCursor(10, 16);
  display.print("Serial Port Complete");
  display.display();

  // Start up the Real Time Clock and update the time. ********************************************************************

  Rtc.Begin();

  compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid()) {
    // Common Causes:
    //    1) first time you ran and the device wasn't running yet
    //    2) the battery on the device is low or even missing

    Serial.println("RTC lost confidence in the DateTime!");
    Rtc.SetDateTime(compiled);
  }

  if (Rtc.GetIsWriteProtected()) {
    Serial.println("RTC was write protected, enabling writing now");
    Rtc.SetIsWriteProtected(false);
  }

  if (!Rtc.GetIsRunning()) {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled) {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  } else if (now > compiled) {
    Serial.println("RTC is newer than compile time. (this is expected)");
  } else if (now == compiled) {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }

  delay(10);
  Serial.println('\n');

  
  display.setCursor(10, 26);
  display.print("Real Time Clock Complete");
  display.display();


  //Wifi access point connection ********************************************************************************************

 WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to Wi-Fi...");
    }

    Serial.println("Connected to Wi-Fi!");

  // Start up the OneWire network **********************************************************************************************
  Serial.println("");

  onewireNET.begin();

  Serial.println("OneWire Network Startup Complete");
  Serial.println("");

  delay(5000);


  // I2C Network Layout & Startup **********************************************************************************************

  // Node 20 address 0x20 - Panel Inputs  ***************************************************
  n20.pinMode(P0, INPUT);  // Switch - Master Enable - yellow
  n20.pinMode(P1, INPUT);  // Switch - Box 1 Master Enable - green
  n20.pinMode(P2, INPUT);  // Switch - Drain Pump - orange
  n20.pinMode(P3, INPUT);  // Switch - Protein Skimmer Enable - blue
  n20.pinMode(P4, INPUT);  // Switch - Display Tank Lights Enable
  n20.pinMode(P5, INPUT);  // Switch - Heat Enable - red
  n20.pinMode(P6, INPUT);  // Switch - Return Pump Enable 
  n20.pinMode(P7, INPUT);  // Switch - Powerhead Enable

  // Node 21 address 0x21 - Panel Outputs **************************************************
  n21.pinMode(P0, OUTPUT);  // Buzzer - dual wired to Output and also to RTC battery through transistor controlled by Node MCU power on.
  n21.pinMode(P1, OUTPUT);  //
  n21.pinMode(P2, OUTPUT);  //
  n21.pinMode(P3, OUTPUT);  //
  n21.pinMode(P4, OUTPUT);  //
  n21.pinMode(P5, OUTPUT);  //
  n21.pinMode(P6, OUTPUT);  //
  n21.pinMode(P7, OUTPUT);  //

  // Node 22 address 0x22 - Panel Inputs *****************************************************
  n22.pinMode(P0, INPUT);  // 
  n22.pinMode(P1, INPUT);  //
  n22.pinMode(P2, INPUT);  //
  n22.pinMode(P3, INPUT);  //
  n22.pinMode(P4, INPUT);  //
  n22.pinMode(P5, INPUT);  //
  n22.pinMode(P6, INPUT);  //
  n22.pinMode(P7, INPUT);  //

  // Node 23 address 0x23 - Field Outputs ***************************************************
  n23.pinMode(P0, OUTPUT);  // Water shut off solenoid valve
  n23.pinMode(P1, OUTPUT);  //
  n23.pinMode(P2, OUTPUT);  //
  n23.pinMode(P3, OUTPUT);  //
  n23.pinMode(P4, OUTPUT);  //
  n23.pinMode(P5, OUTPUT);  //
  n23.pinMode(P6, OUTPUT);  //
  n23.pinMode(P7, OUTPUT);  //

  // Node 24 address 0x24 - Field Inputs ***************************************************
  n24.pinMode(P0, INPUT);  //
  n24.pinMode(P1, INPUT);  //
  n24.pinMode(P2, INPUT);  //
  n24.pinMode(P3, INPUT);  //
  n24.pinMode(P4, INPUT);  //
  n24.pinMode(P5, INPUT);  //
  n24.pinMode(P6, INPUT);  //
  n24.pinMode(P7, INPUT);  //

  // Node 25 address 0x25 - 120VAC Outlet Box 1 - Off during water change *****************
  n25.pinMode(P0, OUTPUT);  // Top left outlet - Drain Pump
  n25.pinMode(P1, OUTPUT);  // Top right outlet - Heater 1
  n25.pinMode(P2, OUTPUT);  // Bottom right outlet - Heater 2
  n25.pinMode(P3, OUTPUT);  // Bottom left outlet - Protein skimmer
  n25.pinMode(P4, INPUT);   // Top left activated
  n25.pinMode(P5, INPUT);   // Top right activated
  n25.pinMode(P6, INPUT);   // Bottom right activated
  n25.pinMode(P7, INPUT);   // Bottom left activated

  // Node 26 address 0x26 - 120VAC Outlet Box 2 - Constant *******************************
  n26.pinMode(P0, INPUT);   // Top left outlet - Light
  n26.pinMode(P1, INPUT);   // Top right outlet - Powerhead
  n26.pinMode(P2, INPUT);   // Bottom right outlet - Open
  n26.pinMode(P3, INPUT);   // Bottom left - Return Pump
  n26.pinMode(P4, OUTPUT);  //
  n26.pinMode(P5, OUTPUT);  //
  n26.pinMode(P6, OUTPUT);  //
  n26.pinMode(P7, OUTPUT);  //
   
   // Node Startup ***********************************************************************
 
  if (n20.begin()) {
    Serial.println("Node 20 OK");
  } else {
    Serial.println("Node 20 KO");
  }
  if (n21.begin()) {
    Serial.println("Node 21 OK");
  } else {
    Serial.println("Node 21 KO");
  }
  if (n22.begin()) {
    Serial.println("Node 22 OK");
  } else {
    Serial.println("Node 22 KO");
  }
  if (n23.begin()) {
    Serial.println("Node 23 OK");
  } else {
    Serial.println("Node 23 KO");
  }
  if (n24.begin()) {
    Serial.println("Node 24 OK");
  } else {
    Serial.println("Node 24 KO");
  }
  if (n25.begin()) {
    Serial.println("Node 25 OK");
  } else {
    Serial.println("Node 25 KO");
  }
  if (n26.begin()) {
    Serial.println("Node 26 OK");
  } else {
    Serial.println("Node 26 KO");
  }
  if (n27.begin()) {
    Serial.println("Node 27 OK");
  } else {
    Serial.println("Node 27 KO");
  }
  
  ArduinoOTA.begin();
 
  // Final startup report out ****************************************************************************************************** 

  Serial.println("Startup Complete...Ready");
  
  Serial.print("Version: ");
  Serial.println(Version);

 display.setCursor(10, 56);
  display.print("Startup Complete");
  display.display();
  delay(5000);
  display.clearDisplay();
  display.display();
}


// Begin the main loop *************************************************************************************************************

void loop() {

    String url = "/temperature"; // Replace with the desired route on the server
    //String request = "Hello from the Client!";
    String request = String(tempSensor1) + ";" + String(tempSensor2) + ";" + String(tempSensor3) + ";" + String(WiFi.RSSI());

WiFiClient client;
    if (client.connect(serverIP, serverPort)) {
        client.print(request);
        while (client.connected()) {
            String response = client.readStringUntil('\n');
            Serial.println("Connected! - Received from server: " + response);
        }
        client.stop();
    } else {
        Serial.println("Failed to connect to server!");
    }

  
 
 // OTA Check *********************************************************************************************************************
ArduinoOTA.handle();

 // Real Time Clock code **********************************************************************************************************
  RtcDateTime now = Rtc.GetDateTime();
  //Serial.println(now.Hour());

  //Serial.println(now.Minute()-compiled.Minute());
  //Serial.println();

  if (!now.IsValid()) {
    // Common Causes:
    //    1) the battery on the device is low or even missing and the power line was disconnected
    Serial.println("RTC lost confidence in the DateTime!");
  }

  // delay(10000); // ten seconds

  //Read all Inputs ***************************************************************************************************************
  PCF8574::DigitalInput N20 = n20.digitalReadAll();
  PCF8574::DigitalInput N22 = n22.digitalReadAll();
  PCF8574::DigitalInput N24 = n24.digitalReadAll();
  PCF8574::DigitalInput N25 = n25.digitalReadAll();
  PCF8574::DigitalInput N26 = n26.digitalReadAll();

  // Temperature sensor read *****************************************************************************************************
  onewireNET.requestTemperatures();
  tempSensor1 = onewireNET.getTempF(sensor1);
  tempSensor2 = onewireNET.getTempF(sensor2);
  tempSensor3 = onewireNET.getTempF(sensor3);
  
    // Evaluate the state of the Master Control Switch ***************************************************************************
  // the pcf8574 I/O is drawn LOW when the switch is on.

  //Master Control Logic
  if (N20.p0 == LOW) {
    //Serial.println("Main Power Input On");

    //BOX 1 Control Logic 
    //Activate (LOW) /Deactivate (HIGH) the Box1 120VAC outlets
    if (N20.p1 == LOW) {
      //Serial.println("Box1 Input on");


      // Top Left Box 1 Outlet On Logic - Drain Pump
      if (N20.p2 == LOW) {
        //Serial.println("Drain Pump On");
        
        if (now.Minute() != minute) {
          PumpON = PumpON + 1;
        }
        
        if (PumpON < PumpStop) {
          n25.digitalWrite(P0, LOW);
          //Serial.println("Top left outlet - Drain Pump ON");
        } 
        else 
        {
          n25.digitalWrite(P0, HIGH);
          //Serial.println("Top left outlet - Drain Pump OFF");
        }

        if (PumpON == PumpReset) { PumpON = 0; }
     } 
     
     else 
     
     {
        n25.digitalWrite(P0, HIGH);
        PumpON = 0;
        //Serial.println("Top left outlet - Drain Pump OFF");
     }
     // Heater 1 Outlet control
     if (N20.p5 == LOW) 
      {
        n25.digitalWrite(P1, LOW);
      } 
      else 
      {
        n25.digitalWrite(P1, HIGH);
      }  //Heater 1 Top Right
      
      // Heater 2 Outlet Control
     if (N20.p5 == LOW) 
      {
        n25.digitalWrite(P2, LOW);
      } 
      else 
      {
        n25.digitalWrite(P2, HIGH);
      }  //Heater 2 Bottom Right
      
      if (N20.p3 == LOW) {
        //Serial.println(now.Hour());
        if (now.Hour() >= 22 || now.Hour() <= 4) {
          n25.digitalWrite(P3, LOW);
        } else {
          n25.digitalWrite(P3, HIGH);
        }
      } else {
        n25.digitalWrite(P3, HIGH);
      }
    } 
    
    else 
    
    {
      //Serial.println("Box1 Input off");
      n25.digitalWrite(P0, HIGH);  //Drain Pump Top Left
      n25.digitalWrite(P1, HIGH);  //Heater 1 Top Right
      n25.digitalWrite(P2, HIGH);  //Heater 2 Bottom Right
      n25.digitalWrite(P3, HIGH);  //Protein Skimmer Bottom Left
    }

    // Box 2 Activate (LOW) / Deactivate (HIGH)  120VAC outlets
    if (N20.p2 == LOW) 
     {
      if (N20.p4 == LOW) {
        n26.digitalWrite(P0, LOW);
      } 
      else 
      {
        n26.digitalWrite(P0, HIGH);
      }  //Display Tank Light Top Left


      if (N20.p7 == LOW)  //Powerhead Top Right
      {
        if (now.Hour() > 18) { n26.digitalWrite(P1, LOW); }
      } 
      else 
      {
        n26.digitalWrite(P1, HIGH);
      }


      if (N22.p0 == LOW) 
      {
        n26.digitalWrite(P2, LOW);
      } 
      else 
      {
        n26.digitalWrite(P2, HIGH);
      }  //Drain Pump Bottom Right
      
      if (N20.p2 == LOW) {
        n26.digitalWrite(P3, LOW);
      } 
      else 
      {
        n26.digitalWrite(P3, HIGH);
      }  //Open / power port for laptop or whatever
    } 
    else 
    {
      n26.digitalWrite(P0, HIGH);  //Display Tank Light Top Left
      n26.digitalWrite(P1, HIGH);  //Protein Skimmer Top Right
      n26.digitalWrite(P2, HIGH);  //Drain Pump Bottom Right
      n26.digitalWrite(P3, HIGH);  //Open / power port for laptop or whatever
    }

  } 
  else 
  {
    //Box 1 power disabled from Master Switch
    n25.digitalWrite(P0, HIGH);  //Drain Pump Top Left
    n25.digitalWrite(P1, HIGH);  //Heater 1 Top Right
    n25.digitalWrite(P2, HIGH);  //Heater 2 Bottom Right
    n25.digitalWrite(P3, HIGH);  //Powerhead

    //Box 2 power disabled from Master Switch
    n26.digitalWrite(P0, HIGH);  //Display Tank Light Top Left
    n26.digitalWrite(P1, HIGH);  //Protein Skimmer Top Right
    n26.digitalWrite(P2, HIGH);  //Drain Pump Bottom Right
    n26.digitalWrite(P3, HIGH);  //Open / power port for laptop or whatever
  }

Serial.println("I/O Check Complete");
  
  // print the temperature sensor reading every minute to Serial port ****************************************************************
  if (now.Minute() != minute) {
    // check the one wire network and list out the found devices
    // deviceCount = onewireNET.getDeviceCount();

    // Serial.print("OneWire found ");
    // Serial.print(deviceCount, DEC);
    // Serial.println(" devices.");
    // Serial.println("");

    // Serial.println("Printing Addresses....");
    // for (int i = 0; i < 3; i++) {
    //   Serial.print("Sensor ");
    //   Serial.print(i + 1);
    //   Serial.print(" : ");
    //   onewireNET.getAddress(sensorAddress, i);
    //   printAddress(sensorAddress);
    // }
   findDevices(ONE_WIRE_BUS);

    Serial.println("");
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    Serial.println("");

    // write the information to the database on the webserver ********************************************************************
    // temperature sensor 1 ******************************************

    // status = client.connect(webserver, 80);
    // if (status == 1) {
    //   Serial.println("");
    //   Serial.print("connected at ");
    //   printDateTime(now);
    //   Serial.println("");
    //   Serial.print("GET /insert_temp.php?temperature=");
    //   Serial.print(tempSensor1);
    //   Serial.println("&tempID=1");
    //   Serial.println("");

    //   client.print("GET /insert_temp.php?temperature=");
    //   client.print(tempSensor1);
    //   client.println("&tempID=1");
    //   client.println("Host: " + String(HOST_NAME));
    //   client.println("Connection: close");
    //   client.println();
    //   client.stop();
    //   status = 0;
    // } else {
    //   Serial.print("Connection Failed Temp 1 = ");
    //   Serial.print(tempSensor1);
    //   Serial.println("");
    //   client.stop();
    // }
    // delay(250);
    // // write the information to the database on the webserver
    // // temperature sensor 2 ********************************************

    // status = client.connect(webserver, 80);
    // if (status == 1) {
    //   Serial.println("");
    //   Serial.print("connected at ");
    //   printDateTime(now);
    //   Serial.println("");
    //   Serial.print("GET /insert_temp.php?temperature=");
    //   Serial.print(tempSensor2);
    //   Serial.println("&tempID=2");
    //   Serial.println("");

    //   client.print("GET /insert_temp.php?temperature=");
    //   client.print(tempSensor2);
    //   client.println("&tempID=2");
    //   client.println("Host: " + String(HOST_NAME));
    //   client.println("Connection: close");
    //   client.println();
    //   client.stop();
    //   status = 0;
    // } else {
    //   Serial.print("Connection Failed Temp 2 = ");
    //   Serial.print(tempSensor2);
    //   Serial.println("");
    //   client.stop();
    // }

    // write the information to the database on the webserver
    // temperature sensor 3 ***********************************************
   // delay(250);
    // status = client.connect(webserver, 80);
    // if (status == 1) {
    //   Serial.print("connected at ");
    //   printDateTime(now);
    //   Serial.println("");
    //   Serial.print("GET /insert_temp.php?temperature=");
    //   Serial.print(tempSensor3);
    //   Serial.println("&tempID=3");
    //   Serial.println("");
    //   client.print("GET /insert_temp.php?temperature=");
    //   client.print(tempSensor3);
    //   client.println("&tempID=3");
    //   client.println("Host: " + String(HOST_NAME));
    //   client.println("Connection: close");
    //   client.println();
    //   client.stop();
    //   status = 0;
    // } else {
    //   Serial.print("Connection Failed Temp 3 = ");
    //   Serial.print(tempSensor3);
    //   Serial.println("");
    //   client.stop();
    // }

    // Update the lcd display every minute with the new data ***********************************************************

    //display.clearDisplay();
    //display.display();



    //Update serial port on process completion ********************************************************************************
    Serial.print("Data collected for minute:  ");
    Serial.println(now.Minute());
    minute = now.Minute();
    uptime_total = uptime_total + 1;
    if(uptime_total >=1440){
        uptime_days = floor(uptime_total/1440);
      }
      else{
        uptime_days = 0;
      }

    if(uptime_days == 0){
      uptime_hours = floor(uptime_total/60);
    }
    else {
      uptime_hours = floor((uptime_total-uptime_days*1440)/60);
    }

    if (uptime_total < 60){
      uptime_minutes = uptime_total;
    }
    else {
      uptime_minutes = (uptime_total - (uptime_days*1440)-(uptime_hours*60));
    }
    Serial.print("The minute has been updated to:  ");
    Serial.println(minute);
    Serial.print("The total uptime is: ");
    Serial.print(uptime_days);
    Serial.print(" days, ");
    Serial.print(uptime_hours);
    Serial.print(" hours, ");
    Serial.print(uptime_minutes);
    Serial.println(" minutes");
    Serial.println(uptime_total);
    displayUpdateRequest = 1;
    Serial.println("");
    Serial.print("Bit 0 in the myData uint8 is: ");

    myData.bit0 = 7;

    Serial.println(myData.bit0);
    

    // Scan the I2C network to make sure all nodes are present and output addresses to serial ***********************************

    byte error, address;
    int nDevices;
    Serial.println("Scanning I2C Network....");
    nDevices = 0;
    for (address == 1; address < 127; address++) {
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
      if (error == 0) {
        Serial.print("I2C device found at 0x");
        if (address < 16) {
          Serial.print("0");
        }
        Serial.println(address, HEX);
        nDevices++;
      } else if (error == 4) {
        Serial.print("Unknown error at address 0x");
        if (address < 16) {
          Serial.print("0");
        }
        Serial.println(address, HEX);
      }
    }
    if (nDevices == 0) {
      Serial.println("No I2C devices found\n");
    } else {
      Serial.println("done\n");
    }
  }

  
  //*------------------HTML Page Code---------------------*// ***************************************************************
  /*
  wificlient.println("HTTP/1.1 200 OK");  //
  wificlient.println("Content-Type: text/html");
  wificlient.println("");
  wificlient.println("<!DOCTYPE HTML>");
  wificlient.println("<html>");

  //Head
  wificlient.println("<head>");
  wificlient.println("<title>Beekerhouse Saltwater Aquarium</title>");
  //client.println("<meta http-equiv=\"refresh\" content=\"30\" /e>");
  wificlient.println("</head>");

  //Body
  wificlient.println("<body>");
  wificlient.println(" Beekerhouse Saltwater Aquarium: <br>");
  //client.println((String)" version:" + strDateTime(compiled));
  wificlient.print(" Version: ");
  wificlient.println(strDateTime(compiled));
  wificlient.println("<br><br>");
  //client.println((String) " Current Time: " + strDateTime(now));
  wificlient.print(" Current Time: ");
  wificlient.println(strDateTime(now));

  wificlient.println("<br><br>");

  wificlient.print("Well room sensor = ");
  wificlient.print(sensorValue);
  wificlient.print(" C= ");
  //wificlient.print(temperatureC);
  wificlient.print(" F= ");
  // wificlient.println (temperatureF);
  wificlient.println("<br><br>");
  wificlient.println("</body>");


  */

 //LCD write function
 // display.clearDisplay() – all pixels are off
 // display.drawPixel(x,y, color) – plot a pixel in the x,y coordinates
 // display.setTextSize(n) – set the font size, supports sizes from 1 to 8
 // display.setCursor(x,y) – set the coordinates to start writing text
 // display.print(“message”) – print the characters at location x,y
 // display.display() – call this method for the changes to make effect
 //char lcdPage1[6][128] = {"","","","","",""}; //Date & Time Overview
 //char lcdPage2[6][128] = {"","","","","",""}; // Drain Pump Control
 //char lcdPage3[6][128] = {"","","","","",""}; // Output & Input Status
 //char lcdWrite[6][128] = {"","","","","",""};
 // int lcdX[6] = {1,1,1,1,1,1};
 // int lcdY[6] = {5,16,26,36,46,56};
 //int lcdPageSelect = 1;
int found = 0;
String mystr = String(uptime_total); //integer to string example
String mystr2 = String(tempSensor1);
//strcpy(Page1.Line2, mystr.c_str() );
Serial.println("mystr");
strcpy(Page1.Line1, "BeekerTank");
Serial.println("Line1");
snprintf(Page1.Line2, sizeof(Page1.Line2), "%s%.0f", "Ambient Temp: ", tempSensor1, "\0");
Serial.println("Line2");
snprintf(Page1.Line3, sizeof(Page1.Line3), "%s%.0f", "Display Temp: ", tempSensor3, "\0");
Serial.println("Line3");
snprintf(Page1.Line4, sizeof(Page1.Line4), "%s%.0f", "Sump Temp:    ", tempSensor2), "\0";
Serial.println("Line4");
snprintf(Page1.Line5, sizeof(Page1.Line5), "%s%d", "Pump Status:  ", PumpON, "\0");
Serial.println("Line5");
snprintf(Page1.Line6, sizeof(Page1.Line6), "%s%d%s%d%s%d%s", "Up:", uptime_days, "d ", uptime_hours, "hr ", uptime_minutes, "m!", "\0");
Serial.println("Line6");
lcdLastWrite = lcdWrite;
Serial.println("Enter Page Select");
if(lcdPageSelect == 1)
 {
    
    lcdWrite = Page1;
    Serial.println("Display Update Requested Page 1");
    displayUpdateRequest = 1;
    
 }
else if(lcdPageSelect == 2)
 {
     lcdWrite = Page2;
    Serial.println("Display Update Requested Page 2");
    displayUpdateRequest = 1;
 }
else if(lcdPageSelect == 3)
 {
    lcdWrite = Page3;
    Serial.println("Display Update Requested Page 3");
    displayUpdateRequest = 1;
 }
else 
 {
   Serial.println("No Page Select Found");
   strcpy(lcdWrite.Line1, "No Data");
   strcpy(lcdWrite.Line2, "No Data");
   strcpy(lcdWrite.Line3, "No Data");
   strcpy(lcdWrite.Line4, "No Data");
   strcpy(lcdWrite.Line5, "No Data");
 }

found = strcmp(lcdLastWrite.Line1 , lcdWrite.Line1);
found = found + strcmp(lcdLastWrite.Line2, lcdWrite.Line2);
found = found + strcmp(lcdLastWrite.Line3, lcdWrite.Line3);
found = found + strcmp(lcdLastWrite.Line4,lcdWrite.Line4);
found = found + strcmp(lcdLastWrite.Line5,lcdWrite.Line5);
found = found + strcmp(lcdLastWrite.Line6,lcdWrite.Line6);

Serial.print("Found is equal to: ");
Serial.println(found);

//set the text size and color
 display.setTextSize(1);
 display.setTextColor(WHITE);
 
 //output the updated display screen only if the screen is different from the last time
 if (displayUpdateRequest == 1 && found!=0)
 {
   Serial.println("Updating Screen");
  display.clearDisplay();   
  display.display();
  
  display.setCursor(lcdX[0], lcdY[0]);
  display.print(lcdWrite.Line1);
  display.setCursor(lcdX[1], lcdY[1]);
  display.print(lcdWrite.Line2);
  display.setCursor(lcdX[2], lcdY[2]);
  display.print(lcdWrite.Line3);
  display.setCursor(lcdX[3], lcdY[3]);
  display.print(lcdWrite.Line4);
  display.setCursor(lcdX[4], lcdY[4]);
  display.print(lcdWrite.Line5);
  display.setCursor(lcdX[5], lcdY[5]);
  display.print(lcdWrite.Line6);

//complete the display update
 display.display();
 displayUpdateRequest = 0;
 }





  //*------------------HTML Page Code---------------------*// *******************************************

}
//End of main code ***************************************************************************************




#define countof(a) (sizeof(a) / sizeof(a[0]))




// printDateTime function returns a formatted date from the RTC data*************************************
void printDateTime(const RtcDateTime& dt) {
  char datestring[20];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             dt.Month(),
             dt.Day(),
             dt.Year(),
             dt.Hour(),
             dt.Minute(),
             dt.Second());
  Serial.print(datestring);
}

String strDateTime(const RtcDateTime& dt) {
  char datestring[20];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             dt.Month(),
             dt.Day(),
             dt.Year(),
             dt.Hour(),
             dt.Minute(),
             dt.Second());
  return datestring;
}

// printAddress ************************************************************************************************
void printAddress(DeviceAddress sensorAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    Serial.print("0x");
    if (sensorAddress[i] < 0x10) Serial.print("0");
    Serial.print(sensorAddress[i], HEX);
    if (i < 7) Serial.print(", ");
  }
  Serial.println("");
}

// wifiStart **************************************************************************************************
int wifiStart(char ssID[], char psWD[]) {
  int result;
  Serial.println("wifiStart function started");
  Serial.print("SSID from function: ");
  Serial.println(ssID);

  WiFi.begin(ssID, psWD);

  int i = 0;

  while (WiFi.status() != WL_CONNECTED && i < 30) {  // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i);
    Serial.print(' ');
    //display.clearDisplay();
    display.setCursor(10, 36);
    display.print(i);
    display.display();
  }
  if (WiFi.status() != WL_CONNECTED) {
    display.setCursor(10, 36);
    display.print("No Wifi");
    display.display();
    WiFi.printDiag(Serial);
  } else {
    Serial.println('\n');
    Serial.println("Wifi Connection established!");
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());

    //Serial.println("Connected to wifi");
    //Serial.println("\nStarting connection...");
    display.setCursor(10, 36);
    display.print("Wifi OK");
    display.display();


    // Next try to connect to the webserver for data collection.


    //status = client.connect(webserver, 80);
    // Serial.println("Attempting to contact Webserver");
    // int ii=0;
    // while (status == 0 && ii<=10) {  // Wait for the connection to the webserver for data collection
    //   delay(1000);
    //   Serial.print(++ii);
    //   Serial.print(' ');
    // }
    // Serial.println(status);

    // Serial.println("Connected to Webserver for data collection");
    // Serial.print("My IP address: ");
    // for (byte thisByte = 0; thisByte < 4; thisByte++) {
    //   // print the value of each byte of the IP address:
    //   Serial.print(WiFi.localIP()[thisByte], DEC);
    //   Serial.print(".");
    //   display.setCursor(10, 46);
    //   display.print("IP ADDR OK");
    //   display.display();
    // }
  }

  return result;
}

uint8_t findDevices(int pin)
{
  OneWire ow(pin);

  uint8_t address[8];
  uint8_t count = 0;


  if (ow.search(address))
  {
    Serial.print("\nuint8_t pin");
    Serial.print(pin, DEC);
    Serial.println("[][8] = {");
    do {
      count++;
      Serial.print("  {");
      for (uint8_t i = 0; i < 8; i++)
      {
        Serial.print("0x");
        if (address[i] < 0x10) Serial.print("0");
        Serial.print(address[i], HEX);
        if (i < 7) Serial.print(", ");
      }
      Serial.println("  },");
    } while (ow.search(address));

    Serial.println("};");
    Serial.print("// nr devices found: ");
    Serial.println(count);
  }

  return count;
}