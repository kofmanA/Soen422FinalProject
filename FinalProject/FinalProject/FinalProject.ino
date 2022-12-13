#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include "Ubidots.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Can ctrl+F TODO1 to find where the code must be implemented.
//TODO1: Integrate the microphone into the start of the game (needs to read a value before the game starts).
//TODO2: Implement the buzzer (make a short noise every time the user inputs and a long noise when the game has finished. Can also make a beep for each light turning on during the startup animation).
//TODO3: Implement printing to the TTGO ex. While waiting for the user to log in, print "Find instructions at 172....", at the end of the game display their score as well as their high score.

//OLED pins
#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

//Pins for LEDs and Buttons.

//OLD PINS
//int redLEDPin = 15;
//int yellowButtonPin = 16;

int redLEDPin = 5;
int yellowButtonPin = 19;
int yellowLEDPin = 17;
int greenLEDPin = 21;
int blueLEDPin = 14;
int redButtonPin = 2;
int greenButtonPin = 13;
int blueButtonPin = 12;

int micDigitalOut = 0;
int micAnalogOut = 37;


int buzzerPin = 27;

int currentUserHighScore = 0;



//Login information to connect to wifi.

// Alex's Hotspot
// char* ssid = "Perlman Residence";
// const char* password = "18160466";

//Max's Login information to connect to wifi.
char* ssid = "Can you see me";
const char* password = "testtest";

//Score float initialized to 0.0.
float score = 0.0;

//Creation of difficulty global variable.
float difficulty;

//Set default variable level to 1.
int level = 1;

//Allocate memory for global arrays to track the values of the level and user input.
int levels[25];
int userInput[25];

//Global variable to track the amount of values that a user has enterred.
int userInputCount = 0;

//Ubidots set up.
const char* ubidotstoken= "BBFF-YLwPhx8bdcwnTie0AE2tGb22I83xNz";
Ubidots ubidots(ubidotstoken, UBI_HTTP);

//Create a web server object.
WebServer server(80);

//Global variable to track the username of the person that is logged in, initally nobody.
String loggedInUn = "";


//Function gets called when a user tries to access the first page of the server, instructs them of different pages which they can access.
void firstPage(){
  //The following HTML code is returned to the user so that they can know which pages are available to them.
  String content = "<html><body>If you have already played before, go to /signin <br>";
  content += "If you haven't played before, go to /signup <br>";
  content += "If you would like instructions, go to /instructions <br>";
  content += "If you would like to set your difficulty, go to /difficulty<br></body></html>";
  server.send(200, "text/html", content);
}

//Function that gets called by the signup page.
void signUp(){
  String msg;
  //The next code block is only entered if a username has been entered by the user.
  if (server.hasArg("USERNAME")){
    //Changing values enterred on the form into correct format.
    String thisun = server.arg("USERNAME");
    char unarray[thisun.length()+1];
    int arrayln = thisun.length()+1;
    server.arg("USERNAME").toCharArray(unarray, arrayln);
    //Check if the name already exists, if it doesn't, create a new user with a default difficulty of 1.0.
    if(ubidots.get("users", unarray) != ERROR_VALUE){
      ubidots.add(unarray, 1.0);
      ubidots.send("users", unarray);
      loggedInUn = thisun;
      msg = thisun + " has been created in the database!";
    }else{
      msg = "This username is taken, please enter another one.";
    }
    }
  //Html that is returned if the user would like to create a user profile.
  String content = "<html><body><form action='/signup' method='POST'>New User? Enter your name below.<br>";
  content += "User:<input type='text' name='USERNAME' placeholder='Your name:'><br>";
  content += "<input type='submit' name='NewUser' value='Submit'></form>" + msg + "<br>";
  server.send(200, "text/html", content);
}

