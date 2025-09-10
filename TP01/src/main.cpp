#include <Arduino.h>
#include "Device.h"

#define LED_VENT 5
#define LED_RIEGO 2
#define POTENC 34
#define SENSOR 14
#define BOTON 18 

Device _device(128, 64, -1, SENSOR, DHT22);
int umbral_minimo;
bool riego_encendido = false;
bool pantalla = false;
bool ant_temp_est, ant_hum_est;

// put function declarations here:
void esp32_io_setup(void);
void pantalla1(float temp, int ref, bool estado);
void pantalla2(float hum, bool estado);

void setup() {
  Serial.begin(9600);
  esp32_io_setup();
  _device.begin();

  umbral_minimo = (int) random(40, 61); // valor aleatorio entre 4 y 60 inclusive

  _device.showDisplay(String(umbral_minimo) + "%");
  Serial.println("INICIO SISTEMA\nUmbral aleatorio: " + String(umbral_minimo) + "%");

  delay(500);
}

void loop() {
  // Recibimos el potenciometro y lo mapeamos entre 15 y 40 °C
  int temp_referencia = map(analogRead(POTENC), 0, 4095, -40, 80);

  // Leer temperatura actual
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

  // Leer humedad actual
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

  // Mostrar y actualizar pantalla
  if (pantalla){
    pantalla1(temp, temp_referencia, temp > temp_referencia);
  }else{
    pantalla2(hum, hum < umbral_minimo);
  }

  // Si se apreta el boton, cambiar pantalla
  if (digitalRead(BOTON) == LOW){
    pantalla = !pantalla;
  }

  // Guardar estado de temperatura y humedad anterior
  ant_temp_est = temp > temp_referencia;
  ant_hum_est = hum < umbral_minimo;
  
  delay(500);
}

// put function definitions here:
void esp32_io_setup(void) {
  pinMode(LED_VENT, OUTPUT);
  pinMode(LED_RIEGO, OUTPUT);
  pinMode(POTENC, INPUT);
  pinMode(BOTON, INPUT_PULLUP);
}

// Mostrar pantalla 1 (Temperatura)
void pantalla1 (float temp, int ref, bool estado){
  String est;
  if (estado){
    est = "Encendido";
  }else{
    est = "Apagado";
  }
  _device.showDisplay("Temperatura: " + String(temp) + "'C\nReferencia: " + String(ref) + "'C\nEstado: " + est);
}

// Mostrar pantalla 2 (Humedad)
void pantalla2 (float hum, bool estado){
  String est;
  if (estado){
    est = "Encendido";
  }else{
    est = "Apagado";
  }
  _device.showDisplay("Humedad: " + String(hum) + "%\nUmbral: " + String(umbral_minimo) + "%\nEstado: " + est);
}