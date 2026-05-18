#include <Arduino.h>
#include "XY2_100.h"
#include "Laser.h"
#include "SerialGCodeParser.h"
#include <Wire.h>
#include "Menu.h"
#include "Relay.h"
#include <UARTComms.h>
#include <SDGcode.h>

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
    STARTFAN = 10,
    STOPFAN = 11,
    ONECYCLE = 12,
    PRINTCUBE = 13,



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
UARTComms i2c; // UART link to Internal Board (Serial6: TX=pin 24/A10, RX=pin 25/A11)
SDGcode sdGcodeReader(BUILTIN_SDCARD);
Relays *relays; 


struct SdPrintSession {
  bool active = false;
  bool paused = false;
  std::vector<std::vector<String>> lines;
  size_t nextLine = 0;
  String selectedPath = "";
};

SdPrintSession sdPrintSession;

int determineAction(int action);
void startSdPrintSession(const String &path);
void stopSdPrintSession();
void togglePauseSdPrintSession();
void serviceSdPrintSession();
String joinTokens(const std::vector<String> &tokens);
bool mainGcodeFunction(const std::vector<String> &tokens);


void waitForResponse(){
  while(!handleSwitch()){
        //gcodeInterpreter->activateGuideLaser();

        menu->waitforResponse = 1; // Wait for plate to be loaded, and chamber degassed
        menu->update(false);
      }
      // gcodeInterpreter->deactivateGuideLaser();
      menu->waitforResponse = 2;
      menu->update(false);

}

// Block until the Internal Board reports idle (d1==2 from Xiao) or timeout.
// Called after sending any motion command so GCode lines execute in order.
// Also services the Pause / Cancel buttons on the print-status screen so the
// user can interact during motion waits.
static void waitForInternalBoard(uint32_t timeoutMs = 600000) {
  uint32_t deadline = millis() + timeoutMs;
  while (millis() < deadline) {
    if (i2c.update() && i2c.lastData1() == 2) return;
    menu->update(handleSwitch()); // keep display alive and detect button presses
    // Handle cancel / pause raised from the print-status screen.
    if (menu->requestCommand == MenuActions::CANCEL_SD_GCODE_PRINT) {
      menu->requestCommand = 0;
      stopSdPrintSession();
      return; // session is stopped; caller will see active==false and exit
    }
    if (menu->requestCommand == MenuActions::TOGGLE_PAUSE_SD_GCODE_PRINT) {
      menu->requestCommand = 0;
      togglePauseSdPrintSession();
      // Keep waiting for the current motion to finish; the pause flag stops
      // the next G-code line from running once we return.
    }
    delay(10);
  }
  // Timeout — continue anyway so a stalled board doesn't hang the print.
}

String joinTokens(const std::vector<String> &tokens) {
  String command = "";
  for (size_t i = 0; i < tokens.size(); i++) {
    if (i > 0) {
      command += " ";
    }
    command += tokens[i];
  }
  return command;
}

void startSdPrintSession(const String &path) {
  if (path.length() == 0) {
    return;
  }

  if (!sdGcodeReader.begin()) {
    menu->gcodePrintActive = false;
    menu->gcodeCurrentLineText = "SD init failed";
    return;
  }

  sdPrintSession.lines = sdGcodeReader.parseGcodeFile(path);
  if (sdPrintSession.lines.empty()) {
    menu->gcodePrintActive = false;
    menu->gcodeCurrentLineText = "No gcode lines";
    return;
  }

  sdPrintSession.active = true;
  sdPrintSession.paused = false;
  sdPrintSession.nextLine = 0;
  sdPrintSession.selectedPath = path;

  menu->gcodePrintActive = true;
  menu->gcodePrintPaused = false;
  menu->gcodeCurrentLine = 0;
  menu->gcodeTotalLines = static_cast<int>(sdPrintSession.lines.size());
  menu->gcodeCurrentLineText = "Starting...";
}

void stopSdPrintSession() {
  sdPrintSession.active = false;
  sdPrintSession.paused = false;
  sdPrintSession.lines.clear();
  sdPrintSession.nextLine = 0;
  sdPrintSession.selectedPath = "";

  gcodeInterpreter->deactivateLaser();
  gcodeInterpreter->deactivateGuideLaser();

  menu->gcodePrintActive = false;
  menu->gcodePrintPaused = false;
  menu->gcodeCurrentLine = 0;
  menu->gcodeTotalLines = 0;
  menu->gcodeCurrentLineText = "";
  menu->goToMainMenu();
}

void togglePauseSdPrintSession() {
  if (!sdPrintSession.active) {
    return;
  }

  sdPrintSession.paused = !sdPrintSession.paused;
  menu->gcodePrintPaused = sdPrintSession.paused;
}