//Function that runs when a user would like to login.
void signIn(){
  String msg;
  if (server.hasArg("USERNAME")){
    //If the username can be found in the database, set the value of loggedInUn to the value that was enterred, else, display that this username does not exist.
    String thisun = server.arg("USERNAME");
    char unarray[thisun.length()+1];
    int arrayln = thisun.length()+1;
    server.arg("USERNAME").toCharArray(unarray, arrayln);
    float returnVal = ubidots.get("users", unarray);
    if( returnVal != ERROR_VALUE ){
      loggedInUn = thisun;
      msg = "Have fun " + loggedInUn+"!";
     setDifficultyGame();
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


//Function which is used to set the difficulty in ubidots.
void setDifficultyUbidots(){
  String msg;
  char unarray[loggedInUn.length()+1];
  int arrayln = loggedInUn.length()+1;
  loggedInUn.toCharArray(unarray, arrayln);
  if (server.hasArg("Difficulty")){
    String newDifficulty = server.arg("Difficulty");
    //Only accept difficulty values from 1 to 3.
    if(newDifficulty == "1" || newDifficulty == "2" || newDifficulty == "3"){
      //Add the value to a dots depending on the value that was enterred by a user.
      if(newDifficulty == "1"){
        ubidots.add(unarray, 1.0);
      }else if(newDifficulty == "2"){
        ubidots.add(unarray, 2.0);
      }else{
        ubidots.add(unarray, 3.0);
      }
      //Send the dot to ubidots.
      ubidots.send("users", unarray);
    }else{
      //If an invalid value was enterred, notify the user.
      msg = loggedInUn + ", valid values are from 1-3.";
    }
    setDifficultyGame();
  }
  //Html that is returned if the user would like to create another user profile.
  String content = "<html><body><form action='/setDifficultyUbidots' method='POST'>Enter your preferred diffculty level below:<br>";
  content += "Difficulty (1-3):<input type='text' name='Difficulty' placeholder='Difficulty Level'><br>";
  content += "<input type='submit' name='NewUser' value='Submit'></form>" + msg + "<br>";
  server.send(200, "text/html", content);
}


//Sets the difficulty of the game based on what the user has input as their difficulty level. Difficulty level found by getting from ubidots.
void setDifficultyGame(){
  char unarray[loggedInUn.length()+1];
  int arrayln = loggedInUn.length()+1;
  loggedInUn.toCharArray(unarray, arrayln);
   difficulty = ubidots.get("users", unarray);
}


//Instructions HTML page which details how the game is to be played.
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

//Function called at the end of every game to update the high score.
void updateHighScore(int score){
  char unarray[loggedInUn.length()+1];
  int arrayln = loggedInUn.length()+1;
  loggedInUn.toCharArray(unarray, arrayln);

  //If there is already a high score for this player, enter this block to see if it can be replaced.
  if (ubidots.get("HighScores", unarray) != ERROR_VALUE){
      int currScore = static_cast< int > (ubidots.get("HighScores", unarray));   
      //If the score of the game is higher than the score currently on ubidots, it gets updated.
      if(score > currScore){
         currentUserHighScore = score;
        ubidots.add(unarray, static_cast< float > (score));
        ubidots.send("HighScores", unarray);
        Serial.println("Updated");
      }else{
        currentUserHighScore = currScore;
        Serial.println("Smaller");
      }
  }else{
    //If the user does not have a highscore yet, set it to the score that the user just obtained.
    ubidots.add(unarray, static_cast< float > (score));
    ubidots.send("HighScores", unarray);
    Serial.println("High score initialized");
  }
  Serial.println();
}

void setup(void) {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  delay(5000);
  Serial.begin(115200);
  Serial.println(ssid);
  Serial.println(password);

  //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);
  
  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    while(true); // Don't proceed, loop forever
  }

  // Default Message
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0,0);
  display.print("Simon Game");
  display.display();
  
  //Set LED pins to OUTPUT and set them to LOW.
  pinMode(redLEDPin, OUTPUT);
  pinMode(yellowLEDPin, OUTPUT);
  pinMode(greenLEDPin, OUTPUT);
  pinMode(blueLEDPin, OUTPUT);
  digitalWrite(redLEDPin, LOW);
  digitalWrite(greenLEDPin, LOW);
  digitalWrite(blueLEDPin, LOW);
  digitalWrite(yellowLEDPin, LOW);

  //Set the button pins to input pins.
  pinMode(redButtonPin, INPUT);
  pinMode(yellowButtonPin, INPUT);
  pinMode(greenButtonPin, INPUT);
  pinMode(blueButtonPin, INPUT);

  pinMode(buzzerPin, OUTPUT);
  
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
  server.on("/difficulty", setDifficultyUbidots);
  server.on("/inline", []() {
  server.send(200, "text/plain", "this works without need of authentification");
  });

  //Server begins
  server.begin();

  Serial.print("Open http://");
  Serial.print(WiFi.localIP());
}

//The loop that runs for the game once the setup is complete.
void loop(void) {

  // Default Message
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1.9);
  display.setCursor(0,0);
  display.print("Simon Game");
  display.setCursor(0,20);
  display.print("http://");    
  display.print(WiFi.localIP());  
  display.display();
  
  int micGameStart = 0;
  //  int mic_digital;
  int mic_analog;
  bool endWhileLoop = false;
  //We loop until a user has signaled that they are ready to play by 1. Logging in and 2. Making sound near the microphone to start it.
  while(!endWhileLoop){ //TODO1:       
    if(micGameStart != 0){
      if(loggedInUn != "" ) {
        endWhileLoop = true;   
      }      
    }
      
    mic_analog = analogRead(micAnalogOut);            

    if(mic_analog >= 80 ) {
      if(loggedInUn != ""){
        Serial.print("SOUND MADE ");
        Serial.println(mic_analog);      
        micGameStart = 1;        
      }
    }
    ArduinoOTA.handle();
    server.handleClient();
    delay(2);
  }
  if(micGameStart == 1){
    
    Serial.print("Mic Game STart?: ");
    Serial.println(micGameStart);
    Serial.print("Logged In User:");
    Serial.println(loggedInUn);
    
    //Before the user starts playing, the game is initialized.
    Serial.println("Initializing game");
    initializeGame();

    //Print the entire sequence of lights to the Serial Monitor for debugging purposes.
    for(int i= 0; i<25; i++){
      Serial.println(levels[i]);
    }

    //Once the game has been initialized, we can start playing it.
    Serial.println("playing game");
    playGame();

    //Once the game has finished, update the high score based on the level that the user got to.
    updateHighScore(level);

    
      //Display Game over
    display.clearDisplay();
    display.setTextSize(1.5);
    display.setCursor(0,0);
    display.println("Game Over :(");
    display.setCursor(0,20);
    display.print("Your Score: ");
    display.print(level);
    display.setCursor(0,40);
    display.print(loggedInUn);
    display.print("Your High Score: ");
    display.print(currentUserHighScore);  
    display.setCursor(0,0);
    display.display();
    delay(7000);    
    level = 1;  
  
  }
}

