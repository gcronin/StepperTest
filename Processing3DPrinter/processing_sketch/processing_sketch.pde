import processing.serial.*;
Serial myPort;
int LayoutSize = 700;
int buttonRadius = LayoutSize/12;
int numButtons = 4;
int [] buttonXs = {0,0,0,0};
color circleHighlight, circleColor;
color baseColor;
boolean [] circleOver = {false, false, false, false};
String [] ButtonLabel = {"Home", "Center", "Pen", "Steppers"};
int[] OutData = {1,0,5,8,45,200,0};  //enable, penup/down, xByte1, xByte2, yByte1, yBtye2, center/home
int inData[] = {0,0,0,1425,1200,0};  //limit switches X,Y,Z, cumulative steps X,Y,Z
boolean connectedFlag = false;  //tells us whether we are connected to the Arduino... we should stop sending serial data if we are not
long timeLastConnected;
int [] previousCoordinates = {(23)*LayoutSize/44,13*LayoutSize/22};  //center position
PGraphics lineLayer;
int Xcoor;
int Ycoor;

void setup()
{
  myPort = new Serial(this, "COM11", 38400); // connect to Arduino
  myPort.bufferUntil('\n');  //input buffer is filled until a newline is received, then serialEvent is triggered
  size(LayoutSize, LayoutSize);
  initializeLineLayer();
  circleHighlight = color(204);
  circleColor = color(255);
  baseColor = color(150);
  }

void draw()
{
  background(baseColor);   // erases the display each loop
  
  drawPlatform();
  checkCircles();
  drawButtons();
  ShowConnectionStatus();
  SerialTimeoutCheck();
  if(connectedFlag) sendArduino();
  displayIncomingData();
  drawLines();
}

////////////////////////DRAW PLATFORM////////////////////////////////////////////
void drawPlatform()
{
  stroke(180);
  textSize(LayoutSize/75);
  textAlign(CENTER);
  fill(90);
  String coordinate = "";
  for(int i=0; i<20; i++)  //vertical lines spaced at 150 steps
  {
    line((2+i)*LayoutSize/22, 5*LayoutSize/22, (2+i)*LayoutSize/22, 21*LayoutSize/22);
    coordinate = str(-1425+150*i);
    text(coordinate, (2+i)*LayoutSize/22, 13*LayoutSize/22);
  }
  for(int i=0; i<17; i++)  //horizontal lines spaced at 150 steps
  {
    line(2*LayoutSize/22, (5+i)*LayoutSize/22, 21*LayoutSize/22, (5+i)*LayoutSize/22);
    coordinate = str(1200-150*i);
    text(coordinate, LayoutSize/2, (5+i)*LayoutSize/22);
  }
  stroke(225);
  line(2*LayoutSize/22, (13)*LayoutSize/22, 21*LayoutSize/22, (13)*LayoutSize/22);
  line((23)*LayoutSize/44, 5*LayoutSize/22, (23)*LayoutSize/44, 21*LayoutSize/22);
  int xlocation = int(map(inData[3], 0, 2850, 21*LayoutSize/22, 2*LayoutSize/22));
  int ylocation = int(map(inData[4], 0, 2400, 5*LayoutSize/22, 21*LayoutSize/22));
  stroke(255);
  fill(255);
  ellipseMode(RADIUS);
  ellipse(xlocation, ylocation, LayoutSize/100, LayoutSize/100);
}


////////////////////DRAW BUTTONS WITH MOUSEOVERS////////////////////////////////////
void drawButtons()
{
  ellipseMode(RADIUS);
  textSize(LayoutSize/35); 
  textAlign(CENTER);
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
     text(ButtonLabel[i], buttonXs[i], LayoutSize/8);
     if(i == 2) {
       if(OutData[1] == 1) text("Down", buttonXs[2], LayoutSize/6);
       else if(OutData[1] == 0) text("Up", buttonXs[2], LayoutSize/6); }
     else if(i == 3) {
       if(OutData[0] ==1)  text("Disable", buttonXs[3], LayoutSize/6);
       else if(OutData[0] == 0) text("Enable", buttonXs[3], LayoutSize/6); }
  }
  
}

////////////////////////////IMAGE LAYER SHOWS DRAWN LINES/////////////////////////////////
void initializeLineLayer()
{
  lineLayer = createGraphics(LayoutSize, LayoutSize);
  lineLayer.beginDraw();
  lineLayer.stroke(0, 102, 153);  //draw a tiny line so there is no NULL EXCEPTION is draw() tries to draw image without a mouseClick being made
  lineLayer.line(0, 0, 1, 1);
  lineLayer.endDraw();
}

