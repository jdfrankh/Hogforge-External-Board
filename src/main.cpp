#include <Arduino.h>
#include "XY2_100.h"
#include "Laser.h"
#include "SerialGCodeParser.h"
#include <Wire.h>
#include "Menu.h"
#include <I2CBus.h>

// --- Pin Definitions ---
const int ENCODER_CLK_PIN = 39; // CLK pin (A phase) - MUST support interrupt
const int ENCODER_DT_PIN  = 37; // DT pin (B phase)
const int ENCODER_SW_PIN  = 38; // Switch pin (optional)

const int DELAY_PLATE_HOME = 15000;
const int DELAY_PLATE_LIFT = 15000; 
const int DELAY_SMALL_STEP = 750;
const int DELAY_FULL_SWIPE = 5000;

enum IncomingCommands{

    HOMEALL = 0,
    HOMEBAR = 1,
    HOMEPLATES = 2,
    LIFTONEPLATE = 3,
    LOWERONEPLATE = 4,
    SMALLSTEP = 5,
    RAISEBOTHPLATES = 6,
    PREPPRINT = 7,
    MOVEBAR = 8,
    INITALL = 9,
    ONECYCLE = 10,
    PRINTCUBE = 11,



} incommingCommands;




long lastButtonPressTime = 0; // To manage debounce timing

bool handleSwitch() {
      // This ISR runs when the interruptPin changes state
    if(digitalRead(ENCODER_SW_PIN) == LOW) {
      long currentTime = millis();
      //Serial.println("Switch pressed");
      if (currentTime - lastButtonPressTime > 250) {
        lastButtonPressTime = currentTime;
      return true;
    }
    lastButtonPressTime = currentTime;
  }
    return false;
  }






// Pins for Galvo Control

#define default_clock 21 // Blue + white
#define default_syn 20   // Orange + white
#define default_x 7     // Brown + white
#define default_y 32    // Green + white
#define default_pins default_clock, default_syn, default_x, default_y




Menu* menu;
RotaryEncoder encoder = RotaryEncoder(ENCODER_CLK_PIN, ENCODER_DT_PIN);



//Gcode interpreter init
Laser *laser; 
XY2_100 *galvo;
SerialGCodeParser *gcodeInterpreter;
I2CBus i2c; 

int determineAction(int action);



void setup() {
  Serial.begin(115200);
  //delay(2000); // Wait for serial
  
  
// put your setup code here, to run once:
  
  

  galvo = new XY2_100(default_pins); // Pin variables are stated inside the class .hpp file
  laser = new Laser(Laser::POWER50, 20000, 10); // 50% Power, 20Khz, 10% Duty Cycle
  menu = new Menu(encoder);
  gcodeInterpreter = new SerialGCodeParser(115200, galvo, laser);
  

  Serial.println("Starting up...");
  

  menu->begin();

  i2c.init();
  int connected = i2c.scan();

  
  

   menu->printFoundI2C(connected );
  gcodeInterpreter->resetCounters();
  pinMode(ENCODER_SW_PIN, INPUT);
  

  // Attach the switch interrupt to the SW pin (triggered when the button is pressed, usually LOW)
  //attachInterrupt(digitalPinToInterrupt(ENCODER_SW_PIN), handleSwitch, RISING);

}

void loop() {


  
  menu->update(handleSwitch());//, gcodeInterpreter->returnValue);
  //delay(1);  


  if(menu->requestCommand){
    int result = determineAction(menu->requestCommand);
    if(result == gcodeInterpreter->Success){
      gcodeInterpreter->resetCounters();
      menu->confirmCompletion =true; 
    }
    if(result == gcodeInterpreter->Alarm){
      menu->alarm = 1;
    }
  }
  else{
    gcodeInterpreter->resetCounters();
  }

  //i2c.requestData();
 // i2c.requestData();
  //gcodeInterpreter->activateGuideLaser();
}



