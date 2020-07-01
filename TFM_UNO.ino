// LIBRERiAS //
#include <TimerOne.h>

// CONFIGURACIoN PINES //
int ZC = 2; //ZeroCrossing. La interrupcion 0 esta en el pin digital 2.
int PR = 7; //Control del rele (direccion de giro).
int TRIAC = 6; //Control del TRIAC para tension en los condensadores.
int IGBT1 = 4;
int IGBT2 = 13;
int IGBT3 = 12;
int IGBT4 = 11;
int VP = A0; //Probe de tension en los condensadores.

// VARIABLES GLOBALES //
int cycle = 10; //Variable para eleccion del ciclo de trabajo de los IGBT's.
int vel = 20; //Velocidad inicial 20ms.
int sentido = 0; //Variable para sentido de giro.
int pTriac = 100; //Porcentaje de tension para el disparo del TRIAC.
volatile unsigned long timeOld = 0; //Variable para el calculo del disparo.
String inString = ""; //Declarar variable de entrada para interrupcion Serial.

void setup() {
  pinMode(IGBT1, OUTPUT);
  pinMode(IGBT2, OUTPUT);
  pinMode(IGBT3, OUTPUT);
  pinMode(IGBT4, OUTPUT);
  pinMode(TRIAC, OUTPUT);
  pinMode(PR, OUTPUT);
  pinMode(ZC, INPUT);
  pinMode(VP, INPUT);
  digitalWrite(IGBT1, LOW);
  digitalWrite(IGBT2, LOW);
  digitalWrite(IGBT3, LOW);
  digitalWrite(IGBT4, LOW);
  digitalWrite(TRIAC, LOW);
  digitalWrite(PR, LOW);
  Serial.begin(9600); //Inicializar el puerto serie.
  inString.reserve(200); //Reservar 200 bytes para entrada.
  analogReference(DEFAULT); //Ajustar la referencia de tension.
  Timer1.initialize(1000.*vel); //Configurar interrupcion para saltar cada initSpeed (uS).
  attachInterrupt(digitalPinToInterrupt(ZC), triacControl, CHANGE); //PIN DIGITAL 2. Interrupcion que salta cuando la señal ZC se activa para coordinar el disparo del TRIAC.
}

void loop() {
  unsigned long timeDif = micros() - timeOld;
  if ((timeDif > ((100 - pTriac) * 100)) && (timeDif <= 11000)) { //Limite por si hay desconexion de la señal ZC.
    digitalWrite(TRIAC, HIGH);
  }
  else {
    digitalWrite(TRIAC, LOW);
  }
}

