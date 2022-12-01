#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#include "A4988.h"

// Pin modification
int Step = 14; //GPIO14---D5 of Nodemcu--Step of stepper motor driver
int Dire  = 2; //GPIO2---D4 of Nodemcu--Direction of stepper motor driver
int Sleep = 12; //GPIO12---D6 of Nodemcu-Control Sleep Mode on A4988
int MS1 = 13; //GPIO13---D7 of Nodemcu--MS1 for A4988
int MS2 = 16; //GPIO16---D0 of Nodemcu--MS2 for A4988
int MS3 = 15; //GPIO15---D8 of Nodemcu--MS3 for A4988
 
//Motor Specs
const int spr = 200; //Steps per revolution
int RPM = 80; //Motor Speed in revolutions per minute
int Microsteps = 512; //Stepsize (1 for full steps, 2 for half steps, 4 for quarter steps, etc)
 
//Providing parameters for motor control
A4988 stepper(spr, Dire, Step, MS1, MS2, MS3);

// creating a webServer object
ESP8266WebServer server(80);

WiFiUDP ntpUDP;                         // Define NTP Client to get time
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void setup() {
  // put your setup code here, to run once:
//  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(500);

  startWiFi();              // Try to connect to some given access points. Then wait for a connection
  startServer();          // handle and start the server
  startMotor();          // set up the pins, rpm and microsteps
 
  timeClient.begin();           //initialize the NTP client
  timeClient.setTimeOffset(10800); // set the time zone (1hr = 3600s)
}

// Variables
int wakeUpHours = 7;
int wakeUpMins = 00;

int bedTimeHours = 22;
int bedTimeMins = 00;

bool areClosed = true;

// Constants
const int allSteps = 6000;    // Steps required for the whole path
const String DOMAIN_NAME = "smart";

void loop() {
  // put your main code here, to run repeatedly:
  timeClient.update();
  MDNS.update();
  server.handleClient();

  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();

  if (currentHour == wakeUpHours && currentMinute == wakeUpMins) {
    openCurtains();
  } else if (currentHour == bedTimeHours && currentMinute == bedTimeMins) {
    closeCurtains();
  }
  
  delay(1000);
}

//Methods
void closeCurtains() {
  if (!areClosed) {
    digitalWrite(Sleep, HIGH);
    stepper.move(-allSteps); // ?
    areClosed = true;
    digitalWrite(Sleep, LOW);
  }
}

void openCurtains() {
  if (areClosed) {
    digitalWrite(Sleep, HIGH);
    stepper.move(allSteps); // ?
    areClosed = false;
    digitalWrite(Sleep, LOW);
  }
}

void handleClose() {
  if (!areClosed) {
    closeCurtains();
  }
  
  server.sendHeader("Location", "/");        // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
}

void handleOpen() {
  if (areClosed) {
    openCurtains();
  }
  
  server.sendHeader("Location", "/");        // Add a header to respond with a new location for the browser to go to the home page again
  server.send(303);                         // Send it back to the browser with an HTTP status 303 (See Other) to redirect
}

void handleRoot() {
  String webPage = formatHTML(wakeUpHours, wakeUpMins, bedTimeHours, bedTimeMins); // returns the html file with variables inside it
  server.send(200, "text/html", webPage);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not found");
}

void handleForm() {
  String wakeUpHrsStr = server.arg("wake-up-hours"); 
  String wakeUpMinsStr = server.arg("wake-up-mins");
  String bedTimeHrsStr = server.arg("bed-time-hours"); 
  String bedTimeMinsStr = server.arg("bed-time-mins");

  wakeUpHours = wakeUpHrsStr.toInt();
  wakeUpMins = wakeUpMinsStr.toInt();
  
  bedTimeHours = bedTimeHrsStr.toInt();
  bedTimeMins = bedTimeMinsStr.toInt();

  server.sendHeader("Location", "/");
  server.send(303);
}

void startServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/open", HTTP_POST, handleOpen);
  server.on("/close", HTTP_POST, handleClose);
  server.on("/get", handleForm);
  server.onNotFound(handleNotFound);

  server.begin();     // Actually start the server
  Serial.println("HTTP server started");
}

void startWiFi() {
  const char* ssid = "deo2005";
  const char* pass = "0547245321";

  WiFi.begin(ssid, pass); 
//  Serial.print("Connecting to ");
//  Serial.print(ssid);
//  Serial.println("...");            // Connect to the network
  
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(250);
//    Serial.print('.');
  }

//  Serial.println('\n');
//  Serial.println("Connection established!");  
//  Serial.print("IP address: ");
//  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer

//  if (MDNS.begin(DOMAIN_NAME)) {              // Start the mDNS responder for esp8266.local
//    Serial.println("mDNS responder started");
//  } else {
//    Serial.println("Error setting up MDNS responder!");
//  }
}

void startMotor() {
  pinMode(Step, OUTPUT); //Step pin as output
  pinMode(Dire,  OUTPUT); //Direcction pin as output
  pinMode(Sleep,  OUTPUT); //Set Sleep OUTPUT Control button as output
  digitalWrite(Step, LOW); // Currently no stepper motor movement
  digitalWrite(Dire, LOW);
  
  // Set target motor RPM to and microstepping setting
  stepper.begin(RPM, Microsteps);
}

