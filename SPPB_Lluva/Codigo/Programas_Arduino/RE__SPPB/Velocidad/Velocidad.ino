#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
  #error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define RXD2 16
#define TXD2 17

//Nuevo. Para el calculo de la MODA.
#define MAX_MEASURES 30

// Bluetooth Serial object
BluetoothSerial SerialBT;

int periodo = 17; // Periodo de envio de datos del medidor laser a Arduino, en ms (57 Hz)

int dist = 0; // Distancia actual (instantanea), en mm
int referencia = 0; //Coge la referencia cuando el paciente esta colocado
bool fijar_referencia = 0; //Solo coger la referencia al comienzo de cada prueba
int ida_vuelta = 2; // IDA -> ida_vuelta=0    VUELTA -> ida_vuelta=1   DEFAULT -> ida_vuelta=2

String laser_raw; // Cadena enviada por el laser al ESP32 -> DIST;00025;AMP;0203;TEMP;1147;VOLT;116\r\n
bool start_medida  = 0; //0 -> Ignora los datos enviados por el laser              1 -> Empieza a coger los datos enviados por el laser por el puerto serie

String VarString_BT = "7"; //Numero en formato string que envía Smartphone a ESP32
byte dato_BT = 7; //VarString_BT en formato entero

int array_dist[6000] = {0}; //Array que almacena los datos del tramo de 4, 6 o 8 metros
//int array_dist_interpolado[6000] = {0}; //Array que almacena los datos del tramo de 4, 6 o 8 metros tras interpolar los valores que son cero


bool send_1medida_BT = 0; //1 -> Mandar solo una medida de distancia al Smartphone
bool send_1medida_loop_BT = 0; //!1 -> Mandar medidas de una en una al Smartphone

int i = 0; //Indice array de distancias a enviar al Smartphone

int dist_anterior = 0;
int dist_filtrada  = 0;
int umbral_filtro = 2000;

String tmp_distancia; //Variable intermedia para pasar del string a numero en la variable distancia
int distancia = 0; //Dato recibido del Smartphone con la distancia a recorrer -> 4, 6 o 8 metros
String tmp_distancia_con_arrancada;
int distancia_con_arrancada = 0;

String tmp_distancia_sin_arrancada;
int distancia_sin_arrancada = 0;

bool inicio = 0;
int margen = 2000; // Margen para evitar acceleración y deceleración al principio y fin de la prueba respectivamente, configurado a 1 metro
char *pin = "12345678";


bool coger_referencias = 0; //Coge varias medidas al iniciar la ida o al iniciar la vuelta
int  array_dist_referencia[20] = {0};
int j = 0; //Indice del array array_dist_referencia
int n_dist_referencia = 7; //Numero de medidas que coge al iniciar cada prueba. Impar para coger el valor del medio del array
int ultimo_indice = 0;
int dist_guardar = 0;

int dist_anterior_tmp = 0;

bool marca_T1_enviada = 1;
bool marca_T2_enviada = 1;
bool marca_T3_enviada = 1;
bool marca_T4_enviada = 1;


bool fin_pruebas_igual = 0;

uint32_t timer;
double dt;

int contador_ceros_2m = 0;

/////////////////////NUEVO...
int margin_wobble = 100; // Margen para detectar si el paciente comenzó a caminar.
String tmp_margin_wobble; // Margen para detectar si el paciente comenzó a caminar (orden enviada desde el Smartphone).

bool start_ref_fijada = false;
uint16_t dist_start_ref = 0;
int counter_1medida_loop = 0;

//variables para el calculo de la MODA. Utilizado en el trayecto de VUELTA.
uint16_t array_wobble[MAX_MEASURES] = {0};
int counter_dist_wobble = 0;
int array_freq_measures[MAX_MEASURES] = {0};
int posicion = 0;
int distance = 0;
int counter_aux = 0;
int largest = 0;
int largest_position = 0;
bool start_wobble = 1;
bool margen_superado = 0;
uint16_t margin_mode = 5;
bool parada_en_ida = 0;

//Distancias utilizadas donde debe situarse el paciente para realizar el trayecto de vuelta.
int dist_min_outoflimit_ref = 0; 
int dist_max_outoflimit_ref = 0;

 

//Función de configuración. Se ejecuta solo al inicio
void setup()
{
  //Puerto serie para depuracion
  Serial.begin(115200*2);
  
  //Configuración puerto serie con el láser
  Serial2.begin(256000, SERIAL_8N1, RXD2, TXD2);

  //Configuración del puerto serie para el Bluetooth
  SerialBT.begin("VELOCIDAD HC-05"); //Nombre del dispositivo Bluetooth
  delay(200);

  //Mandar comando al láser -> Comienza "apagado"
  Serial2.println("D");
}

