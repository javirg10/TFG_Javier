#include "BluetoothSerial.h"
#include <Wire.h>
#include "SparkFun_BNO080_Arduino_Library.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
  #error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT; //Objeto para acceder a la librería Bluetooth
BNO080 myIMU; //Objeto para acceder a la librería de la IMU BNO080


////Definición de pines\\\\
//LED
byte pinLED = 12;
//Bus I2C
byte pinSDA = 26;
byte pinSCL = 27;
int freqI2C = 400000; //Fast Mode -> 400kHz
//IMU BNO080
byte pinBOOTN = 5;
byte pinINT = 32;
byte pinRST = 25;

//Variables BNO080
float q[4]; //Array para almacenar los 4 datos de los cuaterniones
float yaw; //Por la ubicaión de la IMU, el YAW es el ángulo que define el desempeño en las sentadillas
float pitch; //No se usa
float roll; //Nose usa


String VarString_BT = "7"; //Numero en formato string que envía Smartphone a ESP32 
int dato_BT = 7; //VarString_BT en formato entero

bool start_send_data = 1; //1->Envío dato por Bluetooth al Smartphone // 0->No envío datos por BT al Smartphone

//Función de configuración. Se ejecuta al inicio.
void setup() 
{
  //Configuración del pin conectado al LED. Encendido siempre.
  pinMode(pinLED, OUTPUT);
  digitalWrite(pinLED, HIGH);

  //Configuración puerto serie para depuración
  Serial.begin(115200); 

  //Configuración puerto serie para el Bluetooth
  SerialBT.begin("Sentadillas"); //Nombre Bluetooth

  //Configuración I2C -> OJO!! Quitar la configuracion en las librerias de los sensores I2C
  Wire.begin(pinSDA, pinSCL, freqI2C);

  //Configuracion de la IMU BNO080
  myIMU.begin(0x4B, Wire, pinINT); //Dirección I2C, Objeto I2C y pin de Interrupción (Hace poolling del pin INT)
  myIMU.enableRotationVector(10); //Configuración para acceder a los cuaterniones a 100Hz -> 10ms
  //Desactivar el Reset del BNO080
  pinMode(pinRST, OUTPUT);
  digitalWrite(pinRST, HIGH);
}

void loop() 
{
  if(SerialBT.available()) //Dato disponible enviado por el Smartphone al ESP32
  {
    VarString_BT = SerialBT.readStringUntil('\n'); //Leo dato hasta recibir \n
    if(VarString_BT.length()>1) //Descarto datos de longitud mayor a 1, es decir, solo acepto datos del 0 al 9
    {
      dato_BT = 0; //Por defecto a 0 -> IMU funcionando
    }
    else
    {
      dato_BT = (int)VarString_BT.toInt(); //Convertir el dato recibido a entero
    }

    //Decodifico el dato que me ha enviado el Smartphone
    switch(dato_BT) //Dato que me envia el Smartphone Send LF
    {
      case 0: //Empezar a mandar datos por BT al Smartphone
        //Vuelvo a configurar la IMU para que haga reset y asegurarme de que funciona correctamente
        myIMU.begin(0x4B, Wire, pinINT); 
        myIMU.enableRotationVector(10);
        start_send_data = 1; //Enviar datos por Bluetooth activado
        break; 
             
      case 1: //Dejar de mandar datos por BT al Smartphone
        start_send_data = 0;
        break;
        
      case 2: //Mismo caso que el 0
        myIMU.begin(0x4B, Wire, pinINT);
        myIMU.enableRotationVector(10);
        start_send_data = 1;
        break;

      case 3: //Envio cadena "Parado#" por Bluetooth al Smartphone
        SerialBT.println("Parado");
        SerialBT.println("#");
        break;  
                                   
      default: //Estado por defecto 

        break;
    }
  } //Fin dato disponible enviado por el Smartphone al ESP32

  //Enviar Yaw por Bluetooth??
  if(start_send_data  == 1)
  {
    if(myIMU.dataAvailable() == true) //Poolling INT pin
    { 
      //Leo los datos de los cuaterniones
      if(myIMU.shtpData[5] == SENSOR_REPORTID_ROTATION_VECTOR && myIMU.shtpHeader[2] == CHANNEL_REPORTS && myIMU.shtpData[0] == SHTP_REPORT_BASE_TIMESTAMP)
      {
        q[0] = myIMU.getQuatI();
        q[1] = myIMU.getQuatJ();
        q[2] = myIMU.getQuatK();
        q[3] = myIMU.getQuatReal();
        
        //Calcular ángulos de Euler con los cuaterniones
        //Envío el YAW por Bluetooth
        yaw = -atan2(2.0f * (q[1] * q[2] + q[0] * q[3]), q[0] * q[0] + q[1] * q[1] - q[2] * q[2] - q[3] * q[3])*180.0f / PI;
        SerialBT.print(String(yaw, 2));
        //pitch = -asin(2.0f * (q[1] * q[3] - q[0] * q[2]))*180.0f / PI;
        //SerialBT.println(pitch);
        //roll = atan2(2.0f * (q[0] * q[1] + q[2] * q[3]), q[0] * q[0] - q[1] * q[1] - q[2] * q[2] + q[3] * q[3])*180.0f / PI;  //180.0f / PI  
        //SerialBT.println(roll);
      }
    }
  } //End if(start_send_data  == 1)
  
} //End loop
