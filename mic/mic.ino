// can use the analog or digital values to determine if a certain decibel treshold has been reached


int sound_digital = 0;

//int sound_analog = 4;
int sound_analog = 37;


void setup(){
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(sound_analog,INPUT);
  pinMode(sound_digital, INPUT);  
}

int redButtonPin = 2;
int yellowButtonPin = 19;
int greenButtonPin = 13;
int blueButtonPin = 12;


void loop(){
  int val_digital = digitalRead(sound_digital);
  int val_analog = analogRead(sound_analog);

  Serial.print(val_analog);
  Serial.print("\n");
  Serial.println(val_digital);

  if ( val_analog >= 30 ) // checking if the analog value is over 2165 (decibels i think?)
  {
    digitalWrite (LED_BUILTIN, HIGH);
    Serial.println("Game started!");
  }
  else
  {
    digitalWrite (LED_BUILTIN, LOW);
  }
}