/*
  Didactic Example for pvCloud Library
  
  TEST FOR PULL REQUEST
   
  Provides a reference implementation for pvCloud Library on Intel Edison

  This is part of the pvCloud project.
  
  Created SEP-26-2015 by Jose Nunez (jose.nunez@intel.com)
  Copyright Intel Corp. 2015

Hardware Connections:
 -VCC = 3.3V
 -SDA = A4 (use inline 10k resistor if your board is 5V)
 -SCL = A5 (use inline 10k resistor if your board is 5V)

 */

//Required libraries
#include <Wire.h>
#include "pvcloud_lib.h"
#include "MPL3115A2.h"
#include<stdlib.h>

//Create instances of the objects
PVCloud pvcloud;
MPL3115A2 mySensor;
int cont = 0;

void setup() {
  pinMode(13, OUTPUT);

  blinkSpecial(3);
  
  delay(10000);
  
  blinkSpecial(4);
  
  Serial.begin(9600);
  Serial.println("SETUP BEGIN!");
  mySensor.begin(); // Get sensor online
  
  //Configure the sensor
  mySensor.setModeAltimeter(); // Measure altitude above sea level in meters
  mySensor.setModeBarometer(); // Measure pressure in Pascals from 20 to 110 kPa
   
  mySensor.setOversampleRate(7); // Set Oversample to the recommended 128
  mySensor.enableEventFlags(); // Enable all three pressure and temp event flags 

  Serial.println("Llamando System Call ...");
  delay(3000);
  String fakeCommand = "arecord ";
  fakeCommand += "-d 20 -f dat /home/root/audio_record.wav";
  system(fakeCommand.buffer);

  test_WriteAsync();
  //test_MonitorAsync();
  
  Serial.println("Fin de programa ...");
  delay(3000);
  
  blinkSpecial(6);  
}

void loop() { 

}

long minMillisBeforeNextRequest = 5000;
long requestCompleteMillis = 0;
bool asyncCallInProgress = false;
long errorMillis = 0;
String returnedValue = "";
String prevReturnedValue = "";

String returnedPressure = "";
String returnedTemperature = "";

String prevReturnedPressure = "";
String prevReturnedTemperature = "";
long errorRetryTimeout = 30000;

void test_WriteAsync(){
 
  Serial.println("test_WriteAsync");
  while(1){
    
    String returnedPressure = prevReturnedPressure;
    String returnedTemperature = prevReturnedTemperature;
    
    // make a string for assembling the data to log:
    String dataPressure = "";
    String dataTemperature = "";
  
    //Lectura de la presion atmosferica
    float pressure = mySensor.readPressure();
    pressure = pressure/100;

    dataPressure += String(int(pressure))+ "."+ String(getDecimal(pressure));

    //Lectura de la temperatura
    float temperature = mySensor.readTemp();
    dataTemperature += String(int(temperature))+ "."+ String(getDecimal(temperature));

    //Envio el dato de la presion al pvCloud
    if(! asyncCallInProgress) {
      if(millis()-requestCompleteMillis > minMillisBeforeNextRequest) {
        pvcloud.WriteAsync("PRESSURE",dataPressure);
        pvcloud.WriteAsync("TEMPERATURE",dataTemperature);
        asyncCallInProgress=true;
      }
    } 
    else {
      returnedPressure = pvcloud.Check("PRESSURE");
      if(returnedPressure!= prevReturnedPressure){
        test_WriteAsync_ProcessChange(returnedPressure,0);
      }

      returnedTemperature = pvcloud.Check("TEMPERATURE");
      if(returnedTemperature!= prevReturnedTemperature){
        test_WriteAsync_ProcessChange(returnedTemperature,1);
      }
    } 
  }  
}

//Receives returnedValue and sensorType (0=pressure 1=temperature)
void test_WriteAsync_ProcessChange(String returnedValue, int sensorType){

  if(sensorType == 0)
  {
    Serial.println("Pressure Change Detected");
    Serial.print("PRV: '");
    Serial.print(prevReturnedPressure);
    Serial.print("'   RV: '");
    Serial.print(returnedValue);
    Serial.println("'");
  
    prevReturnedPressure = returnedValue;  
    serialOut("NEW VALUE DETECTED: " + returnedValue);
    Serial.println("'");
  
  }
  else if(sensorType == 1){
    Serial.println("Temperature Change Detected");
    Serial.print("PRV: '");
    Serial.print(prevReturnedTemperature);
    Serial.print("'   RV: '");
    Serial.print(returnedValue);
    Serial.println("'");
  
    prevReturnedTemperature = returnedValue;  
    serialOut("NEW VALUE DETECTED: " + returnedValue);
    Serial.println("'");
  
  }
      
  if(returnedValue=="PVCLOUD_ERROR"){
    errorMillis = millis();
  }  
}

long millisPrev = 0;

void serialOut (String message){
  long currentMillis = millis();
  String completeMessage = "|";
  completeMessage = completeMessage + currentMillis;
  completeMessage = completeMessage + "| ";
  completeMessage = completeMessage + message;
  completeMessage = completeMessage + " Duration: ";
  long diff = currentMillis - millisPrev;
  completeMessage += diff;

  millisPrev = currentMillis;
  Serial.println(completeMessage);
}

//function to extract decimal part of float
long getDecimal(float val)
{
  int intPart = int(val);
  long decPart = 100*(val-intPart); //I am multiplying by 100 assuming that the foat values will have a maximum of 3 decimal places
                                   //Change to match the number of decimal places you need
  if(decPart>0) return(decPart);           //return the decimal part of float number if it is available
  else if(decPart<0)return((-1)*decPart); //if negative, multiply by -1
  else if(decPart=0)return(00);           //return 0 if decimal part of float number is not available
}

void blinkSpecial(int times)
{
    for(int i=0; i<times; i++){
      digitalWrite(13,HIGH);
      delay(300);
      digitalWrite(13,LOW);
      delay(300);  
    }
}
