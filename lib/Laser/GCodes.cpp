#include "SerialGCodeParser.h"
#include "defs.h"

#define _galvo_mode_adjusted_x                                                 \
  (_galvo->getX() + (_absoluteMode ? _x_origin : 0))
#define _galvo_mode_adjusted_y                                                 \
  (_galvo->getY() + (_absoluteMode ? _y_origin : 0))

void SerialGCodeParser::_g1(int x, int y) {
  if (!_galvo) return;
  // G1 is a cutting move: re-enable the laser unless stopped or guide beam is active.
  if (!_stopped && !_guide_beam_on) _m54();
  int tx = centerX(x), ty = centerY(y);
  int x0 = _galvo->getX();
  int y0 = _galvo->getY();
  float dx = (float)(tx - x0);
  float dy = (float)(ty - y0);
  float dist = sqrtf(dx * dx + dy * dy); // galvo units

  if (dist < 0.5f) return; // already at target

  if (_feedrate_mm_per_min <= 0.0f) {
    // No feedrate set — single jump
    _galvo->setXY(tx, ty);
    return;
  }

  // One step per galvo unit; gives ~5 µm resolution across the build plate.
  int steps = max(1, (int)dist);

  // Total time for the move at the requested feedrate, divided evenly per step.
  float dist_mm = dist / GALVO_UNITS_PER_MM;
  float total_us = dist_mm / _feedrate_mm_per_min * 60.0e6f;
  unsigned long dwell_us = (unsigned long)(total_us / steps);
  if (dwell_us < 1) dwell_us = 1;

  _galvo->setSettingTime(dwell_us);

  // Walk intermediate points; final call uses exact target to avoid rounding drift.
  for (int i = 1; i < steps; i++) {
    float t = (float)i / (float)steps;
    _galvo->setXY((int)(x0 + dx * t + 0.5f), (int)(y0 + dy * t + 0.5f));
  }
  _galvo->setXY(tx, ty);
}

void SerialGCodeParser::_drawCircle(double x, double y, double i, double j,
                                    double r, bool clockwise) {
  int x0 = _galvo_mode_adjusted_x;
  int y0 = _galvo_mode_adjusted_y;

  double x1 = x0 + i;
  double y1 = y0 + j;

  double radius = r;
  if (r == 0) {
    // IJK method - i is the end point, j is the center
    radius = sqrt(pow(x1 - x0, 2) + pow(y1 - y0, 2));
  }

  if (clockwise) {
    _galvo->circleCW(x1, y1, radius, circle_n_points);
  } else {
    _galvo->circleCCW(x1, y1, radius, circle_n_points);
  }
}

void SerialGCodeParser::_g2(double x, double y, double i = 0, double j = 0,
                            double r = 0) {
  _drawCircle(x, y, i, j, r, true);
}

void SerialGCodeParser::_g3(double x, double y, double i = 0, double j = 0,
                            double r = 0) {
  _drawCircle(x, y, i, j, r, false);
}

void SerialGCodeParser::_g4(double seconds) { delay(seconds * 1000); }

void SerialGCodeParser::_g9(int x, int y) { _galvo->setXY(x, y); }

void SerialGCodeParser::_g17() { _currentPlane = XY; }

void SerialGCodeParser::_g18() { _currentPlane = ZX; }

void SerialGCodeParser::_g19() { _currentPlane = YZ; }

void SerialGCodeParser::_g90() { _absoluteMode = true; }

void SerialGCodeParser::_g91() { _absoluteMode = false; }

void SerialGCodeParser::_g92(int newXRelative, int newYRelative) {
  if (!_absoluteMode)
    return;
  int currentX = _galvo->getX();
  int currentY = _galvo->getY();
  _x_origin = currentX + newXRelative;
  _y_origin = currentY + newYRelative;
}

void SerialGCodeParser::_m13(int peakPower, int pulseFrequency,
                             int pulseWidth) {
  if (_laser1) {
    Serial.println("Laser 1 Detected:"); 
    
    _laser1->setPulseFrequency(pulseFrequency); // Set PRR pin
    _laser1->setDutyCycle(pulseWidth);

    _laser1->setPower(peakPower); // Set power pins
  }
  if (_laser2) {
    _laser2->setPower(peakPower);
    _laser2->setPulseFrequency(pulseFrequency);
    _laser2->setDutyCycle(pulseWidth);
  }

  _laser1->prepLaser(); 
}

void SerialGCodeParser::_m14(){
  _laser1->disableLaser();
}

void SerialGCodeParser::_m52(String laser) {
  Serial.println("M52 not supported");
}

void SerialGCodeParser::_m53(String laser) {
  Serial.println("M53 not supported");
}

void SerialGCodeParser::_g0(int x, int y) {
 // _m54();
   _m55(); 
  _galvo->setXY(centerX(x), centerY(y));
 // _m55();
}

void SerialGCodeParser::_g28() { _g0(_x_origin, _y_origin); }

void SerialGCodeParser::_g28p1(int x, int y) {
  bool currentAbsoluteMode = _absoluteMode;
  _absoluteMode = true;
  _x_origin = x;
  _y_origin = y;
  _absoluteMode = currentAbsoluteMode;
}