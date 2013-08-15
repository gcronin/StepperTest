import processing.serial.*;
import controlP5.*;
ControlP5 cp5;
Serial myPort;
int LayoutSize = 700;
int buttonRadius = LayoutSize/14;
int numButtons = 5;
int [] buttonXs = {0,0,0,0,0};
color circleHighlight, circleColor;
color baseColor;
boolean [] circleOver = {false, false, false, false, false};
String [] ButtonLabel = {"Home", "Center", "PenUp", "PenDown", "Steppers"};
int[] OutData = {1,0,5,8,45,200,0};  //enable, penup/down, xByte1, xByte2, yByte1, yBtye2, center/home
int inData[] = {0,0,0,1425,1200,0};  //limit switches X,Y,Z, cumulative steps X,Y,Z
int limitSwitches[] = {0,0,0};
boolean connectedFlag = false;  //tells us whether we are connected to the Arduino... we should stop sending serial data if we are not
boolean penDown = false;
boolean displayImage = true;
String jog_vs_moveto = "moveto"; //jog the pen up/down or moved fixed amount up/down
long timeLastConnected;
int [] previousCoordinates = {(23)*LayoutSize/44,13*LayoutSize/22};  //center position
PGraphics lineLayer;
int Xcoor, Ycoor;
int cookie = 0;
String CookieSizeString;
PImage template;

////////////////////////////PLATFORM GEOMETRY NOTES/////////////////////////////////////////
//
//    Horizontal:  
//        Actual motion = 8.68"
//        Steps:  2850
//        Pixels (with LayoutSize = 700):  604.55
//        Pixels per inch = 604.55/8.68 =  69.65 pix/"
//    Vertical:
//        Actual motion = 7.37"
//        Steps = 2400
//        Pixels (with LayoutSize = 700):  509.09
//        Pixels per inch = 509.09/7.37 =  69.07 pix/"
//
///////////////////////////////////////////////////////////////////////////////////////////////

int pixelsPerInch = 69;
float stepsPerPixel = 4.714;

void setup()
{
  myPort = new Serial(this, "COM11", 38400); // connect to Arduino
  myPort.bufferUntil('\n');  //input buffer is filled until a newline is received, then serialEvent is triggered
  template = loadImage("template.jpg");
  size(LayoutSize, LayoutSize);
  initializeLineLayer();
  circleHighlight = color(204);
  circleColor = color(255);
  baseColor = color(150);
  setupCp5Buttons();
  
  }

void draw()
{
  background(baseColor);   // erases the display each loop
  DrawCookie();
  drawPlatform();
  checkCircles();
  checkJogPressed();
  drawButtons();
  ShowConnectionStatus();
  SerialTimeoutCheck();
  if(connectedFlag) sendArduino();
  displayIncomingData();
  drawLines();
}