// INTERRUPCIoN Y COMANDOS PARA EL PUERTO SERIE RS-232 //
void serialEvent() {
  while (Serial.available()) { //Comprobar si hay datos disponibles en el puerto serie para leer.
    char inChar = (char)Serial.read(); //Leer nuevo caracter.
    inString += inChar; //Añadir nuevo caracter a la variable de entrada.
    if (inChar == '\n') { //Comparacion final de entrada.
      int lastStringLength = inString.length(); //Longitud String.
      String comandoLeido = inString.substring(0, lastStringLength - 1); //Obtener el comando leido.
      int paramPosition = inString.indexOf(':');  //Posicion parametro de entrada.
      // CASOS PARA CONTROL POR COMANDOS //
      if (paramPosition != -1) {
        comandoLeido = "";
        comandoLeido = inString.substring(0, paramPosition);
        if (comandoLeido == "VEL" || comandoLeido == "vel") {
          String valorVelocidad = inString.substring(paramPosition + 1, lastStringLength);
          int velocidad = valorVelocidad.toInt();
          if (velocidad >= 5) { //Velocidad en milisegundos.
            vel = velocidad;
            Timer1.setPeriod(500.*vel); //Configuracion de la interrupcion en uS.
            Serial.print("CAMBIO DE VELOCIDAD A: ");
            Serial.print(vel);
            Serial.print(" ms\n");
          }
          else {
            Serial.print("VELOCIDAD NO PERMITIDA\n");
          }
        }
        else if (comandoLeido == "SEN" || comandoLeido == "sen") {
          if (cycle == 10) {
            String valorGiro = inString.substring(paramPosition + 1, lastStringLength);
            if (valorGiro == "DER\n" || valorGiro == "der\n") {
              Serial.print("SENTIDO DE GIRO: DERECHA\n");
              sentido = 0;
              digitalWrite(PR, LOW);
            }
            else if (valorGiro == "IZQ\n" || valorGiro == "izq\n") {
              Serial.print("SENTIDO DE GIRO: IZQUIERDA\n");
              sentido = 1;
              digitalWrite(PR, HIGH);
            }
            else {
              Serial.print("SENTIDO NO RECONOCIDO\n");
            }
          }
          else {
            Serial.print("NO ES POSIBLE CAMBIAR EL SENTIDO DE GIRO CON EL MOTOR EN MARCHA\n");
          }
        }
        else if (comandoLeido == "CIC" || comandoLeido == "cic") {
          String valorCiclo = inString.substring(paramPosition + 1, lastStringLength);
          int ciclo = valorCiclo.toInt();
          if (ciclo >= 10 && ciclo <= 100) { //Ciclo de trabajo en porcentaje.
            pTriac = ciclo;
            Serial.print("CAMBIO DE CICLO DE TRABAJO A: ");
            Serial.print(ciclo);
            Serial.print(" %\n");
          }
          else {
            Serial.print("CICLO DE TRABAJO NO PERMITIDO\n");
          }
        }
        else if (comandoLeido == "" || comandoLeido == " ") {
          Serial.print("");
        }
        else {
          Serial.print("COMANDO NO RECONOCIDO\n");
        }
      }
      else if (comandoLeido == "VEL?" || comandoLeido == "vel?") {
        Serial.print("VELOCIDAD: ");
        Serial.print(vel);
        Serial.print(" ms\n");
      }
      else if (comandoLeido == "EST?" || comandoLeido == "est?") {
        Serial.print("EL MOTOR ESTA: ");
        if (cycle == 10) {
          Serial.print("PARADO\n");
        }
        else {
          Serial.print("EN MARCHA\n");
        }
      }
      else if (comandoLeido == "SEN?" || comandoLeido == "sen?") {
        Serial.print("EL MOTOR GIRA HACIA LA: ");
        if (sentido == 0) {
          Serial.print("DERECHA\n");
        }
        else {
          Serial.print("IZQUIERDA\n");
        }
      }
      else if (comandoLeido == "CIC?" || comandoLeido == "cic?") {
        Serial.print("EL CICLO DE TRABAJO ES DEL: ");
        Serial.print(pTriac);
        Serial.print(" %\n");
      }
      else if (comandoLeido == "TEN?" || comandoLeido == "ten?") {
        Serial.print("LA TENSION EN LOS CONDENSADORES ES: ");
        Serial.print((1.2573 * analogRead(VP)) - 692.07);
        Serial.print(" V.\n");
      }
      else if (comandoLeido == "START" || comandoLeido == "start") {
        if (cycle == 10) {
          Serial.print("ARRANCANDO MOTOR\n");
          cycle = 0;
          Timer1.setPeriod(500.*vel);
          Timer1.attachInterrupt(generatePulses); //Configurar rutina a realizar cuando salte la interrupcion.
        }
        else {
          Serial.print("EL MOTOR YA ESTA EN MARCHA\n");
        }
      }
      else if (comandoLeido == "STOP" || comandoLeido == "stop") {
        if (cycle != 10) {
          Serial.print("PARANDO MOTOR\n");
          cycle = 10;
          Timer1.detachInterrupt();
          digitalWrite(IGBT1, LOW);
          digitalWrite(IGBT2, LOW);
          digitalWrite(IGBT3, LOW);
          digitalWrite(IGBT4, LOW);
        }
        else {
          Serial.print("EL MOTOR YA ESTA PARADO\n");
        }
      }
      else if (comandoLeido == "CONEXION") {
        Serial.print("OK\n");
      }
      else if (comandoLeido == "" || comandoLeido == " ") {
        Serial.print("");
      }
      else {
        Serial.print("COMANDO NO RECONOCIDO\n");
      }
      inString = ""; //Limpiar registro para nuevo comando.
    }
  }
}

// RUTINA DE GENERACIoN DE PULSOS COMO INTERRUPCIoN POR TIEMPO //
void generatePulses() {
  switch (cycle) {
    case 0:
      digitalWrite(IGBT2, LOW);
      digitalWrite(IGBT4, LOW);
      delay(1);
      digitalWrite(IGBT1, HIGH);
      digitalWrite(IGBT3, HIGH);
      cycle = 1;
      break;
    case 1:
      digitalWrite(IGBT1, LOW);
      digitalWrite(IGBT3, LOW);
      delay(1);
      digitalWrite(IGBT2, HIGH);
      digitalWrite(IGBT4, HIGH);
      cycle = 0;
      break;
    default:
      Timer1.detachInterrupt();
      digitalWrite(IGBT1, LOW);
      digitalWrite(IGBT2, LOW);
      digitalWrite(IGBT3, LOW);
      digitalWrite(IGBT4, LOW);
      break;
  }
}

// RUTINA DE CONTROL DEL TRIAC CUANDO SALTA LA INTERRUPCIoN POR ZC //
void triacControl() {
  timeOld = micros();
}