void serviceSdPrintSession() {
  if (!sdPrintSession.active || sdPrintSession.paused) {
    return;
  }

  if (sdPrintSession.nextLine >= sdPrintSession.lines.size()) {
    stopSdPrintSession();
    return;
  }

  const std::vector<String> &tokens = sdPrintSession.lines[sdPrintSession.nextLine];
  String command = joinTokens(tokens);

  menu->gcodeCurrentLineText = command;
  menu->gcodeCurrentLine = static_cast<int>(sdPrintSession.nextLine + 1);
  menu->gcodeTotalLines = static_cast<int>(sdPrintSession.lines.size());


  if(!mainGcodeFunction(tokens)){
    gcodeInterpreter->executeCommand(command);
  }
  else{
    
  }
  
  sdPrintSession.nextLine++;
}

bool mainGcodeFunction(const std::vector<String> &tokens) {

  //Move Based : 
  //G0  -- Just travel
  // G1 -- Travel with laser on 
  // G3 G4 G9  -- Other Movement Based Commands
  // G28 -- Set Home G28.1 
  //Plane Setting (Redundant): G17 G18 G19 G91 G92
  // M13 -- Prep Laser
  //M14 -- Disable Laser
  // M54 - Enable Laser
  //M55 -- Disable Laser (Redundant with M14 but just in case)

  //M56 -- Set guide beam
  //M57 -- Disable Guide Beam

  //Redundant but taken : M60 M61 M98 M38 M39 M40 M41 M16 M17 M00 M01 M02

  // Needed Here:

  //Enable Vacuum Pump M15
  //Wait for a few seconds G30 T20 (seconds) 
  //Disable Vacuum Pump M16
  //Enable Argon Solenoid  M17
  //Disable Argon Solenoid M18
  //M19 Fill Material (pause, prompt user, wait for button press)
  //M20 Turn circulation fan on
  //M21 Turn circulation fan off
  
  //Home all (plates and bar) G31
  //Prepare for Print - Set a value for how high the plate should be lifted.  G32 Z[value in mm, 1000 = 1mm]
  //Move the bar G33
  //Move one small step G34 Z[value in mm, 1000 = 1mm]
  //Home just Plates G35
  //Home just the bar G36


  if (tokens.empty()) {
    return false;
  }

  // Helper to pull the numeric value following a parameter letter
  // (e.g. "Z1500" -> 1500.0, "T20" -> 20.0).  Returns ``defaultValue``
  // when the letter is not present in the token list.
  auto findParam = [&tokens](char letter, float defaultValue) -> float {
    letter = (char)toupper(letter);
    for (size_t i = 1; i < tokens.size(); ++i) {
      const String &t = tokens[i];
      if (t.length() >= 2 && (char)toupper(t[0]) == letter) {
        return t.substring(1).toFloat();
      }
    }
    return defaultValue;
  };

  const String &code = tokens[0];

  // ---- G-codes ----------------------------------------------------------
  if (code.equalsIgnoreCase("G30")) {
    // Dwell — T<seconds>
    long seconds = (long)findParam('T', 0.0f);
    if (seconds > 0) {
      delay((unsigned long)seconds * 1000UL);
    }
    return true;
  }
  if (code.equalsIgnoreCase("G31")) {
    // Home all (plates + bar)
    i2c.sendData(IncomingCommands::HOMEALL, 0);
    waitForInternalBoard();
    return true;
  }
  if (code.equalsIgnoreCase("G32")) {
    // Prep print — Z value in mm * 1000.  Existing PREPPRINT handler
    // expects the same scaled integer the menu sends (e.g. 49 == 49000).
    int z = (int)findParam('Z', 0.0f);
    i2c.sendData(IncomingCommands::PREPPRINT, z);
    waitForInternalBoard();
    return true;
  }
  if (code.equalsIgnoreCase("G33")) {
    // Move bar (full sweep)
    i2c.sendData(IncomingCommands::MOVEBAR, 0);
    waitForInternalBoard();
    return true;
  }
  if (code.equalsIgnoreCase("G34")) {
    // One small step — Z in mm * 1000
    int z = (int)findParam('Z', 0.0f);
    i2c.sendData(IncomingCommands::SMALLSTEP, z);
    waitForInternalBoard();
    return true;
  }
  if (code.equalsIgnoreCase("G35")) {
    // Home just the plates
    i2c.sendData(IncomingCommands::HOMEPLATES, 0);
    waitForInternalBoard();
    return true;
  }
  if (code.equalsIgnoreCase("G36")) {
    // Home just the bar
    i2c.sendData(IncomingCommands::HOMEBAR);
    waitForInternalBoard();
    return true;
  }

  // ---- M-codes ----------------------------------------------------------
  if (code.equalsIgnoreCase("M15")) {
    // Enable vacuum pump
    if (relays) relays->on(Relays::VACUUM_PUMP);
    return true;
  }
  if (code.equalsIgnoreCase("M16")) {
    // Disable vacuum pump
    if (relays) relays->off(Relays::VACUUM_PUMP);
    return true;
  }
  if (code.equalsIgnoreCase("M17")) {
    // Enable argon solenoid
    if (relays) relays->on(Relays::ARGON_SOLENOID);
    return true;
  }
  if (code.equalsIgnoreCase("M18")) {
    // Disable argon solenoid
    if (relays) relays->off(Relays::ARGON_SOLENOID);
    return true;
  }
  if (code.equalsIgnoreCase("M20")) {
    // Turn circulation fan on (max speed)
    i2c.sendData(IncomingCommands::STARTFAN, 0);
    return true;
  }
  if (code.equalsIgnoreCase("M21")) {
    // Turn circulation fan off
    i2c.sendData(IncomingCommands::STOPFAN, 0);
    return true;
  }
  if(code.equalsIgnoreCase("M19")){
    // Pause gcode execution and prompt the user to fill material.
    // Keep re-drawing the screen until the rotary-encoder button is pressed.
    menu->showFillMaterialPrompt();
    while (!handleSwitch()) {
      menu->showFillMaterialPrompt();
      delay(50);
    }
    return true;
  }

  return false;
}


