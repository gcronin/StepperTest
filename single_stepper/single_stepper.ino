#define stepPinX 14
#define dirPinX 15
#define enablePinX 16
#define stepPinY 5
#define dirPinY 6
#define enablePinY 7
#define xmin 17
#define xmax 18

boolean currentDirectionX = false;
boolean currentDirectionY = false;

void setCurrentDirectionX(boolean dir)
{
  if(dir == false)
  {
      digitalWrite(dirPinX, LOW);
  } else {
      digitalWrite(dirPinX, HIGH);
  }
  currentDirectionX = dir;
  delayMicroseconds(1);
}

void setCurrentDirectionY(boolean dir)
{
  if(dir == false)
  {
      digitalWrite(dirPinY, LOW);
  } else {
      digitalWrite(dirPinY, HIGH);
  }
  currentDirectionY = dir;
  delayMicroseconds(1);
}

void changeDirectionX()
{
  setCurrentDirectionX(!currentDirectionX);
}

void changeDirectionY()
{
  setCurrentDirectionY(!currentDirectionY);
}

void enableStepperX(int isEnabled)
{
  if(isEnabled)
  {
      digitalWrite(enablePinX, LOW); // enable HIGH = stepper driver OFF
  } else {
      digitalWrite(enablePinX, HIGH); // enable HIGH = stepper driver OFF
  }
  // wait a few microseconds for the enable to take effect 
  // (That isn't in the spec sheet I just added it for sanity.) 
  delayMicroseconds(2);
}

void enableStepperY(int isEnabled)
{
  if(isEnabled)
  {
      digitalWrite(enablePinY, LOW); // enable HIGH = stepper driver OFF
  } else {
      digitalWrite(enablePinY, HIGH); // enable HIGH = stepper driver OFF
  }
  // wait a few microseconds for the enable to take effect 
  // (That isn't in the spec sheet I just added it for sanity.) 
  delayMicroseconds(2);
}

void takeSingleStepX()
{
    digitalWrite(stepPinX, LOW);
    delayMicroseconds(2); 
    digitalWrite(stepPinX, HIGH); 
    delayMicroseconds(1000); 
    digitalWrite(stepPinX, LOW);
}

void takeSingleStepY()
{
    digitalWrite(stepPinY, LOW);
    delayMicroseconds(2); 
    digitalWrite(stepPinY, HIGH); 
    delayMicroseconds(1000); 
    digitalWrite(stepPinY, LOW);
}

void setup() 
{
  // Setup X motor 
  pinMode(enablePinX, OUTPUT); 
  pinMode(stepPinX, OUTPUT); 
  pinMode(dirPinX, OUTPUT);
  
    // Setup Y motor 
  pinMode(enablePinY, OUTPUT); 
  pinMode(stepPinY, OUTPUT); 
  pinMode(dirPinY, OUTPUT);
  
  // get into a known state. 
  enableStepperX(false);
  enableStepperY(false);
  // we set the direction pin in an arbitrary direction.
  setCurrentDirectionX(false);
  setCurrentDirectionY(false);
  
  enableStepperX(true);
  enableStepperY(true);
// we set the direction pin in an arbitrary direction. 
  setCurrentDirectionX(true);
  setCurrentDirectionY(true);
  
  pinMode(xmax, INPUT);
  digitalWrite(xmax, HIGH);  //enable pullup resistor
  //pinMode(xmin, INPUT);
  //digitalWrite(xmin, HIGH);  //enable pullup resistor
  Serial.begin(38400);
}

void loop()
{
  int xmaxreading = digitalRead(xmax);
  Serial.println(xmaxreading);
  if(xmaxreading == 0) 
  {
    changeDirectionX();
    changeDirectionY(); 
    //enableStepperX(false);
     //enableStepperY(false); 
  }
  else
  {
     //enableStepperX(true);
     //enableStepperY(true); 
  }
  
   delay(1000);
   for(int i=0; i<200; i++) {takeSingleStepY();}
   delay(1000);
   for(int i=0; i<200; i++) {takeSingleStepX();}
}
