

/*wemos mini d1 pro
  pin  TX  RX  D1  D2  D3  D4  D5  D6   D7   D8
  GPIO 1   3   5   4   0   2   14  12   13   15

  esp-01
  pin      01  GND
          CH  02
  GPIO     RST 00
          VCC 03

*/


#if defined ARDUINO_ARCH_ESP8266  // s'il s'agit d'un ESP8266
#include <ESP8266WiFi.h>
#elif defined ARDUINO_ARCH_ESP32  // s'il s'agit d'un ESP32
#include "WiFi.h"
#endif

#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
const long utcOffsetInSeconds = 7200;  //heure hiver-ete
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "fr.pool.ntp.org", utcOffsetInSeconds);
unsigned long derniereDemande = millis(); // moment de la plus récente demande au serveur NTP
unsigned long derniereMaJ = millis(); // moment de la plus récente mise à jour de l'affichage de l'heure
const int delaiDemande = 10 * 60; // nombre de secondes entre deux demandes consécutives au serveur NTP
unsigned long clock_time ;        //datetime varable to store the current position of the clock's arms
unsigned long rtc_time ;
long seconds ;
long steps = 0 ;
int dir = 1;


#include <Unistep2.h>
// for your motor
//const int stepsPerRevolution = 200;  // change this to fit the number of steps per revolution
// initialize the stepper library on pins D1 à D4 sur wemos:
Unistep2 stepperX(5, 4, 0, 2, 200, 1500);
#define Sec_per_step 0.465     //ammount of seconds per step (change this for different ratios between stepper and minute hand)
#define Hall_pin 12          //pin for the hall effect sensor signal

int etat = HIGH;
const char* ssid = "WifiBox";
const char* password = "PasswordOfWifiBox";

void setup() {
  Serial.begin(9600);
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connexion au reseau WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  delay(100);

  stepperX.run();
  pinMode(Hall_pin, INPUT_PULLUP);
  timeClient.begin();
  delay(100);
  timeClient.update();
  calibrate();
  calc_steps();
  move_stepper();
  derniereMaJ = millis()-58000;
}

void loop() {
  stepperX.run();
  rtc_time = (timeClient.getHours() * 60 + timeClient.getMinutes()) * 60;
  //etat = digitalRead(Hall_pin);

  // est-ce le moment de demander l'heure NTP?
  if ((millis() - derniereDemande) >=  delaiDemande * 1000 ) {
    timeClient.update();
    derniereDemande = millis();

    Serial.println("Interrogation du serveur NTP");
  }

  // est-ce que millis() a débordé?
  if (millis() < derniereDemande ) {
    timeClient.update();
    derniereDemande = millis();
  }

  // est-ce le moment de raffraichir la date indiquée?
  if ((millis() - derniereMaJ) >=   15000 ) {

    afficheHeure();
    calc_steps();                       // use this void to calculate if, and how many steps are neccecary, result is stored in the variable "steps"
    if (steps != 0)                     // only if a step is neccecary this part of the code is used
    {
      move_stepper();
    }


    derniereMaJ = millis();
  }

}


//================================================================================================================
void afficheHeure() {
  Serial.print(timeClient.getHours());
  Serial.print(":");
  Serial.print(timeClient.getMinutes());
  //Serial.print("   -:");
  //Serial.print(rtc_time);
  Serial.print("difference en min : ");
  Serial.print(seconds / 60);
  Serial.print("      -steps:"); Serial.println(steps);
}

//=============================================================================================================================

void calibrate()
{
  rtc_time = (timeClient.getHours() * 60 + timeClient.getMinutes()) * 60;
  //This void matches the physical time of the hands with the time the RTC prescibes.
  //After a power down the Arduino does not know where the hands of the clock are
  //Therefore we use the hall effect sensor to detect when the hands are at a certain time
  //after this we move from this known time to the actual time we want to display

  while (etat == HIGH)                         // while we have not reached the hall effect sensor
  {
    //Serial.print(".");
    delay(1);
    stepperX.run();
    stepperX.move(200 * dir);
    etat = digitalRead(Hall_pin);
  }
  Serial.println("Position found, ");


  //======================================
  //The next line sets the time at which the hands stop moving after hall effect sensor detection
  //in my case this is 5:49 or 17:49 (doesnt matter), change this accordingly
  clock_time = (6 * 60 + 1) * 60 ;   // Set the position of the arms (Year, Month, Day, Hour, minute, second) Year, Month and day are not important
  //======================================

}

//====================================================================================================================================
void calc_steps()
{
  seconds = rtc_time - clock_time;       //calculate the difference between where the arms are and where they should be


  if (seconds > 43200)                        // we have an analog clock so it only displays 12 hours, if we need to turn 16 hours for example we can better turn 4 hours because its the same
  {
    seconds = seconds - 43200;
  }

  if (seconds > 21600)                        // if the difference is bigger than 6 hours its better to turn backwards
  {
    seconds = seconds - 43200;              //convert the seconds to steps
  }
  steps = steps + (seconds / Sec_per_step);

}


//====================================================================================================================================
void move_stepper()
{
  stepperX.move(dir * steps);           //tell the motor how many steps are needed and in which direction
  clock_time = rtc_time;                //since the steps will be made by the motor we can assume the position of the arms is equal to the new position
  steps = 0;
}
