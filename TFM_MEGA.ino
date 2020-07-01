// LIBRERIAS //
#include "TimerFive.h"

// CONFIGURACION PINES //
const int FCT = 2; //Final de Carrera Trasero.
const int FCD = 3; //Final de Carrera Delantero.
const int DIR = 13; //Direccion del motor B.
const int VEL = 11; //PWM del motor B.
const int BRAKE = 8; //Freno del motor B; HIGH frena.
const int CURRENT = A5; //Sensado de corriente del motor B. 3,3V (1024) para 2A.
const int TRIGGER = 7; //Pin Trigger Ultrasonidos.
const int ECHO = 6; //Pin Echo Ultrasonidos.
const int ENCODERP_A = 18; //Pin A Encoder Pequeño.
const int ENCODERP_B = 19; //Pin B Encoder Pequeño.
const int ENCODERG = 20; //Pin Encoder Grande.
const int WHEEL_HOLES = 20; //Agujeros de la rueda para el encoder.

// VARIABLES GLOBALES //
String inString, inString1 = ""; //Declarar variable de entrada para interrupcion Serial y Serial2.
int primerRes = 1; //Parametro de primer mensaje del variador de frecuencia.
int vel = 100; //Velocidad de arranque.
int dir, brake, paromarcha = 0; //Variables de direccion, freno y marcha.
const int timeThreshold = 300; //Filtro para rebote de finales de carrera.
long startTime = 0;
int cont, contUltra = 0; //Contador de arranque.
unsigned int rpm = 0; //RPM.
unsigned long time2, time3 = 0; //Tiempo entre ciclos del enconder grande.
volatile int pulsos = 0; //Contador de pulsos del encoder grande.
unsigned long tiempo_medida, tiempo_medida_antiguo, tiempo_total = 0; //Variables para la gestion del tiempo por la funcion millis() para enviar por el puerto serie.
unsigned int primer_tiempo = 1; //Variable para saber si es la primera vez que se gestiona el tiempo en un ensayo.
static volatile unsigned long rebote = 0; //Tiempo para el filtro antirebote.
volatile int tope_atras, tope_alante, paroUltra = 0; //Flags para finales de carrera y ultrasonidos.
volatile unsigned long avance_lineal = 0; //Variable para contabilizar el avance lineal del encoder pequeño.
const unsigned int distancia_cata_ultra = 5; //Distancia en centímetros a la cata para comenzar el ensayo.
const long temporizador_medidas = 62500; //Intervalo para temporizador para obtencion de datos en us. (1/16 SEGUNDO).
//const unsigned int temporizador_medidas = 125000; //Intervalo para temporizador para obtencion de datos en us. (1/8 SEGUNDO).