void loop()
{
  if (SerialBT.available()) //¿Hay un dato disponible enviado por el Smartphone al ESP32?
  {
    //Serial.println("DATO RECIBIDO POR EL SMARTPHONE"); //Depuracion
    
    VarString_BT = SerialBT.readStringUntil('\n');    //El Smartphone me tiene que mandar el dato seguido de LF

    if (VarString_BT.substring(0, 2) == "DW") {
      dato_BT = 9;
    }
    else if (VarString_BT.length() > 1)
    {
      dato_BT = 0;
    }
    else
    {
      dato_BT = (int)VarString_BT.toInt();
    }

    Serial.println(dato_BT);

    //Decodifico el dato que me ha enviado el Smartphone
    // 0 -> Recibo distancia a recorrer (4, 6 o 8 metros)
    // 1 -> El Smartphone me pide una sola distancia para que se la envie
    // 2 -> El Smartphone me notifica que empieza la prueba de IDA (por tanto el paciente ya esta colocado al inicio)
    // 3 -> El Smartphone me notifica que empieza la prueba de VUELTA (por tanto el paciente ya esta colocado al final)
    switch (dato_BT) //Dato que me envia el Smartphone
    {
      //Datos que tengo que recibir para hacer cada prueba
      // 4m     -> Con=4000  y Sin=2000
      // 6m+2m  -> Con=8000  y Sin=6000
      // 8m+2m  -> Con=10000 y Sin=8000


      case 0: // Recibo distancia, formato --> 0XXXXXYYYYY    XXXXX->Distancia con arrancada (mm), incluye 1metro de arranque     YYYYY->Distancia sin arrancada (mm)
        Serial.println("CASE 0");

        tmp_distancia_con_arrancada = VarString_BT.substring(1, 6);
        distancia_con_arrancada = (int)tmp_distancia_con_arrancada.toInt();

        tmp_distancia_sin_arrancada = VarString_BT.substring(6, 11);
        distancia_sin_arrancada = (int)tmp_distancia_sin_arrancada.toInt();

        if (distancia_con_arrancada == distancia_sin_arrancada + 2000)
        {
          fin_pruebas_igual = 1;
        }
        else
        {
          fin_pruebas_igual = 0;
        }

        Serial.print("Con:");
        Serial.println(distancia_con_arrancada);
        Serial.print("Sin:");
        Serial.println(distancia_sin_arrancada);


        SerialBT.println(distancia); //Es necesario que te lo envie?
        SerialBT.println("#");

        start_medida = 0;
        i = 0;
        break;

      case 1: // Mandar  una sola distancia cuando se solicite por el usuario
        Serial.println("CASE 1");
        resetVariables1medida();
        break;

      case 2: // Hacer prueba de IDA
        Serial.println("CASE 2");
        resetVariablesIda();
        break;

      case 3: // Hacer prueba de VUELTA
        Serial.println("CASE 3");
        resetVariablesVuelta();
        break;

      case 4: // Parar prueba
        Serial.println("CASE 4");

        SerialBT.println("Parado");
        SerialBT.println("#");

        Serial.println("Parado");
        Serial.println("#");

        send_array();

        Serial.println("Parado despues send");
        resetVariablesParada();

        break;

      case 5: // Mandar distancias de una en una de forma continuada cuando se solicite por el usuario hasta superar margen de tambaleo (ida o vuelta).
        Serial.println("CASE 5");
        resetVariables1medidaContinuaIda();
        //send_array();
        break;

      case 6: // Parar de mandar  una sola distancia de forma continuada cuando se solicite por el usuario hasta superar margen de tambaleo.
        Serial.println("CASE 6");
        resetVariables1medidaContinuaVuelta();
        //send_array();
        break;

      case 7: //Mandar comando para "iniciar" el laser
        Serial.println("CASE 7");
        Serial2.println("E");
        break;

      case 8: //Mandar comando para "parar" el laser
        Serial.println("CASE 8");
        Serial2.println("D");
        break;

      case 9: //Actualizar el valor del margen de tambaleo.
        Serial.println("CASE 9");
        Serial.println("Valor actual de MARGEN DE TAMBALEO");
        Serial.println(margin_wobble);
        //SerialBT.println("Valor actual de MARGEN DE TAMBALEO");
        //SerialBT.println(margin_wobble);
        
        tmp_margin_wobble = VarString_BT.substring(2);
        margin_wobble = (int)tmp_margin_wobble.toInt();
        
        Serial.println("Seteado valor de MARGEN DE TAMBALEO");
        Serial.println(margin_wobble);      
        //SerialBT.println("Seteado valor de MARGEN DE TAMBALEO");
        //SerialBT.println(margin_wobble);

        break;

      default: // Estado por defecto
        Serial.println("CASE DEFAULT");
        resetVariablesPorDefecto();
        //laser comienza "apagado"
        Serial2.println("D");
        break;
    }
  } //End dato disponible enviado por el Smartphone al ESP32


  if (start_medida == 1)
  {
    //Serial.println("START MEDIDA...");

    if (Serial2.available()) // || send_1medida_loop_BT == 1)
    {
      //Serial.println("SERIAL 2 AVAILABLE");


      //      dt = (double)(micros() - timer); // Calculate delta time
      //      timer = micros();
      //      Serial.println(dt);

      laser_raw = Serial2.readStringUntil('\n');
      String tmp_dist = laser_raw.substring(5, 10);

      dist = (int)tmp_dist.toInt();

      //Filtrar outliers
      if (coger_referencias == 1) //Coge varias medidas al iniciar la ida o al iniciar la vuelta
      {
        //Serial.println("COGE REFERENCIAS");

        array_dist_referencia[j] = dist;
        //          array_dist_referencia[0] = 1;
        //          array_dist_referencia[1] = 9;
        //          array_dist_referencia[2] = 34;
        //          array_dist_referencia[3] = 11;
        //          array_dist_referencia[4] = 2;
        //          array_dist_referencia[5] = 23;
        //          array_dist_referencia[6] = 5;

        j = j + 1;
        if (j == n_dist_referencia)
        {
          //Serial.println("COGE REFERENCIAS J N_DIST");

          if (ida_vuelta == 0) //IDA    supongo que hace bien las medidas por estar cerca y calculo la mediana por si hay algun outlier
          {
            //Serial.println("COGE REFERENCIAS J N_DIST IDA");

            //Serial.println("inicio sort");
            isort(array_dist_referencia, n_dist_referencia);
            //Serial.println("fin sort");
            dist_anterior = array_dist_referencia[(n_dist_referencia - 1) / 2];
            //Serial.print("Mediana: ");
            //Serial.println(dist_anterior);
            //Serial.print("Array ordenado: ");
            //printArray(array_dist_referencia, n_dist_referencia);
          }
          else //VUELTA  Supongo que hace mal las medidas y cojo el numero más cercano al ultimo valor bueno en la IDA
          {
            //Serial.println("COGE REFERENCIAS J N_DIST VUELTA");

            dist_anterior_tmp = array_dist_referencia[ubicacion(array_dist_referencia, 1000 + array_dist[ultimo_indice])]; //Supongo que al finalizar recorre 1 metro de mas hasta que para
            if (dist_anterior_tmp < 1000 + array_dist[ultimo_indice])
            {
              dist_anterior = array_dist_referencia[ubicacion(array_dist_referencia, 2000 + array_dist[ultimo_indice])];
            }
            else
            {
              dist_anterior = dist_anterior_tmp;
            }
            //              Serial.print("Ultimo ida: ");
            //              Serial.println(array_dist[ultimo_indice]);
            //              Serial.print("Cercano: ");
            //              Serial.println(dist_anterior);
          }
          coger_referencias = 0;
          j = 0;

          //            Serial.println("Array referncias: ");
          //            printArray(array_dist_referencia, n_dist_referencia);

        } //if(j == n_dist_referencia)
      } //Fin if(coger_referencias == 1)
      else
      {
        //Serial.println("NO COGE REFERENCIAS");

        //Fijar referencia solo una vez
        if (fijar_referencia == 1)
        {
          //Serial.println("FIJA REFERENCIA SOLO UNA VEZ");

          fijar_referencia = 0;
          referencia = dist_anterior;
        }

        //FILTRO 1: Si de una distancia a la siguente supero el valor del umbral mantengo la distancia anterior y guardo un cero en el array
        if (abs(dist_anterior - dist) > umbral_filtro)
        {
          //Serial.println("APLICAR FILTRO");

          dist_filtrada = dist_anterior;
          dist_guardar = 0;

          //////////////////////////////
          //            contador_ceros_2m = contador_ceros_2m + 1;
          //            if(contador_ceros_2m == 200)
          //            {
          //              SerialBT.println("Parado");
          //              SerialBT.println("#");
          //
          //              ultimo_indice = ultimo_indice - 200;
          //                SerialBT.println(periodo);
          //
          //              for (int k = 0; k <= ultimo_indice; k++)
          //              {
          //                SerialBT.println(array_dist[k]);
          //              }
          //
          //              SerialBT.println("#");
          //              //send_array();
          //
          //              fijar_referencia = 0;
          //              referencia = 0;
          //              ida_vuelta = 0;
          //              start_medida = 0;
          //              inicio = 0;
          //              i = 0;
          //              coger_referencias = 0;
          //              marca_T1_enviada = 0;
          //              marca_T2_enviada = 0;
          //              marca_T3_enviada = 0;
          //              marca_T4_enviada = 0;
          //
          //              contador_ceros_2m = 0;
          //            }
          //////////////////////////////
        }
        else
        {
          dist_filtrada = dist;
          dist_guardar = dist;

          //////////////////////////////
          // contador_ceros_2m = 0;
          //////////////////////////////

        }
        dist_anterior = dist_filtrada; //Actualizar distancia anterior
        //Fin FILTRO 1

        //Serial.println("FIN FILTRO");

        //Filtrar outliers
        switch (ida_vuelta) {
            //Serial.println("CASES PRUEBA IDA INICIADA");
          case 0:  // Sentido IDA: Inicio Prueba (laser) -> Fin prueba
            // Comprobar umbrales PARA COGER LOS INDICES DEL ARRAY DE CON ARRANCADA Y SIN ARRANCADA
            if (dist_filtrada > referencia + margen + distancia_sin_arrancada && inicio == 1) //Llego al final (dist_sin_arranque + 1metro)
            {
              if (fin_pruebas_igual == 1)
              {
                SerialBT.println("T2"); // Notificar cuando acaba a almacenar distancias
                SerialBT.println("#");

                Serial.println("T2"); // Notificar
                Serial.println(dist_guardar); // Notificar
              }

              SerialBT.println("Fin"); // Notificar cuando acaba a almacenar distancias
              SerialBT.println("#");

              Serial.println("Fin"); // Notificar
              Serial.println(dist_guardar); // Notificar

              inicio = 0;
              start_medida = 0;

              array_dist[i] = dist_guardar;
              ultimo_indice = i;

              delay(200);
              send_array();
            }
            else if (dist_filtrada > referencia + distancia_con_arrancada && inicio == 1 && marca_T2_enviada == 1) //Empiezo a almacenar distancias cuando paso el margen de inicio
            {
              SerialBT.println("T2"); // Notificar
              SerialBT.println("#");

              Serial.println("T2"); // Notificar
              Serial.println(dist_guardar); // Notificar

              marca_T2_enviada = 0;

              array_dist[i] = dist_guardar;
              ultimo_indice = i;
              i++;
            }
            else if (dist_filtrada > referencia + margen && inicio == 1 && marca_T1_enviada == 1) //Empiezo a almacenar distancias cuando paso el margen de inicio
            {
              SerialBT.println("T1"); // Notificar
              SerialBT.println("#");

              Serial.println("T1"); // Notificar
              Serial.println(dist_guardar); // Notificar

              marca_T1_enviada = 0;

              array_dist[i] = dist_guardar;
              ultimo_indice = i;
              i++;
            }
            else
            {
              if (i == 0 && send_1medida_BT == 0 && send_1medida_loop_BT == 0 && margen_superado == 1) // Notificar cuando empieza a almacenar distancias
              {
                SerialBT.println("Inicio");
                SerialBT.println("#");

                Serial.println("Inicio"); // Notificar
                Serial.println(dist_guardar); // Notificar
              }
              array_dist[i] = dist_guardar;
              ultimo_indice = i;
              i++;
            }
            break;

          case 1:  // Sentido VUELTA: Fin prueba -> Inicio Prueba (laser)
            // Comprobar umbrales para almacenar medidas en el array
            Serial.println("CASES PRUEBA VUELTA INICIADA");

            if (dist_filtrada < referencia - distancia_sin_arrancada - margen && inicio == 1) //Supero el margen de inicio mas la distancia
            {
              if (fin_pruebas_igual == 1)
              {
                SerialBT.println("T4"); // Notificar cuando acaba a almacenar distancias
                SerialBT.println("#");

                Serial.println("T4"); // Notificar
                Serial.println(dist_guardar); // Notificar
              }

              SerialBT.println("Fin"); // Notificar cuando acaba a almacenar distancias
              SerialBT.println("#");

              Serial.println("Fin"); // Notificar
              Serial.println(dist_guardar); // Notificar

              inicio = 0;
              start_medida = 0;

              array_dist[i] = dist_guardar;
              ultimo_indice = i;
              i++;

              delay(200);
              send_array();


            }
            else if (dist_filtrada < referencia - distancia_con_arrancada && inicio == 1 && marca_T4_enviada == 1) //Empiezo a almacenar distancias cuando paso el margen de inicio
            {
              SerialBT.println("T4"); // Notificar
              SerialBT.println("#");

              Serial.println("T4"); // Notificar
              Serial.println(dist_guardar); // Notificar

              marca_T4_enviada = 0;

              array_dist[i] = dist_guardar;
              ultimo_indice = i;
              i++;
            }
            else if (dist_filtrada < referencia - margen && inicio == 1 && marca_T3_enviada == 1) //Empiezo a almacenar distancias cuando paso el margen de inicio
            {
              SerialBT.println("T3"); // Notificar
              SerialBT.println("#");

              Serial.println("T3"); // Notificar
              Serial.println(dist_guardar); // Notificar

              marca_T3_enviada = 0;

              array_dist[i] = dist_guardar;
              ultimo_indice = i;
              i++;
            }
            else
            {
              if (i == 0 && send_1medida_BT == 0 && send_1medida_loop_BT == 0 && margen_superado == 1)  // Notificar cuando empieza a almacenar distancias
              {
                SerialBT.println("Inicio");
                SerialBT.println("#");

                Serial.println("Inicio"); // Notificar
                Serial.println(dist_guardar); // Notificar
              }

              array_dist[i] = dist_guardar;
              ultimo_indice = i;
              i++;
            }
            break;

          default:
            break;
        }

        if (send_1medida_BT == 1) // Mandar solo una medida si la solicita el usuario
        {
          //Serial.println("MANDAR UNA SOLA MEDIDA");

          send_1medida_BT = 0;

          SerialBT.println(dist);
          SerialBT.println("#");

        }

        if ((send_1medida_loop_BT == 1) && (ida_vuelta == 0)) //Comienzo automático prueba ida-vuelta
        {
          Serial.println("COMIENZO PRUEBA AUTOMÁTICA IDA");
          if (start_ref_fijada) {
            //Comienza a tomar distancias hasta que se supera el margen de tambaleo.
            Serial.println(dist);

            if (dist >= (dist_start_ref + margin_wobble)) {
              Serial.println("SUPERO MARGEN DE TAMBALEO 1");
              Serial.println(dist);
              margen_superado = 1;
              // Hacer prueba de IDA
              send_1medida_loop_BT = 0;
              resetVariablesIda();
              //start_wobble = 0 ;

            }
          }
          else {
            //fijar referencia inicial descartando la primera medida que se toma.
            if (counter_1medida_loop == 10) {
              dist_start_ref = dist;
              start_ref_fijada = true;
              Serial.print("distancia de referencia IDA: ");
              Serial.println(dist_start_ref);
            }
            else {

              counter_1medida_loop = counter_1medida_loop + 1;
              Serial.print("counter_1medida_loop: ");
              Serial.println(counter_1medida_loop);
            }
          }
        } // Fin comienzo automático ida-vuelta.

        if ((send_1medida_loop_BT == 1) && (ida_vuelta == 1)) //Comienzo automático prueba vuelta-ida
        {
          Serial.println("COMIENZO PRUEBA AUTOMÁTICA VUELTA");
          //filtro distancia recibida.

          //Comprobar si hubo parada en el trayecto de IDA.
          if (parada_en_ida)
          {
            //El sanitario debe llevar al paciente a una distancia igual a: dist_start_ref + distancia_con_arrancada + 1000mm aprox.
            //De esta forma no se supera el margen establecido de 2000mm.
            Serial.println("SE DETECTO PARADA");
            dist_min_outoflimit_ref = array_dist[0] + distancia_con_arrancada;
            dist_max_outoflimit_ref = array_dist[0] + distancia_con_arrancada + 2000;
          }
          else {
                        Serial.println("NO SE DETECTO PARADA");

            dist_min_outoflimit_ref = array_dist[ultimo_indice];
            dist_max_outoflimit_ref = array_dist[ultimo_indice] + 2000;
          }


            Serial.println("***********************");
            Serial.print("dist_min_outoflimit_ref: ");
            Serial.println(dist_min_outoflimit_ref);
            Serial.print("dist_max_outoflimit_ref: ");
            Serial.println(dist_max_outoflimit_ref);
            Serial.print("dist_start_ref: ");
            Serial.println(dist_start_ref);
            
            Serial.print("DISTANCIA ACTUAL (LIMITES)");
                          Serial.println(dist);

          //filtro distancia recibida.
          //Se ignoran todas las distancias que no esten en el rango adecuado, no más de dos metros sobre posición inicial establecida en
          //el trayecto de vuelta.
          if ((dist != 0) && (dist >= dist_min_outoflimit_ref) && (dist <= dist_max_outoflimit_ref))
          //if ((dist >= 1000) && (dist <= (3500)))
          {
            //Comprueba si se ha establecido la distancia a la que el paciente se encuentra del medidor de velocidad.
            if (start_ref_fijada) {
              Serial.println(dist);
              //delay(50);
              //Comienza a comparar distancias hasta que se supera el margen de tambaleo.
              if (dist <= (dist_start_ref - margin_wobble)) {
                Serial.println("SUPERO MARGEN DE TAMBALEO 2");
                Serial.println(dist);
  
                //delay(100);
                //delay(50);
  
                // Hacer prueba de VUELTA
                send_1medida_loop_BT = 0;
                resetVariablesVuelta();
              }
            }
            //CODIGO ANTERIOR (ELSE start_ref_fijada)
          /*  else {
              //fijar referencia inicial descartando la primera medida que se toma.
              if (counter_1medida_loop == 10) {
                dist_start_ref = dist;
                start_ref_fijada = true;
                Serial.print("distancia de referencia VUELTA: ");
                Serial.println(dist_start_ref);
              }
              else {
                counter_1medida_loop = counter_1medida_loop + 1;
                Serial.print("counter_1medida_loop: ");
                Serial.println(counter_1medida_loop);
              }
            }
            */
            //NUEVO CODIGO (ELSE start_ref_fijada)
            else
            //Continua estimando la posición del paciente para el trayecto de VUELTA
            {
              //Ha tomado las medidas necesarias y se calcula la moda.
              if (counter_1medida_loop == MAX_MEASURES) 
              {
                //Calcular la moda.
                //dist_start_ref = getModa(array_wobble);
                //Calcular la moda con márgen.
                dist_start_ref = EncontrarModa(array_wobble, MAX_MEASURES, margin_mode);
                start_ref_fijada = true;
                Serial.print("***********************  distancia de referencia VUELTA: ");
                Serial.println(dist_start_ref);
                
              }
              else
              {
                //Comprobar si la medida recibida es "valida". (eliminar outliers)
                ///if ((dist != 0) && (dist >= C) && (dist <= (distancia_con_arrancada + 1500)))
                //if ((dist >= 1000) && (dist <= (3500)))
                //Se considera una medida valida.
               // {
                  //Guardar medida en array_wobble para el calculo de la moda.
                  array_wobble[counter_dist_wobble] = dist;
                  counter_dist_wobble = counter_dist_wobble + 1;
                  counter_1medida_loop = counter_1medida_loop + 1;
    
               // }
              }
            } //Fin if start_ref_fijada
          } // Fin filtro distancia recibida.
        } // Fin comienzo automático ida-vuelta.
      } //Fin else coger referencias
    }
  }
}

