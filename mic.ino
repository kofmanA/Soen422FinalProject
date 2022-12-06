// can use the analog or digital values to determine if a certain decibel treshold has been reached


int sound_digital = 0;
int sound_analog = 4;

void setup(){
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(sound_digital, INPUT);  
}

void loop(){
  digitalWrite (led, HIGH);
  int val_digital = digitalRead(sound_digital);
  int val_analog = analogRead(sound_analog);

  Serial.print(val_analog);
  Serial.print("\t");
  Serial.println(val_digital);

  if (val_analog >= 2165) // checking if the analog value is over 2165 (decibels i think?)
  {
    digitalWrite (LED_BUILTIN, HIGH);
    delay(3000);
    Serial.println("Game started!");
    }
  else
  {
    digitalWrite (LED_BUILTIN, LOW);
    }
}
