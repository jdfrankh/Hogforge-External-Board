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

    _laser1->turnOn();

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

int SerialGCodeParser::activateGuideLaser(){


  if(counters[0] == 0){
    Serial.println("Activating Guide Laser");
    _m13(8, 30000, 128); // Prepare the laser
    _m56("L1"); // Turn Guide Beam on
    if(_laser1->getAlarmState()){
      return Alarm;

    }

  
    //_g90();
    //_g92(buildPlateOriginX,buildPlateOriginY); //Absolute Distance mode 
    //_g28();

    delay(10);

    _x_origin = buildPlateOriginX;
    _y_origin = buildPlateOriginY;

    _g0(_x_origin, _y_origin);
    //_g1('X', buildPlateOriginX);
    //_g1('Y', buildPlateOriginY);

    //_g28();
    

    
   // _g1('X', buildPlateOriginX-plateSizeX);
   // _g1('Y', buildPlateOriginY-plateSizeY);

    counters[0]++; 
  }

  //Serial.println(counters[1]);

  //50 is slow
  //500 is relatively fast
  //1599 is extremley fast 
  counters[1] += 100;

  switch(counters[2]){
    case 0:
      _g0(0, counters[1]);
    break;
    case 1:
      _g0(counters[1], plateSizeY);
    break;
    case 2:
      _g0(plateSizeX, plateSizeY - counters[1]);
    break;
    case 3:
      _g0(plateSizeX - counters[1], 0);
    case 4:
      _g0(counters[3], counters[1]  );
    break;

    default:

    break;
  }
  smallDelay();

  counters[1]++;
  if(counters[1] > plateSizeX ){
    counters[1] = 0;

    if(counters[2] !=  4 || counters[3] == plateSizeX ){
      counters[2]++;
      counters[3] = 0;
    }
    else{
      counters[3] += 100;
    }

    if(counters[2] > 4){
      counters[2] = 0;
      
    }
  }

  counters[1]++;

  return Cycle;
}

int SerialGCodeParser::activateFiberLaserDebug(){



  if(counters[0] == 0){
    _m57("L1"); // Turn the guide beam off

    _x_origin = buildPlateOriginX;
    _y_origin = buildPlateOriginY;

    _g28(); // Home galvo

    _m13(70, 50000, 155); // Prepare the laser
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
    

  }

  for(int i = 0; i < plateSizeY; i++){
     _g0(0, i);
     smallDelay();

  }

  for(int i = 0; i < plateSizeX; i++){
     _g0(i, plateSizeY);
     smallDelay();

  }
  for(int i = plateSizeY; i > 0; i--){
     _g0(plateSizeX, plateSizeY - i);
     smallDelay();

  }
  for(int i = plateSizeX; i > 0; i--){
     _g0(plateSizeX - i ,  0);
     smallDelay();

  }

  for(int i = 0; i < plateSizeX; i += 5){
    for(int j =0; j < plateSizeY; j++){
      _g0(i, j);
      smallDelay();
    }

  }

  _m55();

  resetCounters();
  return Success;
   //delay(10);

  
  

  //Serial.println(counters[1]);

  //50 is slow
  //500 is relatively fast
  //1599 is extremley fast 

  /*
  counters[1] += 75;

  switch(counters[2]){
    case 0:
      _g0(0, counters[1]);
    break;
    case 1:
      _g0(counters[1], plateSizeY);
    break;
    case 2:
      _g0(plateSizeX, plateSizeY - counters[1]);
    break;
    case 3:
      _g0(plateSizeX - counters[1], 0);
    case 4:
      _g0(counters[3], counters[1]  );
    break;

    default:

    break;
  }
  smallDelay();

  counters[1]++;
  if(counters[1] > plateSizeX ){
    counters[1] = 0;

    if(counters[2] !=  4 || counters[3] == plateSizeX ){
      counters[2]++;
      counters[3] = 0;
    }
    else{
      counters[3] += 75;
    }

    if(counters[2] > 4){
      counters[2] = 0;
      
    }
  }

  counters[1]++;

  return Cycle;

  */ 
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