void setup() {
  pinMode(FCT, INPUT);
  pinMode(FCD, INPUT);
  pinMode(DIR, OUTPUT);
  pinMode(VEL, OUTPUT);
  pinMode(BRAKE, OUTPUT);
  pinMode(CURRENT, INPUT);
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(ENCODERG, INPUT);
  pinMode(ENCODERP_A, INPUT_PULLUP);
  pinMode(ENCODERP_B, INPUT_PULLUP);
  analogWrite(VEL, 0); //Parar motor.
  digitalWrite(DIR, dir); //Configurar sentido de giro inicial.
  digitalWrite(BRAKE, brake); //Quitar freno.
  Serial.begin(9600); //Inicializar el puerto serie.
  Serial2.begin(9600); //Inicializar el puerto serie 2.
  inString.reserve(200); //Reservar 200 bytes para entrada Serial.
  inString1.reserve(200); //Reservar 200 bytes para entrada Serial2.
  analogReference(DEFAULT); //Ajustar la referencia de tension.
  attachInterrupt(digitalPinToInterrupt(FCT), FCTi, CHANGE); //Interrupcion para el final de carrera trasero.
  attachInterrupt(digitalPinToInterrupt(FCD), FCDi, CHANGE); //Interrupcion para el final de carrera delantero.
  Timer5.initialize(temporizador_medidas); //Temporizador para obtencion de datos en us. (1/8 SEGUNDO).
  if (digitalRead(FCT) == HIGH) { //Comprobacion inicial final de carrera trasero.
    tope_atras = 1;
  }
  else {
    tope_atras = 0;
  }
  if (digitalRead(FCD) == HIGH) { //Comprobacion inicial final de carrera delantero.
    tope_alante = 1;
  }
  else {
    tope_alante = 0;
  }
  Serial.println("INICIALIZANDO CONEXION SERIE CON EL VARIADOR DE FRECUENCIA.");
  Serial2.print("CONEXION\n");
  while (Serial2.available() == 0 && cont <= 5) {
    Serial.println(".");
    delay(1000);
    cont++;
  }
  while (Serial2.available()) {
    char inChar = (char)Serial2.read(); //Leer nuevo caracter.
    inString1 += inChar; //Añadir nuevo caracter a la variable de entrada.
  }
  if (cont <= 5 && inString1 == "OK\n") {
    Serial.println("\nCONEXION ESTABLECIDA. LISTO PARA OPERAR.");
  }
  else {
    Serial.println("\nIMPOSIBLE ESTABLECER LA CONEXION.");
  }
  inString1 = "";
}

void loop() {
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read(); //Leer nuevo caracter.
    inString += inChar; //Añadir nuevo caracter a la variable de entrada.
    if (inChar == '\n') { //Comparacion final de entrada.
      String primerPos = inString.substring(0, 1); //Obtener el comando leido.
      int lastStringLength = inString.length(); //Longitud String.
      if (primerPos == "2") {
        String comandoLeido = inString.substring(1, lastStringLength - 1); //Obtener el comando leido.
        if (comandoLeido != "") {
          Serial.println("ENVIADO AL VARIADOR DE FRECUENCIA: " + comandoLeido);
          Serial2.print(comandoLeido + "\n");
        }
      }
      else {
        String comandoLeido = inString.substring(0, lastStringLength - 1); //Obtener el comando leido.
        int paramPosition = inString.indexOf(':');  //Posicion parametro de entrada.
        if (paramPosition != -1) {
          comandoLeido = "";
          comandoLeido = inString.substring(0, paramPosition);
          if (comandoLeido == "VEL" || comandoLeido == "vel") {
            String valorVelocidad = inString.substring(paramPosition + 1, lastStringLength);
            int velocidad = valorVelocidad.toInt();
            if (velocidad < 256 && velocidad > 0) {
              vel = velocidad;
              Serial.print("CAMBIO DE VELOCIDAD: ");
              Serial.println((String)vel);
              if (paromarcha == 1) {
                analogWrite(VEL, vel);
              }
            }
            else {
              Serial.println("VELOCIDAD NO PERMITIDA");
            }
          }
          /*else if (comandoLeido == "" || comandoLeido == "") {

            }*/
          else if (comandoLeido == "" || comandoLeido == " ") {
            Serial.print("");
          }
          else {
            Serial.println("COMANDO NO RECONOCIDO");
          }
        }
        else if (comandoLeido == "START" || comandoLeido == "start") {
          if (paromarcha == 0) {
            if (tope_atras == 1 && dir == 1) {
              Serial.println("NO SE PUEDE AVANZAR, ESTA MUY CERCA DEL TOPE TRASERO");
            }
            else if (tope_alante == 1 && dir == 0) {
              Serial.println("NO SE PUEDE AVANZAR, ESTA MUY CERCA DEL TOPE DELANTERO");
            }
            else {
              paromarcha = 1;
              brake = 0;
              digitalWrite(BRAKE, brake);
              analogWrite(VEL, vel);
              digitalWrite(DIR, dir);
              Serial.println("MOVIENDO BASE");
            }
          }
          else {
            Serial.println("LA BASE YA SE ESTA MOVIENDO");
          }
        }
        else if (comandoLeido == "STOP" || comandoLeido == "stop") {
          if (paromarcha == 1) {
            paromarcha = 0;
            brake = 1;
            digitalWrite(BRAKE, brake);
            analogWrite(VEL, 0);
            Serial.println("DETENIENDO BASE");
          }
          else {
            Serial.println("LA BASE YA ESTA PARADA");
          }
        }
        else if (comandoLeido == "CAMBIO" || comandoLeido == "cambio") {
          Serial.print("CAMBIO DE DIRECCION: ");
          if (dir == 0) {
            dir = 1;
            digitalWrite(DIR, dir);
            Serial.println("RETROCESO");
          }
          else {
            dir = 0;
            digitalWrite(DIR, dir);
            Serial.println("AVANCE");
          }
        }
        else if (comandoLeido == "VEL?" || comandoLeido == "vel?") {
          Serial.print("VELOCIDAD: ");
          Serial.println((String)vel);
        }
        else if (comandoLeido == "EST?" || comandoLeido == "est?") {
          Serial.print("LA BASE ESTA: ");
          if (paromarcha == 0) {
            Serial.println("PARADA");
          }
          else {
            Serial.println("EN MARCHA");
          }
        }
        else if (comandoLeido == "SEN?" || comandoLeido == "sen?") {
          Serial.print("LA BASE ESTA EN: ");
          if (dir == 0) {
            Serial.println("AVANCE");
          }
          else {
            Serial.println("RETROCESO");
          }
        }
        else if (comandoLeido == "TEST" || comandoLeido == "test") {
          test();
        }
        else if (comandoLeido == "CERO" || comandoLeido == "cero") {
          cero();
        }
        /*else if (comandoLeido == "" || comandoLeido == "") {

          }*/
        else if (comandoLeido == "" || comandoLeido == " ") {
          Serial.print("");
        }
        else {
          Serial.println("COMANDO NO RECONOCIDO");
        }
      }
      inString = "";
    }
  }
}