String formatHTML(int wakeUpHours, int wakeUpMins, int bedTimeHours, int bedTimeMins) {
  String html = "<!DOCTYPE html>\n    <html lang=\"en\">\n        <head>\n            <meta charset=\"UTF-8\">\n            <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n            <title>Automated curtains</title>\n            <link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">\n            <link rel=\"preconnect\" href=\"https://fonts.gstatic.com\" crossorigin>\n            <link href=\"https://fonts.googleapis.com/css2?family=Montserrat:wght@300;400;500;700&display=swap\" rel=\"stylesheet\">    \n            <style>\n                *,\n                *::after,\n                *::before {\n                    -webkit-box-sizing: border-box;\n                    box-sizing: border-box;\n                }\n\n                :root {\n                    --purple: #775B84;\n                    --off-white: #E9E9E9;\n                    --white: #FFFFFF;\n                    --background-color: #F3F3F3;\n                }\n\n                body, h2, p {\n                    margin: 0;\n                }\n\n                body {\n                    font-family: \"Montserrat\", sans-serif;\n                    font-size: 1.3125rem;\n                    background: var(--background-color);\n                    display: grid;\n                    place-items: center;\n                    line-height: 1.6;\n                    width: min(90%, 30rem);\n                    margin: 0 auto;\n                }\n\n                #open {\n                    background: var(--purple);\n                    border-radius: 20px;\n                    border: none;\n                    color: var(--white);\n                    box-shadow: 4px 5px 4px rgba(0, 0, 0, 0.1);\n                    padding: 0.2em 2.5em;\n                    width: min(100%, 60rem);\n                    margin-top: 6.5rem;\n                    font-size: 2rem;\n                }\n\n                #close {\n                    background: var(--white);\n                    border-radius: 20px;\n                    outline: none;\n                    color: var(--purple);\n                    box-shadow: 4px 5px 4px rgba(0, 0, 0, 0.1);\n                    border: none;\n                    padding: 0.2em 1em;\n                    margin: 1rem auto;\n                    margin-bottom: 3rem;\n                    width: min(100%, 60rem);\n                    font-size: 2rem;\n                }\n\n                #update {\n                    background: var(--white);\n                    border-radius: 20px;\n                    outline: none;\n                    color: var(--purple);\n                    box-shadow: 4px 5px 4px rgba(0, 0, 0, 0.1);\n                    border: none;\n                    padding: 0.3em 1em;\n                    margin: 2.5rem auto;\n                    width: min(70%, 16rem);\n                    font-size: 1.3125rem;\n                    display: block;\n                }\n\n                h2 {\n                    line-height: 1;\n                    text-align: center;\n                    font-size: 1.3125em;\n                    font-weight: normal;\n                    margin-top: 2rem;\n                    margin-bottom: .5rem;\n                    color: #6e5779;\n                }\n\n                .label {\n                    border: none;\n                    background-color: var(--off-white);\n                    font-size: 1.5em;\n                    width: min(25%, 40rem);\n                    text-align: center;\n                    border-radius: 10px;\n                    padding: .5em ;\n                    font-weight: 700;\n                    box-shadow: 1px 1px 2px rgba(0, 0, 0, 0.1);\n                    color: #67576e;\n                }\n\n                .inputs {\n                    text-align: center;\n                    margin: 0 auto;\n                }\n\n                text {\n                    font-weight: bold;\n                    font-size: 1em;\n                }\n\n                .container {\n                    margin-inline: auto;\n                    width: min(90%, 60rem);\n                    display: flex;\n                    flex-direction: column;\n                }\n\n                .buttons {\n                    margin: 1rem auto;\n                }\n\n                input::-webkit-outer-spin-button,\n                input::-webkit-inner-spin-button {\n                -webkit-appearance: none;\n                margin: 0;\n                }\n\n                input[type=number] {\n                -moz-appearance: textfield;\n                }\n\n            </style>\n        </head>\n        <body>\n            <div class=\"container\">\n                <div class=\"buttons\">\n                    <form action=\"/open\" method=\"POST\">\n                        <input class=\"btn-animation\" id=\"open\" type=\"submit\" value=\"OPEN\">\n                    </form>\n                    <form action=\"/close\" method=\"POST\">\n                        <input class=\"btn-animation\" id=\"close\" type=\"submit\" value=\"CLOSE\">\n                    </form>\n                </div>\n            </div>\n            <div class=\"container\">\n                <div class=\"alarm-section\">  \n      <form action=\"/get\">              <div class=\"wake-up\">\n                        <h2>WAKE UP</h2>\n                        <div class=\"inputs\">\n                            <input class=\"label\" type=\"number\" name=\"wake-up-hours\" value=\"";

  html += String(wakeUpHours);
  html += "\" max=\"24\" min=\"0\">\n <text>:</text>\n <input class=\"label\" type=\"number\" name=\"wake-up-mins\" value=\"";

  if (wakeUpMins < 10) {
    html += "0";
  }
  html += String(wakeUpMins);
  html += "\" max=\"60\" min=\"0\">\n </div>\n </div>\n <div class=\"wake-up\">\n <h2>BED TIME</h2>\n                        <div class=\"inputs\">\n                              <input class=\"label\" type=\"number\" name=\"bed-time-hours\" value=\"";


  html += String(bedTimeHours);
  html += "\" max=\"24\" min=\"0\">\n                                <text>:</text>\n                                <input class=\"label\" type=\"number\" name=\"bed-time-mins\" value=\"";
  if (bedTimeMins < 10) {
    html += "0";
  }
  html += String(bedTimeMins);
  html += "\" max=\"60\" min=\"0\">\n                                <input class=\"btn-animation\" id=\"update\" type=\"submit\" value=\"UPDATE\">\n                                               </div>\n                    </div>\n   </form>           </div>\n            </div>\n        </body>\n    </html>";

  return html;
}
