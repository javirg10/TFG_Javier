#include "BluetoothSerial.h"
#include <Wire.h>
#include "SparkFun_BNO080_Arduino_Library.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT; // Bluetooth Serial object
BNO080 myIMU;

//LED
byte pinLED = 12;
//I2C
byte pinSDA = 26;
byte pinSCL = 27;
int freqI2C = 400000; //400kHz
//BNO080
byte pinBOOTN = 5;
byte pinINT = 32;
byte pinRST = 25;

//Variables BNO080
float q[4];
float yaw;
float pitch;
float roll;


String VarString_BT = "7"; //Numero en formato string que envía Smartphone a ESP32 
int dato_BT = 7; //VarString_BT en formato entero

bool start_send_data = 1;

void setup() 
{
  //Led encendido siempre
  pinMode(pinLED, OUTPUT);
  digitalWrite(pinLED, HIGH);
  
  Serial.begin(115200);
  
  SerialBT.begin("Sentadillas"); // Bluetooth device name

  //Configurar I2C -> OJO!! Quitar la configuracion en las librerias de los sensores I2C
  Wire.begin(pinSDA, pinSCL, freqI2C);

  //Configuracion de la IMU BNO080
  myIMU.begin(0x4B, Wire, pinINT);
  myIMU.enableRotationVector(10); //Send data update every 50ms
  //Desactivar el Reset
  pinMode(pinRST, OUTPUT);
  digitalWrite(pinRST, HIGH);
}

void loop() 
{
  if(SerialBT.available()) // Dato disponible enviado por el Smarphone al ESP32
  {
    VarString_BT = SerialBT.readStringUntil('\n');
    if(VarString_BT.length()>1)
    {
      dato_BT = 0;
    }
    else
    {
      dato_BT = (int)VarString_BT.toInt();
    }

    //Decodifico el dato que me ha enviado el Smartphone
    switch(dato_BT) //Dato que me envia el Smartphone Send LF
    {
      case 0: // Empezar a mandar datos por BT al Smartphone
        myIMU.begin(0x4B, Wire, pinINT);
        myIMU.enableRotationVector(10); //Send data update every 10 ms
        start_send_data = 1;
        break; 
             
      case 1: // Dejar de mandar datos por BT al Smartphone
        start_send_data = 0;
        break;
        
      case 2: // Resetear la IMU
        myIMU.begin(0x4B, Wire, pinINT);
        myIMU.enableRotationVector(10); //Send data update every 50ms
        start_send_data = 1;
        break;

      case 3: //
        SerialBT.println("Parado");
        SerialBT.println("#");
        break;  
                                   
      default: // Estado por defecto 

        break;
    }
  } //Fin dato disponible enviado por el Smartphone al ESP32


  if(start_send_data  == 1)
  {
    if (myIMU.dataAvailable() == true) //Poolling INT pin
    { 
      //Cuaterniones
      if(myIMU.shtpData[5] == SENSOR_REPORTID_ROTATION_VECTOR && myIMU.shtpHeader[2] == CHANNEL_REPORTS && myIMU.shtpData[0] == SHTP_REPORT_BASE_TIMESTAMP)
      {
        q[0] = myIMU.getQuatI();
        q[1] = myIMU.getQuatJ();
        q[2] = myIMU.getQuatK();
        q[3] = myIMU.getQuatReal();
        yaw = -atan2(2.0f * (q[1] * q[2] + q[0] * q[3]), q[0] * q[0] + q[1] * q[1] - q[2] * q[2] - q[3] * q[3])*180.0f / PI; // Declination
        SerialBT.println(yaw);
//        pitch = -asin(2.0f * (q[1] * q[3] - q[0] * q[2]))*180.0f / PI;
//        SerialBT.println(pitch);
        //roll = atan2(2.0f * (q[0] * q[1] + q[2] * q[3]), q[0] * q[0] - q[1] * q[1] - q[2] * q[2] + q[3] * q[3])*180.0f / PI;  //180.0f / PI  
        //SerialBT.println(roll);
      }
    }
  }
  
} //End loop