void setup() {
  Serial.begin(115200);
  delay(2000); // Wait for serial
  
  
// put your setup code here, to run once:
  
  

  galvo = new XY2_100(default_pins); // Pin variables are stated inside the class .hpp file
  laser = new Laser(Laser::POWER50, 20000, 10); // 50% Power, 20Khz, 10% Duty Cycle
  menu = new Menu(encoder);
  gcodeInterpreter = new SerialGCodeParser(115200, galvo, laser);
  

  Serial.println("Starting up...");
  

  menu->begin();

  // Serial6 on Teensy 4.1: TX = pin 24 (A10), RX = pin 25 (A11)
  i2c.init(&Serial6, 115200, UART_ROLE_MASTER);
  int connected = i2c.scan();

  
  

   menu->printFoundI2C(connected );
  gcodeInterpreter->resetCounters();
  // INPUT_PULLUP (not INPUT): the switch is wired to GND with no external
  // pull-up, so a floating pin would pick up noise from the CLK/DT lines
  // during rotation and look like phantom button presses — making the menu
  // bounce between Main and Print whenever the encoder is turned.
  pinMode(ENCODER_SW_PIN, INPUT_PULLUP);
  

  // Attach the switch interrupt to the SW pin (triggered when the button is pressed, usually LOW)
  //attachInterrupt(digitalPinToInterrupt(ENCODER_SW_PIN), handleSwitch, RISING);

}

void loop() {
  menu->update(handleSwitch());

  if (menu->requestCommand) {
    int command = menu->requestCommand;
    menu->requestCommand = 0;

    if (command == MenuActions::START_SD_GCODE_PRINT) {
      startSdPrintSession(menu->selectedGcodePath);
    } else if (command == MenuActions::TOGGLE_PAUSE_SD_GCODE_PRINT) {
      togglePauseSdPrintSession();
    } else if (command == MenuActions::CANCEL_SD_GCODE_PRINT) {
      stopSdPrintSession();
    } else {
      int result = determineAction(command);
      if(result == gcodeInterpreter->Success){
        gcodeInterpreter->resetCounters();
        menu->confirmCompletion = true;
      }
      if(result == gcodeInterpreter->Alarm){
        menu->alarm = 1;
      }
    }
  }

  serviceSdPrintSession();
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
      // Single atomic command: the Internal Board homes both plates, waits
      // for homing to finish (otherwise the limit-switch ISRs stomp any
      // queued moveTo target), then lifts BuildPlate to totalPlateDistance
      // (120000, hardcoded in Internal main.h) and LoadPlate to arg 2.
      i2c.sendData(IncomingCommands::PREPPRINT, 0, 60000);
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
    i2c.sendData(IncomingCommands::SMALLSTEP, 1000);
    return gcodeInterpreter->Success;
    break;
  case MenuActions::START_FAN:
    i2c.sendData(IncomingCommands::STARTFAN, 0);
    return gcodeInterpreter->Success;
    break;
  case MenuActions::STOP_FAN:
    i2c.sendData(IncomingCommands::STOPFAN, 0);
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
    //break;

    i2c.sendData(IncomingCommands::PREPPRINT, 10); // 10000
    delay(DELAY_PLATE_LIFT);
    //i2c.sendData(IncomingCommands::SMALLSTEP, 325);
    //delay(DELAY_SMALL_STEP);
    //break;


    waitForResponse(); // Wait for user to load plate and degas chamber


    
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
