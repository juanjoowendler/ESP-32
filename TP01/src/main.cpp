#include <Arduino.h>
#include "Device.h"

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
bool ejecutarOpcion = false;
bool potencActivo = true;
int temp_referencia;
int estadoVent;
bool riegoPrendido = false;
String serialLine;

// put function declarations here:
void esp32_io_setup(void);
void mostrar_menu(void);
void readEncoder();
void hacerOpcion(float temp, int temp_referencia, float hum);
void modificarUmbralHumedad();
void modificarTempManual();
void activarPotenc();
void manualVentilacion();
void manualRiego();


void setup() {
  Serial.begin(9600);
  esp32_io_setup();
  // Si gira el encoder, interrumpir y llamar a la funcion readEncoder
  attachInterrupt(digitalPinToInterrupt(CLK), readEncoder, FALLING);
  _device.begin();

  umbral_minimo = (int)random(40, 61); // valor aleatorio entre 4 y 60 inclusive

  _device.showDisplay(String(umbral_minimo) + "%");
  Serial.println("INICIO SISTEMA\nUmbral aleatorio: " + String(umbral_minimo) + "%");

  delay(500);

  mostrar_menu();
}

void loop() {
  // Recibimos el potenciometro y lo mapeamos entre -5 y 60 °C
  if(potencActivo){
    temp_referencia = map(analogRead(POTENC), 0, 4095, -5, 60);
  }

  // Temperatura actual
  float temp = _device.readTemp();

  // Verificar si cambio el estado
  if ((temp > temp_referencia) != ant_temp_est) {
    // Si la temperatura es menor a la de referencia encendemos el LED, sino lo apagamos
    if (temp > temp_referencia){
      digitalWrite(LED_VENT, HIGH);
      Serial.println("Ventilador prendido");
    } else {
      digitalWrite(LED_VENT, LOW);
      Serial.println("Ventilador apagado");
    }
  }

  // Humedad actual
  float hum = _device.readHum();

  // Verificar si cambio el estado
  if ((hum < umbral_minimo) != ant_hum_est) {
    // Si la humedad es menor al umbral minimo, el LED parpadea, sino se apaga
    if (hum < umbral_minimo) {
      Serial.println("Riego activado");
      riegoPrendido = true;
    } else {
      // Apagar LED
      digitalWrite(LED_RIEGO, LOW);
      riegoPrendido = false;
      riego_encendido = false;
      Serial.println("Riego desactivado");
    }
  }

  if (riegoPrendido) {
    // Utilizar un bool para generar parpadeo
    if (riego_encendido) {
      digitalWrite(LED_RIEGO, HIGH);
    } else {
      digitalWrite(LED_RIEGO, LOW);
    }
    riego_encendido = !riego_encendido;
  }

  // Si se apreta el boton, cambiar entre menu y opcion
  if (digitalRead(BOTON) == LOW) {
    if (ejecutarOpcion){
      switch (opc) {
      case 1:
        modificarTempManual();
        break;
      case 2:
        modificarUmbralHumedad();
        break;
      case 3:
        activarPotenc();
        break;
      case 4:
        manualVentilacion();
        break;
      case 5:
        manualRiego();
        break;
      }
    }else {
      mostrarMenu = !mostrarMenu;
      // Verificar si mostrar menu o hacer una opcion
      if (mostrarMenu)
        mostrar_menu(); // Mostrar menu
    }
  }

  if (ant_opc != opc)
    mostrar_menu(); // Si cambia la opcion, actualizar menu
  if (!mostrarMenu)
    hacerOpcion(temp, temp_referencia, hum); // Hacer opcion si no se muestra el menu

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
void mostrar_menu(void) {
  const String opciones[] = {
      "Mostrar estado invernadero",
      "Modificar temp. referencia",
      "Modificar umbral de humedad",
      "Activar potenciometro",
      "Ventilacion manual",
      "Riego manual"};

  String menu = "";
  for (int i = 0; i < 6; i++) {
    if ((opc == 4 || opc == 5)&& i == 0) continue; 
    if (i == opc) {
      menu += "-> " + opciones[i] + "\n";
    } else {
      menu += "- " + opciones[i] + "\n";
    }
  }
  _device.showDisplay(menu);
}

// Cambiar opcion
void readEncoder() {
  if (mostrarMenu) {
    int dtValue = digitalRead(DT);
    if (dtValue == HIGH) {
      opc += 5;
      opc %= 6;
    }
    if (dtValue == LOW) {
      opc++;
      opc %= 6;
    }
  }
}

// Seleccionar Opcion
void hacerOpcion(float temp, int temp_referencia, float hum) {

  switch (opc) {
  case 0:
    _device.showDisplay("Temperatura: " + String(temp) + "'C \nReferencia: " + String(temp_referencia) +
                        "'C \n\nHumedad: " + String(hum) + "% \nUmbral: " + String(umbral_minimo));
    break;
  case 1:
    ejecutarOpcion = true;
    _device.showDisplay("Inserte un nuevo valor de referencia para la temperatura (-5 a 60): \n\n Presione el boton al ingresar el valor.");
    break;
  case 2:
    ejecutarOpcion = true;
    _device.showDisplay("Inserte nuevo valor de humedad (40-60): \n\n Presione el boton al ingresar el valor.");
    break;
  case 3:
    ejecutarOpcion = true;
    _device.showDisplay("Potenciometro activado.");
    break;
  case 4:
    ejecutarOpcion = true;
     _device.showDisplay("Ventilacion forzada");
    break;
  case 5:
    ejecutarOpcion = true;
     _device.showDisplay("Riego forzada");
    break;
  }

}

void modificarUmbralHumedad() {
  if (Serial.available()) {
    String aux = "";

    while (Serial.available()) {
      char c = (char)Serial.read();
      // aceptar dígitos del 0 al 9, descartar cualquier otra cosa
      if (c >= '0' && c <= '9') {
        aux += c;
      }else if (c == '\n') break;
    }

    aux.trim(); // quitar espacios

    int valor = aux.toInt();
    if (valor >= 40 && valor <= 60) {
      umbral_minimo = valor;
      Serial.print("Nuevo umbral_humedad = ");
      Serial.println(umbral_minimo);
    } else {
      Serial.println("Error: valor fuera de rango (40-60).");
    }
  }

  mostrarMenu = true;
  mostrar_menu();
  ejecutarOpcion = false;
}

void modificarTempManual() {

  potencActivo = false;
  if (Serial.available() > 0) {         // ¿hay datos en el buffer?

    String input = Serial.readStringUntil('\n');  // leo hasta Enter
    int nuevaRef = input.toInt();       // convierto a entero

    if (nuevaRef != 0 || input == "0") {  // validación básica
      temp_referencia = nuevaRef;
      Serial.println("Nueva temperatura de referencia: " + String(temp_referencia) + "°C");
    } else {
      Serial.println("Entrada invalida. Ingrese un número entero.");
    }
  }

  mostrarMenu = true;
  mostrar_menu();
  ejecutarOpcion = false;
}

void activarPotenc(){
  potencActivo = true;
  Serial.println("Potenciometro activado");

  mostrarMenu = true;
  mostrar_menu();
  ejecutarOpcion = false;
}


void manualVentilacion(){
  int estadoVent = digitalRead(LED_VENT); 

  if (estadoVent == HIGH) {
    digitalWrite(LED_VENT, LOW);            
    Serial.println("Ventilador apagado");
  } else {
    digitalWrite(LED_VENT, HIGH);           
    Serial.println("Ventilador prendido");
  }
  mostrarMenu = true;
  mostrar_menu();
  ejecutarOpcion = false;
}

void manualRiego(){
  if (riegoPrendido) {
    digitalWrite(LED_RIEGO, LOW);   
    riegoPrendido = false;     
    Serial.println("Riego apagado");
    Serial.println(riegoPrendido);

  } else {
    digitalWrite(LED_RIEGO, HIGH);  
    riegoPrendido = true;     
    Serial.println("Riego prendido");
    Serial.println(riegoPrendido);
  }
   
  mostrarMenu = true;
  mostrar_menu();
  ejecutarOpcion = false;
}