void serialEvent2() {
  while (Serial2.available()) {
    if (primerRes == 1) {
      Serial.print("VARIADOR DE FRECUENCIA: ");
      primerRes = 0;
    }
    char inChar = (char)Serial2.read(); //Leer nuevo caracter.
    if (inChar == '\n') { //Comparacion final de entrada.
      Serial.println(inString1);
      inString1 = "";
      primerRes = 1;
    }
    else {
      inString1 += inChar; //Añadir nuevo caracter a la variable de entrada.
    }
  }
}

void checkDistanceUltra() {
  digitalWrite(TRIGGER, LOW); //TRIGGER por proteccion.
  delayMicroseconds(1000);
  digitalWrite(TRIGGER, HIGH); //TRIGGER.
  delayMicroseconds(10);
  digitalWrite(TRIGGER, LOW); //Apagar TRIGGER.
  float tiempo = pulseIn(ECHO, HIGH);  //Diferencia de tiempos entre pulsos.
  float distancia = (tiempo / 2) / 29.1; //Convertir diferencia de tiempos a distancia.
  if (distancia <= distancia_cata_ultra && paromarcha == 1 && dir == 0) { //Distancia a la cata.
    if (contUltra >= 10) { //Filtro medida ultrasonidos.
      paroUltra = 1; //Flag de paro por ultrasonidos.
      paromarcha = 0;
      brake = 1;
      digitalWrite(BRAKE, brake);
      analogWrite(VEL, 0);
      contUltra = 0;
      Serial.println("DETENIENDO BASE. CATA CERCANA (MENOS DE " + (String)distancia_cata_ultra + " CM)");
    }
    contUltra++;
  }
}

