const int LEDpin = 13;  //Shows Checksum Errors

//////////////////////////Serial Connection Variables//////////////////////////////////
String inputString = "";  //string from Processing
const int MAX_ARGS = 7; //enable, penup, xbyte1, xbyte2, ybyte1, ybyte2, center/home
int data[MAX_ARGS] = {0,0,0,0,0,0,0};  //enable, penup, xbyte1, xbyte2, ybyte1, ybyte2, center/home
int mode = 0;
boolean errorFlag = false; // raised when checksum error occurs

///////////////////////////FROSTRUDER Relay Setup/////////////////////////////////
const int reliefValve = 10;
const int pressureValve = 4;
bool penDownFlag = false;

/////////////////////////////////STEPPER PINS//////////////////////////////
const int enablePinXYZ = 17;
const int stepPin[3] = {3, 15, 9};  //Arduino Pins for Steppers Steps
const int dirPin[3] = {2, 16, 8};    //Arduino Pins for Steppers Directions
const int limitStop[3] = {19, 18, 14};  //Arduino Pins for Limit Switches

/////////////////////////////////STEPPER VARIABLES//////////////////////////////
int StepperDirection[3] = {0,0,0};   //Variables for X,Y,Z directions
String currentZlocation = "up";
bool SteppersEnabled = true;
bool currentDirectionZ = true;
byte limitReading[3] = {0,0,0};  // Readings of X, Y, Z limit switches
long cumulativeSteps[3] = {0,0,0};  //Summation of the X, Y, Z steps taken
const int stepperSpeed = 3;  // this is the delay between steps in ms
const int totalStepsXdirection = 2850;  //  1/8th stepping
const int totalStepsYdirection = 2400;  // 1/8th stepping



void setup()
{
  pinMode(LEDpin, OUTPUT);
  digitalWrite(LEDpin, LOW);
  Serial.begin(38400);  //setup communications to Processing
  establishContact();  // send a byte to establish contact until receiver responds
  setupFrostruder();
  setupSteppers();
  setupLimitSwitches();
  findHomePosition();
  findCenterPosition();   
  
}

void loop()
{
  getInputString();
  readLimitSwitches();
  resetZAxisZeroPosition();
  FrostingCheckPenPosition();
  completeStepperAction();
  sendOutputString();
  delay(10);
}

//////////////////////////FROSTRUDER FUNCTIONS///////////////////////////////////////////////
void setupFrostruder()
{
  pinMode(reliefValve, OUTPUT);
  digitalWrite(reliefValve, LOW);
  pinMode(pressureValve, OUTPUT);
  digitalWrite(pressureValve, LOW);
}

void FrostingTurnOn()
{
  digitalWrite(pressureValve, HIGH);
  digitalWrite(reliefValve, LOW);
} 
  
void FrostingTurnOff()
{
  digitalWrite(pressureValve, LOW);
  digitalWrite(reliefValve, HIGH);
  delay(500);
  digitalWrite(reliefValve, LOW);
}

void FrostingCheckPenPosition()
{
  if(data[1] == 2 || data[1] == 4) {
      penDownFlag = true;  }
  else if(data[1] == 1 || data[1] == 3) {
      penDownFlag = false;  }
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
   
   if(data[0]) {  //do the following only if steppers are enabled
     PenUpDown(data[1]);
   
     if(data[6]!= 2 && !data[6]) {
       int xPosition = data[2] + (data[3]<<8);
       int yPosition = data[4] + (data[5]<<8);
       moveToPosition(xPosition, yPosition);
     }
     
     if(data[6] == 2) //Go to Home
       findHomePosition();
     else if(data[6]) //Go to Center
       findCenterPosition();
   }
 }
}