///////////////////////////////BUTTONS CONTROL MAKING INTERFACE PRETTY//////////////////////////////
void setupCp5Buttons()
{
   //PFont pfont = createFont("Arial",20,true); // use true/false for smooth/no-smooth
   //ControlFont font = new ControlFont(pfont,241);
  
   cp5 = new ControlP5(this);
  
   cp5.addButton("clear")  // CLEAR BUTTON WILL ERASE EXISTING LINES
     .setValue(0)
     .setPosition(12*LayoutSize/14,29*LayoutSize/30)  // bottom right corner
     .setSize(50,25)
     ; 
     
   cp5.addButton("LoadImage")  // CLEAR BUTTON WILL ERASE EXISTING LINES
     .setValue(0)
     .setPosition(6*LayoutSize/14,29*LayoutSize/30)  // bottom right corner
     .setSize(70,25)
     ;   
     
   cp5.addSlider("cookie")
     .setPosition(8*LayoutSize/14,29*LayoutSize/30)
     .setSize(170,25)
     .setRange(0,10) // values can range from big to small as well
     .setValue(0)
     .setNumberOfTickMarks(20)
     .setSliderMode(Slider.FLEXIBLE)
     //.setColorTickMark(color(255,105,180))
     .setHandleSize(50)
     ;
   
   cp5.getController("LoadImage").setCaptionLabel("Load Image");
   cp5.getController("LoadImage").getCaptionLabel().setSize(12);
   cp5.getController("LoadImage").getCaptionLabel().align(ControlP5.CENTER, ControlP5.CENTER); 
   cp5.getController("clear").setCaptionLabel("Clear");
   cp5.getController("clear").getCaptionLabel().setSize(12);
   cp5.getController("clear").getCaptionLabel().align(ControlP5.CENTER, ControlP5.CENTER);   
   cp5.getController("cookie").getValueLabel().align(ControlP5.CENTER, ControlP5.TOP_OUTSIDE).setPaddingY(5);
   cp5.getController("cookie").getCaptionLabel().align(ControlP5.CENTER, ControlP5.CENTER);
   cp5.getController("cookie").setColorValueLabel(color(255));
   cp5.getController("cookie").setColorCaptionLabel(color(255));
   cp5.getController("cookie").setDecimalPrecision(2);
   cp5.getController("cookie").setCaptionLabel("Size of Cookie");
   cp5.getController("cookie").getCaptionLabel().setSize(12);
   cp5.getController("cookie").getValueLabel().setSize(12);

}

float ConvertStepsToInches(int steps)
{
  int temp = int(steps/stepsPerPixel/pixelsPerInch*100);  // in = steps x (pixel/step) x (inch/pixel)
  return float(temp)/100;
}

public void clear(int theValue) {
  lineLayer.clear();
}

public void LoadImage(int theValue) {
  displayImage = !displayImage;
}

void DrawCookie()
{
  CookieSizeString = str(cookie);
  CookieSizeString += " inches"; 
  cp5.getController("cookie").setValueLabel(CookieSizeString);
  ellipseMode(CENTER);
  stroke(0,255,0);
  fill(255,255,0);
  ellipse((23)*LayoutSize/44,13*LayoutSize/22, cookie*pixelsPerInch, cookie*pixelsPerInch);
  
  if(displayImage) image(template, 0, 0);
}

