#include "Laser.h"
#include <Arduino.h>

// There are several pins here that need additional definition for simple functionality

Laser::Laser( POWER_STATES power, unsigned int pulseFrequency,
             double dutyCycle) {

  // Set pin modes for laser control pins
 

  for(int i = 0; i < sizeof(powerPumpPins); i++){
    pinMode(powerPumpPins[i], OUTPUT);
  }
  for(int i = 0; i < sizeof(alarmPins); i++){
    pinMode(alarmPins[i], INPUT);
  }
  pinMode(laserEnablePin, OUTPUT);
  pinMode(MOSignalPin, OUTPUT);
  pinMode(ModulateEnable, OUTPUT);
  pinMode(guide_beam_l1_pin, OUTPUT);

  // Use registries to set frequency for PRR - 20Khz - 60kHz -- Set low as possible to enourage welding behavior

  pinMode(PRRPin, OUTPUT);

  turnOff();


  setPower(power);
  _power = power;
  _pulseFrequency = pulseFrequency; 
  setPulseFrequency(pulseFrequency);
  _dutyCycle = dutyCycle;

  isParamsSet = true;


}


void Laser::prepLaser(){ // Set Interlock high, enable high, and sync on

  setPower(_power);

  analogWrite(PRRPin, _dutyCycle); // 0 - 255 - sync pin

  digitalWrite(MOSignalPin, HIGH); // Set interlock high or pin 6
  delay(500);

  digitalWrite(laserEnablePin, HIGH); // Set pin 8 high or enable
  

}

void Laser::disableLaser(){ // Set Interlock high, enable high, and sync on

  digitalWrite(laserEnablePin, LOW); // Set pin 8 high or enable

  delay(500);

  digitalWrite(MOSignalPin, LOW); // Set interlock high or pin 6

  setPower(0);

  //analogWrite(PRRPin, _dutyCycle); // 0 - 255 - sync pin

  
  

  
  

}


int Laser::turnOn() {
  
// Note: This is expecting that the pulse frequency is already set. 

  

  // SET PRR FREQUENCY HERE

  
  digitalWrite(ModulateEnable, HIGH);
  //delay(500);

  return getAlarmState();
}

int Laser::getAlarmState(){
  byte getTwoBitAlarm = 0b00;

  // Create the two bit alarm
  getTwoBitAlarm = digitalRead(alarmPins[0]) | digitalRead(alarmPins[1]);

  return int(getTwoBitAlarm); 
}

void Laser::turnOff() { // 
  // Make sure laser is OFF s

  digitalWrite(ModulateEnable, LOW);

  delay(500);
  
    //Turn off the power to all pins first

  //Change the frequency of the PPR to zero
  // Use registries to alter the duty cycle % 
  //----------------------------------------------------

  
  bool isLaserOn = false;
  bool isParamsSet = false;


}


void Laser::setPower(int power) { 
  _power = power;

  if(_power > 8){
    _power = 8;
  }
  else if(_power < 0){
    _power = 0;
  }

  //Enable each bit based on the power
  for(int i = 0; i < _power; i++){
    if(power > i){
      Serial.printf("Setting pin: %d to HIGH", powerPumpPins[i]);
      Serial.println();
      digitalWrite(powerPumpPins[i], HIGH);
    }
  }

 }

void Laser::setPulseFrequency(unsigned int pulseFrequency) {

  bool wasLaserOn = false;
  /* Setting the freuquency allows for different welding patterns for
  the laser, this parameter should be treated as a one-time assignment before
  the print begins 
  */
  // Make sure the laser if off
  if(digitalRead(laserEnablePin)){ // If the laser is enabled, turn it off
    digitalWrite(laserEnablePin, LOW);
    wasLaserOn = true;
  }

  delay(5); 

  _pulseFrequency = pulseFrequency;
  analogWriteFrequency(PRRPin, _pulseFrequency); // 0 - 255
  // Alter the frequency for the laser here

  delay(5);

  if(wasLaserOn){
    digitalWrite(laserEnablePin, HIGH);
  }

}

void Laser::setDutyCycle(double dutyCycle) {
   bool wasLaserOn = false;

  // Make sure the laser is off
  if(digitalRead(laserEnablePin)){ // If the laser is enabled, turn it off
    digitalWrite(laserEnablePin, LOW);
    wasLaserOn = true;
  }

 
  if (dutyCycle > 255) {
    dutyCycle = 255;
  } else if (dutyCycle < 10) {
    dutyCycle = 10;
  }
  _dutyCycle = dutyCycle;

  // Set the duty cycle for the machine


  delay(10);

  if(wasLaserOn){
    digitalWrite(laserEnablePin, HIGH);
  }

}

double Laser::getPower() { return _power; }

unsigned int Laser::getPulseFrequency() { return _pulseFrequency; }

double Laser::getDutyCycle() { return _dutyCycle; }

double Laser::getPowerInJoulesPerSecond() {
  return _power * 0.001 * _dutyCycle;
}