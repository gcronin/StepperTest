import processing.serial.*;
Serial myPort;
int LayoutSize = 700;
int buttonRadius = LayoutSize/12;
int numButtons = 4;
int [] buttonXs = {0,0,0,0};
color circleHighlight, circleColor;
color baseColor;
boolean [] circleOver = {false, false, false, false};
String [] ButtonLabel = {" Home   ", " Center ", "   Pen  ", "Steppers"};
int[] OutData = {1,0,5,8,45,200,0};  //enable, penup/down, xByte1, xByte2, yByte1, yBtye2, center/home
int inData[] = {0,0,0,0,0,0};  //limit switches X,Y,Z, cumulative steps X,Y,Z
boolean connectedFlag = false;  //tells us whether we are connected to the Arduino... we should stop sending serial data if we are not
long timeLastConnected;

void setup()
{
  myPort = new Serial(this, "COM11", 38400); // connect to Arduino
  myPort.bufferUntil('\n');  //input buffer is filled until a newline is received, then serialEvent is triggered
  size(LayoutSize, LayoutSize);
  circleHighlight = color(204);
  circleColor = color(255);
  baseColor = color(150);
}

void draw()
{
  background(baseColor);   // erases the display each loop
  checkCircles();
  drawButtons();
  ShowConnectionStatus();
  SerialTimeoutCheck();
  if(connectedFlag) sendArduino();
  displayIncomingData();
}


////////////////////DRAW BUTTONS WITH MOUSEOVERS////////////////////////////////////
void drawButtons()
{
  ellipseMode(RADIUS);
  textSize(LayoutSize/35); 
  for(int i=0; i<numButtons; i++) {
     buttonXs[i] = (i+1)*LayoutSize/(numButtons+1);
     if(circleOver[i] == true)
     {
         stroke(circleColor);
         fill(circleHighlight);
     }
     else
     {
         stroke(255);
         fill(circleColor);
     }
     ellipse(buttonXs[i], LayoutSize/8, buttonRadius, buttonRadius);
     if(circleOver[i] == true) fill(0);
     else fill(circleHighlight);
     text(ButtonLabel[i], buttonXs[i] - LayoutSize/18, LayoutSize/8);
     if(i == 2) {
       if(OutData[1] == 1) text("  Down", buttonXs[2] - LayoutSize/18, LayoutSize/6);
       else if(OutData[1] == 0) text("   Up", buttonXs[2] - LayoutSize/18, LayoutSize/6); }
     else if(i == 3) {
       if(OutData[0] ==1)  text("Disable", buttonXs[3] - LayoutSize/18, LayoutSize/6);
       else if(OutData[0] == 0) text(" Enable", buttonXs[3] - LayoutSize/18, LayoutSize/6); }
  }
  
}

//////////////////////////////CHECK TO SEE IF MOUSE IF OVER BUTTONS///////////////////////////
void checkCircles()
{
  for(int i=0; i<numButtons; i++) {
    circleOver[i] = overCircle(buttonXs[i], LayoutSize/8, buttonRadius);
  }
}

boolean overCircle(int x, int y, int radius) 
{
  float disX = x - mouseX;
  float disY = y - mouseY;
  if(sqrt(sq(disX) + sq(disY)) < radius ) {
    return true;
  } else {
    return false;
  }
}


/////////////////////////////DEAL WITH MOUSE CLICKS/////////////////////////////////////////////
void mouseClicked()
{
   if(overCircle(buttonXs[0], LayoutSize/8, buttonRadius) == true)  //first button pressed
      OutData[6] = 2; // Go to Home Position
   else if(overCircle(buttonXs[1], LayoutSize/8, buttonRadius) == true)  //second button pressed
      OutData[6] = 1; // Go to Center Position
   else if(overCircle(buttonXs[2], LayoutSize/8, buttonRadius) == true)  //third button pressed
      OutData[1] = 1-OutData[1]; // Toggle Pen Up/Down
   else if(overCircle(buttonXs[3], LayoutSize/8, buttonRadius) == true)  //fourth button pressed
      OutData[0] = 1-OutData[0]; // Toggle Steppers Enabled/Disabled
}

//////////////////////////////////READ SERIAL INCOMING DATA//////////////////////////////
void serialEvent(Serial myPort)  // this executes whenever \n character is received
{   
  // read the serial buffer:
  String myString = myPort.readStringUntil('\n');
  // if you got any bytes other than the linefeed:
  myString = trim(myString);
 
  // split the string at the commas
  // and convert the sections into integers:
  inData = int(split(myString, ','));
  connectedFlag = true;
  timeLastConnected = millis();
}

///////////////////////////CHECK FOR TIMEOUT CONDITION/////////////////////////////////
void SerialTimeoutCheck()
{
  if(millis() - timeLastConnected > 200) 
    connectedFlag = false;
}


void ShowConnectionStatus()
{
  textSize(LayoutSize/35);
  if(connectedFlag) {
    fill(0,255,0);
    text("Connected", LayoutSize/10, 9*LayoutSize/10); }
  else {
    fill(255,0,0);
    text("Disconnected", LayoutSize/10, 9*LayoutSize/10); }
}

////////////////////////////SEND DATA TO ARDUINO///////////////////////////////////////
// Sends 10 bytes total (2 handshake plus 7 data bytes plus checksum)
void sendArduino() //all char values need to be between 0 and 255
{
    int checksum = 0;  
    myPort.write(char(0xFF));   //handshake #1
    myPort.write(char(0xFE));   //handshake #2
    for(int i=0; i<7; i++)  {
      myPort.write(char(OutData[i]));
      checksum += OutData[i]; }
    checksum = checksum%255;
    myPort.write(char(checksum));
    OutData[6] = 0;  //reset the home or center byte so it doesn't get sent over and over again
}


void displayIncomingData()
{
  fill(255);
  textSize(12);
  text("Xswitch:", 10, 200);
  text(inData[0], 10, 250);
  text("Yswitch:", 50, 200);
  text(inData[1], 50, 250);
  text("Zswitch:", 100, 200);
  text(inData[2], 100, 250);
  text("Xlocation:", 150, 200);
  text(inData[3], 150, 250);
  text("Ylocation:", 200, 200);
  text(inData[4], 200, 250);
  text("Zlocation:", 250, 200);
  text(inData[5], 250, 250); 
}
