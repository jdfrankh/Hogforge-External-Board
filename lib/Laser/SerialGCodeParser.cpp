#include "SerialGCodeParser.h"
SerialGCodeParser::SerialGCodeParser(int baudRate, XY2_100 *galvo,
                                     Laser *laser1)
    : SerialGCodeParser(baudRate, galvo, laser1, nullptr) {

    
    }

SerialGCodeParser::SerialGCodeParser(int baudRate, XY2_100 *galvo,
                                     Laser *laser1, Laser *laser2) {
  _baudRate = baudRate;
  _galvo = galvo;
  _laser1 = laser1;
  _laser2 = laser2;
  _stopped = false;

  Serial.begin(_baudRate);
  Serial.println("Up");

  if(!connectSD()){
    Serial.println("Card Failed to Read");
  }


}

bool SerialGCodeParser::connectSD(){
    return SD.begin(chipSelect);
}

bool SerialGCodeParser::isMediaPresent(){
  return SD.mediaPresent(); 
}

bool SerialGCodeParser::executeCommand(const String &command) {
  if (command.length() == 0) {
    return false;
  }

  _parse(command);
  return true;
}

// This uses UART to determine Gcode, lets retrofit to SPI for SD card 

// The Gcode + Mcodes are already well established here with GRBL.
// It would be wise not to alter too much of the code until most of it is understood


/// @brief Mocdes should directly control externals


// Computer Software has to program in curves

/*
Start Procedure:

@ Start: SD Card inserted. No gas in chamber, material is loaded in LOADING Position

1. Set loader to start 
2. Begin loading Gas proedure (Start vaccum chamber AND Open solenoid Mx, activate argon flow solenid (set fill here), My F 80, Close Solenoid Valve Mz)
3. Actiave Laser (M54) -- DO NOT USE M13 

4. Gcode ... 




*/


void smallDelay(int delaySpeed = 1){
  int countUp = 0;
  for(int j = 0; j <= delaySpeed; j++){
    countUp++;
  }
}

void SerialGCodeParser::resetCounters()
{
//  Serial.println("Resetting counters ");
  for(int i = 0; i < 5; i++){
    counters[i] = 0;
  }
}

int SerialGCodeParser::centerX(int desiredX){

  return desiredX + _x_origin;
}

int SerialGCodeParser::centerY(int desiredY){

  return desiredY + _y_origin;
}

/* Activate the laser to see if it is functional. No movement will be done here*/
int SerialGCodeParser::activateLaser(){
  Serial.println(counters[0]);
  counters[0]++;

  if(counters[0] == 2){
    Serial.println("Prepping Fiber Laser");
    _m13(8, 30000, 128); // Prepare the laser
    _m56("L1"); // Turn Guide Beam on
    _x_origin = buildPlateOriginX;
    _y_origin = buildPlateOriginY;
    _g28();

    int code = _laser1->getAlarmState();

    Serial.printf("Alarm State: %d", code);

    if(code){
      deactivateLaser();
      return Alarm;
    }
    else{
      Serial.println(" --- Proceeding with activation of laser");
    }
    

    return Cycle;
  }
  
  

  counters[1]++;

  if(counters[1] > 200 && counters[2] == 0){
    Serial.println("FIBER LASER STARTING"); 
    _m57("L1"); // Turn the guide beam off

    //_laser1->turnOn();
    _m54(); // Turn Laser on
    return Success;

  }

 
  if(_laser1->getAlarmState() == 0){
    return Cycle;
  }
  else{
    return Alarm;
  }
  

}

int SerialGCodeParser::deactivateLaser(){
  Serial.println("Deactivating Fiber Laser");
  _laser1->turnOff();
   _m14();
    Serial.println("Laser OFF");

    return Success;
}

//Activate the guide beam and be directed by the galvo. Should serve as a test to see if the galvo is functioning normally.

