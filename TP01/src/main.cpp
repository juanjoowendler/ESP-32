#include <Arduino.h>
#include "Device.h"
// Libreria para encoder en ESP32 (usa PCNT internamente)
#include <ESP32Encoder.h>

// Pines del hardware
#define PIN_LED_VENTILADOR 23
#define PIN_LED_RIEGO 2
#define PIN_POTENCIOMETRO 32  // 34
#define PIN_SENSOR_DHT 33 //14

// Pines del encoder rotatorio
#define PIN_ENCODER_CLK 18
#define PIN_ENCODER_DT 5
#define PIN_BOTON_ENCODER 19

// Inicialización del dispositivo con display y sensor
Device invernadero(128, 64, -1, PIN_SENSOR_DHT, DHT22);

// Variables globales
int umbralHumedadMinima;         // Umbral de humedad (40–60%)
int opcionMenu = 0;              // Opción seleccionada en el menú
int opcionAnterior;              // Opción anterior para detectar cambios
bool riegoBlinkEstado = false;   // Estado de parpadeo del riego
bool tempEstadoAnterior;         // Estado anterior del control de temperatura
bool humEstadoAnterior;          // Estado anterior del control de humedad
bool mostrarMenu = true;         // Indica si se muestra el menú
bool ejecutarOpcion = false;     // Indica si se está ejecutando una opción
bool potenciometroActivo = true; // Control manual de temperatura por potenciómetro
int temperaturaReferencia;       // Temperatura deseada
int estadoVentilador;            // Estado del ventilador
bool riegoActivo = false;        // Estado actual del riego
String lineaSerial;              // Entrada recibida por Serial

// Declaración de funciones
void configurarEntradasSalidas();
void mostrarMenuOpciones();
void ejecutarAccion(float temp, int temperaturaReferencia, float hum);
void cambiarUmbralHumedad();
void cambiarTempManual();
void activarPotenciometro();
void forzarVentilacion();
void forzarRiego();

// Encoder (reemplaza manejo manual por interrupciones)
ESP32Encoder encoder;
int64_t lastEncoderCount = 0;


// ---------------- SETUP ----------------
void setup() {
  Serial.begin(9600);
  configurarEntradasSalidas();

  // Inicializar encoder usando la librería (cuadrupla media)
  // attachHalfQuad usa PCNT y evita manejar ISRs manuales
  encoder.attachHalfQuad(PIN_ENCODER_CLK, PIN_ENCODER_DT);
  encoder.clearCount();
  lastEncoderCount = encoder.getCount();

  invernadero.begin();

  // Definir un umbral inicial de humedad aleatorio entre 40 y 60%
  umbralHumedadMinima = (int)random(40, 61);

  invernadero.showDisplay(String(umbralHumedadMinima) + "%");
  Serial.println("INICIO SISTEMA\nUmbral aleatorio: " + String(umbralHumedadMinima) + "%");

  delay(500);
  mostrarMenuOpciones();
}


// ---------------- LOOP ----------------
void loop() {
  // Leer valor del potenciómetro si está activado
  if(potenciometroActivo){
    temperaturaReferencia = map(analogRead(PIN_POTENCIOMETRO), 0, 4095, -5, 60);
  }

  // Leer encoder y actualizar opción del menú cuando se muestra
  int64_t encCount = encoder.getCount();
  if (mostrarMenu && encCount != lastEncoderCount) {
    int64_t delta = encCount - lastEncoderCount;
    opcionMenu = (opcionMenu + (int)delta) % 6;
    if (opcionMenu < 0) opcionMenu += 6;
    lastEncoderCount = encCount;
  }

  // Leer temperatura actual
  float temperaturaActual = invernadero.readTemp(); 

  // Control automático del ventilador
  if ((temperaturaActual > temperaturaReferencia) != tempEstadoAnterior) { // Si cambia el estado del ventilador
    if (temperaturaActual > temperaturaReferencia){ // Apagar o Encender el ventilador
      digitalWrite(PIN_LED_VENTILADOR, HIGH);
      Serial.println("Ventilador encendido");
    } else {
      digitalWrite(PIN_LED_VENTILADOR, LOW);
      Serial.println("Ventilador apagado");
    }
  }

  // Leer humedad actual
  float humedadActual = invernadero.readHum();

  // Control automático del riego
  if ((humedadActual < umbralHumedadMinima) != humEstadoAnterior) { // Si cambia el estado de la humedad
    if (humedadActual < umbralHumedadMinima) { // Activar o Desactivar el riego
      Serial.println("Riego activado");
      riegoActivo = true;
    } else {
      digitalWrite(PIN_LED_RIEGO, LOW);
      riegoActivo = false;
      riegoBlinkEstado = false;
      Serial.println("Riego desactivado");
    }
  }

  // Si el riego está activo, hacer parpadear el LED
  if (riegoActivo) {
    digitalWrite(PIN_LED_RIEGO, riegoBlinkEstado ? HIGH : LOW); 
    riegoBlinkEstado = !riegoBlinkEstado;
  }

  // Detectar pulsación del botón del encoder
  if (digitalRead(PIN_BOTON_ENCODER) == LOW) {
    if (ejecutarOpcion){
      switch (opcionMenu) {
        case 1: cambiarTempManual(); break;
        case 2: cambiarUmbralHumedad(); break;
        case 3: activarPotenciometro(); break;
        case 4: forzarVentilacion(); break;
        case 5: forzarRiego(); break;
      }
    } else {
      mostrarMenu = !mostrarMenu;
      if (mostrarMenu)
        mostrarMenuOpciones();
    }
  }

  // Actualizar pantalla si cambió la opción
  if (opcionAnterior != opcionMenu)
    mostrarMenuOpciones();

  // Ejecutar la opción seleccionada si no está mostrando el menú
  if (!mostrarMenu)
    ejecutarAccion(temperaturaActual, temperaturaReferencia, humedadActual);

  // Guardar estados anteriores
  tempEstadoAnterior = temperaturaActual > temperaturaReferencia;
  humEstadoAnterior = humedadActual < umbralHumedadMinima;
  opcionAnterior = opcionMenu;

  delay(100);
}


