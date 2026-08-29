#ifndef NEW_SOURCE_MENU_H
#define NEW_SOURCE_MENU_H

// Один кадр UI: ImGui-меню + ESP-оверлей. Вызывается каждый кадр.
void menu_render();

// Глобальные настройки (объявлены в game.cpp как g_cheat; здесь — ESP-настройки).
struct EspSettings {
    bool  enabled = true;
    bool  box = true;
    bool  name = true;
    bool  health = true;
    bool  distance = true;
    bool  weapon = false;
    bool  skeleton = false;
    bool  teamCheck = true;     // не рисовать сокомандников как врагов
    float maxDistance = 200.f;
};

extern EspSettings g_esp;

#endif // NEW_SOURCE_MENU_H
