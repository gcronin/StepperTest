#define stepPinX 3
#define dirPinX 2
#define stepPinY 15
#define dirPinY 16
#define stepPinZ 9
#define dirPinZ 8
#define enablePinXYZ 17
#define xStop 19
#define yStop 18
#define zStop 14

boolean currentDirectionX = false;
boolean currentDirectionY = false;
boolean currentDirectionZ = false;

int xStopReading;
int yStopReading;
int zStopReading;  

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

void setCurrentDirectionZ(boolean dir)
{
  if(dir == false)
  {
      digitalWrite(dirPinZ, LOW);
  } else {
      digitalWrite(dirPinZ, HIGH);
  }
  currentDirectionZ = dir;
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

void changeDirectionZ()
{
  setCurrentDirectionZ(!currentDirectionZ);
}

void enableStepperXYZ(int isEnabled)
{
  if(isEnabled)
  {
      digitalWrite(enablePinXYZ, LOW); // enable HIGH = stepper driver OFF
  } else {
      digitalWrite(enablePinXYZ, HIGH); // enable HIGH = stepper driver OFF
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

void takeSingleStepZ()
{
    digitalWrite(stepPinZ, LOW);
    delayMicroseconds(2); 
    digitalWrite(stepPinZ, HIGH); 
    delayMicroseconds(1000); 
    digitalWrite(stepPinZ, LOW);
}
void setup() 
{

  pinMode(enablePinXYZ, OUTPUT); 

  // Setup X motor 
  pinMode(stepPinX, OUTPUT); 
  pinMode(dirPinX, OUTPUT);
  
    // Setup Y motor 
  pinMode(stepPinY, OUTPUT); 
  pinMode(dirPinY, OUTPUT);
  
    // Setup Z motor 
  pinMode(stepPinZ, OUTPUT); 
  pinMode(dirPinZ, OUTPUT);
  
  
  // get into a known state. 
  enableStepperXYZ(false);
  
  // we set the direction pin in an arbitrary direction.
  setCurrentDirectionX(false);
  setCurrentDirectionY(false);
  setCurrentDirectionZ(false);
  
  enableStepperXYZ(true);
  
  // we set the direction pin in an arbitrary direction. 
  setCurrentDirectionX(true);
  setCurrentDirectionY(true);
  setCurrentDirectionZ(true);
  
  pinMode(xStop, INPUT);
  digitalWrite(xStop, HIGH);  //enable pullup resistor
  pinMode(yStop, INPUT);
  digitalWrite(yStop, HIGH);  //enable pullup resistor
  pinMode(zStop, INPUT);
  digitalWrite(zStop, HIGH);  //enable pullup resistor

  Serial.begin(38400);
}

void loop()
{
  readLimitSwitches(0);
  
  if(yStopReading == 0) 
  {
    changeDirectionX();
    changeDirectionY(); 
    changeDirectionZ();
    //enableStepperXYZ(false);
    
  }
  else
  {
     enableStepperXYZ(true);
     
  }
  
   delay(1000);
   for(int i=0; i<200; i++) {takeSingleStepY();}
   delay(1000);
   for(int i=0; i<200; i++) {takeSingleStepX();}
   delay(1000);
   for(int i=0; i<200; i++) {takeSingleStepZ();}
}

void readLimitSwitches(bool debug)
{
  xStopReading = digitalRead(xStop);
  yStopReading = digitalRead(yStop);
  zStopReading = digitalRead(zStop);
  if(debug)
  {
    Serial.print("x:");
    Serial.println(xStopReading);
  
    Serial.print("y:");
    Serial.println(yStopReading);
  
    Serial.print("z:");
    Serial.println(zStopReading);
  }
}
