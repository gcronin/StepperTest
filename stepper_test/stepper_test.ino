#define stepPinX 3
#define dirPinX 2

#define enablePinXYZ 4

boolean currentDirectionX = false;

int incomingByte = 0;   // for incoming serial data


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



void changeDirectionX()
{
  setCurrentDirectionX(!currentDirectionX);
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


void setup() 
{

  pinMode(enablePinXYZ, OUTPUT); 

  // Setup X motor 
  pinMode(stepPinX, OUTPUT); 
  pinMode(dirPinX, OUTPUT);
  Serial.begin(38400);
  Serial.println("Press A for Out");
  Serial.println("Press B for In");
  

  
  
  // get into a known state. 
  enableStepperXYZ(false);
  
  // we set the direction pin in an arbitrary direction.
  setCurrentDirectionX(false);

  
  enableStepperXYZ(true);
  
  // we set the direction pin in an arbitrary direction. 
  setCurrentDirectionX(true);


  Serial.begin(38400);
}

void loop()
{
    if (Serial.available() > 0) 
  {
     // read the incoming byte:
     incomingByte = Serial.read();

    if(incomingByte == 65) {//  A pressed
           setCurrentDirectionX(true);
           for(int i=0; i<200; i++) {takeSingleStepX();}
       }
     else if(incomingByte == 66) { //  B pressed
           setCurrentDirectionX(false);
           for(int i=0; i<200; i++) {takeSingleStepX();}
       
      }
  }   

}