// ---------------- FUNCIONES ----------------

// Configurar entradas y salidas de ESP32
void configurarEntradasSalidas() {
  pinMode(PIN_LED_VENTILADOR, OUTPUT);
  pinMode(PIN_LED_RIEGO, OUTPUT);
  pinMode(PIN_POTENCIOMETRO, INPUT);

  // Encoder (dejar pullups internos para señales estables)
  pinMode(PIN_ENCODER_CLK, INPUT_PULLUP);
  pinMode(PIN_ENCODER_DT, INPUT_PULLUP);
  pinMode(PIN_BOTON_ENCODER, INPUT_PULLUP);
}

// Mostrar menú de opciones en el display
void mostrarMenuOpciones() {
  const String opciones[] = {
      "Mostrar estado invernadero",
      "Modificar temp. referencia",
      "Modificar umbral de humedad",
      "Activar potenciometro",
      "Ventilacion manual",
      "Riego manual"};

  String menu = "";
  for (int i = 0; i < 6; i++) {
    if ((opcionMenu == 4 || opcionMenu == 5) && i == 0) continue; 
    if (i == opcionMenu) {
      menu += "-> " + opciones[i] + "\n";
    } else {
      menu += "- " + opciones[i] + "\n";
    }
  }
  invernadero.showDisplay(menu);
}

// (El manejo del encoder por interrupciones se reemplazó por la librería ESP32Encoder)

// Ejecutar acción según opción elegida
void ejecutarAccion(float temp, int temperaturaReferencia, float hum) {
  switch (opcionMenu) {
    case 0: // Mostrar estado invernadero
      invernadero.showDisplay(
        "Temperatura: " + String(temp) + "'C \nReferencia: " + String(temperaturaReferencia) +
        "'C \n\nHumedad: " + String(hum) + "% \nUmbral: " + String(umbralHumedadMinima) + "%");
      break;
    case 1: // Modificar temp. referencia
      ejecutarOpcion = true;
      invernadero.showDisplay("Ingrese nueva temp. (-5 a 60'C)\n\nPresione boton para confirmar");
      break;
    case 2: // Modificar umbral de humedad
      ejecutarOpcion = true;
      invernadero.showDisplay("Ingrese nuevo umbral (40-60%)\n\nPresione boton para confirmar");
      break;
    case 3: // Activar potenciometro
      ejecutarOpcion = true;
      invernadero.showDisplay("Potenciometro activado.");
      break;
    case 4: // Ventilacion manual
      ejecutarOpcion = true;
      invernadero.showDisplay("Ventilacion manual");
      break;
    case 5: // Riego manual
      ejecutarOpcion = true;
      invernadero.showDisplay("Riego manual");
      break;
  }
}

// Cambiar umbral de humedad desde Serial
void cambiarUmbralHumedad() {
  if (Serial.available()) {
    String aux = "";
    while (Serial.available()) {
      char c = (char)Serial.read();
      if (c >= '0' && c <= '9') {
        aux += c;
      } else if (c == '\n') break;
    }
    aux.trim();

    int valor = aux.toInt();
    if (valor >= 40 && valor <= 60) {
      umbralHumedadMinima = valor;
      Serial.print("Nuevo umbral de humedad: ");
      Serial.println(umbralHumedadMinima);
    } else {
      Serial.println("Error: valor fuera de rango (40-60).");
    }
  }

  mostrarMenu = true;
  mostrarMenuOpciones();
  ejecutarOpcion = false;
}

// Cambiar temperatura de referencia desde Serial
void cambiarTempManual() {
  potenciometroActivo = false;
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    int nuevaRef = input.toInt();

    if (nuevaRef != 0 || input == "0") {
      temperaturaReferencia = nuevaRef;
      Serial.println("Nueva temperatura de referencia: " + String(temperaturaReferencia) + "°C");
      Serial.println("Potenciometro desactivado. Para volver a activarlo vaya al menu de opciones.");
    } else {
      Serial.println("Entrada invalida. Ingrese un número entero.");
    }
  }

  mostrarMenu = true;
  mostrarMenuOpciones();
  ejecutarOpcion = false;
}

// Activar lectura por potenciómetro
void activarPotenciometro(){
  potenciometroActivo = true;
  Serial.println("Potenciometro activado");

  mostrarMenu = true;
  mostrarMenuOpciones();
  ejecutarOpcion = false;
}

// Control manual del ventilador
void forzarVentilacion(){
  int estadoVentilador = digitalRead(PIN_LED_VENTILADOR); 
  if (estadoVentilador == HIGH) {
    digitalWrite(PIN_LED_VENTILADOR, LOW);            
    Serial.println("Ventilador apagado");
  } else {
    digitalWrite(PIN_LED_VENTILADOR, HIGH);           
    Serial.println("Ventilador encendido");
  }
  mostrarMenu = true;
  mostrarMenuOpciones();
  ejecutarOpcion = false;
}

// Control manual del riego
void forzarRiego(){
  if (riegoActivo) {
    digitalWrite(PIN_LED_RIEGO, LOW);   
    riegoActivo = false;     
    Serial.println("Riego apagado");
  } else {
    digitalWrite(PIN_LED_RIEGO, HIGH);  
    riegoActivo = true;     
    Serial.println("Riego encendido");
  }
   
  mostrarMenu = true;
  mostrarMenuOpciones();
  ejecutarOpcion = false;
}