void FCTi() {
  if (millis() - startTime > timeThreshold) {
    if (dir == 1) {
      tope_atras = 1;
      paromarcha = 0; //Parar motor.
      brake = 1;
      digitalWrite(BRAKE, brake);
      analogWrite(VEL, 0);
      //Serial.println("TOPE ATRAS");
    }
    else {
      tope_atras = 0;
      //Serial.println("DESACTIVAR TOPE ATRAS");
    }
    startTime = millis();
  }
}

void FCDi() {
  if (millis() - startTime > timeThreshold) {
    if (dir == 0) {
      tope_alante = 1;
      paromarcha = 0; //Parar motor.
      brake = 1;
      digitalWrite(BRAKE, brake);
      analogWrite(VEL, 0);
      //Serial.println("TOPE ALANTE");
    }
    else {
      tope_alante = 0;
      //Serial.println("DESACTIVAR TOPE ALANTE");
    }
    startTime = millis();
  }
}

void contadorPulsos() {
  if (digitalRead(ENCODERG) && (micros() - rebote > 500)) {
    rebote = micros(); //Filtro antirebote.
    pulsos++; //Aumenta pulsos.
  }
}

void encoder_lineal() {
  avance_lineal++;
}

void medidas() {
  detachInterrupt(digitalPinToInterrupt(ENCODERG)); //Deshabilitar interrupcion de counter.
  Timer5.detachInterrupt();
  rpm = (60000 / WHEEL_HOLES) / (millis() - time2) * pulsos; //Calcular RPM.
  time2 = millis(); //Almacenar valor tiempo en ms.
  time3 = micros(); //Almacenar valor tiempo en us.
  pulsos = 0;
  Serial.println("D1:" + (String)rpm); //D1 son las RPM del encoder grande enviadas por puerto serie a Matlab.
  Serial.println("D2:" + (String)analogRead(CURRENT)); //D2 es la corriente que consume el motor pequeño (desplazamiento) enviada por puerto serie a Matlab en valor analógico.
  Serial.println("D3:" + (String)avance_lineal); //D3 es el avance lineal del motor.
  if (primer_tiempo == 1) {
    Serial.println("D4:0"); //D4 es el tiempo acumulado exacto en ms que ocurre en el proceso de medida utilizando la funcion millis().
    primer_tiempo = 0;
  }
  else {
    tiempo_total += time3 - tiempo_medida_antiguo;
    Serial.println("D4:" + (String)tiempo_total); //D4 es el tiempo acumulado exacto en ms que ocurre en el proceso de medida utilizando la funcion millis().
  }
  tiempo_medida_antiguo = time3;
  attachInterrupt(digitalPinToInterrupt(ENCODERG), contadorPulsos, RISING); //Habilitacion de la interrupcion de counter de nuevo.
  Timer5.attachInterrupt(medidas);
}

void cero() {
  if (tope_atras == 0) {
    Serial.println("WORK"); //Variable de indicación de proceso en marcha.
    Serial.println("RETROCEDIENDO A POSICION INICIAL, ESPERE...");
    dir = 1;
    paromarcha = 1;
    brake = 0;
    digitalWrite(BRAKE, brake);
    digitalWrite(DIR, dir);
    analogWrite(VEL, 255);
    while (tope_atras == 0);
    Serial.println("POSICION INICIAL");
    Serial.println("NOWORK"); //Variable de indicación de fin de proceso en marcha.
  }
  else {
    Serial.println("LA BASE ESTA EN POSICION INICIAL, NO ES NECESARIO RETROCEDER");
  }
}

void cerotest() {
  if (tope_atras == 0) {
    Serial.println("RETROCEDIENDO A POSICION INICIAL");
    dir = 1;
    paromarcha = 1;
    brake = 0;
    digitalWrite(BRAKE, brake);
    digitalWrite(DIR, dir);
    analogWrite(VEL, 255);
    while (tope_atras == 0);
    Serial.println("POSICION INICIAL");
  }
  else {
    Serial.println("LA BASE ESTA EN POSICION INICIAL, NO ES NECESARIO RETROCEDER");
  }
}