//////////////////////////////////CALCULATE and MOVE to NEW POSITION//////////////////////////////
void moveToPosition(int xPosition, int yPosition)
{
  int xSteps = xPosition - cumulativeSteps[0];
  int ySteps = yPosition - cumulativeSteps[1];
  if(xSteps > 0)
    StepperDirection[0] = 1;   // set x-axis to move left
  else
    StepperDirection[0] = 2;   // set x-axis to move right
  if(ySteps > 0)
    StepperDirection[1] = 1;   // set y-axis to move backward
  else
    StepperDirection[1] = 2;   // set y-axis to move forward
  setDirectionsXY();
  if(checkMove(abs(xSteps), abs(ySteps))) { // is move valid?
    if(abs(xSteps) > 10 || abs(ySteps) > 10) {
      if(penDownFlag) {
        FrostingTurnOn(); }}
    moveXY(abs(xSteps), abs(ySteps));
    if(abs(xSteps) > 10 || abs(ySteps) > 10) {
      if(penDownFlag) {
        FrostingTurnOff(); }}
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
        else if(mode == 7) //YBYTE2
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
  byte limitswitches = limitReading[0] + (limitReading[1]<<1) + (limitReading[2]<<2);
  outputString += limitswitches;
  outputString += ",";  //add a comma
  int Xcoor = data[2] + (data[3]<<8);
  outputString += Xcoor;
  outputString += ",";  //add a comma
  int Ycoor = data[4] + (data[5]<<8);
  outputString += Ycoor;
  outputString += ",";  //add a comma
  for(int i=0; i<3; i++)
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
    if(currentDirectionZ)  //Stepping up
      ++cumulativeSteps[2];
    else  //Stepping down
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

void takeSingleStepFast(int pin)
{
    digitalWrite(pin, LOW);
    delayMicroseconds(2); 
    digitalWrite(pin, HIGH); 
    delayMicroseconds(1000); 
    digitalWrite(pin, LOW);
    IncrementCumulativeSteps(pin);
}

void takeXSteps(int pin, int num_steps)
{
  for(int i=0; i<num_steps; i++)
    takeSingleStep(pin);
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
    int slope[2], iterationsPerLeftoverStep1, iterationsPerLeftoverStep2, leftoverSteps1, leftoverSteps2;
    
    //VERTICAL LINE
    if(_Xsteps == 0) 
        for(int i=0; i<_Ysteps; i++)  {takeSingleStep(stepPin[1]); }
    
    //HORIZONAL LINE
    else if(_Ysteps == 0)  
        for(int i=0; i<_Xsteps; i++)  {takeSingleStep(stepPin[0]); }
    
    // GENERIC LINE
    else {
      
      /////////////////////////////////////////CALCULATIONS FOR INTERWEAVE////////////////////////////////////////////////////////
      slope[0] = _Xsteps >= _Ysteps ? _Xsteps/_Ysteps : _Ysteps/_Xsteps;
      slope[1] = _Xsteps >= _Ysteps ? _Xsteps%_Ysteps : _Ysteps%_Xsteps;
      if(slope[1] !=0) {
        iterationsPerLeftoverStep1 = _Xsteps >= _Ysteps ? (_Ysteps/slope[1]) + 1 : (_Xsteps/slope[1]) + 1;
        leftoverSteps1 = _Xsteps >= _Ysteps ? slope[1] - _Ysteps/iterationsPerLeftoverStep1 : slope[1] - _Xsteps/iterationsPerLeftoverStep1;
        if(leftoverSteps1 != 0) {
          iterationsPerLeftoverStep2 = _Xsteps >= _Ysteps ? (_Ysteps/leftoverSteps1) + 1 : (_Xsteps/leftoverSteps1) + 1;
          leftoverSteps2 = _Xsteps >= _Ysteps ? leftoverSteps1 - _Ysteps/iterationsPerLeftoverStep2 : leftoverSteps1 - _Xsteps/iterationsPerLeftoverStep2;
        }
        else
          iterationsPerLeftoverStep2, leftoverSteps2 = 0;
      }
      else
         iterationsPerLeftoverStep1, iterationsPerLeftoverStep2, leftoverSteps1, leftoverSteps2 = 0;
      
      ////////////////////////////////SIMPLER INTERWEAVE TACKS-ON EXTRA STEPS AT BEGINNING/////////////////////////////////
      if(leftoverSteps2 > (min(_Ysteps, _Xsteps) - slope[1]) ) {  
          
         for(int i=0; i<min(_Ysteps, _Xsteps); i++) //number of iterations
         {
           takeSingleStep(_Xsteps >= _Ysteps ? stepPin[1] : stepPin[0] ); 
           for(int j=0; j<slope[0]; j++)
               takeSingleStep(_Xsteps >= _Ysteps ? stepPin[0] : stepPin[1]);
           if(i < slope[1])
               takeSingleStep(_Xsteps >= _Ysteps ? stepPin[0] : stepPin[1]);
          }
      }
      ///////////////////////////////////////MORE COMPLEX INTERWEAVE ADDS EXTRA STEPS THROUGHOUT//////////////////////////////
      else  
      {
        for(int i=0; i<min(_Ysteps, _Xsteps); i++) { //number of iterations
          takeSingleStep(_Xsteps >= _Ysteps ? stepPin[1] : stepPin[0]); 
          for(int j=0; j<slope[0]; j++)
              takeSingleStep(_Xsteps >= _Ysteps ? stepPin[0] : stepPin[1]);
          if(i%iterationsPerLeftoverStep1 == 0 && (min(_Ysteps, _Xsteps) - i) >= iterationsPerLeftoverStep1 )
              takeSingleStep(_Xsteps >= _Ysteps ? stepPin[0] : stepPin[1]);
          if(i%iterationsPerLeftoverStep2 == 0 && (min(_Ysteps, _Xsteps) - i) >= iterationsPerLeftoverStep2)
              takeSingleStep(_Xsteps >= _Ysteps ? stepPin[0] : stepPin[1]);
          if(i < leftoverSteps2)
              takeSingleStep(_Xsteps >= _Ysteps ? stepPin[0] : stepPin[1]);
        }
      }
    }  
}

void PenUpDown(int penlocation)
{
  long zsteps = 45000;
  if(penlocation == 4 && currentZlocation == "up")  //Pen DOWN, currently up
  {
    if(currentDirectionZ)
      changeDirectionZ();
    for(long j=0; j<zsteps; j++)
            takeSingleStepFast(stepPin[2]);
    currentZlocation = "down";
  }
  else if(penlocation == 3 && currentZlocation == "down")  //Pen UP, currently down
  {
    if(!currentDirectionZ)
      changeDirectionZ();
    for(long j=0; j<zsteps; j++)
            takeSingleStepFast(stepPin[2]);
    currentZlocation = "up";
  }
  else if(penlocation == 1 && !currentDirectionZ)  
  {
    changeDirectionZ();
    takeXSteps(stepPin[2], 10);
  }
  else if(penlocation == 1)
  {
    takeXSteps(stepPin[2], 10);
  }
  else if(penlocation == 2 && currentDirectionZ) 
  { 
    changeDirectionZ();
    takeXSteps(stepPin[2], 10);
  }
  else if(penlocation == 2)
  {
    takeXSteps(stepPin[2], 10);
  }
}

///////////////////////////////CHECK MOVE IS IN  SIDE BOUNDARIES//////////////////////////
bool checkMove(int _Xsteps, int _Ysteps)
{
      if(StepperDirection[0] == 1 && (_Xsteps+cumulativeSteps[0]) > totalStepsXdirection)   //Going Left and move will exceed  
	return 0;
      else if(StepperDirection[0] == 2 && _Xsteps > cumulativeSteps[0] )   //Going right and move will exceed  
	return 0;
     else if(StepperDirection[1] == 1 && (_Ysteps+cumulativeSteps[1]) > totalStepsYdirection)   //Going backward and move will exceed  
	return 0;
      else if(StepperDirection[1] == 2 && _Ysteps > cumulativeSteps[1] )   //Going forward and move will exceed  
	return 0;
      return 1;
}

////////////////////////////////FIND HOME POSITION///////////////////////////////////////
bool findHomePosition()
{
  readLimitSwitches();
  StepperDirection[0] = 2;   // set x-axis to move right
  StepperDirection[1] = 2;   // set y-axis to move forward
  setDirectionsXY();
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
		StepperDirection[0] = 1;   // set x-axis to move left
	else
		StepperDirection[0] = 2;   // set x-axis to move right
	if(ySteps > 0)
		StepperDirection[1] = 1;   // set y-axis to move backward
	else
		StepperDirection[1] = 2;   // set y-axis to move forward
        setDirectionsXY();	
        moveXY(abs(xSteps), abs(ySteps));
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

void resetZAxisZeroPosition()
{
  if(limitReading[2] == 0)
    cumulativeSteps[2] = 0;  //reset Z-axis counter
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