int SerialGCodeParser::activateGuideLaser(std::function<bool()> cancelCheck){

  if(counters[0] == 0){
    Serial.println("Activating Guide Laser");
    _m13(8, 30000, 128); // Prepare the laser
    _m56("L1"); // Turn Guide Beam on
    if(_laser1->getAlarmState()){
      return Alarm;
    }

    delay(10);

    _x_origin = buildPlateOriginX;
    _y_origin = buildPlateOriginY;

    _g0(_x_origin, _y_origin);
    counters[0]++;
  }

  // Perimeter — cancel checked per side
  for(int i = plateSizeY / 2 * -1; i <= plateSizeY /2 ; i += 100){
    if(cancelCheck && cancelCheck()){ _m57("L1"); resetCounters(); return Idle; }
    _g1(plateSizeX/2 * -1, i);
  }
  for(int i = plateSizeX / 2 * -1; i <= plateSizeX /2 ; i += 100){
    if(cancelCheck && cancelCheck()){ _m57("L1"); resetCounters(); return Idle; }
    _g1(i, plateSizeY / 2);
  }
  for(int i = plateSizeY / 2; i >= plateSizeY / 2 * -1; i -= 100){
    if(cancelCheck && cancelCheck()){ _m57("L1"); resetCounters(); return Idle; }
    _g1(plateSizeX / 2, i);
  }
  for(int i = plateSizeX / 2; i >= plateSizeX / 2 * -1; i -= 100){
    if(cancelCheck && cancelCheck()){ _m57("L1"); resetCounters(); return Idle; }
    _g1(i, plateSizeY / 2 * -1);
  }

  // Fill — cancel checked per column
  for(int x = plateSizeX / 2 * -1; x <= plateSizeX /2 ; x += 100){
    if(cancelCheck && cancelCheck()){ _m57("L1"); resetCounters(); return Idle; }
    for(int y = plateSizeY / 2 * -1; y <= plateSizeY /2 ; y += 100){
      _g1(x, y);
    }
  }

  _m57("L1"); // Turn guide beam off when done
  resetCounters();
  return Success;
}

// Traces the perimeter of a custom-sized square so the user can preview where the laser will fire.
int SerialGCodeParser::activateGuideLaserPreview(std::function<bool()> cancelCheck,int sizeX, int sizeY){

  if(counters[0] == 0){
    Serial.printf("Guide Laser Preview: %d x %d\n", sizeX, sizeY);
    _m13(8, 30000, 128);
    _m56("L1"); // Turn guide beam on
    if(_laser1->getAlarmState()){ return Alarm; }
    delay(10);
    _x_origin = buildPlateOriginX;
    _y_origin = buildPlateOriginY;
    _g0(_x_origin, _y_origin);
    counters[0]++;
  }

  // Trace perimeter twice so it is clearly visible.
  // Uses the same corner-anchored coordinate system as activateGuideLaser:
  // (0,0) = bottom-left of build plate, (sizeX,sizeY) = top-right.
  for(int pass = 0; pass < 2; pass++){
     for(int i = plateSizeY / 2 * -1; i <= plateSizeY /2 ; i += 100){
    if(cancelCheck && cancelCheck()){ _m57("L1"); resetCounters(); return Idle; }
    _g1(plateSizeX/2 * -1, i);
  }
  for(int i = plateSizeX / 2 * -1; i <= plateSizeX /2 ; i += 100){
    if(cancelCheck && cancelCheck()){ _m57("L1"); resetCounters(); return Idle; }
    _g1(i, plateSizeY / 2);
  }
  for(int i = plateSizeY / 2; i >= plateSizeY / 2 * -1; i -= 100){
    if(cancelCheck && cancelCheck()){ _m57("L1"); resetCounters(); return Idle; }
    _g1(plateSizeX / 2, i);
  }
  for(int i = plateSizeX / 2; i >= plateSizeX / 2 * -1; i -= 100){
    if(cancelCheck && cancelCheck()){ _m57("L1"); resetCounters(); return Idle; }
    _g1(i, plateSizeY / 2 * -1);
  }
  }

  _m57("L1");
  resetCounters();
  return Success;
}

