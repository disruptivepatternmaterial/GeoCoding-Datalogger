#include <Adafruit_GPS.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>

#include <Adafruit_SH110X.h>
#include <Adafruit_MCP9808.h>

#include <DailyStruggleButton.h>

Adafruit_MCP9808  sensor;
Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);

#define cardSelect 4
#define cardDetect 7
#define GREENLEDPIN 8
#define REDLEDPIN 13

#define VBATPIN A7
#define DEBUG

#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5

unsigned int longPressTime = 1000;
byte multiHitTarget = 2;
unsigned int multiHitTime = 400;
DailyStruggleButton myBUTTON_A;
DailyStruggleButton myBUTTON_B;
DailyStruggleButton myBUTTON_C;

char filename[15];
float temperature;
uint32_t displaytimer = millis();
uint32_t logtimer = millis();
uint32_t loopcount = 0;
int timerLimit = 1000;
String utc_time;

#define GPSSerial Serial1
Adafruit_GPS GPS(&GPSSerial);
#define GPSECHO false

File logfile;

void setup() {

  Serial.begin(115200);
  delay(500);

  pinMode(GREENLEDPIN, OUTPUT);
  digitalWrite(GREENLEDPIN, LOW);
  pinMode(REDLEDPIN, OUTPUT);
  digitalWrite(REDLEDPIN, LOW);

  pinMode(BUTTON_A, INPUT_PULLUP);

  myBUTTON_A.set(BUTTON_A, buttonAEvent, INT_PULL_UP);
  myBUTTON_A.enableLongPress(1000);
  myBUTTON_B.set(BUTTON_B, buttonBEvent, INT_PULL_UP);
  myBUTTON_B.enableLongPress(1000);
  myBUTTON_C.set(BUTTON_C, buttonCEvent, INT_PULL_UP);
  myBUTTON_C.enableLongPress(1000);

  pinMode(cardDetect, INPUT_PULLUP);

  //setup display
  display.begin(0x3C, true); // Address 0x3C default
  display.setRotation(1);
  display.setTextColor(SH110X_WHITE);
  display.clearDisplay();
  display.display();
  display.setCursor(0, 0);

  //setup sensor
  sensor.begin();

  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_ALLDATA);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);

  // Set the update rate
  // Note you must send both commands below to change both the output rate (how often the position
  // is written to the serial line), and the position fix rate.
  // 1 Hz update rate
  //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  //GPS.sendCommand(PMTK_API_SET_FIX_CTL_1HZ);
  // 5 Hz update rate- for 9600 baud you'll have to set the output to RMC or RMCGGA only (see above)
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_5HZ);
  GPS.sendCommand(PMTK_API_SET_FIX_CTL_5HZ);
  // 10 Hz update rate - for 9600 baud you'll have to set the output to RMC only (see above)
  // Note the position can only be updated at most 5 times a second so it will lag behind serial output.
  //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_10HZ);
  //GPS.sendCommand(PMTK_API_SET_FIX_CTL_5HZ);

  // Request updates on antenna status, comment out to keep quiet
  //GPS.sendCommand(PGCMD_ANTENNA);
  // Turn off updates on antenna status, if the firmware permits it
  GPS.sendCommand(PGCMD_NOANTENNA);

  delay(1000);

  GPSSerial.println(PMTK_Q_RELEASE);

  while (!digitalRead(cardDetect)) {
    Serial.println("Card not installed");
    display.print("PLEASE INSERT A CARD");
    display.display();
    delay(5000);
    display.setCursor(0, 0);
  }
  //display.setFont(&TomThumb);

  // see if the card is present and can be initialized:
  if (!SD.begin(cardSelect)) {
    Serial.println("Card init. failed!");
    error(2);
  }

  strcpy(filename, "GPSLOG00.CSV");
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = '0' + i / 10;
    filename[7] = '0' + i % 10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
  }

  logfile = SD.open(filename, FILE_WRITE);
  if ( ! logfile ) {
    // Serial.print("Couldnt create "); Serial.println(filename);
    error(3);
  }
  // Serial.print("Writing to "); Serial.println(filename);
  logfile.println("row,alt,lat,lon,speed,temp,head,utc_d,utc_t");
}

void error(uint8_t errno) {
  uint8_t i;
  for (i = 0; i < errno; i++) {
    digitalWrite(REDLEDPIN, HIGH);
    delay(100);
    digitalWrite(REDLEDPIN, LOW);
    delay(100);
  }
  for (i = errno; i < 10; i++) {
    delay(200);
  }
}

