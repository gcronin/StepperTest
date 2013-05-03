const int LEDpin = 13;  //Shows Checksum Errors

//////////////////////////Serial Connection Variables//////////////////////////////////
String inputString = "";  //string from Processing
const int MAX_ARGS = 7; //enable, penup, xbyte1, xbyte2, ybyte1, ybyte2, center/home
int data[MAX_ARGS] = {0,0,0,0,0,0,0};  //enable, penup, xbyte1, xbyte2, ybyte1, ybyte2, center/home
int mode = 0;
boolean errorFlag = false; // raised when checksum error occurs

/////////////////////////////////STEPPER PINS//////////////////////////////
const int enablePinXYZ = 17;
const int stepPin[3] = {3, 15, 9};  //Arduino Pins for Steppers Steps
const int dirPin[3] = {2, 16, 8};    //Arduino Pins for Steppers Directions
const int limitStop[3] = {19, 18, 14};  //Arduino Pins for Limit Switches

/////////////////////////////////STEPPER VARIABLES//////////////////////////////
int StepperDirection[3] = {0,0,0};   //Variables for X,Y,Z directions
int StepperSteps[3] = {0,0,0};  //Variables for number of steps in X, Y, Z directions
String currentZlocation = "down";
bool SteppersEnabled = true;
bool currentDirectionZ = false;
int limitReading[3] = {0,0,0};  // Readings of X, Y, Z limit switches
int cumulativeSteps[3] = {0,0,0};  //Summation of the X, Y, Z steps taken
const int stepperSpeed = 1;  // this is the delay between steps in ms
const int totalStepsXdirection = 2844;  //  1/8th stepping
const int totalStepsYdirection = 2442;  // 1/8th stepping



void setup()
{
  pinMode(LEDpin, OUTPUT);
  digitalWrite(LEDpin, LOW);
  Serial.begin(38400);  //setup communications to Processing
  setupSteppers();
  setupLimitSwitches();
  findHomePosition();
  findCenterPosition();   
  cumulativeSteps[0] = totalStepsXdirection/2; //patch is necessary because these are not getting set correctly in findCenterPosition()... 
  cumulativeSteps[1] = totalStepsYdirection/2;
  establishContact();  // send a byte to establish contact until receiver responds
}

void loop()
{
  sendOutputString();
  getInputString();
  readLimitSwitches();
  completeStepperAction();
  delay(10);
}
  
////////////////////////////MOVE, LIFT, ENABLE////////////////////////////////////////////////
void completeStepperAction()
{
 if(!errorFlag)
 { 
   if(data[0]) //Enable Steppers
     enableStepperXYZ(true);
   else //Disable Steppers
     enableStepperXYZ(false);
   
   PenUpDown(data[1]);
   
   if(data[6] == 2) //Go to Home
     findHomePosition();
   else if(data[6]) //Go to Center
     findCenterPosition();
 }
}

  
  
///////////////////////////////////Establish Initial Serial Connection/////////////////////////////  
void establishContact() {
  while (Serial.available() <= 0) {
    Serial.println("0,0,0,0,0,0");   // send an initial string
    delay(300);
  }
}

////////////////////////////////////////READ in SERIAL STRING//////////////////////////////////////  
void getInputString()
{
  digitalWrite(LEDpin, LOW);
  int checksumRead = 0;
  int checksumCalculated = 0;
  while(Serial.available() > 0)
  {
       // We need to 0xFF at start of packet
       if(mode == 0)  // start of new packet
       {         
         if(Serial.read() == 0xFF)  //HANDSHAKE #1
         {
            mode = 1;
          }
       }
       else if(mode == 1)
       {
         if(Serial.read() == 0xFE)  //HANDSHAKE #2
         {
            mode = 2;
         }
       }
       else if(mode == 2) //ENABLE
        {   
            data[0] = Serial.read();
            mode = 3;
        }
        else if(mode == 3) //PEN UP/DOWN
        {   
            data[1] = Serial.read();
            mode = 4;
        }
        else if(mode == 4) //XBYTE1
        {   
            data[2] = Serial.read();
            mode = 5;
        }
        else if(mode == 5) //XBYTE2
        {   
            data[3] = Serial.read();
            mode = 6;   
        }  
        else if(mode == 6) //YBYTE1
        {   
            data[4] = Serial.read();
            mode = 7;   
        }
        else if(mode == 7) //YBYTE1
        {
          data[5] = Serial.read();
          mode = 8;
        } 
        else if(mode == 8) //CENTER/HOME
        {
          data[6] = Serial.read();
          mode = 9;
        }  
        else if(mode == 9) //CHECKSUM
        {
          checksumRead = Serial.read();
          mode = 0;
        }   
    }
    for(int i=0; i<MAX_ARGS; i++)  
      checksumCalculated += data[i];
    checksumCalculated = checksumCalculated%255;
    if(checksumRead != checksumCalculated) {
      errorFlag = true;
      digitalWrite(LEDpin, HIGH);
    }
    else errorFlag = false;
}