//Make a cube like shape with the fiber laser to test its functionality
int SerialGCodeParser::activateFiberLaserDebug(std::function<bool()> cancelCheck, int power){

  Serial.println("Activating Fiber Laser Debug Mode");

  if(counters[0] == 0){
    _m57("L1"); // Turn the guide beam off

    _x_origin = buildPlateOriginX;
    _y_origin = buildPlateOriginY;

    _g28(); // Home galvo

    _m13(power, 50000, 128); // Prepare the laser
    _m54(); // Turn Laser on
    _g0(0,0); // Also home galvo (center of chamber) 
    int code = _laser1->getAlarmState();

    Serial.printf("Alarm State: %d", code);

    if(code){
      _laser1->turnOff();
      _m55(); // Turn off laser
      return Alarm;
    }
    else{
      Serial.println(" --- Proceeding with activation of laser");
    }
    

    counters[0]++;

    _m54(); // Re-enable laser (turned off by _g0 rapid move above)

  }

  Serial.println("Scanning perimeter...");

  for(int i = 0; i < plateSizeY; i++){
    if(cancelCheck && cancelCheck()){ _m55(); resetCounters(); return Idle; }
     _g1(0, i);
  
  }

  for(int i = 0; i < plateSizeX; i++){
    if(cancelCheck && cancelCheck()){ _m55(); resetCounters(); return Idle; }
     _g1(i, plateSizeY);

  }

  for(int i = plateSizeY; i > 0; i--){
    if(cancelCheck && cancelCheck()){ _m55(); resetCounters(); return Idle; }
     _g1(plateSizeX, plateSizeY - i);

  }

  for(int i = plateSizeX; i > 0; i--){
    if(cancelCheck && cancelCheck()){ _m55(); resetCounters(); return Idle; }
     _g1(plateSizeX - i, 0);

  }

  Serial.println("Scanning fill...");

  for(int i = 0; i < plateSizeX; i += 5){
    if(cancelCheck && cancelCheck()){ _m55(); resetCounters(); return Idle; }
    if(i % 100 == 0){ Serial.printf("  Fill column %d / %d\n", i, plateSizeX); }
    for(int j = 0; j < plateSizeY; j++){
      if(cancelCheck && cancelCheck()){ _m55(); resetCounters(); return Idle; }
      _g1(i, j);
  
    }
  }

  _m55();

  resetCounters();
  return Success;
   //delay(10);

  
}

int SerialGCodeParser::deactivateGuideLaser(){


  _m57("L1"); // Turn the guide beam off
    if(_laser1->getAlarmState()){
      return Alarm;

    }

  return Cycle;
}

// Go ahead and add some commands, and retrofit this .cpp file to implement a reader from an SD card

void SerialGCodeParser::listen() {


  File gcodeFile = SD.open("gcode.txt"); 

  if(gcodeFile){
    while (gcodeFile.available()) {
      String command = gcodeFile.readStringUntil('\n');
      while (gcodeFile.available() > 0) {
        command += "\n" + gcodeFile.readStringUntil('\n');
      }
      int start = 0;
      int end = command.indexOf('\n');
      while (end != -1) {
        String singleCommand = command.substring(start, end);
        gcodeFile.println(singleCommand);
        // Parse runs the GCODE command associated with the value
        _parse(singleCommand);
        start = end + 1;
        end = command.indexOf('\n', start);
      }
      // Handle the last command if there is no trailing newline
      if (start < command.length()) {
        String singleCommand = command.substring(start);
        gcodeFile.println(singleCommand);
        _parse(singleCommand);
      }
    
    
    // Keep this item for later
    _galvo->tick();
    }
  }
}
