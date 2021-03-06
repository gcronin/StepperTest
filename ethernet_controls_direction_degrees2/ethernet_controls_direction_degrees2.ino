#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>

const int maxLength = 50;
const bool debug = 1;  //1 == Serial Connection ON, 0 == Serial Connection OFF
const float degrees_per_step = 1;  //change this as appropriate based on microstepping... full step 1.8, half step 0.9, quarter step 0.45, eighth step 0.225, sixteenth step 0.1125
const int stepperSpeed = 1;  // this is the delay between steps in ms
const int totalStepsXdirection = 2844;  //  1/8th stepping
const int totalStepsYdirection = 2442;  // 1/8th stepping

/////////////////////////////////STEPPER PINS//////////////////////////////
const int enablePinXYZ = 17;
const int stepPin[3] = {3, 15, 9};  //Arduino Pins for Steppers Steps
const int dirPin[3] = {2, 16, 8};    //Arduino Pins for Steppers Directions
const int limitStop[3] = {19, 18, 14};  //Arduino Pins for Limit Switches

/////////////////////////////////STEPPER VARIABLES//////////////////////////////
int StepperDegrees[3] = {0,0,0};  //Variables for X,Y,Z degrees
int StepperDirection[3] = {0,0,0};   //Variables for X,Y,Z directions
int StepperSteps[3] = {0,0,0};  //Variables for number of steps in X, Y, Z directions
String currentZlocation = "down";
bool SteppersEnabled = true;
bool currentDirectionZ = false;
int limitReading[3] = {0,0,0};  // Readings of X, Y, Z limit switches
int cumulativeSteps[3] = {0,0,0};  //Summation of the X, Y, Z steps taken

/////////////////////////////////ETHERNET VARIABLES//////////////////////////////////
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x92, 0x58 };
byte ip[] = { 192, 168, 1, 10 };
String inString = ""; 
String actionstring = "";
EthernetServer server(80);

//////////////////////////////////SETUP////////////////////////////////////////
void setup()
{
  if(debug) Serial.begin(38400);
  Ethernet.begin(mac, ip);
  server.begin();
  setupSteppers();
  setupLimitSwitches();
  //findHomePosition();
  //findCenterPosition();   
  cumulativeSteps[0] = totalStepsXdirection/2; //patch is necessary because these are not getting set correctly in findCenterPosition()... 
  cumulativeSteps[1] = totalStepsYdirection/2;
}