int determineAction(int action){
  switch (action)
  {
  case MenuActions::ACTIVATE_GUIDE_LASER:
      return gcodeInterpreter->activateGuideLaser();
    break;
  case MenuActions::ACTIVATE_FIBER_LASER:
      return gcodeInterpreter->activateLaser();
   // return gcodeInterpreter->activateFiberLaserDebug();
    break;
  case MenuActions::DEACTIVATE_GUIDE_LASER:
      return gcodeInterpreter->deactivateGuideLaser();
    break;
  case MenuActions::DEACTIVATE_FIBER_LASER:
      return gcodeInterpreter->deactivateLaser();
    break;
  case MenuActions::HOME_PUSHBAR:
     i2c.sendData(IncomingCommands::HOMEBAR);
     return gcodeInterpreter->Success;
    
    break;
  case MenuActions::HOME_PLATES:
      i2c.sendData(IncomingCommands::HOMEPLATES, 0);
      return gcodeInterpreter->Success;
    break;
  case MenuActions::PREP_PRINT:
      i2c.sendData(IncomingCommands::PREPPRINT, 49); // 49 * 1000
      return gcodeInterpreter->Success;
    break;
  case MenuActions::HOME_ALL:
      i2c.sendData(IncomingCommands::HOMEALL, 0);
      return gcodeInterpreter->Success;
    break;
  case MenuActions::MOVE_BAR:
      i2c.sendData(IncomingCommands::MOVEBAR, 0);
      return gcodeInterpreter->Success;
    break;
  case MenuActions::SMALL_STEP:
    i2c.sendData(IncomingCommands::SMALLSTEP, 50);
    return gcodeInterpreter->Success;
    break;
  case MenuActions::ONE_CYCLE:
    i2c.sendData(IncomingCommands::SMALLSTEP, 50);
    delay(100);
    i2c.sendData(IncomingCommands::MOVEBAR, 0);
    return gcodeInterpreter->Success;
    break;
  case MenuActions::PRINT_CUBE:
    //i2c.sendData(IncomingCommands::HOMEALL,0);
    //delay(DELAY_PLATE_HOME);
    //delay(DELAY_FULL_SWIPE); // homing speed

    i2c.sendData(IncomingCommands::PREPPRINT, 10); // 10000
    delay(DELAY_PLATE_LIFT);
    //i2c.sendData(IncomingCommands::SMALLSTEP, 325);
    //delay(DELAY_SMALL_STEP);

    while(!handleSwitch()){
      //gcodeInterpreter->activateGuideLaser();

      menu->waitforResponse = 1; // Wait for plate to be loaded, and chamber degassed
      menu->update(false);
    }
    // gcodeInterpreter->deactivateGuideLaser();
    menu->waitforResponse = 2;
    menu->update(false);
    //i2c.sendData(IncomingCommands::MOVEBAR, 0);
    //Serial.println("Moving Bar ");
    //delay(DELAY_FULL_SWIPE);

    //delay(2500);

    
    for(int i = 0; i < 13; i++){
     gcodeInterpreter->activateFiberLaserDebug(); // Turn laser on and off here
     //delay(7500);
     Serial.print("Laser Done. Layer: "); Serial.println(i);
  
      i2c.sendData(IncomingCommands::SMALLSTEP, 40); // 200 // 50
      Serial.println("Small Step");

      delay(7500);
      //delay(DELAY_SMALL_STEP);
      //i2c.sendData(IncomingCommands::MOVEBAR, 0);
      //Serial.println("Moving Bar ");
      //delay(DELAY_FULL_SWIPE);
      
    }
    delay(5000);
    
    //gcodeInterpreter->deactivateGuideLaser();
    

    menu->waitforResponse = 0;
    return gcodeInterpreter->Success;
    break;
  
  default:
    break;
  };

  

  //i2c.sendData(10,11); // Placeholder to keep I2C active
  
}