void interpolar_array(void)
{
  int contador_ceros = 0;
  int k = 0;
  int posicion_ceros[ultimo_indice + 1] = {0};
  int indice_ceros = 0;
  int separacion = 0;

  for (k = 0; k <= ultimo_indice; k++)
  {
    if (array_dist[k] == 0)
    {
      indice_ceros = k;
      posicion_ceros[indice_ceros] = 1;
      while (array_dist[k + 1] == 0)
      {
        posicion_ceros[indice_ceros] = posicion_ceros[indice_ceros] + 1;
        posicion_ceros[k + 1] = 0;
        k = k + 1;
      }
      indice_ceros = 0;
    }
    else
    {
      posicion_ceros[k] = 0;
    }
  }

  //  for(k = 0; k <= ultimo_indice; k++)
  //  {
  //    Serial.print("posicion_ceros: ");
  //    Serial.println(posicion_ceros[k]);
  //  }

  for (int kk = 0; kk <= ultimo_indice; kk++)
  {
    if (posicion_ceros[kk] != 0)
    {

      if (ida_vuelta == 0) //ida
      {
        separacion = (array_dist[kk - 1 + posicion_ceros[kk] + 1] - array_dist[kk - 1]) / (posicion_ceros[kk] + 1);
      }
      else //vuelta
      {
        separacion = (array_dist[kk - 1] - array_dist[kk - 1 + posicion_ceros[kk] + 1]) / (posicion_ceros[kk] + 1);
      }

      //      Serial.print("Separacion: ");
      //      Serial.println(separacion);

      for (int kkk = 0; kkk <= posicion_ceros[kk] - 1; kkk++)
      {
        if (ida_vuelta == 0) //ida
        {
          array_dist[kk + kkk] = array_dist[kk - 1] + (separacion * (kkk + 1));
        }
        else //vuelta
        {
          array_dist[kk + kkk] = array_dist[kk - 1] - (separacion * (kkk + 1));
        }
      }
    }
  }
  k = 0;
  contador_ceros = 0;
  separacion = 0;
  indice_ceros = 0;
  for (k = 0; k <= ultimo_indice; k++)
  {
    posicion_ceros[k] = 0;
  }
}


