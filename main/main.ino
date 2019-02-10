#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>

//The HTML for the UI stored in constant varibles.
const char* indexhtml PROGMEM = "<!DOCTYPE html>\n<html>\n<head>\n  <meta charset=\"utf-8\">\n  <meta name=\"viewport\" content=\"width=device-width\">\n  <title>Humidity</title>\n  <style>\n  html{\n    background: blue;\n  }\n  body{\n    margin:0;\n    text-align: center;\n    padding-top: 30px;\n  }\n  #status {\n    display: inline;\n    padding:5px;\n    background: skyblue;\n    border-radius: 10px;\n    border: solid grey 8px;\n    border-style: groove;\n  }\n  .values{\n    background: skyblue;\n    display: block;\n    width: 90%;\n    padding: 20px;\n    margin: 40px auto;\n    border-radius: 10px;\n    border: solid grey 10px;\n    border-style: groove;\n    text-align: center;\n  }\n\n  .values h2{\n    padding: 20px;\n    display: inline;\n    font-family:monospace;\n  }\n\n  #runningx{\n    background:Red;\n  }\n  #is_running{\n    background:lime;\n  }\n  </style>\n  <script>\n    setTimeout(function(){\n      location.reload()\n    },20000)\n  </script>\n</head>\n<body>\n  <h1 id=\"status\">Status:</h1>\n<div class=\"values\" id=\"hum\">\n  <h2>Humidity(%):</h2>\n  <h2>Humidityx</h2>\n</div>\n<div class=\"values\" id=\"temp\">\n  <h2>Temperature(C):</h2>\n  <h2>TempX</h2>\n</div>\n<div class=\"values\" id=\"Time\">\n  <h2>Time:</h2>\n  <h2>TimeX</h2>\n</div>\n<div class=\"values\" id=\"Target\">\n  <h2>Humidity Target(%):</h2>\n  <h2>TargetX</h2>\n</div>\n<div class=\"values\" id=\"runningx\">\n  <h2>Not Running</h2>\n</div>\n\n<div class=\"values\" id=\"Current_settings\">\n  <h2>Current Settings:</h2><br>\n  <h2>Modex</h2><br>\n  <h2>Settingsx</h2>\n</div>\n<a href=\"Settings\"><h1 id=\"status\">Settings<h1></a>\n</body>\n</html>\n";
const char* settingshtml PROGMEM = "<!DOCTYPE html>\n<html>\n<head>\n  <meta charset=\"utf-8\">\n  <meta name=\"viewport\" content=\"width=device-width\">\n  <title>Humidity</title>\n  <style>\n  html{\n    background: blue;\n  }\n  body{\n    margin:0;\n    text-align: center;\n    padding-top: 30px;\n  }\n  #status {\n    display: inline;\n    padding:5px;\n    background: skyblue;\n    border-radius: 10px;\n    border: solid grey 8px;\n    border-style: groove;\n  }\n  form{\n    background: skyblue;\n    width: 90%;\n    padding: 20px;\n    margin: 40px auto;\n    border-radius: 10px;\n    border: solid grey 10px;\n    border-style: groove;\n    text-align: center;\n  }\n\n  .divspam h2{\n    padding: 20px;\n    display: inline;\n    font-family:monospace;\n  }\n  input{\n    font-size: 20px;\n    font-family:monospace;\n  }\n  </style>\n</head>\n<body>\n  <h1 id=\"status\">Settings</h1>\n<div class=\"divspam\">\n    <form action=\"landing_static\" method=\"post\">\n      <h1>Static mode</h1>\n      <h2>Turn ON:</h2><input type=\"radio\" name=\"check\" value=\"ON\" checked></input><br>\n      <h2>Turn OFF:</h2><input type=\"radio\" name=\"check\" value=\"OFF\"></input><br>\n      <input type=\"password\" name=\"password\" placeholder=\"Admin Password\"><br>\n      <input type=\"Submit\" value=\"Change\"></input>\n    </form>\n  </div>\n<div class=\"divspam\">\n  <form action=\"landing_target\" method=\"post\">\n    <h1>Target Mode:</h1>\n    <h2>Humidity Target(%):</h2><input type=\"number\" name=\"target\" min=\"0\" max=\"100\" value=\"85\"><br>\n    <input type=\"password\" name=\"password\" placeholder=\"Admin Password\"><br>\n    <input type=\"Submit\" value=\"Change\">\n  </form>\n</div>\n\n<div class=\"divspam\">\n  <form action=\"landing_cycle\" method=\"post\">\n    <h1>24h cycle:</h1>\n    <h2>Humidity Target standard(%):</h2><input type=\"number\" name=\"Target_stand\" min=\"0\" max=\"100\" value=\"82\"><br>\n    <h2>Humidity Target morning/evening(%):</h2><input type=\"number\" name=\"target_exc\" min=\"0\" max=\"100\" value=\"92\"><br>\n    <h2>Morning Start(h):</h2><input type=\"number\" name=\"m_start\" min=\"0\" max=\"23\" value=\"6\"><br>\n    <h2>Evening Start(h):</h2><input type=\"number\" name=\"e_start\" min=\"0\" max=\"23\" value=\"18\"><br>\n    <h2>Morning/Evening range(h):</h2><input type=\"number\" name=\"range\" min=\"0\" max=\"23\" value=\"2\"><br>\n    <input type=\"password\" name=\"password\" placeholder=\"Admin Password\"><br>\n    <input type=\"Submit\" value=\"Change\">\n  </form>\n</div>\n<a href=\"./\"><h1 id=\"status\">Home<h1></a>\n</body>\n</html>\n";

