#pragma once
#include <Arduino.h>
#include <Wire.h>


#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

class RotaryEncoder {
public:
    RotaryEncoder(int encoderCLK, int encoderDT);
    ~RotaryEncoder();
    void begin();
    int32_t getPosition();
    void readEncoder();         // non‑static handler called from ISR bridge
    void setPosition(int32_t newPosition);
    void setMinMax(int32_t minPosition, int32_t maxPosition);
    void setWrapAround(bool wrap);
    void setStepsPerNotch(int steps);
    



private:
    int clkPin;
    int dtPin;
    int swPin;

    long debounceDelay = 50; // milliseconds
    long lastEncoderMoveTime = 0; // To manage debounce timing

    bool minMaxSet = false;
    // --- State Variables (MUST be volatile for use in ISR) ---
    volatile int32_t encoderPos = 0;
    volatile int lastClkState = LOW;

    int currentClkState;
    int dtState;

    uint32_t minPos = 0;
    uint32_t maxPos = 0;

    int steps = 1; // Default to 1 step per notch

    bool wrapAround = false;
    
    static RotaryEncoder* instance;
    static void rotaryEncoderISR();

};





#endif // ROTARY_ENCODER_H