//Randomly initialize the levels of the game.
void initializeGame(){
  //The colour of the light is defined by numbers from 1-4
  for(int i=0; i<25; i++){
    levels[i] = floor(random(1,5));
  }
}

//Output x LEDs for duration based on user difficulty where x depends on the level that the user is currently on.
void displayLevel(){
  Serial.println("displayLevel()");
  Serial.print("Here is level: ");

  // DISPLAY LEVEL TTGO SCREEN
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(3);
  display.print("Level: ");
  display.println(level);
  display.setCursor(0,20);
  display.display();  
  
  Serial.println(level);
  userInputCount = 0;
  for(int i=0; i<level; i++){
    if(levels[i]==1){
      digitalWrite(redLEDPin, HIGH);
      delay(1000);
      digitalWrite(redLEDPin, LOW);
      delay(100);
    }else if(levels[i] == 2){
      digitalWrite(greenLEDPin, HIGH);
      delay(1000);
      digitalWrite(greenLEDPin, LOW);
      delay(100);
    }else if(levels[i] == 3){
      digitalWrite(yellowLEDPin, HIGH);
      delay(1000);
      digitalWrite(yellowLEDPin, LOW);
      delay(100);
    }else{
      digitalWrite(blueLEDPin, HIGH);
      delay(1000);
      digitalWrite(blueLEDPin, LOW);
      delay(100);
    }  
  }
}