////////////////////////////REINITIALIZE IMAGE LAYER EACH CYCLE///////////////////////////////
void drawLines() {
  lineLayer.beginDraw();
  lineLayer.stroke(0, 102, 153);  //draw a tiny line so there is no NULL EXCEPTION is draw() tries to draw image without a mouseClick being made
  lineLayer.line(0, 0, 1, 1);
  lineLayer.endDraw();
  image(lineLayer, 0, 0);  //redraw previously drawn lines
}

//////////////////////////////CHECK TO SEE IF MOUSE IS OVER BUTTONS///////////////////////////
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
   if(overCircle(buttonXs[0], LayoutSize/8, buttonRadius) == true) { //first button pressed
      OutData[6] = 2; // Go to Home Position
      lineLayer.clear();
      previousCoordinates[0] = 21*LayoutSize/22; //Zero Positions in Processing
      previousCoordinates[1] = 5*LayoutSize/22;
      for(int i=0; i<4; i++) OutData[i+2] = 0;  //Zero Positions for Arduino
   }
   else if(overCircle(buttonXs[1], LayoutSize/8, buttonRadius) == true) { //second button pressed
      OutData[6] = 1; // Go to Center Position
      lineLayer.clear();
      previousCoordinates[0] = (23)*LayoutSize/44;
      previousCoordinates[1] = 13*LayoutSize/22;
      OutData[2] = 1450&0xFF;
      OutData[3] = (1450>>8)&0xFF;
      OutData[4] = 1200&0xFF;
      OutData[5] = (1200>>8)&0xFF;
   }
   else if(overCircle(buttonXs[2], LayoutSize/8, buttonRadius) == true)  //third button pressed
      OutData[1] = 1-OutData[1]; // Toggle Pen Up/Down
   else if(overCircle(buttonXs[3], LayoutSize/8, buttonRadius) == true)  //fourth button pressed
      OutData[0] = 1-OutData[0]; // Toggle Steppers Enabled/Disabled
   else if(mouseX > 2*LayoutSize/22 && mouseX < 21*LayoutSize/22 && mouseY > 5*LayoutSize/22 && mouseY < 21*LayoutSize/22) {
      Xcoor = mouseX;
      Ycoor = mouseY;
      if(OutData[1]==0) {  //Pen is Down
        lineLayer.beginDraw();
        lineLayer.stroke(255, 0, 0);
        lineLayer.line(previousCoordinates[0],previousCoordinates[1], Xcoor, Ycoor);
        lineLayer.endDraw();
      }
      previousCoordinates[0] = Xcoor;
      previousCoordinates[1] = Ycoor;

      Xcoor = int(map(Xcoor, 2*LayoutSize/22, 21*LayoutSize/22, 2850, 0));
      Ycoor = int(map(Ycoor, 5*LayoutSize/22, 21*LayoutSize/22, 0, 2400));
      OutData[2] = Xcoor&0xFF;
      OutData[3] = (Xcoor>>8)&0xFF;
      OutData[4] = Ycoor&0xFF;
      OutData[5] = (Ycoor>>8)&0xFF;
      
   }
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
  textAlign(LEFT);
  if(connectedFlag) {
    fill(0,255,0);
    text("Connected", LayoutSize/40, 39*LayoutSize/40); }
  else {
    fill(255,0,0);
    text("Disconnected", LayoutSize/40, 39*LayoutSize/40); }
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
  ellipseMode(RADIUS);
  stroke(0);
  textSize(LayoutSize/50);
  textAlign(LEFT);
  for(int i=0; i<3; i++)
  {
    if(inData[i]==1) fill(0,255,0);
    else fill(255,0,0);
    ellipse(LayoutSize/20, (i+1)*LayoutSize/15, LayoutSize/75, LayoutSize/75);
    fill(255);
    if(i==0) text("X limit", LayoutSize/50, (i+1)*LayoutSize/15-LayoutSize/40);
    else if(i==1) text("Y limit", LayoutSize/50, (i+1)*LayoutSize/15-LayoutSize/40);
    else text("Z limit", LayoutSize/50, (i+1)*LayoutSize/15-LayoutSize/40);
  }

  
  fill(255);
  textSize(9);
  text("Xcoor Sent:", 10, 200);
  text(Xcoor, 10, 225);
  text("Xcoor Received:", 10, 250);
  text(inData[1], 10, 275);
  text("Current Xcoor:", 10, 300);
  text(inData[3], 10, 325);
  text("Ycoor Sent:", 10, 375);
  text(Ycoor, 10, 400);
  text("Ycoor Received:", 10, 425);
  text(inData[2], 10, 450);
  text("Current Ycoor:", 10, 475);
  text(inData[4], 10, 500);

}
