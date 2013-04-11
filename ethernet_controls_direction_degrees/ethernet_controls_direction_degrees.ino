#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#define maxLength 50

/////////////////////////////////STEPPER VARIABLES//////////////////////////////
#define stepPinX 14
#define dirPinX 15
#define enablePinX 16
#define stepPinY 5
#define dirPinY 6
#define enablePinY 7
#define xmin 17
#define xmax 18

int Xdegrees = 0;
int Ydegrees = 0;
int Xdir;
int Ydir;
float degrees_per_step = 1.8;
boolean currentDirectionX = false;
boolean currentDirectionY = false;
int Xsteps = 0;  //number of steps to take in Xdirection
int Ysteps = 0;  //number of steps to take in Ydirection
int slope[2] = {0,0};  // first is slope, second is remainder

/////////////////////////////////ETHERNET VARIABLES//////////////////////////////////
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x92, 0x58 };
byte ip[] = { 192, 168, 1, 10 };
String inString = String(maxLength); 
EthernetServer server(80);


//////////////////////////////////SETUP////////////////////////////////////////
void setup()
{
  Serial.begin(38400);
  
  // Setup Ethernet
  Ethernet.begin(mac, ip);
  server.begin();
  
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
   
}
void loop()
{
  EthernetClient client = server.available();
  if (client) {
    boolean current_line_is_blank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (inString.length() < maxLength) {
          inString = inString + c;
         }        
        if (c == '\n' && current_line_is_blank) 
        {
          Serial.println(inString);
          if (inString.indexOf("?") > -1)  { 
           int xdeg = inString.indexOf("r");
           int x_dir = inString.indexOf("x");
           int ydeg = inString.indexOf("g");
           int y_dir = inString.indexOf("y");
           int End = inString.indexOf("H");
           String temp = inString.substring((xdeg+2), (x_dir-1));
           char temparray[10];
           temp.toCharArray(temparray, sizeof(temparray));
           Xdegrees = atoi(temparray); 
           temp = inString.substring((x_dir+2), (ydeg-1));
           temp.toCharArray(temparray, sizeof(temparray));
           Xdir = atoi(temparray); 
           temp = inString.substring((ydeg+2), (y_dir-1));
           temp.toCharArray(temparray, sizeof(temparray));
           Ydegrees = atoi(temparray); 
           temp = inString.substring((y_dir+2), (End-1));
           temp.toCharArray(temparray, sizeof(temparray));
           Ydir = atoi(temparray); 
           Serial.print("x degrees: ");
           Serial.print(Xdegrees);
           Serial.print("   x direction: ");
           Serial.print(Xdir);
           Serial.print("   y degrees: ");
           Serial.print(Ydegrees);
           Serial.print("   y direction: ");
           Serial.println(Ydir);
          } 
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          client.println("<html><head></head><body>");
          client.println("<h1>Stepper Motor Control Interface</h1>");
          client.println("<h2>X-axis:</h2>");
          client.println("<form method=get>X degrees:<input type=text size=3 name=r>Maximum = 32000<br>");
          client.println("<input type=radio name=x value=1 CHECKED>Clockwise");
          client.println("<input type=radio name=x value=2>Counterclockwise<br>");
          client.println("<h2>Y-axis:</h2>");
          client.println("Y degrees:<input type=text size=3 name=g>Maximum = 32000<br>");
          client.println("<input type=radio name=y value=1 CHECKED>Clockwise");
          client.println("<input type=radio name=y value=2>Counterclockwise<br>");
          client.println("<input type=submit value=submit></form></body></html>");  
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
    Xsteps = convertdegreestosteps(Xdegrees);
    Ysteps = convertdegreestosteps(Ydegrees);
    if(Xdir == 1) {setCurrentDirectionX(true);} 
    else if(Xdir == 2) {setCurrentDirectionX(false);}
    if(Ydir == 1) {setCurrentDirectionY(true);} 
    else if(Ydir == 2) {setCurrentDirectionY(false);}
    interweave(Xsteps, Ysteps, slope);
    if(Xsteps == 0) 
    {
      for(int i=0; i<Ysteps; i++)  {takeSingleStepY(); }
    }
    else if(Ysteps == 0)
    {
      for(int i=0; i<Xsteps; i++)  {takeSingleStepX(); }
    }
    else if(Xsteps > Ysteps)
    {
      for(int i=0; i<Ysteps; i++) 
      {
        takeSingleStepX(); 
        for(int j=0; j<slope[0]; j++)
            takeSingleStepY();
        if(i < slope[1])
            takeSingleStepY();
      }
    }
    else
    {
      for(int i=0; i<Xsteps; i++) 
      {
        takeSingleStepY(); 
        for(int j=0; j<slope[0]; j++)
            takeSingleStepX();
        if(i < slope[1])
            takeSingleStepX();
      }
    }
    Xdegrees = 0;
    Ydegrees = 0;

}



///////////////////////////////////////////STEPPER CONTROL FUNCTIONS////////////////////////////////////////////////////

//////////////////////////////DIRECTION CONTROLS///////////////////////////////////////
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

//////////////////////////////ENABLE CONTROLS///////////////////////////////////////
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

//////////////////////////////STEP CONTROLS///////////////////////////////////////
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

int convertdegreestosteps(int _degrees) //NOTE THIS INTRODUCES ERROR PARTICULARLY AT LOW DEGREES BECAUSE REMAINDER IS DROPPED WHEN CASTING FLOAT TO INT!
{
  float temp;
  temp = _degrees/degrees_per_step;
  return int(temp);
}

void interweave(int _Xsteps, int _Ysteps, int *slope)  //need to add zero protection here...
{
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
}