//Function which waits until a user has input a value to then enter it to the user's array of moves.
void readUserInput(){
  bool enterred = false;
  Serial.println("readUserInput()");

  //Runs until a user has enterred a value. userInputCount is incremented so that the while loop which called this function can stop after the user has input the correct amount of values for this level.
  //Light sequence indicating the start of a game.
  // RED = 1
  // GREEN = 2
  // YELLOW = 3
  // BLUE = 4
  while(!enterred){
    if(digitalRead(redButtonPin)){
      userInput[userInputCount] = 1;
      userInputCount++;
      enterred = true;
      digitalWrite(redLEDPin, HIGH);
      delay(250);
      digitalWrite(redLEDPin, LOW);
      Serial.println("Red Button");
    }else if(digitalRead(greenButtonPin)){
      userInput[userInputCount] = 2;
      userInputCount++;
      enterred = true;
      digitalWrite(greenLEDPin, HIGH);
      delay(250);
      digitalWrite(greenLEDPin, LOW);
      Serial.println("Green Button");
    }else if(digitalRead(yellowButtonPin)){
      userInput[userInputCount] = 3;
      userInputCount++;
      enterred = true;
      digitalWrite(yellowLEDPin, HIGH);
      delay(250);
      digitalWrite(yellowLEDPin, LOW);
      Serial.println("yellow Button");
    }else if(digitalRead(blueButtonPin)){
      userInput[userInputCount] = 4;
      userInputCount++;
      enterred = true;
      digitalWrite(blueLEDPin, HIGH);
      delay(250);
      digitalWrite(blueLEDPin, LOW);
      Serial.println("Blue Button");
    }
  }
  
}

//Function which describes how a level is played.
void playLevel(){
  Serial.println("playLevel()");
  //First the level is displayed to the user.
  displayLevel();
  //Then, we read the user input until the user has input enough values for this level.
  while(userInputCount < level){
    Serial.print("userInputCount: ");
    Serial.println(userInputCount);
    Serial.print("Level: ");
    Serial.println(level);
    readUserInput();
  }  
}

//Function which verifies if the user correctly input the sequence of LEDs.
bool verifyEntries(){
  Serial.println("verifyEntries()");
  bool valid = true;
  //Loop through the user input array and the level array, compare values, if the values don't line up, set the values to invalid to indicate that the player lost.
  for(int i=0; i<level; i++){
    if(levels[i]!=userInput[i]){
      valid = false;
      Serial.print("User Input : ");
      Serial.println(userInput[i]);
    }
  }

  //If the user input valid values, increment the level by 1.
  if(valid){
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Level ");
    display.println(level); 
    display.setCursor(0,20);
    display.setTextSize(1.5);
    display.display(); 
    level++ ;
  }  
  return valid;
}

//Function which contains the logic of how the game is played.
void playGame(){
  Serial.println("playGame()");

  //Light sequence indicating the start of a game.
  digitalWrite(redLEDPin, HIGH);
  
  delay(500);
  digitalWrite(yellowLEDPin, HIGH);
  digitalWrite(buzzerPin,HIGH);
  delay(500);
  digitalWrite(buzzerPin,LOW);
  digitalWrite(blueLEDPin, HIGH);
  delay(500);
  digitalWrite(greenLEDPin, HIGH);
  delay(500);
  digitalWrite(redLEDPin, LOW);
  digitalWrite(yellowLEDPin, LOW);
  digitalWrite(blueLEDPin, LOW);
  digitalWrite(greenLEDPin, LOW);
  
  //Initialize a boolean to true so that the while loop below can be enterred for the first level.
  bool correctInput = true;

  //Run the while loop until the player has won the game or they have not input the correct values.
  while(level <=25 && correctInput){
    playLevel();
    correctInput = verifyEntries();
  }

}