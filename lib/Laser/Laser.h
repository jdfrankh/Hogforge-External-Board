#pragma once

//#ifndef LASER_PULSE_BITNESS
//#define LASER_PULSE_BITNESS 8
//#endif

#define guide_beam_l1_pin 9 // Activate guide beam



class Laser {
private:



  int _pin;
  
  unsigned int _pulseFrequency; // 2-80 khz

  double _dutyCycle; // Duty cycle does not do anything for now. An analog signal is oly shot into the PRR channel to control the frequency. 
  bool isParamsSet = false;
  

  //Power Pins, Alarm Pins, Enable Pin, Modulation Pin, PRR Pins
  const int powerPumpPins[8] = {31, 30, 29, 28,36,35,34,33};//{33, 34, 35, 36, 28, 29, 30, 31};
  const int alarmPins[3] = {10, 3, 2};
  const int laserEnablePin = 8; // Should be labeled modulate on the PCB 
  const int MOSignalPin = 6;
  const int ModulateEnable = 12;
  const int PRRPin = 11;

public:

  bool isLaserOn = false;

  enum POWER_STATES: int{
    POWER0 = 0,
    POWER50 = 1,
    POWER75 = 2,
    POWER87 = 3,
    POWER93 = 4,

  }; 

  /// @brief Construct a new Laser object
    //Pulse Repeating Rate PRR : https://www.google.com/search?q=what+is+the+prr+of+a+fiber+laser&client=safari&sca_esv=8c23647d28d0699c&rls=en&sxsrf=AE3TifMOwbcQmKnO-tFLiemx9JeT1JXy6A%3A1759442367565&ei=v_XeaPmjIsnykPIP-56c2QE&ved=0ahUKEwi5_8jKwYaQAxVJOUQIHXsPJxsQ4dUDCBA&uact=5&oq=what+is+the+prr+of+a+fiber+laser&gs_lp=Egxnd3Mtd2l6LXNlcnAiIHdoYXQgaXMgdGhlIHByciBvZiBhIGZpYmVyIGxhc2VyMggQABiABBiiBDIIEAAYogQYiQUyCBAAGIAEGKIEMgUQABjvBUi5EFDwDFi0DnACeAGQAQCYAZIBoAGJAqoBAzAuMrgBA8gBAPgBAZgCBKACmwLCAgoQABiwAxjWBBhHmAMAiAYBkAYIkgcDMi4yoAfcBrIHAzAuMrgHkALCBwUwLjIuMsgHDQ&sclient=gws-wiz-serp
    // In short: The lower the pulse duration and the lower the duty cylce %, the higher the power. Usually welding occurs of tens of hz, but the fiber laser's lowest frequency is in thousands of hz.
  // WARNING: ENABLE MOSIGNAL BEFORE ENABLING LASER THOUGH LASER PIN. CAN CAUSE DAMAGE


  /// @param power The laser's power in *Watts*
  /// @param pulseFrequency The laser's pulse frequency in *Hz*
  /// @param dutyCycle The laser's duty cycle as a *percentage* ([0, 1])
  


  int _power = POWER50; // Power will be a value from 0 to 100, we have to define a custom variable here due to the bit states the fiber laser gives

  Laser(POWER_STATES power, unsigned int pulseFrequency, double dutyCycle);
  int turnOn();
  void turnOff();
  void prepLaser();
  void disableLaser();
  int getAlarmState(); // Should return a two bit value
  // 0 - Temperature Alarm - Most likely overheating has occured
  // 1 - Normal Operatio
  // 2 - High relection alarm - hitting a highly reflective material
  // 3 - MO Alarm - ?? - Probably MO has not be set.
  void setPower(int power);
  void setPulseFrequency(unsigned int pulseFrequency);
  void setDutyCycle(double dutyCycle);
  double getPower();
  unsigned int getPulseFrequency();
  double getDutyCycle();
  double getPowerInJoulesPerSecond();


};