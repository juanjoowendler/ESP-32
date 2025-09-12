#ifndef _MENU
#define _MENU

#include <Arduino.h>


class Menu{
  public:
    Menu(String opcs[], size_t n, bool enMenu);
    ~Menu();
    String mostrarMenu();
    void changeOpcion(byte DT);
    byte getOpcion();
    bool changeEnMenu();
    bool getEnMenu();

  private:
    byte opc;
    bool enMenu;
    String* opciones;
    size_t numOpciones;
};

#endif