void test() {
  if (digitalRead(FCT) == HIGH) { //Comprobacion inicial final de carrera trasero.
    tope_atras = 1;
  }
  else {
    tope_atras = 0;
  }
  if (digitalRead(FCD) == HIGH) { //Comprobacion inicial final de carrera delantero.
    tope_alante = 1;
  }
  else {
    tope_alante = 0;
  }
  paroUltra = 0; //Reset variable ultrasonidos.
  avance_lineal = 0; //Reset variable de avance lineal.
  primer_tiempo = 1; //Reset variable de tiempo para funcion millis().
  tiempo_total = 0; //Reset variable acumulativa de tiempo.
  Serial.println("WORK"); //Variable de indicación de proceso en marcha.
  Serial.println("INICIANDO TEST, ESPERE...");
  cerotest(); //Volver a posicion inicial.
  Serial.println("APROXIMACION A LA CATA");
  dir = 0; //Mover base hasta la distancia de seguridad a la cata.
  paromarcha = 1;
  brake = 0;
  digitalWrite(BRAKE, brake);
  digitalWrite(DIR, dir);
  analogWrite(VEL, 255);
  while (paroUltra == 0) {
    checkDistanceUltra();
  }
  delay(1000);
  Serial.println("ARRANCANDO MOTOR");
  Serial2.print("SEN:DER\n");
  Serial2.print("VEL:5\n");
  Serial2.print("START\n");
  delay(2000);
  Serial2.print("VEL:10\n");
  delay(2000);
  Serial2.print("VEL:20\n");
  delay(2000);
  Serial.println("REALIZANDO MEDICIONES");
  attachInterrupt(digitalPinToInterrupt(ENCODERG), contadorPulsos, RISING); //Interrupcion para el contador del encoder grande.
  attachInterrupt(digitalPinToInterrupt(ENCODERP_A), encoder_lineal, CHANGE); //Interrupcion A para encoder pequeño.
  attachInterrupt(digitalPinToInterrupt(ENCODERP_B), encoder_lineal, CHANGE); //Interrupcion B para encoder pequeño.
  Timer5.attachInterrupt(medidas); //Habilitar interrupcion temporizada para toma de medidas.
  dir = 0; //Avanzar.
  paromarcha = 1;
  brake = 0;
  digitalWrite(BRAKE, brake);
  digitalWrite(DIR, dir);
  analogWrite(VEL, 150);
  while (tope_alante == 0) {
  }
  detachInterrupt(digitalPinToInterrupt(ENCODERG)); //Deshabilitar interrupcion de contadorPulsos.
  detachInterrupt(digitalPinToInterrupt(ENCODERP_A)); //Deshabilitar interrupcion A de encoder_lineal.
  detachInterrupt(digitalPinToInterrupt(ENCODERP_B)); //Deshabilitar interrupcion B de encoder_lineal.
  Timer5.detachInterrupt();
  Serial.println("DETENIENDO MOTOR");
  Serial2.print("STOP\n");
  delay(3000);
  Serial2.print("SEN:IZQ\n");
  Serial2.print("VEL:5\n");
  Serial2.print("START\n");
  delay(1000);
  Serial.println("EXTRAYENDO BROCA DE LA CATA");
  dir = 1; //Retroceder.
  paromarcha = 1;
  brake = 0;
  digitalWrite(BRAKE, brake);
  digitalWrite(DIR, dir);
  analogWrite(VEL, 150);
  delay(4000);
  Serial2.print("STOP\n");
  Serial2.print("SEN:DER\n");
  cerotest();
  Serial.println("TEST FINALIZADO");
  Serial.println("FINTEST"); //Variable de indicación de fin de proceso en marcha para guardar datos.
  while (Serial2.available() > 0) {
    Serial2.read();
  }
}