// Mandar array al finalizar con formato: periodo*1000 array_dist[0] array_dist[1] ... array_dist[n] #
void send_array(void)
{

  //  array_dist[0] = 5;
  //  array_dist[1] = 10;
  //  array_dist[2] = 0;
  //  array_dist[3] = 0;
  //  array_dist[4] = 40;
  //  array_dist[5] = 30;
  //  array_dist[6] = 0;
  //  array_dist[7] = 0;
  //  array_dist[8] = 0;
  //  array_dist[9] = 50;
  //  array_dist[10] = 0;
  //  array_dist[11] = 0;
  //  array_dist[12] = 0;
  //  array_dist[13] = 70;

  //  array_dist[0] = 70;
  //  array_dist[1] = 0;
  //  array_dist[2] = 0;
  //  array_dist[3] = 0;
  //  array_dist[4] = 50;
  //  array_dist[5] = 0;
  //  array_dist[6] = 0;
  //  array_dist[7] = 0;
  //  array_dist[8] = 40;
  //  array_dist[9] = 30;
  //  array_dist[10] = 0;
  //  array_dist[11] = 11;
  //  array_dist[12] = 0;
  //  array_dist[13] = 1;
  //
  //  ultimo_indice = 14-1;
  //  ida_vuelta = 0;

  //  Serial.println("Array sin interpolar: ");
  //  printArray(array_dist, ultimo_indice+1);
  //  Serial.println("Antes interpolar");
  //  if(dato_BT !=4)
  //  {
  //  interpolar_array();
  //  }
  //  Serial.println("Despues interpolar");
  //  Serial.println("Array interpolado: ");
  //  printArray(array_dist, ultimo_indice+1);

  SerialBT.println(periodo);

  Serial.print("ULTIMO INDICE: ");
  Serial.println(ultimo_indice);
  for (int k = 0; k <= ultimo_indice; k++)
  {
    SerialBT.println(array_dist[k]);
    Serial.println(array_dist[k]);
  }

  SerialBT.println("#");
}


