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
#include "Adafruit_MPL3115A2.h"
#include<stdlib.h>

//Create instances of the objects
PVCloud pvcloud;
Adafruit_MPL3115A2 mySensor;//SENSOR FOR PRESSURE/ALTITUDE/TEMPERATURE
int cont = 0;

void setup() {
  Serial.begin(9600);
  logMessage("SETUP BEGIN!");
  pinMode(13, OUTPUT);//BLINK
  
  blinkSpecial(3);
  delay(3000);
  blinkSpecial(4);
  
  logMessage("SETUP - Configurando Programa...");

  logMessage("Inicializando Sensor...");
  if(!mySensor.begin()){
    logMessage("Sensor no esta presente...");
  }
  
  Serial.println("SETUP Completo!");
  blinkSpecial(3);  
}

void loop() { 
  /**
   * STEPS: 
   * STEP 1 - Capture Sensor Data (TEMPERATURE, PRESSURE)
   * STEP 2 - SEND Sensor Data to pvCloud
   * STEP 3 - Record ()
   * STEP 4 - Send Recorded file to pvCloud
   * STEP 5 - GET INTERVAL SETTINGS FROM pvCloud
   **/
   logMessage("Leyendo temperatura...");
   float temperature = readTemperature();
   if(temperature > -1000){
      String strTemp = convertFloatToString(temperature);
      logMessage("Temperatura: " + strTemp);
      logMessage("Leyendo presion...");


      float pressure = readPressure();
      if( pressure > -1000){
        String strPress = convertFloatToString(pressure);
          logMessage("Presion: " + strPress);
          logMessage("Enviando datos a pvCloud...");
          if(sendDataToPVCloud(strTemp, strPress)){
              logMessage("Capturando sonido...");
              if(captureSound()){
                  logMessage("Enviando sonido a pvCloud...");
                  if(sendSoundTopvCloud()){
                    
                  } else {
                    logMessage("No pude enviar el sonido a pvCloud");
                  }
              } else {
                logMessage("No pude grabar sonido");
              }
          } else {
               logMessage("No pude enviar los datos a pvCloud");
          }
          
      } else {
          logMessage("No pude leer la presion");
      }
   } else {
      logMessage("No pude leer la temperatura");
   }

   delay(10000);
}

float readTemperature(){
   float tempC = mySensor.getTemperature();
   return tempC;
}

float readPressure(){
  float pascals = mySensor.getPressure();
  // Our weather page presents pressure in Inches (Hg)
  // Use http://www.onlineconversion.com/pressure.htm for other units
  // float inchesHG = pascals/3377;
  return pascals;
}

bool sendDataToPVCloud(String temperature, String pressure){
  // {"Temperature":"255.03","Pressure":"19200.00"}
  String pvCloudValue = "{\"Temperature\":\""+temperature + "\",\"Pressure\":\"" + pressure + "\"}";
  logMessage("pvCloud Value: " + pvCloudValue);
  pvcloud.Write("TEMP_PRESS",pvCloudValue);
  return true;
}

bool captureSound(){
  String recordCommand = "arecord -f cd -c 1 -d 10 -r 44100 -D hw:2,0 soundfile.wav";
  recordCommand = "ls > soundfile.wav";
  system(recordCommand.buffer);    

  return true;
}

bool sendSoundTopvCloud(){
  pvcloud.SendFile("WAVFILE","soundfile.wav");
  return true;
}

long prevMillis = 0;
void logMessage(String message){
  long currentMillis = millis();
  if(prevMillis == 0) prevMillis = millis();
  
  String completeMessage = "|";
  completeMessage = completeMessage + currentMillis;
  completeMessage = completeMessage + "| ";
  completeMessage = completeMessage + message;
  completeMessage = completeMessage + " Duration: ";
  long duration = currentMillis - prevMillis;
  
  completeMessage += duration;

  prevMillis = currentMillis;
  Serial.println(completeMessage);
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

String convertFloatToString(float value){
  return String(int(value)) + "." + String(getDecimal(value));
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
    float pressure = mySensor.getPressure();
    pressure = pressure/100;

    dataPressure += String(int(pressure))+ "."+ String(getDecimal(pressure));

    //Lectura de la temperatura
    float temperature = mySensor.getTemperature();
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




