// IMPORTACIONES
#include <Arduino.h>
#include "Device.h"

// DEFINICION DE PINES
#define LED_VENT 5
#define LED_RIEGO 2
#define POTENC 34
#define SENSOR 14

// ENCODER
#define CLK 32
#define DT 33
#define BOTON 12

Device _device(128, 64, -1, SENSOR, DHT22);
int umbral_minimo;
int opc = 0;
int ant_opc;
bool riego_encendido = false;
bool ant_temp_est, ant_hum_est;
bool mostrarMenu = true;

// put function declarations here:
void esp32_io_setup(void);
void mostrar_menu(void);
void readEncoder();
void hacerOpcion(float temp, int temp_referencia, float hum);

// CONFIGURACION INICIAL
void setup() {
  Serial.begin(9600);
  esp32_io_setup();
  // Si gira el encoder, interrumpir y llamar a la funcion readEncoder
  attachInterrupt(digitalPinToInterrupt(CLK), readEncoder, FALLING);
  _device.begin();

  umbral_minimo = (int) random(40, 61); // valor aleatorio entre 4 y 60 inclusive

  _device.showDisplay(String(umbral_minimo) + "%");
  Serial.println("INICIO SISTEMA\nUmbral aleatorio: " + String(umbral_minimo) + "%");

  delay(500);

  mostrar_menu();
}

// BUCLE PRINCIPAL
void loop() {
  // Recibimos el potenciometro y lo mapeamos entre 15 y 40 °C
  int temp_referencia = map(analogRead(POTENC), 0, 4095, -40, 80);

  // Temperatura actual
  float temp = _device.readTemp();

  // Verificar si cambio el estado
  if ((temp > temp_referencia) != ant_temp_est){
    // Si la temperatura es menor a la de referencia encendemos el LED, sino lo apagamos
    if (temp > temp_referencia){
      digitalWrite(LED_VENT, HIGH);
      Serial.println("Ventilador prendido");
    }else{
      digitalWrite(LED_VENT, LOW);
      Serial.println("Ventilador apagado");
    }
  }

  // Humedad actual
  float hum = _device.readHum();

  // Verificar si cambio el estado
  if ((hum < umbral_minimo) != ant_hum_est){
    // Si la humedad es menor al umbral minimo, el LED parpadea, sino se apaga
    if (hum < umbral_minimo){
      Serial.println("Riego activado");
    }else{
      // Apagar LED
      digitalWrite(LED_RIEGO, LOW);
      riego_encendido = false;
      Serial.println("Riego desactivado");
    }
  }

  if (hum < umbral_minimo){
    // Utilizar un bool para generar parpadeo
    if (riego_encendido) {
      digitalWrite(LED_RIEGO, HIGH);
    } else {
      digitalWrite(LED_RIEGO, LOW);
    }
    riego_encendido = !riego_encendido;
  }

  // Si se apreta el boton, cambiar entre menu y opcion
  if (digitalRead(BOTON) == LOW){
    mostrarMenu = !mostrarMenu;
    // Verificar si mostrar menu o hacer una opcion
    if (mostrarMenu) mostrar_menu(); //Mostrar menu
  }

  if (ant_opc != opc) mostrar_menu(); // Si cambia la opcion, actualizar menu
  if (!mostrarMenu) hacerOpcion(temp, temp_referencia, hum); // Hacer opcion si no se muestra el menu

  // Guardar estado de temperatura y humedad anterior
  ant_temp_est = temp > temp_referencia;
  ant_hum_est = hum < umbral_minimo;
  // Guardar anterior opcion
  ant_opc = opc;
  
  delay(100);
}

// put function definitions here:
void esp32_io_setup(void) {
  pinMode(LED_VENT, OUTPUT);
  pinMode(LED_RIEGO, OUTPUT);
  pinMode(POTENC, INPUT);
  // ENCODER
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(BOTON, INPUT_PULLUP);
}

// Actualizar menu con nueva opcion
void mostrar_menu(void){
  const String opciones[] = {
    "Mostrar estado invernadero",
    "Modificar valores referencia",
    "Forzar activacion/detencion ventilacion",
    "Forzar activacion/detencion riego"
  };

  String menu = "";
  for(int i = 0; i < 4; i++){
    if (i == opc){
      menu += "-> " + opciones[i] + "\n";
    }else{
      menu += "- " + opciones[i] + "\n";
    }
  }

  _device.showDisplay(menu);
}

// Cambiar opcion
void readEncoder() {
  if (mostrarMenu){
    int dtValue = digitalRead(DT);
    if (dtValue == HIGH) {
      opc++;
      opc %= 4; 
    }
    if (dtValue == LOW) {
      opc += 3;
      opc %= 4;
    }
  }
}

// Seleccionar Opcion
void hacerOpcion(float temp, int temp_referencia, float hum){
  
  switch (opc) {
  case 0:
   _device.showDisplay("Temperatura: " + String(temp) + "'C \nReferencia: " + String(temp_referencia) +
                "'C \n\nHumedad: " + String(hum) + "% \nUmbral: " + String(umbral_minimo));
    break;
  
  case 1:
    break;
  }
}