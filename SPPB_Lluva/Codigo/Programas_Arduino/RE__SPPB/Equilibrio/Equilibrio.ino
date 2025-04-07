#include <TimerOne.h>
#include <SoftwareSerial.h>

//Led modulo Bluetooth conectado al pin 3, EN LA PARTE DEL SWITCH
SoftwareSerial ModBluetooth(2, 3); // RX | TX

short int Buzzer = 12;

///////////////// Ubicacion alfombrillas ///////////////////////
//        4
//        3
//      1   2
//      
//      Posición paralela -> 1-2
//      Posición semi-tandem -> 1-3 o 2-3
//      Posición tandem -> 3-4
//      
///////////////// Ubicacion alfombrillas ///////////////////////

//Pines asociados a cada Led
short int Led_1 = 11;
short int Led_2 = 9;
short int Led_3 = 8;
short int Led_4 = 10;

//Pines asociados a cada plantilla
short int alfombrilla_1 = 3;
short int alfombrilla_2 = 0;
short int alfombrilla_3 = 1;
short int alfombrilla_4 = 2;

short int posicion = 4; // 1 paralela, 2 semitandem, 3 tandem
unsigned long counter_timer = 0; // Contador que se incrementa
char VarChar; //Dato recibido por Bluetooth

int tiempo_prueba = 100; //10 segundos
int flag_tiempo = 0; //0->No se ha cumplido el tiempo en equilibrio, 1->Se ha cumplido el tiempo en equilibrio
boolean enable_timer = 0; //1->Timer empieza a contar
int dato_leido = 0; //Hay un dato leido por Bluetooth

//Valor del ADC de cada plantilla
int val_alfombrilla1 = 0;
int val_alfombrilla2 = 0;
int val_alfombrilla3 = 0;
int val_alfombrilla4 = 0;

//Indica si una plantilla esta activada (1) o desactivada (0)
int plantilla1 = 0;
int plantilla2 = 0;
int plantilla3 = 0;
int plantilla4 = 0;

int umbral = 800; //Umbral de cuentas del ADC para considerar que está en equilibrio. 10 bits de 0 a 1024
                  //Si el valor del ADC es menor al umbral se considera que la plantilla está pisada (paciente en equilibrio)

int cont_plantillas_activas = 0; //Evitar que el paciente pise tres o mas plantillas
int posicion_seleccionada_medico = 7; //Para saber que prueba tiene seleccionado el medico en la app

void setup()
{
  Serial.begin(9600);
  
  pinMode(Buzzer, OUTPUT);

  //Configurar pines de los Leds como salidas
  pinMode(Led_1, OUTPUT);
  pinMode(Led_2, OUTPUT);
  pinMode(Led_3, OUTPUT);
  pinMode(Led_4, OUTPUT);
  
  digitalWrite(Buzzer, LOW);

  //Apagar los Leds por defecto
  digitalWrite(Led_1, LOW);
  digitalWrite(Led_2, LOW);
  digitalWrite(Led_3, LOW);
  digitalWrite(Led_4, LOW);

  ModBluetooth.begin(9600);
  
  Timer1.initialize(100000); // Dispara cada 100 ms
  Timer1.attachInterrupt(incrementa_contador); // Activa la interrupcion y la asocia a la función para incrementar el contador
}