////////////////////////////////////////SEND a SERIAL STRING////////////////////////////////////// 
void sendOutputString()
{
  String outputString = "";
  for(int i=0; i<3; i++)
  {
    outputString += limitReading[i];  //add the next data point
    outputString += ",";  //add a comma
  }
  for(int i=0; i<MAX_ARGS; i++)
  {
    outputString += cumulativeSteps[i];  //add the next data point
    outputString += ",";  //add a comma
  }
  Serial.println(outputString);
}

///////////////////////////////////////////STEPPER CONTROL FUNCTIONS////////////////////////////////////////////////////

//////////////////////////////DIRECTION CONTROLS//////////////////////////////////////
void setCurrentDirection(boolean dir, int pin)
{
  if(dir == false)
  {
      digitalWrite(pin, LOW);
  } else {
      digitalWrite(pin, HIGH);
  }
  delayMicroseconds(1);
}

void setCurrentDirectionZ(boolean dir)
{
  if(dir == false)
  {
      digitalWrite(dirPin[2], LOW);
  } else {
      digitalWrite(dirPin[2], HIGH);
  }
  currentDirectionZ = dir;
  delayMicroseconds(1);
}

void changeDirectionZ()
{
  setCurrentDirectionZ(!currentDirectionZ);
}
//////////////////////////////ENABLE CONTROLS///////////////////////////////////////
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

void IncrementCumulativeSteps(int pin)
{
  if(pin == stepPin[0])  //X axis
  {
    if(StepperDirection[0] == 1)  //Stepping to the left
      ++cumulativeSteps[0];
    else if(StepperDirection[0] == 2) //Stepping to the right
      --cumulativeSteps[0];
  }
  else if(pin == stepPin[1])  //Y axis
  {
    if(StepperDirection[1] == 1) //Stepping to the back
      ++cumulativeSteps[1];
    else if(StepperDirection[1] == 2) //Stepping to the front
      --cumulativeSteps[1];
  }
  else if(pin == stepPin[2]) //Z axis
  {
    if(currentZlocation == "up")
      ++cumulativeSteps[2];
    else if(currentZlocation == "down") //Stepping to the front
      --cumulativeSteps[2];
  }
}


//////////////////////////////STEP CONTROLS///////////////////////////////////////
void takeSingleStep(int pin)
{
    digitalWrite(pin, LOW);
    delayMicroseconds(2); 
    digitalWrite(pin, HIGH); 
    delayMicroseconds(1000); 
    digitalWrite(pin, LOW);
    delay(stepperSpeed);
    IncrementCumulativeSteps(pin);
}

void setDirectionsXY()
{
      for(int i=0; i<2; i++)   //do this for each stepper motors X and Y
      {
        if(StepperDirection[i] == 1) {setCurrentDirection(true, dirPin[i]);}   //move left(X) or back(Y)
        else if(StepperDirection[i] == 2) {setCurrentDirection(false, dirPin[i]);}  //move right(X) or front(Y)
      }
}

void moveXY(int _Xsteps, int _Ysteps) //This function includes interweave so X and Y motors move at the same time
{
  int slope[2]; 
  if(_Xsteps > _Ysteps)
   {
    slope[0] = _Xsteps/_Ysteps;
    slope[1] = _Xsteps%_Ysteps;
   }
   else
   {
    slope[0] = _Ysteps/_Xsteps;
    slope[1] = _Ysteps%_Xsteps;
   }
   if(_Xsteps == 0) 
    {
      for(int i=0; i<_Ysteps; i++)  {takeSingleStep(stepPin[1]); }
    }
    else if(_Ysteps == 0)
    {
      for(int i=0; i<_Xsteps; i++)  {takeSingleStep(stepPin[0]); }
    }
    else if(_Xsteps > _Ysteps)
    {
      for(int i=0; i<_Ysteps; i++) 
      {
        takeSingleStep(stepPin[0]); 
        for(int j=0; j<slope[0]; j++)
            takeSingleStep(stepPin[1]);
        if(i < slope[1])
            takeSingleStep(stepPin[0]);
      }
    }
    else
    {
      for(int i=0; i<_Xsteps; i++) 
      {
        takeSingleStep(stepPin[1]); 
        for(int j=0; j<slope[0]; j++)
            takeSingleStep(stepPin[0]);
        if(i < slope[1])
            takeSingleStep(stepPin[1]);
      }
    }
}

