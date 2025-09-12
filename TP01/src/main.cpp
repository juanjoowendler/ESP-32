#include <Arduino.h>
#include "Device.h"
#include "Menu.h"

#define LED_VENT 5
#define LED_RIEGO 2
#define POTENC 34
#define SENSOR 14

// ENCODER
#define CLK 32
#define DT 33
#define BOTON 12

Device _device(128, 64, -1, SENSOR, DHT22);
String opcionesMenu[] = {
    "Mostrar estado invernadero",
    "Modificar valores referencia",
    "Forzar activacion/detencion ventilacion",
    "Forzar activacion/detencion riego"
};
Menu menuPrincipal(opcionesMenu, 4, true);
int umbral_minimo;
byte ant_opc;
bool riego_encendido = false;
bool ant_temp_est, ant_hum_est;
volatile bool encoderMoved = false;

// put function declarations here:
void esp32_io_setup(void);
void hacerOpcion(float temp, int temp_referencia, float hum);
void readEncoder();
void mostrar_menu();
void mostrar_menu(Menu menu);

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

void IRAM_ATTR readEncoder() {
  encoderMoved = true;
}


extern "C" {
#include "esp_heap_caps.h"
}

void loop() {
  // Comprobación de integridad de heap
  if (!heap_caps_check_integrity_all(true)) {
    Serial.println("[ERROR] Corrupción de heap detectada");
    delay(1000);
  }
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
    Serial.println(1);
    // Verificar si mostrar menu o hacer una opcion
    if (menuPrincipal.changeEnMenu()) mostrar_menu(); //Mostrar menu
  }

  if (ant_opc != menuPrincipal.getOpcion()) mostrar_menu(); // Si cambia la opcion, actualizar menu
  if (!menuPrincipal.getEnMenu()) hacerOpcion(temp, temp_referencia, hum); // Hacer opcion si no se muestra el menu

  // Guardar estado de temperatura y humedad anterior
  ant_temp_est = temp > temp_referencia;
  ant_hum_est = hum < umbral_minimo;
  // Guardar anterior opcion
  ant_opc = menuPrincipal.getOpcion();
  
  delay(100);
    if (encoderMoved) {
      menuPrincipal.changeOpcion(DT);
      encoderMoved = false;
    }
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

// Seleccionar Opcion
void hacerOpcion(float temp, int temp_referencia, float hum){
  
  switch (menuPrincipal.getOpcion()) {
  case 0:
   _device.showDisplay("Temperatura: " + String(temp) + "'C \nReferencia: " + String(temp_referencia) +
                "'C \n\nHumedad: " + String(hum) + "% \nUmbral: " + String(umbral_minimo));
    break;
  
  case 1:
    
    break;
  }
}
/*
void readEncoder(){
  menuPrincipal.changeOpcion(DT);
}*/

void mostrar_menu(){
  mostrar_menu(menuPrincipal);
}

void mostrar_menu(Menu menu){
  _device.showDisplay("Menu");
}