void loop()
{
  //Lectura valor de todas las plantillas
  val_alfombrilla1 = analogRead(alfombrilla_1);
  val_alfombrilla2 = analogRead(alfombrilla_2);
  val_alfombrilla3 = analogRead(alfombrilla_3);
  val_alfombrilla4 = analogRead(alfombrilla_4);

  //Encender Leds asociados a las plantillas que estan pisadas
  //Se encienden a nivel alto


  //Encender solo las luces asociadas a cada posicion
  if(posicion_seleccionada_medico == 0) //Paralela
  {
    //Plantilla 1 pisada -> Enciendo el Led1
    if (val_alfombrilla1 < umbral) 
    {
      digitalWrite(Led_1, HIGH);
      plantilla1 = 1;
      cont_plantillas_activas++;
    }
    else //Plantilla 1 no pisada
    {
      digitalWrite(Led_1, LOW);
      plantilla1 = 0;
    }

    //Plantilla 2 pisada -> Enciendo el Led2
    if (val_alfombrilla2 < umbral) 
    {
      digitalWrite(Led_2, HIGH);
      plantilla2 = 1;
      cont_plantillas_activas++;
    }
    else //Plantilla 2 no pisada
    {
      digitalWrite(Led_2, LOW);
      plantilla2 = 0;
    }

    //Apago los otros Leds
    digitalWrite(Led_3, LOW); 
    digitalWrite(Led_4, LOW);
  }
  else if(posicion_seleccionada_medico == 1) //Semi-tandem
  {
    //Plantilla 1 pisada -> Enciendo el Led1
    if (val_alfombrilla1 < umbral) 
    {
      digitalWrite(Led_1, HIGH);
      plantilla1 = 1;
      cont_plantillas_activas++;
    }
    else //Plantilla 1 no pisada
    {
      digitalWrite(Led_1, LOW);
      plantilla1 = 0;
    }

    //Plantilla 2 pisada -> Enciendo el Led2
    if (val_alfombrilla2 < umbral) 
    {
      digitalWrite(Led_2, HIGH);
      plantilla2 = 1;
      cont_plantillas_activas++;
    }
    else //Plantilla 2 no pisada
    {
      digitalWrite(Led_2, LOW);
      plantilla2 = 0;
    }

    //Plantilla 3 pisada -> Enciendo el Led3
    if (val_alfombrilla3 < umbral) 
    {
      digitalWrite(Led_3, HIGH);
      plantilla3 = 1;
      cont_plantillas_activas++;
    }
    else //Plantilla 3 no pisada
    {
      digitalWrite(Led_3, LOW);
      plantilla3 = 0;
    }

    //Apago los otros Leds
    digitalWrite(Led_4, LOW);
  }
  else if(posicion_seleccionada_medico == 2) //Tandem
  {
    //Plantilla 3 pisada -> Enciendo el Led3
    if (val_alfombrilla3 < umbral) 
    {
      digitalWrite(Led_3, HIGH);
      plantilla3 = 1;
      cont_plantillas_activas++;
    }
    else //Plantilla 3 no pisada
    {
      digitalWrite(Led_3, LOW);
      plantilla3 = 0;
    }
    
    //Plantilla 4 pisada -> Enciendo el Led4
    if (val_alfombrilla4 < umbral) 
    {
      digitalWrite(Led_4, HIGH);
      plantilla4 = 1;
      cont_plantillas_activas++;
    }
    else //Plantilla 4 no pisada
    {
      digitalWrite(Led_4, LOW);
      plantilla4 = 0;
    }    

    //Apago los otros Leds
    digitalWrite(Led_1, LOW);
    digitalWrite(Led_2, LOW);
  }


//  //Notificar que mas de dos plantillas pisadas
//  if(cont_plantillas_activas>=3)
//  {
//    ModBluetooth.println("Pies mal colocados");
//    ModBluetooth.println("#");
//    digitalWrite(Buzzer, HIGH);
//  }
//  else
//  {
//    digitalWrite(Buzzer, LOW);
//  }
  cont_plantillas_activas = 0;

  //Hay un dato disponible para leer por Bluetooth
  if (ModBluetooth.available())
  {
    VarChar = ModBluetooth.read();  //Smartphone envia los datos sin CR ni LF
    Serial.println(VarChar);
    dato_leido = 1;
    switch (VarChar) {
      case '1': //Iniciar la Posición paralela
        posicion = 1;
        counter_timer = 0;
        enable_timer = 1;
        ModBluetooth.println("Midiendo tiempo en posición paralela...");
        ModBluetooth.println("#");
        break;
        
      case '2': //Iniciar la Posición semitandem
        posicion = 2;
        counter_timer = 0;
        enable_timer = 1;
        ModBluetooth.println("Midiendo tiempo en posición semitandem...");
        ModBluetooth.println("#");
        break;
        
      case '3': //Iniciar la Posición tandem
        posicion = 3;
        counter_timer = 0;
        enable_timer = 1;
        ModBluetooth.println("Midiendo tiempo en posición tandem...");
        ModBluetooth.println("#");
        break;
        
      case '4':
        posicion=4;
        counter_timer = 0;
        enable_timer = 0;
        break;
        
      case '5': //Mensaje de advertencia
        posicion=5;
        //ModBluetooth.println("...");
        //ModBluetooth.println("#");
        ModBluetooth.println("Por favor, coloque los pies en una posición correcta");
        ModBluetooth.println("#");
        break;
        
      case '6': //Posicion paralela seleccionada por el medico
        posicion_seleccionada_medico = 0;
        dato_leido = 0;
        break;
        
      case '7': //Posicion semi-tandem seleccionada por el medico
        posicion_seleccionada_medico = 1;
        dato_leido = 0;
        break;
        
      case '8': //Posicion tandem seleccionada por el medico
        posicion_seleccionada_medico = 2;
        dato_leido = 0;
        break;  
                      
      default:
        dato_leido = 0;
        break;
    }
  }

  if (dato_leido == 1)
  {
    switch (posicion) {
      case 1: //Posicion paralela (solo se evaluan las plantillas 1-2)      
        if((plantilla1==1) and (plantilla2==1)) //Las dos plantillas activas
        {                      
          if((val_alfombrilla1 > umbral) or (val_alfombrilla2 > umbral)) 
          {
            if(flag_tiempo == 0)
            {
              perdida_equilibrio();  
            }   
          }
          if(flag_tiempo == 1)
          {
            equilibrio();     
          }          
        }
        else //Alguna plantilla se ha desactivado
        {
          if(flag_tiempo == 0)
          {
            perdida_equilibrio();
          }
        }
        break;
        
      case 2: //Posición semitandem (se evaluan plantillas 1-3 y 2-3)
        if((plantilla1==1) and (plantilla3==1)) //Las dos plantillas activas 1 y 3
        {                      
          if((val_alfombrilla1 > umbral) or (val_alfombrilla3 > umbral)) 
          {
            perdida_equilibrio();     
          }
          if(flag_tiempo == 1)
          {
            equilibrio();     
          }          
        }
        else if((plantilla2==1) and (plantilla3==1)) //Las dos plantillas activas 2-3
        {
          if((val_alfombrilla2 > umbral) or (val_alfombrilla3 > umbral)) 
          {
            perdida_equilibrio();     
          }
          if(flag_tiempo == 1)
          {
            equilibrio();     
          }           
        }
        else //Alguna plantilla se ha desactivado
        {
          perdida_equilibrio();
        }
        break;
        
      case 3: //Posición tandem (se evaluan plantillas 3-4)
        if((plantilla3==1) and (plantilla4==1)) //Las dos plantillas activas 3-4
        {                      
          if((val_alfombrilla3 > umbral) or (val_alfombrilla4 > umbral)) 
          {
            perdida_equilibrio();     
          }
          if(flag_tiempo == 1)
          {
            equilibrio();     
          }          
        }
        else //Alguna plantilla se ha desactivado
        {
          perdida_equilibrio();
        }
        break;
        
      case '4':     
        flag_tiempo = 0;
        enable_timer = 0;
        counter_timer = 0;
        dato_leido = 0;
        break;
      
      default:
        //dato_leido = 0; 
        break;
    }    
  }
}

//Funcion que incrementa el contador hasta que llega al tiempo establecido (tiempo_prueba)
void incrementa_contador()
{
  if(enable_timer == 1)
  {
    counter_timer++;
  }
  if(counter_timer == tiempo_prueba)
  {
    flag_tiempo = 1;
    counter_timer = 0;
  }
}

void perdida_equilibrio()
{
  flag_tiempo = 0;
  enable_timer = 0;
  counter_timer = 0;

if(VarChar == '6' || VarChar == '7' || VarChar == '8')
{
  ModBluetooth.println("NO Se superó el tiempo en equilibrio exitosamente");
  ModBluetooth.println("#");
}
else
{
  ModBluetooth.println("NO Se superó el tiempo en equilibrio exitosamente");
  ModBluetooth.println("#");
}
  dato_leido = 0;
   
}

void equilibrio()
{
  flag_tiempo = 0;
  enable_timer = 0;
  counter_timer = 0;
  ModBluetooth.println("Se superó el tiempo en equilibrio exitosamente");
  ModBluetooth.println("#"); 
  dato_leido = 0;
   
}