//isort(numbers, sizeof(numbers)/sizeof(numbers[0]));
void isort(int *a, int n)
{
  for (int i = 1; i < n; ++i)
  {
    int j = a[i];
    int k;

    for (k = i - 1; (k >= 0) && (j < a[k]); k--)
    {
      a[k + 1] = a[k];
    }

    a[k + 1] = j;
  }
}


int ubicacion(int vector[], int valor)
{
  int tmp = 0;
  int dif = valor - vector[0];

  for (int ii = 0; ii < 5; ii++) {
    if (abs(valor - vector[ii]) < dif) {
      tmp = ii; // me quedo con esa ubiacion
    }
  }
  return tmp;
}

void printArray(int *a, int n)
{
  for (int i = 0; i < n; i++)
  {
    Serial.print(a[i]);
    Serial.print(' ');
  }
  Serial.println();
}

void resetVariables1medida()
{
  start_medida = 1;
  send_1medida_BT = 1;
  send_1medida_loop_BT = 0;
  i = 0;
}
void resetVariablesIda()
{
  fijar_referencia = 1;
  referencia = 0;
  ida_vuelta = 0;
  start_medida = 1;
  inicio = 1;
  i = 0;
  coger_referencias = 1;
  marca_T1_enviada = 1;
  marca_T2_enviada = 1;
}

