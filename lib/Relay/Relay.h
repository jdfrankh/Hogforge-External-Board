#include <Arduino.h>



class Relays {
public:
    Relays(std::vector<int> pins) {}


    enum RelayPin {
        ARGON_SOLENOID = 26,
        VACUUM_PUMP = 27,

    };

    void init( ) {
        pinMode(ARGON_SOLENOID, OUTPUT);
        pinMode(VACUUM_PUMP, OUTPUT);

    }

    void on(RelayPin relayPin) {
        digitalWrite(relayPin, HIGH);

    }

    void off(RelayPin relayPin) {
        digitalWrite(relayPin, LOW);
    }

    
};