//network ID and Key
const char* ssid = "Enter your network name here";
const char* password = "Enter your network password here";
//Initiation of the varibles storing Humidity, Temperature and time,
float hum = 0.0;
float temp = 0.0;
String Time = "";

//Password to be able to change settings for the humidifier (Example: ON/OFF, Target and mode)
String pswd = "x";

//The offset at which the humidifer goes over the target when on
float offset = 5.0;

//The standard values for the Day/night cycle mode, 6-8 & 18-20 target is 97% humidity, otherwise 83%
int mode2values[] = {6,18,2};
float mode2mist[] = {83.0, 97.0};

//Arry determining what mode is currently active, only one should be true at the time
bool mist_mode[] = {false,false,true};

HTTPClient http;

//The target for static mode
float Target = 85.0;

bool running_mist = false;

//Initiating DHT (humidity and Temperature sensor)
#define DHTTYPE DHT21
#define DHTPIN D6
DHT dht(DHTPIN, DHTTYPE);

//Initiating Webb server on port 80 (Standard port in HTTP)
ESP8266WebServer server(80);

//Below is what happens when user access root of website or the url "/"
void handleRoot() {
  //We replace the placeholder values with actual values in index
  String humidity_S = indexhtml;
  humidity_S.replace("Humidityx", String(hum));
  humidity_S.replace("TempX", String(temp));
  if(mist_mode[0]){
    humidity_S.replace("TargetX", "None");
  }else{
    humidity_S.replace("TargetX", String(Target));
  }
  humidity_S.replace("TimeX", Time);
  //when mister is running we write running and give it class is_running (this makes background green)
  if(running_mist){
    humidity_S.replace("runningx", "is_running");
    humidity_S.replace("Not Running", "Running");
  }
  //Below we write out the settings for the active mode.
  if(mist_mode[0]){
    humidity_S.replace("Modex", "Static-Mode");
    if(running_mist){
      humidity_S.replace("Settingsx", "ON");
    }else{
      humidity_S.replace("Settingsx", "OFF");
    }
  }else if(mist_mode[1]){
    humidity_S.replace("Modex", "Targe-Mode");
    humidity_S.replace("Settingsx", String(Target)+"%");
  }else{
    humidity_S.replace("Modex", "24h Cycle-mode");
    String tempsetting = "Start-Times: "+String(mode2values[0]) +" and "+ String(mode2values[1])+"<br>Range: "+String(mode2values[2])+"<br>Target(s): "+String(mode2mist[0])+"%("+mode2mist[1]+"%)";
    humidity_S.replace("Settingsx", tempsetting);
  }
  //We send the code as a 200 response (successfull response) of type html
  server.send(200, "text/html", humidity_S);
}

