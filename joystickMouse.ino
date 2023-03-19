#include <BleMouse.h>

// LEFT-JOYSTICK
#define LH 34 //horizontal axis
#define LV 35 //vertical axis
#define LS 15 //joystick button

int axiscenterH ;
int axiscenterV ;

//two libraries are used:
// + NimBLE-Arduino
// + https://github.com/wakwak-koba/ESP32-NimBLE-Mouse
BleMouse bleMouse;

void setup() {
  Serial.begin(115200);
  //Default ADC resolution is 12bits; we only need 4, as human hand cannot feel that much, and to reduce noise also:
  analogReadResolution(4);
  //register internal pull up resistor for the button (if you don't have an external one):
  pinMode(LS,INPUT_PULLUP);
  //At resting position, the potientiometer usually off center a little.
  //For ex: with ADC values ranging from 0 -> 1023, the resting position is not 512, but ~450 or ~550.
  //So, let's calibrate the true center:
  //  notes: the calibration code below assume that at reset, the joystick is not moved for about 1 second. 
  int sumH=0, sumV=0;
  for (int i=0; i< 10000; i++) {
    sumH += analogRead(LH);
    sumV += analogRead(LV);
  }
  axiscenterH = sumH / 10000;
  axiscenterV = sumV / 10000;
  Serial.printf("center: %d\t%d\n",axiscenterH,axiscenterV);

  Serial.println("Starting BLE work!");
  bleMouse.begin();
}

void loop() {
  if(bleMouse.isConnected()) {


    int value1 = analogRead(LH);
    int value2 = analogRead(LV);
    Serial.printf("%05d\t%05d\t\t",value1,value2);

    //apply average moving filter to make the mouse movement smoother:
    int valueH = avg_filter1(value1);
    int valueV = avg_filter2(value2);
    //Serial.printf("%05d\t%05d\t\t",valueH,valueV);



/*
    //This code is used to combat the off center of the potentiometer, 
    //For ex: ADC values  :  0   -> 400 (center) -> 1023.
    //need to be mapped to: -512 -> 0   (center) -> 511 .
    //But the map function just truncate values.
    //We need to implement round-to-nearest to use this code.
    //Anyway, my joystick just off center a little bit, so I don't need this anyway.
    const int axisminH = 0;
    const int axisminV = 0;
    const int axismaxH = 4095;
    const int axismaxV = 4095;
    int correctedvalueH, correctedvalueV;

    const int max_mickey = 4;
    if (valueH < axiscenterH) correctedvalueH = map(valueH,axisminH,axiscenterH,max_mickey,0);
    else correctedvalueH = map(valueH,axiscenterH,axismaxH,0,-max_mickey);

    if (valueV < axiscenterV) correctedvalueV = map(valueV,axisminV,axiscenterV,-max_mickey,0);
    else correctedvalueV = map(valueV,axiscenterV,axismaxV,0,max_mickey);
    
    //Serial.printf("crect: %05d\t%05d\t",correctedvalueH,correctedvalueV);
*/  
    //int x = avg_filter1(correctedvalueH);
    //int y = avg_filter2(correctedvalueV);
    //int x = correctedvalueH;
    //int y = correctedvalueV;
    int x = - (valueH - axiscenterH);
    int y =    valueV - axiscenterV ;
    Serial.printf("%05d\t%05d\t",x,y);
    

    //Here are where the magic is peformed:
    //We'll apply "negative inertia transfer function" of Thinkpad trackpoint
    // to make the mouse movement more responsive and intuitive.

    //this algorithm gives you two things to modify:
    const int I = 4; //the inertia gain factor, the larger it is, the quicker the mouse reach max speed.
    //#define TRANSFER_FUNC //not sure what the hell it does yet. But it seems that I don't need it.
    
    int abs_x = abs(x); int abs_y = abs(y);
    int z = abs_x + abs_y - ((2*min(abs_x,abs_y))/3); //approximate of sqrt(x^2 + y^2)
    //if (z<3) z = 0; //use this to ignore noise at the center position. But I used avg filter, so I don't need this.
    static int z0 = z;
    //Serial.printf("z: %05d\t%05d\t",z0,z);
    int zi = ( (z - z0)*I ) + z;
    int zi2 = abs(zi);

    #ifdef TRANSFER_FUNC
    int Z = transfer_func[zi2];              // f is transfer function, implement by lookup table in lut.h
    int ratio = zi==0? 0 : Z/zi2;            
    int uX = x*ratio; int uY = y*ratio;
    int X = (zi==0)? 0 : (zi>0)? uX : -uX;  
    int Y = (zi==0)? 0 : (zi>0)? uY : -uY; 
    #else 
    int Z = zi2;
    int ratio = z==0? 0 : Z/z;
    int uX = x*ratio; int uY = y*ratio;
    int X = uX; int Y = uY;
    #endif

    z0 = z; //save z to use at the next iter

    
    Serial.printf("%05d\t%05d\t",X,Y);
    //if (X!=0 || Y!=0) 
    bleMouse.move(X,Y);
    //delay(1);
    Serial.println("");



    // read the mouse button and click or not click:
    // if the mouse button is pressed:
    if (digitalRead(LS) == LOW) {
      // if the mouse is not pressed, press it:
      if (!bleMouse.isPressed(MOUSE_LEFT)) {
        bleMouse.press(MOUSE_LEFT);
      }
    }
    // else the mouse button is not pressed:
    else {
      // if the mouse is pressed, release it:
      if (bleMouse.isPressed(MOUSE_LEFT)) {
        bleMouse.release(MOUSE_LEFT);
      }
    }

    
  }
}


//average moving filter:
//the larger the window size, the smoother it is. But with the trade off of larger input delay.
#define WINDOW_SIZE 4

int avg_filter1 (int input) {

static int INDEX = 0;
int VALUE = 0;
static int SUM = 0;
static int READINGS[WINDOW_SIZE];
static int AVERAGED = 0;

  SUM = SUM - READINGS[INDEX];       // Remove the oldest entry from the sum
  VALUE = input; //analogRead(IN_PIN);        // Read the next sensor value
  READINGS[INDEX] = VALUE;           // Add the newest reading to the window
  SUM = SUM + VALUE;                 // Add the newest reading to the sum
  INDEX = (INDEX+1) % WINDOW_SIZE;   // Increment the index, and wrap to 0 if it exceeds the window size

  AVERAGED = SUM / WINDOW_SIZE;      // Divide the sum of the window by the window size for the result

  return(AVERAGED);
}

int avg_filter2 (int input) {

static int INDEX = 0;
int VALUE = 0;
static int SUM = 0;
static int READINGS[WINDOW_SIZE];
static int AVERAGED = 0;

  SUM = SUM - READINGS[INDEX];       // Remove the oldest entry from the sum
  VALUE = input; //analogRead(IN_PIN);        // Read the next sensor value
  READINGS[INDEX] = VALUE;           // Add the newest reading to the window
  SUM = SUM + VALUE;                 // Add the newest reading to the sum
  INDEX = (INDEX+1) % WINDOW_SIZE;   // Increment the index, and wrap to 0 if it exceeds the window size

  AVERAGED = SUM / WINDOW_SIZE;      // Divide the sum of the window by the window size for the result

  return(AVERAGED);
}



