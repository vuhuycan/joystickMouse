#include <BleMouse.h>

// LEFT-JOYSTICK
#define LH 34
#define LV 35
#define LS 15


BleMouse bleMouse;

void setup() {
  Serial.begin(115200);
  analogReadResolution(10);
  pinMode(LS,INPUT_PULLUP);
  Serial.println("Starting BLE work!");
  bleMouse.begin();
}

void loop() {
  if(bleMouse.isConnected()) {


    int value1 = analogRead(LH);
    int value2 = analogRead(LV);
    //Serial.printf("%05d\t%05d\t",value1,value2);


    //int valueH = avg_filter1(value1);
    //int valueV = avg_filter2(value2);
    int valueH = value1;
    int valueV = value2;


    const int axiscenterH = 497;
    const int axiscenterV = 487;
    const int axisminH = 0;
    const int axisminV = 0;
    const int axismaxH = 1023;
    const int axismaxV = 1023;
    int correctedvalueH, correctedvalueV;

/*
    static int avgH = axiscenterH;
    static int avgV = axiscenterV;
    static int items =1;
    avgH = (avgH*items + valueH)/(items +1);
    avgV = (avgV*items + valueV)/(items +1);
    items +=1;
    //Serial.printf("avg %05d\t%05d\n",avgH,avgV);
*/
/*
    int dedzoneH = axiscenterH / 20; // 5%
    if ((valueH < (axiscenterH + dedzoneH)) && (valueH > (axiscenterH - dedzoneH))) valueH = axiscenterH;
    else valueH = value1;
    int dedzoneV = axiscenterV / 20; 
    if ((valueV < (axiscenterV + dedzoneV)) && (valueV > (axiscenterV - dedzoneV))) valueV = axiscenterV;
    else valueV = value2;
    Serial.printf("dedzon: %05d\t%05d\t",valueH,valueV);
*/

    const int max_mickey = 12;
    if (valueH < axiscenterH) correctedvalueH = map(valueH,axisminH,axiscenterH,max_mickey,0);
    else correctedvalueH = map(valueH,axiscenterH,axismaxH,0,-max_mickey);

    if (valueV < axiscenterV) correctedvalueV = map(valueV,axisminV,axiscenterV,-max_mickey,0);
    else correctedvalueV = map(valueV,axiscenterV,axismaxV,0,max_mickey);
    
    //Serial.printf("crect: %05d\t%05d\t",correctedvalueH,correctedvalueV);
    
    
    //if (abs(correctedvalueV) > 1 && abs(correctedvalueH) > 1)
    //bleMouse.move(correctedvalueH,correctedvalueV);


    int x = avg_filter1(correctedvalueH);
    int y = avg_filter2(correctedvalueV);
    //int x = correctedvalueH;
    //int y = correctedvalueV;
    Serial.printf("xy: %05d\t%05d\t",x,y);
    

    const int I = 4;
    
    int abs_x = abs(x); int abs_y = abs(y);
    int z = abs_x + abs_y - ((2*min(abs_x,abs_y))/3); //approximate of sqrt(x^2 + y^2)
    //if (z<3) z = 0; 
    static int z0 = z;
    //Serial.printf("z: %05d\t%05d\t",z0,z);
    int zi = ( (z - z0)*I ) + z;
    int zi2 = abs(zi);
    int Z = zi2;                        //Z = f(zi2); // f is transfer function, implement by a lookup table
    int ratio = (z==0)? 0 : Z/z;        //ratio = Z/zi2;
    int uX = x*ratio; int uY = y*ratio;
    int X = uX; //zi<0? -uX : uX;             //(zi==0)? 0 : (zi>0)? : x*ratio : - x*ratio;
    int Y = uY; //zi<0? -uY : uY;             //(zi==0)? 0 : ...                   y*ratio;
    z0 = z;

    
    //Serial.printf("%05d\t%05d\t",X,Y);
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



#define WINDOW_SIZE 16

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