//When not found we print out a 404 screen simply listing all arguments for easier debugging
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void) {
  //Set D4 to output (This pin controls the relay)
  pinMode(D4, OUTPUT);
  Serial.begin(115200);
  //Inintate wifi connection
  WiFi.mode(WIFI_STA);
  WiFi.hostname("test");
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  //When connected print IP address to serial
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  //add listener for calls to "/", sends calls to function handleRoot
  server.on("/", handleRoot);
  //when calls are sent to "/settings" we simply respond with the static settings page
  server.on("/Settings", []() {
    server.send(200, "text/html",  settingshtml);
  });

//Below are the calls to change settings to static
  server.on("/landing_static", []() {
  //We check if password provided is correct, if not we send a 302 redirect response to the Settings page
    if(server.arg(1) != pswd){
      server.sendHeader("Location", String("/Settings"), true);
      server.send ( 302, "text/plain", "");
      return;
    }
//We make sure the first element of the mode array is true
    mist_mode[0] = true;
    mist_mode[1] = false;
    mist_mode[2] = false;
//depending on the request we either turn on mister or off
    if(server.arg(0) == "ON"){
      running_mist = true;
    }else{
      running_mist = false;
    }
//Send a 302 redirect back to index
    server.sendHeader("Location", String("/"), true);
    server.send ( 302, "text/plain", "");
  });

//Below is listener to change to target mode and its settings
  server.on("/landing_target", []() {
//Checks password
    if(server.arg(1) != pswd){
      server.sendHeader("Location", String("/Settings"), true);
      server.send ( 302, "text/plain", "");
      return;
    }
//change the mode to make sure its correct
    mist_mode[0] = false;
    mist_mode[1] = true;
    mist_mode[2] = false;

//Takes the target and converts to float in order to be able to do mathematics with it
    Target = server.arg(0).toFloat();

//redirects back to Index with a 302
    server.sendHeader("Location", String("/"), true);
    server.send ( 302, "text/plain", "");
  });
//listen for request to change to 24h cycle mode
  server.on("/landing_cycle", []() {
//check password
    if(server.arg(5) != pswd){
      server.sendHeader("Location", String("/Settings"), true);
      server.send ( 302, "text/plain", "");
      return;
    }
//enables correct mode
    mist_mode[0] = false;
    mist_mode[1] = false;
    mist_mode[2] = true;

//Saves all the values in thir correct slot and arry
    mode2mist[0] = server.arg(0).toFloat();
    mode2mist[1] = server.arg(1).toFloat();

    mode2values[0] = server.arg(2).toInt();
    mode2values[1] = server.arg(3).toInt();
    mode2values[2] = server.arg(4).toInt();
//send back to index with a 302
    server.sendHeader("Location", String("/"), true);
    server.send ( 302, "text/plain", "");
  });
//Listener for all outer request, forward response to fucktion handleNotFound
  server.onNotFound(handleNotFound);
//Starts the server
  server.begin();
  Serial.println("HTTP server started");
//Starts the humidity and temperature sensor
  dht.begin();
}

//varible to make sure updates are not too frequent
unsigned long lastcheck = 0;

void loop(void) {
  server.handleClient();
 //If it has been 10 seconds since last check it does anotherone
  unsigned long timecheck = millis();
    if(timecheck - lastcheck >= 10000){
      lastcheck = timecheck;
      //reads temp and humidity
      hum = dht.readHumidity();
      temp = dht.readTemperature();

      //calls the public time API available at worldclockapi.com
      http.begin("http://worldclockapi.com/api/jsonp/cet/now?callback=mycallback");
      int httpCode = http.GET();
      String payload = http.getString();
      //Extracts the part of the response of the call that is the TIME (TODO: convert into json and extract that way, more rubus)
      int start = payload.indexOf("currentDateTime")+29;
      Time = payload.substring(start, start+5);
      http.end();
    }
    //If the mister should be running we set D4 to high, this closes the relay
    if(running_mist){
      digitalWrite(D4, HIGH);
     }else{
      digitalWrite(D4, LOW);
     }
    //Below we check if the mister has met its target (not if mode is static since it doesnt go off target then)
    if(!mist_mode[0]){
      if(running_mist){
        //turns off the mister if its above the target offset (offset is set to 5 by default, this so it will go a bit over the target)
        if(hum >=Target +offset || hum >= 98){
          running_mist = false;
        }
      }else{
        //turn on mister if its below target
        if(hum<Target){
          running_mist = true;
        }
      }
    }

    //If its 24h cycle mode we check what target should be active with this function
    if(mist_mode[2]){
      get_target();
    }
}

void get_target(){
  //Only check the hour (ignoring what minute it is)
  String currentt = Time.substring(0, 2);
  int timecomp = currentt.toInt();
  //If time is within either of the starting times and its range we return set target to the second setting (usually higher humidity)
  for(int i = 0; i < 2; i++){
       if(mode2values[i] + mode2values[2] >= timecomp && mode2values[i] <= timecomp){
          Target = mode2mist[1];
          return;
        }
  }
  //If not we set Target to the standard humidity
  Target = mode2mist[0];
  return;
}
