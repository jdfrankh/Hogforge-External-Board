#include "I2CBus.h"

I2CBus::I2CBus(){

}

void I2CBus::init(){

    Wire2.setSCL(24);
    Wire2.setSDA(25);
    Wire2.begin(); // join i2c bus

    // Also set up on receive and on event in main
    
}

void I2CBus::sendData(int data1, int data2 = 0, int data3 = 0) {
  uint8_t error = 0;
  do{
    //Serial.printf("Sending Data: %d", data1);
    //Serial.println();
    //Write message to the slave
    Wire2.beginTransmission(I2C_INTERNAL_ADDR);
    Wire2.print(highByte(data1));
    Wire2.print(lowByte(data1));
    Wire2.print(highByte(data2));
    Wire2.print(lowByte(data2));
    Wire2.print(highByte(data3));
    Wire2.print(lowByte(data3));
    error = Wire2.endTransmission(true);
    //Serial.printf("endTransmission: %u\n", error);
    delay(1);
  } while(error != 0);
}


void I2CBus::requestData() {
  //Read 16 bytes from the slave
  uint8_t bytesReceived = Wire2.requestFrom(I2C_INTERNAL_ADDR, 16);
  //Serial.printf("requestFrom: %u\n", bytesReceived);
  if ((bool)bytesReceived) {  //If received more than zero bytes
    uint8_t temp[bytesReceived];

    Wire2.readBytes(temp, bytesReceived);
   // Serial.print(" : ");
    for(int i = 0; i < bytesReceived; i++){
    //  Serial.print(char(temp[i]));
    }
   // Serial.println();
    //log_print_buf(temp, bytesReceived);
}
}



//============



int I2CBus::scan() {


  int nDevices = 2000;
  for (int address = 1; address < 127; address++) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire2.beginTransmission(address);
    int error = Wire2.endTransmission();

    if (error == 0) {
      if (address == 0x50) {
        nDevices =+ 1;
        Serial.println("Found Daughter Board!");
      }
      else if(address == 0x3C){
        Serial.println("Found the screen");
      }
      else{
        nDevices =+ 100;
        Serial.println("Found unknown item on bus");
        Serial.println(address);
      }


    } else if (error==4) {
      Serial.print(F("Unknown error at address 0x"));
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }
  }
  if (nDevices == 0) {
    Serial.println(F("No I2C devices found"));
  } else {
    Serial.println(F("done"));
  }
  Serial.println();

  return nDevices;
}



//=============
