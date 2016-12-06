// STEPPER VARIABLE DECLARATIONS//
const int stepPinX = 2;
const int dirPinX = 3;
const int enablePinXYZ = 4;
boolean currentDirectionX = false;
unsigned long dlyuS = 0;

// NOTE FREQUENCY VARIABLE DECLARATIONS//
float C4 = 261.63;
float D4 = 293.66;
float E4 = 329.63;
float F4 = 349.23;
float Fsharp4 = 369.99;
float G4 = 392.00;
float A_4 = 440.00; // can't use A4 because of analog4 pin declaration conflict
float B4 = 493.88;
float C5 = 523.25;
float D5 = 587.33;
float E5 = 659.25;
float F5 = 698.46;
float G5 = 783.99;
float pitch_multiplier = .7;

// NOTE DURATION VARIABLE DECLARATIONS //
float wholeNote = 1.0;
float quarterNote = 1.0/4.0;
float halfNote = 1.0/2.0;
float eighthNote = 1.0/8.0;
float sixteenthNote = 1.0/16.0;
float dottedEighthNote = eighthNote * 1.5;
float dottedQuarterNote = quarterNote * 1.5;
int tempo = 750;
float duration;

// SONG DECLARATION //
const float StarSpangledBanner [ 2 ][ 101 ] = {
  { G4,E4,          //measure 0
    C4,E4,G4,       //measure 1
    C5,E5,D5,       //measure 2
    C5,E4,Fsharp4,  //measure 3
    G4,G4,G4,       //measure 4
    E5,D5,C5,       //measure 5
    B4,A_4,B4,      //measure 6
    C5,C5,G4,       //measure 7
    E4,C4,G4,E4,    //measure 8
    C4,E4,G4,       //measure 9
    C5,E5,D5,       //measure 10
    C5,E4,Fsharp4,  //measure 11
    G4,G4,G4,       //measure 12
    E5,D5,C5,       //measure 13
    B4,A_4,B4,      //measure 14
    C5,C5,G4,       //measure 15
    E4,C4,E5,E5,    //measure 16
    E5,F5,G5,       //measure 17
    G5,F5,E5,       //measure 18
    D5,E5,F5,       //measure 19
    F5,F5,          //measure 20
    E5,D5,C5,       //measure 21
    B4,A_4,B4,      //measure 22
    C5,E4,Fsharp4,  //measure 23
    G4,G4,          //measure 24
    C5,C5,C5,B4,    //measure 25
    A_4,A_4,A_4,    //measure 26
    D5,F5,E5,D5,C5, //measure 27
    C5,B4,G4,G4,    //measure 28
    C5,D5,E5,F5,    //measure 29
    G5,C5,D5,       //measure 30
    E5,F5,D5,       //measure 31
    C5},            //measure 32
    
  { dottedEighthNote,sixteenthNote,                           //measure 0
    quarterNote,quarterNote,quarterNote,                      //measure 1
    halfNote,dottedEighthNote,sixteenthNote,                  //measure 2
    quarterNote,quarterNote,quarterNote,                      //measure 3
    halfNote,eighthNote,eighthNote,                           //measure 4
    dottedQuarterNote,eighthNote,quarterNote,                 //measure 5
    halfNote,dottedEighthNote,sixteenthNote,                  //measure 6
    quarterNote,quarterNote,quarterNote,                      //measure 7
    quarterNote,quarterNote,dottedEighthNote,sixteenthNote,   //measure 8
    quarterNote,quarterNote,quarterNote,                      //measure 9
    halfNote,dottedEighthNote,sixteenthNote,                  //measure 10
    quarterNote,quarterNote,quarterNote,                      //measure 11
    halfNote,eighthNote,eighthNote,                           //measure 12
    dottedQuarterNote,eighthNote,quarterNote,                 //measure 13
    halfNote,dottedEighthNote,sixteenthNote,                  //measure 14
    quarterNote,quarterNote,quarterNote,                      //measure 15
    quarterNote,quarterNote,eighthNote,eighthNote,            //measure 16
    quarterNote,quarterNote,quarterNote,                      //measure 17
    halfNote,eighthNote,eighthNote,                           //measure 18
    quarterNote,quarterNote,quarterNote,                      //measure 19
    halfNote,quarterNote,                                     //measure 20
    dottedQuarterNote,eighthNote,quarterNote,                 //measure 21
    halfNote,dottedEighthNote,sixteenthNote,                  //measure 22
    quarterNote,quarterNote,quarterNote,                      //measure 23
    halfNote,quarterNote,                                     //measure 24
    quarterNote,quarterNote,eighthNote,eighthNote,            //measure 25
    quarterNote,quarterNote,quarterNote,                      //measure 26
    quarterNote,eighthNote,eighthNote,eighthNote,eighthNote,  //measure 27
    quarterNote,quarterNote,eighthNote,eighthNote,            //measure 28
    dottedQuarterNote,eighthNote,eighthNote,eighthNote,       //measure 29
    halfNote,eighthNote,eighthNote,                           //measure 30
    dottedQuarterNote,eighthNote,quarterNote,                 //measure 31
    wholeNote                                                 //measure 32
    }
};

// STEPPER FUNCTIONS //
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

// SONG FUNCTION DECLARATIONS //
void note(float freq)
    {
      float period = (1.0/freq) ;
      period = period * 1E+6;
      dlyuS = period/2;
      
      digitalWrite(stepPinX, HIGH);
      delayMicroseconds(dlyuS);
      digitalWrite(stepPinX, LOW);
      delayMicroseconds(dlyuS);
    }




void setup() 
{
  pinMode(enablePinXYZ, OUTPUT); 

  // Setup X motor 
  pinMode(stepPinX, OUTPUT); 
  pinMode(dirPinX, OUTPUT);
  
  // get into a known state. 
  enableStepperXYZ(false);
  
  // we set the direction pin in an arbitrary direction.
  setCurrentDirectionX(false);
  
  enableStepperXYZ(true);
  
  // we set the direction pin in an arbitrary direction. 
  setCurrentDirectionX(true);
}


void loop()
{
  for(int i = 0; i < 101; i++)
  {
    digitalWrite(dirPinX, !digitalRead(dirPinX));
    duration = StarSpangledBanner[1][i] * tempo;
    for (int j=0; j<duration; j++)
     {
        note(StarSpangledBanner[0][i]*pitch_multiplier);
     }
  }
   
  delay(500000);

}

