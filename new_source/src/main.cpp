// ============================================================================
//  Oxide Survival Island — простой внешний чит (root executable).
//  ImGui меню + функции. БЕЗ СКАНА ПАМЯТИ:
//    классы резолвятся по статическим RVA (script.json TypeInfoPointers):
//    il2cpp_base + TYPEINFO_RVA -> Il2CppClass* -> static_fields @ 0xB8.
//  Оффсеты обновлены под Dump14.
// ============================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <thread>
#include <atomic>

#include "Android_draw/draw.h"
#include "proc.h"
#include "offsets.h"
#include "game.h"
#include "menu.h"

static std::atomic<bool> g_run{true};

// Размеры экрана (из ANativeWindowCreator display info).
extern android::ANativeWindowCreator::DisplayInfo displayInfo;

static void on_signal(int) { g_run = false; }

int main(int argc, char *argv[]) {
    signal(SIGINT,  on_signal);
    signal(SIGTERM, on_signal);
    signal(SIGSEGV, on_signal);
    signal(SIGPIPE, SIG_IGN);

    printf("== Oxide Simple Menu ==\n");
    printf("waiting for game %s ...\n", off::PACKAGE);

    // 1) найти процесс игры (ждать пока запустится).
    while (g_run) {
        g_pid = proc_find_pid(off::PACKAGE);
        if (g_pid > 0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    if (!g_run) return 0;
    printf("game pid = %d\n", (int)g_pid);

    // 2) поднять overlay (ANativeWindow + EGL + ImGui).
    screen_config();
    int sx = displayInfo.height > displayInfo.width ? displayInfo.height : displayInfo.width;
    int sy = displayInfo.height < displayInfo.width ? displayInfo.height : displayInfo.width;
    if (sx <= 0) sx = 2400;
    if (sy <= 0) sy = 1080;
    native_window_screen_x = sx;
    native_window_screen_y = sy;

    if (!initGUI_draw((uint32_t)sx, (uint32_t)sy, true)) {
        fprintf(stderr, "overlay init failed\n");
        return 1;
    }
    Touch_Init(displayInfo.width, displayInfo.height, displayInfo.orientation, true);

    // 3) фоновый поток: резолв классов + обновление игрового кэша.
    std::thread worker([]() {
        while (g_run) {
            if (g_pid <= 0) {
                g_pid = proc_find_pid(off::PACKAGE);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
            if (!g_game.attached) {
                g_il2cppBase = proc_module_base(off::MODULE);
            }
            game_resolve();
            if (g_game.ready) {
                g_game.screenW = (float)native_window_screen_x;
                g_game.screenH = (float)native_window_screen_y;
                game_update();
            }
            // ~60 Гц для данных; запись аима тоже отсюда.
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    });

    // 4) главный цикл рендера.
    while (g_run) {
        drawBegin();
        g_game.screenW = (float)native_window_screen_x;
        g_game.screenH = (float)native_window_screen_y;
        menu_render();
        drawEnd();
    }

    shutdown();
    worker.join();
    printf("exit\n");
    return 0;
}