void loop()                     // run over and over again
{
  myBUTTON_B.poll();
  myBUTTON_C.poll();
  // read data from the GPS in the 'main loop'
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  if (GPSECHO)
    if (c) Serial.print(c);
  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trying to print out data
    //Serial.print(GPS.lastNMEA()); // this also sets the newNMEAreceived() flag to false
    if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
      return; // we can fail to parse a sentence in which case we should just wait for another
  }
  sensors_event_t event;
  sensor.getEvent(&event);
  temperature = event.temperature;
  // approximately every 2 seconds or so, print out the current stats
  if (millis() - logtimer > timerLimit) {
    logtimer = millis(); // reset the timer
    utc_time = "";
    Serial.print("temperature: ");
    Serial.println(event.temperature);

    Serial.print("\nTime: ");
    if (GPS.hour < 10) {
      Serial.print('0');
      utc_time = utc_time + "0";
    }
    Serial.print(GPS.hour, DEC); Serial.print(':');
    utc_time = utc_time + String(GPS.hour) + ":";
    if (GPS.minute < 10) {
      Serial.print('0');
      utc_time = utc_time + "0";
    }
    Serial.print(GPS.minute, DEC); Serial.print(':');
    utc_time = utc_time + String(GPS.minute) + ":";
    if (GPS.seconds < 10) {
      Serial.print('0');
      utc_time = utc_time + "0";
    }
    Serial.print(GPS.seconds, DEC); Serial.print('.');
    utc_time = utc_time + String(GPS.seconds) + ".";
    if (GPS.milliseconds < 10) {
      Serial.print("00");
      utc_time = utc_time + "00";
    } else if (GPS.milliseconds > 9 && GPS.milliseconds < 100) {
      Serial.print("0");
      utc_time = utc_time + "0";
    }
    Serial.println(GPS.milliseconds);
    utc_time = utc_time + GPS.milliseconds;
    Serial.print("Date: ");
    Serial.print(GPS.day, DEC); Serial.print('/');
    Serial.print(GPS.month, DEC); Serial.print("/20");
    Serial.println(GPS.year, DEC);
    Serial.print("Fix: "); Serial.print((int)GPS.fix);
    Serial.print(" quality: "); Serial.println((int)GPS.fixquality);
    if (GPS.fix) {
      Serial.print("Location: ");
      Serial.print(GPS.latitudeDegrees, 10);
      Serial.print(", ");
      Serial.println(GPS.longitudeDegrees, 10);
      Serial.print("Speed (knots): "); Serial.println(GPS.speed);
      Serial.print("Angle: "); Serial.println(GPS.angle);
      Serial.print("Altitude: "); Serial.println(GPS.altitude, 4);
      Serial.print("Satellites: "); Serial.println((int)GPS.satellites);
      String utc_date = "20" + String(GPS.year) + "/" + String(GPS.month) + "/" + String(GPS.day);
      String dataString = String(loopcount + 1) + "," + String(GPS.altitude, 4) + "," + String(GPS.latitudeDegrees, 12) + "," + String(GPS.longitudeDegrees, 12) + "," + String(GPS.speed * 0.51444444444) + "," + String(event.temperature) + "," + String(GPS.angle) + "," +  utc_date + "," + utc_time;
      Serial.println(dataString);

      logfile.println(dataString);
      //logfile.close();
      loopcount++;

    }
  }
    if (millis() - displaytimer > 100) {
    displaytimer = millis(); // reset the timer
  doDisplay();
    }
  int check = checkCard();
  if (check == 0){
  } else {
     logfile = SD.open(filename, FILE_WRITE);
  }
}

void doDisplay() {
  display.setCursor(0, 0);
  display.clearDisplay();
   display.print("lat:");
  display.println(GPS.latitudeDegrees, 12);
   display.print("lon:");
  display.println(GPS.longitudeDegrees, 12);
  
  display.print("alt:");
  display.print(GPS.altitude,1);
    display.print("m spd:");
  display.print(GPS.speed * 0.51444444444,1);
    display.println("mps");
    
  display.print("time:");
  display.println(utc_time);
  
  display.print("sats:");
  display.print((int)GPS.satellites);
 display.print(" ang:");
  display.println(GPS.angle,1);

   display.print("temp:");
  display.print(temperature,1);
  display.print(" batt:");
  display.println(checkBattery());

    display.print(loopcount);
      display.print(" @ ");
    display.print(timerLimit / 1000);
  display.println(" s/log");

  display.print("file:");
  display.println(filename);

  //yield();
  display.display();
}

void buttonAEvent(byte btnStatus) {
  switch (btnStatus) {
    case onPress:
      break;
    case onRelease:
      break;
    case onLongPress:
    
      break;
  }
}

void buttonBEvent(byte btnStatus) {
  switch (btnStatus) {
    // onPress is indicated when the button is pushed down
    case onPress:
      break;
    // onRelease is indicated when the button is let go
    case onRelease:
      timerLimit = timerLimit - 1000;
      if (timerLimit <= 0) {
        timerLimit = 1000;
      }
      break;
    case onLongPress:
      timerLimit = timerLimit - 10000;
      if (timerLimit <= 0) {
        timerLimit = 1000;
      }
      break;
  }
}

void buttonCEvent(byte btnStatus) {
  switch (btnStatus) {
    case onPress:
      break;
    case onRelease:
      timerLimit = timerLimit + 1000;
      break;
    case onLongPress:
    if ( timerLimit == 1000) {
     timerLimit = timerLimit + 9000;
    } else {
      timerLimit = timerLimit + 10000;
      break; }
  }
}

int checkCard() {
     int counter = 0;
  while (!digitalRead(cardDetect)) {
    //Serial.println("Card not installed");
    display.setCursor(0, 0);
    display.clearDisplay();
    display.print("PLEASE INSERT A CARD");
    display.display();
    delay(3000);
    display.setCursor(0, 0);
    counter++;
  }
  return counter;
}


float checkBattery() {
  float measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  //Serial.print("VBat: " ); Serial.println(measuredvbat);
  return measuredvbat;
}

/*
   1 - fix sdram card removal and reinstert
*/
