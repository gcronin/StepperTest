#define stepPin 7
#define dirPin 6
#define enablePin 5
#define stepPulseWidthInMicroSec 2
#define setupTimeInMicroSec 1

boolean currentDirection = false;

void setCurrentDirection(boolean dir)
{
  if(dir == false)
  {
      digitalWrite(dirPin, LOW);
  } else {
      digitalWrite(dirPin, HIGH);
  }
  currentDirection = dir;
  delayMicroseconds(setupTimeInMicroSec);
}

void changeDirection()
{
  setCurrentDirection(!currentDirection);
}

void enableStepper(int isEnabled)
{
  if(isEnabled)
  {
      digitalWrite(enablePin, LOW); // enable HIGH = stepper driver OFF
  } else {
      digitalWrite(enablePin, HIGH); // enable HIGH = stepper driver OFF
  }
  // wait a few microseconds for the enable to take effect 
  // (That isn't in the spec sheet I just added it for sanity.) 
  delayMicroseconds(2);
}

void takeSingleStep()
{
    digitalWrite(stepPin, LOW);
    delayMicroseconds(stepPulseWidthInMicroSec); 
    digitalWrite(stepPin, HIGH); 
    delayMicroseconds(stepPulseWidthInMicroSec); 
    digitalWrite(stepPin, LOW);
}

void setup() 
{
  // We set the enable pin to be an output 
  pinMode(enablePin, OUTPUT); // then we set it HIGH so that the board is disabled until we 
  pinMode(stepPin, OUTPUT); 
  pinMode(dirPin, OUTPUT);
  
  // get into a known state. 
  enableStepper(false);
  // we set the direction pin in an arbitrary direction.
  setCurrentDirection(false);
  
  enableStepper(true);
// we set the direction pin in an arbitrary direction. 
  setCurrentDirection(true);
}

void loop()
{
   takeSingleStep();
   delay(1000);
}