void resetVariablesVuelta()
{
  fijar_referencia = 1;
  referencia = 0;
  ida_vuelta = 1;
  start_medida = 1;
  inicio = 1;
  i = 0;
  coger_referencias = 1;
  marca_T3_enviada = 1;
  marca_T4_enviada = 1;
}

void resetVariablesParada()
{
  fijar_referencia = 0;
  referencia = 0;

  if (ida_vuelta == 0) 
  {
    parada_en_ida = true;
  }
  ida_vuelta = 0;
  start_medida = 0;
  inicio = 0;
  i = 0;
  coger_referencias = 0;
  marca_T1_enviada = 0;
  marca_T2_enviada = 0;
  marca_T3_enviada = 0;
  marca_T4_enviada = 0;
  send_1medida_loop_BT = 0;
  send_1medida_BT = 0;


}

void resetVariables1medidaContinuaIda()
{
  start_medida = 1;
  send_1medida_BT = 0;
  send_1medida_loop_BT = 1;
  start_ref_fijada = false;
  dist_start_ref = 0;
  counter_1medida_loop = 0;
  ida_vuelta = 0;
  inicio = 0;
  i = 0;
  start_wobble = 1;
  parada_en_ida = 0;
}

void resetVariables1medidaContinuaVuelta()
{
  start_medida = 1;
  send_1medida_BT = 0;
  send_1medida_loop_BT = 1;
  start_ref_fijada = false;
  dist_start_ref = 0;
  counter_1medida_loop = 0;
  ida_vuelta = 1;
  inicio = 0;
  i = 0;
    start_wobble = 1;


  counter_dist_wobble = 0;
  posicion = 0;
  distance = 0;
  counter_aux = 0;
  largest = 0;
  largest_position = 0;

  dist_min_outoflimit_ref = 0; 
  dist_max_outoflimit_ref = 0;
}

