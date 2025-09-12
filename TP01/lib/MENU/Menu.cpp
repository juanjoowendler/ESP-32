#include "Menu.h"


Menu::Menu(String opcs[], size_t n, bool enMenu) {
    numOpciones = n;
    opciones = new String[numOpciones];
    std::copy(opcs, opcs + numOpciones, opciones);
    opc = 0;
    this->enMenu = enMenu;
}

Menu::~Menu() {
    delete[] opciones;
}

/***
 * Devuelve un String con el menu completo, destacando la opcion seleccionada
 */
String Menu::mostrarMenu() {
    Serial.println(3);
    String menu = "";
    for (size_t i = 0; i < numOpciones; i++) {
        if (i == opc) {
            menu += "-> " + opciones[i] + "\n";
        } else {
            menu += "- " + opciones[i] + "\n";
        }
    }
    return menu;
}

/***
 * Cambia la opcion dependiendo del encoder y si el menu está en pantalla
 */
void Menu::changeOpcion(byte DT) {
    if (enMenu && numOpciones > 0) {
        int dtValue = digitalRead(DT);
        if (dtValue == HIGH) {
            opc++;
            opc %= numOpciones;
        }
        if (dtValue == LOW) {
            opc += numOpciones - 1;
            opc %= numOpciones;
        }
    }
}

/***
 * Devuelve la opcion actual [0:length]
 */
byte Menu ::getOpcion(){
    return opc;
}

/***
 * Alterna la visualizacion del menu, y devuelve su nuevo estado
 */
bool Menu ::changeEnMenu(){
    Serial.println(2);
    enMenu = !enMenu;
    return enMenu;
}

/***
 * Devuelve el estado del menu
 */
bool Menu ::getEnMenu(){
    return enMenu;
}