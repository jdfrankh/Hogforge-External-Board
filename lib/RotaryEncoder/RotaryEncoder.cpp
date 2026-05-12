#include "RotaryEncoder.h"


RotaryEncoder* RotaryEncoder::instance = nullptr;


// C-style ISR wrapper that forwards to the current instance's handler
static void RotaryEncoder::rotaryEncoderISR() {
    if (instance) {
        instance->readEncoder();
    }
}
/*
static void RotaryEncoder::switchISR() {
    if (instance2) {
        instance2->handleSwitch();
    }
}
*/

RotaryEncoder::RotaryEncoder(int encoderCLK, int encoderDT) {

    // store pointer for ISR bridge (single-instance design)
    instance = this;
   // instance2 = this;
    clkPin = encoderCLK;
    dtPin = encoderDT;
    attachInterrupt(digitalPinToInterrupt(encoderCLK), rotaryEncoderISR, CHANGE);
  //  attachInterrupt(digitalPinToInterrupt(encoderSW), switchISR, FALLING);
}

RotaryEncoder::~RotaryEncoder() {
    // Destructor
}





void RotaryEncoder::readEncoder(){
      // Read the current state of the CLK pin
    long time = millis();
    
    if(time - lastEncoderMoveTime > debounceDelay) {
      currentClkState = digitalReadFast(clkPin); // PINC4 & (1<< 2);
      dtState = digitalReadFast(dtPin);
      
      // Check if the CLK pin has moved from LOW to HIGH (RISING edge)
      // This is the ideal trigger point for a reliable count
          // Read the DT pin state to determine the direction of rotation
    if(lastClkState != currentClkState){
      Serial.printf("CLK: %d, DT: %d\n", currentClkState, dtState);
      dtState == currentClkState ? encoderPos-- : encoderPos++;
    }

    if(minMaxSet){
        if(encoderPos < minPos){
            if(wrapAround){
                encoderPos = maxPos;
            } else {
                encoderPos = minPos;
            }
        } else if(encoderPos > maxPos){
            if(wrapAround){
                encoderPos = minPos;
            } else {
                encoderPos = maxPos;
            }
        }
    }



    lastClkState = currentClkState;
    lastEncoderMoveTime = time;

    }
}

void RotaryEncoder::setPosition(int32_t newPosition) {
    encoderPos = newPosition;
}

int32_t RotaryEncoder::getPosition() {
    return encoderPos;
}

void RotaryEncoder::setMinMax(int32_t minPosition, int32_t maxPosition) {
    minPos = minPosition;
    maxPos = maxPosition;
    minMaxSet = true;
}

void RotaryEncoder::setWrapAround(bool wrap) {
    wrapAround = wrap;
}

void RotaryEncoder::begin(){
    pinMode(clkPin, INPUT);
    pinMode(dtPin, INPUT);
  
  // We use CHANGE to catch both rising and falling edges, but the ISR logic only counts on RISING.
    attachInterrupt(digitalPinToInterrupt(clkPin), rotaryEncoderISR, CHANGE);
;
}