void resetVariablesPorDefecto()
{
  fijar_referencia = 0;
  referencia = 0;
  ida_vuelta = 0;
  start_medida = 0;
  inicio = 0;
  i = 0;
  coger_referencias = 0;
  marca_T1_enviada = 0;
  marca_T2_enviada = 0;
  marca_T3_enviada = 0;
  marca_T4_enviada = 0;

  send_1medida_loop_BT = 0;
  start_ref_fijada = false;
  dist_start_ref = 0;
  counter_1medida_loop = 0;
}

int getModa (int *array) 
{
  //visualizar los datos recogidos.
      Serial.print("ARRAY RECIBIDO EN FUNCION MODA");

  for (counter_dist_wobble = 0; counter_dist_wobble < MAX_MEASURES; counter_dist_wobble++)
  {
    Serial.println(array[counter_dist_wobble]);
  }

  //PREPARAR VARIABLES PARA EL CÁLCULO DE LA MODA.
  //Inicializar array auxiliar
  for (counter_dist_wobble = 0; counter_dist_wobble < MAX_MEASURES; counter_dist_wobble++)
  {
    array_freq_measures[counter_dist_wobble] = 0;
  }

  //Recorrer el array recibido para almacenar en un array auxiliar el número de repeticiones de cada número según la posición ocupada.
  for (counter_dist_wobble = 0; counter_dist_wobble < MAX_MEASURES; counter_dist_wobble++)
  {
    distance = array[counter_dist_wobble];
    posicion = counter_dist_wobble;

    for(counter_aux = counter_dist_wobble; counter_aux < MAX_MEASURES; counter_aux++)
    {
      if(array[counter_aux] == distance)
      {
        array_freq_measures[posicion]++;
      }
    }
  }

  largest = array_freq_measures[0];
  largest_position = 0;

  for(counter_dist_wobble = 0; counter_dist_wobble < MAX_MEASURES; counter_dist_wobble++)
  {
    if(array_freq_measures[counter_dist_wobble] > largest)
    {
      largest_position = counter_dist_wobble;
      largest = array_freq_measures[counter_dist_wobble];
    }
  }
  
//visualizar la frecuencia de los datos recogidos.
      Serial.print("ARRAY FRECUENCIAS EN FUNCION MODA");

  for (counter_dist_wobble = 0; counter_dist_wobble < MAX_MEASURES; counter_dist_wobble++)
  {
    Serial.println(array_freq_measures[counter_dist_wobble]);
  }
  //Visualizar la distancia con mayor frecuencia de aparición.
  Serial.print (" MODA: ");
  Serial.println (array[largest_position]);

  return array[largest_position];
  
}

float EncontrarModa(uint16_t *Datos, int N, uint16_t margen)
{
  register int t, w;                  //Contadores dentor del vector
  uint16_t md, AnteriorModa;      //moda temporal y moda anterior al ciclo
  int cont, antcont;                //Contadores de la repeticion del dato
  
  AnteriorModa=0;
  antcont=0;
  for (t=0; t< N; ++t) {
    md = Datos[t];
    cont = 1;
    for(w=t+1; w < N; ++w)
      //if(md == Datos[w])
      if(Datos[w]<=md+margen && Datos[w]>=md-margen) 
        cont++;
    if(cont > antcont) {
      AnteriorModa = md;
      antcont = cont;
    }
  }
  return AnteriorModa;
}
