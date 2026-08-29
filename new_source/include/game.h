#ifndef NEW_SOURCE_GAME_H
#define NEW_SOURCE_GAME_H

#include <cstdint>
#include <string>
#include <vector>
#include "math.h"

// Структура одного игрока (то, что нужно меню/фичам).
struct Player {
    uint64_t pm = 0;          // адрес PlayerManager
    Vec3     pos;             // мировая позиция (lastTickPosition)
    Vec3     headPos;         // позиция головы (хитбокс)
    Vec3     chestPos;
    float    health = 0.f;    // m_MaxHealth (отдельного current нет)
    bool     alive = false;
    bool     local = false;
    bool     teammate = false;
    bool     friend_ = false;
    bool     inAir = false;
    bool     crouching = false;
    float    distance = 0.f;
    std::string name;
    std::string userId;
    std::string weapon;

    // экранные координаты бокса (заполняются при отрисовке)
    bool  onScreen = false;
    Vec2  screenHead, screenFoot;
    Vec2  screenName;
};

// Состояние игры/чита.
struct GameState {
    bool      attached = false;   // игра найдена, база есть
    bool      ready = false;      // классы зарезолвились (static_fields)
    uint64_t  klassPlayerManager = 0;
    uint64_t  pmStaticFields = 0;
    uint64_t  localPlayer = 0;
    ViewBasis camera;
    int       playerCount = 0;
    float     screenW = 0, screenH = 0;
};

extern GameState g_game;

// Резолв классов по СТАТИЧЕСКИМ RVA (script.json TypeInfoPointers).
// Никакого скана памяти: base + RVA -> Il2CppClass* -> static_fields@0xB8.
// Возвращает true когда PlayerManager-класс готов.
bool game_resolve();

// Обновить кэш игроков + камеру. Вызывается каждый кадр из render-потока.
void game_update();

// Список игроков (заполняется game_update).
const std::vector<Player> &game_players();

// --- Применение чит-функций (запись в память игры) ---
struct CheatSettings {
    // aimbot
    bool  aimbot = false;
    float aimFov = 5.0f;        // градусы
    float aimSmooth = 3.0f;     // 1..10
    bool  aimVisibleOnly = false;
    int   aimBone = 0;          // 0 head, 1 chest
    bool  aimTeammates = false;
    // camera
    bool  fovChanger = false;
    float fovValue = 90.f;
    // player
    bool  speedHack = false;
    float speedValue = 2.0f;
    bool  flyHack = false;
    bool  noRecoil = false;
};

extern CheatSettings g_cheat;

// Применить записи (aimbot/fov/speed) — вызывается из game_update.
void game_apply();

#endif // NEW_SOURCE_GAME_H
