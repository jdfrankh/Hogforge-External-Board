#include "RotaryEncoder.h"


RotaryEncoder* RotaryEncoder::instance = nullptr;


// C-style ISR wrapper that forwards to the current instance's handler.
// Must be free-standing (not `static` at definition site, that's a different
// keyword from the in-class `static`).
void RotaryEncoder::rotaryEncoderISR() {
    if (instance) {
        instance->readEncoder();
    }
}

RotaryEncoder::RotaryEncoder(int encoderCLK, int encoderDT) {
    // Single-instance design; ISR forwards through `instance`.
    instance = this;
    clkPin = encoderCLK;
    dtPin = encoderDT;
    // attachInterrupt is done in begin() after pinMode, not here, so the
    // ISR can't fire on a floating pin before pull-ups are enabled.
}

RotaryEncoder::~RotaryEncoder() {}


// Called from ISR on every CLK edge. Decodes one count per CLK edge.
//
// Some encoder modules (incl. many KY-040 variants) rest with CLK alternating
// HIGH and LOW between detents, so a single click only produces ONE CLK
// edge, not a full HIGH->LOW->HIGH cycle. If we only counted rising edges
// we would miss every other click. Counting both edges with direction
// derived from (clk == dt) keeps the sign consistent across both halves of
// the quadrature cycle.
//
// No Serial / millis() debounce in here: mechanical encoders fire edges
// every 1-5 ms during a normal spin, so any ms-scale debounce drops counts.
void RotaryEncoder::readEncoder() {
    int clk = digitalReadFast(clkPin);
    int dt  = digitalReadFast(dtPin);

    // Real edge?
    if (clk == lastClkState) {
        return;
    }
    lastClkState = clk;

    // Direction is consistent across both rising and falling edges:
    //   CW  spin: rising edge sees DT==HIGH, falling edge sees DT==LOW  -> clk == dt -> +1
    //   CCW spin: rising edge sees DT==LOW,  falling edge sees DT==HIGH -> clk != dt -> -1
    if (clk == dt) {
        encoderPos++;
    } else {
        encoderPos--;
    }

    if (minMaxSet) {
        if (encoderPos < (int32_t)minPos) {
            encoderPos = wrapAround ? (int32_t)maxPos : (int32_t)minPos;
        } else if (encoderPos > (int32_t)maxPos) {
            encoderPos = wrapAround ? (int32_t)minPos : (int32_t)maxPos;
        }
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
    // INPUT_PULLUP: most KY-040 / generic encoder modules either rely on the
    // MCU's pull-ups or have weak external ones. With plain INPUT the lines
    // float between detents and the ISR fires on noise.
    pinMode(clkPin, INPUT_PULLUP);
    pinMode(dtPin,  INPUT_PULLUP);

    // Prime lastClkState with the pin's real resting level so the very first
    // edge isn't mis-counted as a transition from LOW.
    lastClkState = digitalReadFast(clkPin);

    // Trigger on every CLK edge; readEncoder() filters to rising edges.
    attachInterrupt(digitalPinToInterrupt(clkPin), rotaryEncoderISR, CHANGE);
}

// DEBUG: Print encoder position to Serial every time it changes
static int32_t lastDebugPos = 0;
void RotaryEncoder::debugPrintPosition() {
    int32_t pos = getPosition();
    if (pos != lastDebugPos) {
        Serial.print("[RotaryEncoder] Position: ");
        Serial.println(pos);
        lastDebugPos = pos;
    }
}