void PenUpDown(int penlocation)
{
  int zsteps = 1200;
  if(penlocation == 0 && currentZlocation == "up")  //Pen DOWN, currently up
  {
    changeDirectionZ();
    for(int j=0; j<zsteps; j++)
            takeSingleStep(stepPin[2]);
    currentZlocation = "down";
  }
  else if(penlocation == 1 && currentZlocation == "down")  //Pen UP, currently down
  {
    changeDirectionZ();
    for(int j=0; j<zsteps; j++)
            takeSingleStep(stepPin[2]);
    currentZlocation = "up";
  }
}

///////////////////////////////CHECK MOVE IS INSIDE BOUNDARIES//////////////////////////
bool checkMove()
{
      if(StepperDirection[0] == 1 && (StepperSteps[0]+cumulativeSteps[0]) > totalStepsXdirection)   //Going Left and move will exceed  
	return 0;
      else if(StepperDirection[0] == 2 && StepperSteps[0] > cumulativeSteps[0] )   //Going right and move will exceed  
	return 0;
     else if(StepperDirection[1] == 1 && (StepperSteps[1]+cumulativeSteps[1]) > totalStepsYdirection)   //Going backward and move will exceed  
	return 0;
      else if(StepperDirection[1] == 2 && StepperSteps[1] > cumulativeSteps[1] )   //Going forward and move will exceed  
	return 0;
      return 1;
}

////////////////////////////////FIND HOME POSITION///////////////////////////////////////
bool findHomePosition()
{
  readLimitSwitches();
  setCurrentDirection(false, dirPin[1]);  // set y-axis to move forward
  setCurrentDirection(false, dirPin[0]);  // set x-axis to move right
  while(limitReading[1] || limitReading[0])  //X or Y stop is NOT pressed... limit switches read 0 when pressed
  {
    if(limitReading[1])  //Y is NOT pressed... move Y stepper
      takeSingleStep(stepPin[1]);
    if(limitReading[0])  //X is NOT pressed... move X stepper
      takeSingleStep(stepPin[0]);
    readLimitSwitches();
  }
  for(int i=0; i<2; i++)  //reset the counter for X and Y steppers
    cumulativeSteps[i] = 0;
  return 1;
}
  
///////////////////////////////////FIND CENTER POSITION////////////////////////////////////////
bool findCenterPosition()
{
	int xSteps = totalStepsXdirection/2 - cumulativeSteps[0];
	int ySteps = totalStepsYdirection /2- cumulativeSteps[1];
	if(xSteps > 0)
		setCurrentDirection(true, dirPin[0]);  //move to left
	else
		setCurrentDirection(false, dirPin[0]);  //move to right
	if(ySteps > 0)
		setCurrentDirection(true, dirPin[1]);  //move back
	else
		setCurrentDirection(false, dirPin[1]);  //move forward
	moveXY(xSteps, ySteps);
}
  
  
////////////////////////////////LIMIT SWITCHES//////////////////////////////////////////
void setupLimitSwitches()
{
  for(int i = 0; i < 3; i++)
  {
    pinMode(limitStop[i], INPUT);
    digitalWrite(limitStop[i], HIGH);  //enable pullup resistor
  }
}

void readLimitSwitches()
{
  for(int i = 0; i < 3; i++)
    limitReading[i] = digitalRead(limitStop[i]);

}

////////////////////////////////SETUP STEPPERS///////////////////////////////////////////
void setupSteppers()
{
  pinMode(enablePinXYZ, OUTPUT);
  
  // Setup X motor 
  pinMode(stepPin[0], OUTPUT); 
  pinMode(dirPin[0], OUTPUT);
  
    // Setup Y motor 
  pinMode(stepPin[1], OUTPUT); 
  pinMode(dirPin[1], OUTPUT);
  
    // Setup Z motor 
  pinMode(stepPin[2], OUTPUT); 
  pinMode(dirPin[2], OUTPUT);
  
  // get into a known state. 
  enableStepperXYZ(false);
  // we set the direction pin in an arbitrary direction.
  for(int i = 0; i<3; i++) setCurrentDirection(false, dirPin[i]);
  
  enableStepperXYZ(true);
  // we set the direction pin in an arbitrary direction. 
  for(int i = 0; i<3; i++) setCurrentDirection(true, dirPin[i]);
}
