

        // I2C control stuff
#include "Wire.h"

//===================
// Using I2C to send and receive structs between two Arduinos
//   SDA is the data connection and SCL is the clock connection
//   On an Uno  SDA is A4 and SCL is A5
//   On an Mega SDA is 20 and SCL is 21
//   GNDs must also be connected
//===================

#define I2C_INTERNAL_ADDR 0x55

// https://forum.arduino.cc/t/use-i2c-for-communication-between-arduinos/653958/4

class I2CBus{


    private:

       
       
        uint32_t i = 0;

       // const byte otherAddress = 0x55;


    public:

        I2CBus();

        int scan();
        void sendData(int data1, int data2 = 0, int data3 = 0);

       void requestData();


        void init();
        bool update(); // Return true if new data has received. Also update the tx and rx data




    


    



};