////////////////////////DRAW PLATFORM////////////////////////////////////////////
void drawPlatform()
{
  stroke(180);  textSize(LayoutSize/75);
  textAlign(CENTER);
  fill(90);
  String coordinate = "";
  for(int i=0; i<20; i++)  //vertical lines spaced at 150 steps
  {
    line((2+i)*LayoutSize/22, 5*LayoutSize/22, (2+i)*LayoutSize/22, 21*LayoutSize/22);
    coordinate = str(ConvertStepsToInches(-1425+150*i));
    text(coordinate, (2+i)*LayoutSize/22, 13*LayoutSize/22);
  }
  for(int i=0; i<17; i++)  //horizontal lines spaced at 150 steps
  {
    line(2*LayoutSize/22, (5+i)*LayoutSize/22, 21*LayoutSize/22, (5+i)*LayoutSize/22);
    coordinate = str(ConvertStepsToInches(1200-150*i));
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
     if(i == 4) {
       if(OutData[0] ==1)  text("Disable", buttonXs[i], LayoutSize/6);
       else if(OutData[0] == 0) text("Enable", buttonXs[i], LayoutSize/6); }
  }
  if(jog_vs_moveto == "jog") 
  {
    if(overCircle(buttonXs[2], LayoutSize/35, buttonRadius/4)) {
        stroke(circleColor);
        fill(circleHighlight);
    }  
    else {
        stroke(0);
        fill(0);
    }
    ellipse(buttonXs[2], LayoutSize/35, buttonRadius/4, buttonRadius/4);
    if(overCircle((3*buttonXs[2]+5*buttonXs[3])/8, LayoutSize/35, buttonRadius/4)) {
        stroke(circleColor);
        fill(circleHighlight);
    }  
    else {
        stroke(255);
        fill(255);
    }
    ellipse((3*buttonXs[2]+5*buttonXs[3])/8, LayoutSize/35, buttonRadius/4, buttonRadius/4);
  }
  
  else if(jog_vs_moveto == "moveto")
  {
    if(overCircle(buttonXs[2], LayoutSize/35, buttonRadius/4)) {
        stroke(circleColor);
        fill(circleHighlight);
    }  
    else {
        stroke(255);
        fill(255);
    }
    ellipse(buttonXs[2], LayoutSize/35, buttonRadius/4, buttonRadius/4);
    if(overCircle((3*buttonXs[2]+5*buttonXs[3])/8, LayoutSize/35, buttonRadius/4)) {
        stroke(circleColor);
        fill(circleHighlight);
    }  
    else {
        stroke(0);
        fill(0);
    }
    ellipse((3*buttonXs[2]+5*buttonXs[3])/8, LayoutSize/35, buttonRadius/4, buttonRadius/4);
  }
  fill(90);
  textSize(LayoutSize/40);
  text("Jog", (5*buttonXs[2]+2*buttonXs[3])/7, LayoutSize/30);
  text("MoveTo", (0.5*buttonXs[4]+9.5*buttonXs[3])/10, LayoutSize/30);
  
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

/////////////////////////////DEAL WITH MOUTH PRESSED FOR JOGGING///////////////////////////////
void checkJogPressed()
{
  if(mousePressed && (overCircle(buttonXs[2], LayoutSize/8, buttonRadius) == true) && jog_vs_moveto == "jog"){ //Pen Up button pressed in jog mode
    OutData[1] = 1;
    penDown = false;
  }
  else if(mousePressed && (overCircle(buttonXs[3], LayoutSize/8, buttonRadius) == true) && jog_vs_moveto == "jog") {//Pen Down button pressed in jog mode
    OutData[1] = 2;
    penDown = true;
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
   else if(overCircle(buttonXs[2], LayoutSize/8, buttonRadius) == true && jog_vs_moveto == "moveto") { //third button pressed
      OutData[1] = 3; // Pen Up
      penDown = false; }
   else if(overCircle(buttonXs[3], LayoutSize/8, buttonRadius) == true && jog_vs_moveto == "moveto") { //fourth button pressed
      OutData[1] = 4; // Pen Down
      penDown = true; }
   else if(overCircle(buttonXs[4], LayoutSize/8, buttonRadius) == true)  //fifth button pressed
      OutData[0] = 1-OutData[0]; // Toggle Steppers Enabled/Disabled
   else if(mouseX > 2*LayoutSize/22 && mouseX < 21*LayoutSize/22 && mouseY > 5*LayoutSize/22 && mouseY < 21*LayoutSize/22) {
      Xcoor = mouseX;
      Ycoor = mouseY;
      if(penDown == true) {  //Pen is Down
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
   else if(overCircle((3*buttonXs[2]+5*buttonXs[3])/8, LayoutSize/35, buttonRadius/4) == true) //moveto button pressed
     jog_vs_moveto = "moveto";
   else if(overCircle(buttonXs[2], LayoutSize/35, buttonRadius/4) == true) //jog button pressed
     jog_vs_moveto = "jog";
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
  byte mask = 1;
  limitSwitches[0] = inData[0] & mask;
  mask = 2;
  limitSwitches[1] = (inData[0] & mask)>>1;
  mask = 4;
  limitSwitches[2] = (inData[0] & mask)>>2;
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
    OutData[1] = 0;  //reset the pen up/down byte so it doesn't get sent over and over again
}

void displayIncomingData()
{
  ellipseMode(RADIUS);
  stroke(0);
  textSize(LayoutSize/50);
  textAlign(LEFT);
  for(int i=0; i<3; i++)
  {
    if(limitSwitches[i]==1) fill(0,255,0);
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
  text("Pen Location:", 10, 525);
  text(inData[5], 10, 550);

}
