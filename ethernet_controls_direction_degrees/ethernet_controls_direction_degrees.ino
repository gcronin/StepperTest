#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>

const int maxLength = 50;
const bool debug = 1;  //1 == Serial Connection ON, 0 == Serial Connection OFF
const float degrees_per_step = 1.8;  //change this as appropriate based on microstepping... full step 1.8, half step 0.9, quarter step 0.45, eighth step 0.225, sixteenth step 0.1125
const int stepperSpeed = 10;  // this is the delay between steps in ms

/////////////////////////////////STEPPER PINS//////////////////////////////
const int enablePinXYZ = 17;
const int stepPin[3] = {3, 15, 9};  //Arduino Pins for Steppers Steps
const int dirPin[3] = {2, 16, 8};    //Arduino Pins for Steppers Directions
const int limitStop[3] = {19, 18, 14};  //Arduino Pins for Limit Switches

/////////////////////////////////STEPPER VARIABLES//////////////////////////////
int StepperDegrees[3] = {0,0,0};  //Variables for X,Y,Z degrees
int StepperDirection[3] = {0,0,0};   //Variables for X,Y,Z directions
int StepperSteps[3] = {0,0,0};  //Variables for number of steps in X, Y, Z directions
String currentZlocation = "up";
int SteppersEnabled = 1;
bool currentDirectionZ = false;

/////////////////////////////////ETHERNET VARIABLES//////////////////////////////////
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x92, 0x58 };
byte ip[] = { 192, 168, 1, 10 };
String inString = String(maxLength); 
EthernetServer server(80);

//////////////////////////////////SETUP////////////////////////////////////////
void setup()
{
  if(debug) Serial.begin(38400);
  Ethernet.begin(mac, ip);
  server.begin();
  setupSteppers();  
}

/////////////////////////////LOOP///////////////////////////////////////////
void loop()
{
    EthernetSendParseData();
    TurnOnOffMotors(SteppersEnabled);  
    for(int i=0; i<2; i++)   //do this for each stepper motors X and Y
    {
      StepperSteps[i] = convertdegreestosteps(StepperDegrees[i]);  //200 steps per 360 degrees
      if(StepperDirection[i] == 1) {setCurrentDirection(true, dirPin[i]);}   //rotate counterclockwise
      else if(StepperDirection[i] == 2) {setCurrentDirection(false, dirPin[i]);}  //rotate counterclockwise
    }
    PenUpDown();
    moveXY(StepperSteps[0], StepperSteps[1]);  // moves X and Y motors along a line simultaneously
    StepperDegrees[0] = 0;
    StepperDegrees[1] = 0;
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

void TurnOnOffMotors(int isEnabled)
{
  if(isEnabled == 2)  //OFF
    enableStepperXYZ(false);
  else if(isEnabled)  //ON
    enableStepperXYZ(true);
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
}

int convertdegreestosteps(int _degrees) //NOTE THIS INTRODUCES ERROR PARTICULARLY AT LOW DEGREES BECAUSE REMAINDER IS DROPPED WHEN CASTING FLOAT TO INT!
{
  float steps;
  steps = _degrees/degrees_per_step;
  return int(steps);
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
            takeSingleStep(stepPin[1]);
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
            takeSingleStep(stepPin[0]);
      }
    }
}

void PenUpDown()
{
  if(StepperDirection[2] == 2 && currentZlocation == "up")  //Pen DOWN, currently up
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


///////////////////////////////////////Serial Code////////////////////////////////////////////////
void SendSerialData()
{
  Serial.print("x degrees: ");
  Serial.print(StepperDegrees[0]);
  Serial.print("   x direction: ");
  Serial.print(StepperDirection[0]);
  Serial.print("   y degrees: ");
  Serial.print(StepperDegrees[1]);
  Serial.print("   y direction: ");
  Serial.print(StepperDirection[1]);
  Serial.print("   Z Selected: ");
  Serial.print(StepperDirection[2]);
  Serial.print("    Current Z Location: ");
  Serial.print(currentZlocation);
  Serial.print("   Enabled: ");
  Serial.println(SteppersEnabled);
}


////////////////////////////////////////////ETHERNET SCAFFOLDING CODE///////////////////////////////////////////////
void EthernetSendParseData()
{
  EthernetClient client = server.available();
  if (client) {
    boolean current_line_is_blank = true;
    while (client.connected()) 
    {
      if (client.available())   //read-in the message from the client character by character
      {
        char c = client.read();
        if (inString.length() < maxLength) 
          inString = inString + c;
               
        if (c == '\n' && current_line_is_blank) //we've reached and end of line meaning we got the whole message
        {
          if(debug) 
            Serial.println(inString);
          
          if (inString.indexOf("?") > -1)  
          { 
              ParseFormData();
              if(debug)
                 SendSerialData();
          } 
          sendFormToClient(client);
          break;
        }
        if (c == '\n') {
          current_line_is_blank = true;
        } else if (c != '\r') {
          current_line_is_blank = false;
        }
      }
    }
    delay(1);
    inString = "";
    client.stop();
  }
}

////////////////////////////////////////////READ DATA FROM FORM///////////////////////////////////////////////
void ParseFormData()
{
  int enabled = inString.indexOf("e");
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
}


int SplitStringPullInteger(int index1, int index2)
{
  String temp = inString.substring((index1+2), (index2-1));
  char temparray[10];
  temp.toCharArray(temparray, sizeof(temparray));
  return atoi(temparray);
}

///////////////////////////////////////HTML Code////////////////////////////////////////////////
void sendFormToClient(EthernetClient client)
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();
  client.println("<html><head></head><body>");
  client.println("<h1>Stepper Motor Control Interface</h1>");
  client.println("<form method=get><input type=radio name=e value=1>Enable Steppers");
  client.println("<input type=radio name=e value=2>Disable Steppers<br>");
  client.println("<h2>X-axis:</h2>");
  client.println("X degrees:<input type=text size=3 name=r>Maximum = 32000<br>");
  client.println("<input type=radio name=x value=1 CHECKED>Left");
  client.println("<input type=radio name=x value=2>Right<br>");
  client.println("<h2>Y-axis:</h2>");
  client.println("Y degrees:<input type=text size=3 name=g>Maximum = 32000<br>");
  client.println("<input type=radio name=y value=1 CHECKED>Back");
  client.println("<input type=radio name=y value=2>Front<br>");
  client.println("<h2>Pen:</h2>");
  client.println("<input type=radio name=z value=1 CHECKED>Pen Up");
  client.println("<input type=radio name=z value=2>Pen Down<br>");
  client.println("<input type=submit value=submit></form></body></html>");  
}

