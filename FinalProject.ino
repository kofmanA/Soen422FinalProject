#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include "Ubidots.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

//Initialization of global variables such as ssid and password so they can be used multiple times. IncorrectPassword and lockedOut are used to track how many incorret passwords have
//been entered. The username and password accepted by the server are also initialized here.
int incorrectPassword;
bool lockedOut;
const char* ssid = "Can you see me";
const char* password = "testtest";
const char* ubidotstoken= "BBFF-YLwPhx8bdcwnTie0AE2tGb22I83xNz";
Ubidots ubidots(ubidotstoken, UBI_HTTP);
WebServer server(80);
String loggedInUn;

//The html code returned below was taken from an example found below:
//https://github.com/wemos/Arduino_ESP8266/blob/master/libraries/ESP8266WebServer/examples/SimpleAuthentification/SimpleAuthentification.ino

//Function gets called when a user tries to access the login page of the server.
void firstPage(){
  //The following HTML code is returned to the user so that they can know which pages are available to them.
  String content = "<html><body>If you have already played before, go to /signin <br>";
  content += "If you haven't played before, go to /signup <br>";
  content += "If you would like instructions, go to /instructions <br>";
  content += "If you would like to set your difficulty, go to /difficulty<br></body></html>";
  server.send(200, "text/html", content);
}

void signUp(){
  String msg;
  //The next code block is only entered if a username and password have been entered by the user.
  if (server.hasArg("USERNAME")){
    //If the username and password match the values that we have set above, enter this code block. It will turn the green LED on and turn the servo.
    String thisun = server.arg("USERNAME");
    char unarray[thisun.length()+1];
    int arrayln = thisun.length()+1;
    server.arg("USERNAME").toCharArray(unarray, arrayln);
    if(ubidots.get("users", unarray) != ERROR_VALUE){
      ubidots.add(unarray, 0.0);
      ubidots.send("users", unarray);
      loggedInUn = thisun;
      msg = thisun + " has been created in the database!";
    }else{
      msg = "This username is taken, please enter another one.";
    }
    }
      //Html that is returned if the user would like to create another user profile.
  String content = "<html><body><form action='/signup' method='POST'>New User? Enter your name below.<br>";
  content += "User:<input type='text' name='USERNAME' placeholder='Your name:'><br>";
  content += "<input type='submit' name='NewUser' value='Submit'></form>" + msg + "<br>";
  server.send(200, "text/html", content);
}

void signIn(){
  String msg;
  if (server.hasArg("USERNAME")){
    //If the username and password match the values that we have set above, enter this code block. It will turn the green LED on and turn the servo.
    String thisun = server.arg("USERNAME");
    char unarray[thisun.length()+1];
    int arrayln = thisun.length()+1;
    server.arg("USERNAME").toCharArray(unarray, arrayln);
    float returnVal = ubidots.get("users", unarray);
    if( returnVal != ERROR_VALUE ){
      loggedInUn = thisun;
      msg = "Have fun " + loggedInUn+"!";
    }else{
      msg = "This username does not exist, to create an account, go to /signup.";
    }
    }

  //Html that is returned if the user would like to create another user profile.
  String content = "<html><body><form action='/signin' method='POST'>Enter your name below to login.<br>";
  content += "User:<input type='text' name='USERNAME' placeholder='Your name:'><br>";
  content += "<input type='submit' name='NewUser' value='Submit'></form>" + msg + "<br>";
  server.send(200, "text/html", content);
  
}

void difficulty(){
  String msg;
  char unarray[loggedInUn.length()+1];
  int arrayln = loggedInUn.length()+1;
  loggedInUn.toCharArray(unarray, arrayln);
  //The next code block is only entered if a username and password have been entered by the user.
  if (server.hasArg("Difficulty")){
    //If the username and password match the values that we have set above, enter this code block. It will turn the green LED on and turn the servo.
    String newDifficulty = server.arg("Difficulty");
    if(newDifficulty == "1" || newDifficulty == "2" || newDifficulty == "3"){
      if(newDifficulty == "1"){
        ubidots.add(unarray, 1.0);
      }else if(newDifficulty == "2"){
        ubidots.add(unarray, 2.0);
      }else{
        ubidots.add(unarray, 3.0);
      }
      ubidots.send("users", unarray);
    }else{
      msg = loggedInUn + ", valid values are from 1-3.";
    }
}
      //Html that is returned if the user would like to create another user profile.
  String content = "<html><body><form action='/difficulty' method='POST'>Enter your preferred diffculty level below:<br>";
  content += "Difficulty (1-3):<input type='text' name='Difficulty' placeholder='Difficulty Level'><br>";
  content += "<input type='submit' name='NewUser' value='Submit'></form>" + msg + "<br>";
  server.send(200, "text/html", content);
}





void instructions(){
  //The following HTML code is returned to the user to explain set up instructions as well as how the game works.`  ```````````                     ``                                                  
  String content = "<html><body>The instructions of the game can be found below:<br>";
  content += "1. Sign up, in the case that you have already signed up, sign in. <br>";
  content += "2. The default difficulty is 1, if you would to increase it, go to /difficulty <br>";
  content += "3. Once the steps above are done, the game can be started by clapping near the microphone. <br>";
  content += "<br><br><br> ------------------------------ <br><br><br>";
  content += "How to play: <br>";
  content += "A sequence of lights will be played, once the last light turns on, press the buttons in front of the lights in the same order that the lights played. <br>";
  content += "If you input the sequence correctly, another light will be added to the sequence and it will play again. <br>";
  content += "If you input the sequence incorrectly, the game will end. <br>";
  content += "The maximum score is 50, once a player has reached 50, the game will end automatically.<br></body></html>";
  server.send(200, "text/html", content); 
}

void setup(void) {
  //Initialization of values and setting of pins.
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  incorrectPassword = 0;
  lockedOut=false;
  delay(5000);
  Serial.begin(115200);
  Serial.println(ssid);
  Serial.println(password);
  pinMode(21,OUTPUT);
  pinMode(13, OUTPUT);
  //Set the wifi mode to station and then log in to wifi using wifi.begin. We will print the status of the wifi until the wifi is connected.
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(WiFi.status());
    delay(1000);
  }
  ArduinoOTA.begin();
  Serial.println(WiFi.localIP());

  //Setting the functions that run based on the link that the user enters.
  server.on("/", firstPage);
  server.on("/signin", signIn);
  server.on("/signup", signUp);
  server.on("/instructions", instructions);
  server.on("/difficulty", difficulty);
  server.on("/inline", []() {
  server.send(200, "text/plain", "this works without need of authentification");
  });

  //Server begins
  server.begin();

  Serial.print("Open http://");
  Serial.print(WiFi.localIP());
}

//The loop constatnly handles the client, a 2 ms delay is used to give the CPU a break.
void loop(void) {
  ArduinoOTA.handle();
  server.handleClient();
  delay(2);//allow the cpu to switch to other tasks
}