/////////////////////////////LOOP///////////////////////////////////////////
void loop()
{
    EthernetSendParseData();
    if(actionstring == "enable")
      SteppersEnabled = !SteppersEnabled;
    else if(actionstring == "home") {}
      //findHomePosition();
    else if(actionstring == "center"){}
      //findCenterPosition();
    else if(actionstring == "penup")
      StepperDirection[2] = 1 - StepperDirection[2];
    enableStepperXYZ(SteppersEnabled);
    if(SteppersEnabled)
    {
      PenUpDown();
      setStepsAndDirectionsXY();
      if(checkMove()) 
       {  
           moveXY(StepperSteps[0], StepperSteps[1]);  // moves X and Y motors along a line simultaneously
        }
        else
        {
          Serial.println("Move Exceeds Boundaries");
        }
    }
    StepperDegrees[0] = 0;
    StepperDegrees[1] = 0;
    actionstring = "";

    //}

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

int convertdegreestosteps(int _degrees) //NOTE THIS INTRODUCES ERROR PARTICULARLY AT LOW DEGREES BECAUSE REMAINDER IS DROPPED WHEN CASTING FLOAT TO INT!
{
  float steps;
  steps = _degrees/degrees_per_step;
  return int(steps);
}

void setStepsAndDirectionsXY()
{
      for(int i=0; i<2; i++)   //do this for each stepper motors X and Y
      {
        StepperSteps[i] = convertdegreestosteps(StepperDegrees[i]);  //Convert degrees to steps based on "degrees_per_step"
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

void PenUpDown()
{
  if(StepperDirection[2] == 0 && currentZlocation == "up")  //Pen DOWN, currently up
  {
    changeDirectionZ();
    for(int j=0; j<1200; j++)
            takeSingleStep(stepPin[2]);
    currentZlocation = "down";
  }
  else if(StepperDirection[2] == 1 && currentZlocation == "down")  //Pen UP, currently down
  {
    changeDirectionZ();
    for(int j=0; j<1200; j++)
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




////////////////////////////////////////////ETHERNET SCAFFOLDING CODE///////////////////////////////////////////////
void EthernetSendParseData()
{
  EthernetClient client = server.available();

  if (client) {
    boolean current_line_is_blank = true;
    boolean sentHeader = false;
    boolean foundaction = false;
    boolean foundr = false;
    while (client.connected()) 
    {
      if (client.available())   //read-in the message from the client character by character
      {
        if(!sentHeader)
        {
            // send a standard http response header
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println();
            client.println("<!DOCTYPE HTML>");
            client.println("<html>");
            sentHeader = true;
        }   
        
        char c = client.read();
        
        if(c == 'r')
          foundr = true;
        
        else if(c == 'a' && !foundr) 
          foundaction = true;
          
        if (inString.length() < maxLength && (foundaction || foundr)) 
          inString = inString + c;   
        
        if (inString.indexOf("_") > -1)  
        { 
              if(debug) 
                Serial.println(inString);
              ParseFormData(foundaction, foundr);
              foundaction = false;
              foundr = false;
              break;   
        } 
        
        if (c == '\n' && current_line_is_blank) //we've reached and end of line meaning we got the whole message
        {
          break;
        }
         
        if (c == '\n') 
          current_line_is_blank = true;
        else if (c != '\r') 
          current_line_is_blank = false;    
          
      }  // end if client available
    } // end while client connected
    
    sendFormToClient(client);
    delay(1);
    inString = "";
    client.stop();
  }
}

////////////////////////////////////////////READ DATA FROM FORM///////////////////////////////////////////////
void ParseFormData(boolean _foundaction, boolean _foundr)
{
    if(_foundaction)
    {
      int action = inString.indexOf("action");
      int underscore = inString.indexOf("_");
      actionstring = inString.substring((action+7), underscore);
      Serial.println(actionstring);
    }
    else if(_foundr)
    {
      int rlocate = inString.indexOf("r");
      int ampersign = inString.indexOf("&", rlocate);
      actionstring = inString.substring((rlocate+2), ampersign);
      Serial.println(actionstring);
      int decimal = actionstring.indexOf("C");
      String ReadDegrees = actionstring.substring(0, decimal-2);
      char temparray[10];
      ReadDegrees.toCharArray(temparray, sizeof(temparray));
      StepperDegrees[0] = atoi(temparray);
      if(StepperDegrees[0] < 0)
      {
        StepperDirection[0] = 1;
        StepperDegrees[0] = -StepperDegrees[0];
      }
      else
      {
        StepperDirection[0] = 2;
      }
      ReadDegrees = actionstring.substring(decimal, actionstring.length());
      ReadDegrees.toCharArray(temparray, sizeof(temparray));
      StepperDegrees[1] = atoi(temparray);
      Serial.print("   Ysteps:");
      Serial.print(StepperDegrees[1]);
      if(StepperDegrees[1] < 0)
      {
        StepperDirection[1] = 2;
        StepperDegrees[1] = -StepperDegrees[1];
      }
      else
      {
        StepperDirection[1] = 1;
      }
    }
}

/*
////////////////////////////////////////////READ DATA FROM FORM///////////////////////////////////////////////
void ParseFormData()
{
  int action = inString.indexOf("action");
  int underscore = inString.indexOf("_");
  String temp = inString.substring((action+7), underscore);
  Serial.println(temp);
  if(temp == "disable")
    SteppersEnabled = 2;
  else if(temp == "enable")
    SteppersEnabled = 1;
  else if(temp == "home")
    findHomePosition();
  else if(temp == "penup")
    StepperDirection[2] = 1;
  else if(temp == "pendown")
    StepperDirection[2] = 2;
  int xdeg = inString.indexOf("r");
  int x_dir = inString.indexOf("x");
  int ydeg = inString.indexOf("g");
  int y_dir = inString.indexOf("y");
  int zlocation = inString.indexOf("z");
  int End = inString.indexOf("H");
  SteppersEnabled = SplitStringPullInteger(enabled, xdeg);  //Steppers Enabled or Disabled
  StepperDegrees[0] = SplitStringPullInteger(xdeg, x_dir); //X degrees
  StepperDirection[0] = SplitStringPullInteger(x_dir, ydeg); //X direction
  StepperDegrees[1] = SplitStringPullInteger(ydeg, y_dir); //Y degrees
  StepperDirection[1] = SplitStringPullInteger(y_dir, zlocation); //Y direction
  StepperDirection[2] = SplitStringPullInteger(zlocation, End); //Pen Up or Down
}*/
/*

int SplitStringPullInteger(int index1, int index2)
{
  String temp = inString.substring((index1+2), (index2-1));
  char temparray[10];
  temp.toCharArray(temparray, sizeof(temparray));
  return atoi(temparray);
}
*/
///////////////////////////////////////HTML Code////////////////////////////////////////////////
void sendFormToClient(EthernetClient client)
{
  client.println("<body><h1>Stepper Motor Control Interface</h1>");
    
  client.println("<h3>Steppers On/Off</h3>");
  client.println("<form method=get><input type=hidden name=e>");
  client.println("<button type=submit name=action value=enable_stepper>Toggle Stepper Enable</button>");
  
  client.println("<h3>Preset Positions</h3><form method=get><input type=hidden name=h><button type=submit name=action value=home_stepper>Go To Home Position</button>");
  client.println("<form method=get><input type=hidden name=c><button type=submit name=action value=center_stepper>Go To Center Position</button>");

  client.println("<h3>Pen Up/Down</h3>");
  client.println("<form method=get><input type=hidden name=pu>");
  client.println("<button type=submit name=action value=penup_stepper>Toggle Pen</button>");
  
  client.println("<h3>Draw Line</h3>");
  client.println("<h4>X-axis:</h4>");
  client.println("<form method=get>X,Y steps:<input type=text size=8 name=r><br><button type=submit name=action value=_submit>Submit</form>");
  /*client.println("");
  client.println("<input type=radio name=x value=1 CHECKED>Left");
  client.println("<input type=radio name=x value=2>Right<br>");
  client.println("<h4>Y-axis:</h4>");
  client.println("Y steps:<input type=text size=3 name=g>Maximum = 2442<br>");
  client.println("<input type=radio name=y value=1 CHECKED>Back");
  client.println("<input type=radio name=y value=2>Front<br>");
  client.println("</form>");*/
  
  /*client.println("<h2>CurrentLocations:</h2>");
  client.print("X-axis: ");
  client.print(cumulativeSteps[0]);
  client.print("   Y-axis: ");
  client.println(cumulativeSteps[1]);*/
  client.println("</body></html>");  
}


