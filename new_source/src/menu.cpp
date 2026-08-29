#include "menu.h"
#include "game.h"
#include "proc.h"
#include "offsets.h"
#include "math.h"

#include "imgui.h"
#include <cstdio>
#include <string>
#include <vector>

EspSettings g_esp;

// Цвета
static const ImU32 COL_ENEMY   = IM_COL32(255, 60, 60, 255);
static const ImU32 COL_TEAM    = IM_COL32(60, 200, 255, 255);
static const ImU32 COL_BG      = IM_COL32(20, 22, 30, 230);
static const ImU32 COL_ACCENT  = IM_COL32(120, 170, 255, 255);

static void draw_esp(ImDrawList *dl) {
    if (!g_esp.enabled || !g_game.ready) return;

    float w = (float)g_game.screenW;
    float h = (float)g_game.screenH;
    if (w <= 0 || h <= 0) return;

    const ViewBasis &cam = g_game.camera;
    const auto &players = game_players();

    char buf[160];

    for (const Player &p : players) {
        if (p.local || !p.alive) continue;
        if (p.teammate && g_esp.teamCheck) {
            // сокомандников рисуем синим, но компактно (только имя)
            if (!g_esp.name) continue;
        }
        if (p.distance > g_esp.maxDistance) continue;

        Vec2 sHead, sFoot;
        bool okHead = world_to_screen(cam, p.headPos, w, h, sHead);
        Vec3 foot(p.pos.x, p.pos.y, p.pos.z);
        bool okFoot = world_to_screen(cam, foot, w, h, sFoot);
        if (!okHead || !okFoot) continue;

        float boxH = sFoot.y - sHead.y;
        if (boxH < 6.f) continue;
        float boxW = boxH * 0.55f;
        float cx = (sHead.x + sFoot.x) * 0.5f;
        float x1 = cx - boxW * 0.5f;
        float x2 = cx + boxW * 0.5f;
        float y1 = sHead.y;
        float y2 = sFoot.y;

        ImU32 col = p.teammate ? COL_TEAM : COL_ENEMY;

        if (g_esp.box) {
            dl->AddRect(ImVec2(x1, y1), ImVec2(x2, y2), col, 0.f, 0, 1.8f);
        }

        if (g_esp.health) {
            float frac = p.health / 100.f;
            if (frac < 0.f) frac = 0.f;
            if (frac > 1.f) frac = 1.f;
            float hx = x1 - 6.f;
            dl->AddRectFilled(ImVec2(hx - 2.f, y1), ImVec2(hx + 1.f, y2),
                              IM_COL32(0, 0, 0, 180));
            ImU32 hpcol = IM_COL32((int)(255 * (1.f - frac)),
                                   (int)(255 * frac), 60, 255);
            dl->AddRectFilled(ImVec2(hx - 1.f, y2 - (y2 - y1) * frac),
                              ImVec2(hx, y2), hpcol);
        }

        float textY = y1 - 16.f;
        if (g_esp.name && !p.name.empty()) {
            snprintf(buf, sizeof(buf), "%s", p.name.c_str());
            ImVec2 ts = ImGui::CalcTextSize(buf);
            float tx = cx - ts.x * 0.5f;
            dl->AddText(ImVec2(tx + 1, textY + 1), IM_COL32(0, 0, 0, 200), buf);
            dl->AddText(ImVec2(tx, textY), col, buf);
            textY -= 14.f;
        }

        if (g_esp.distance) {
            snprintf(buf, sizeof(buf), "%.0fm", p.distance);
            ImVec2 ts = ImGui::CalcTextSize(buf);
            float tx = cx - ts.x * 0.5f;
            dl->AddText(ImVec2(tx + 1, textY + 1), IM_COL32(0, 0, 0, 200), buf);
            dl->AddText(ImVec2(tx, textY), IM_COL32(220, 220, 220, 255), buf);
        }

        if (g_esp.weapon && !p.weapon.empty()) {
            snprintf(buf, sizeof(buf), "%s", p.weapon.c_str());
            ImVec2 ts = ImGui::CalcTextSize(buf);
            float tx = cx - ts.x * 0.5f;
            dl->AddText(ImVec2(tx + 1, y2 + 2.f), IM_COL32(0, 0, 0, 200), buf);
            dl->AddText(ImVec2(tx, y2 + 1.f), IM_COL32(230, 230, 160, 255), buf);
        }
    }
}

void menu_render() {
    ImGuiIO &io = ImGui::GetIO();
    ImDrawList *bg = ImGui::GetBackgroundDrawList();
    draw_esp(bg);

    // Простое меню
    ImGui::SetNextWindowSize(ImVec2(360, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(80, 140), ImGuiCond_FirstUseEver);
    ImGui::Begin("Oxide Simple Menu", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

    // статус
    ImVec4 statusCol = g_game.ready ? ImVec4(0.3f, 1.f, 0.4f, 1.f)
                                    : ImVec4(1.f, 0.6f, 0.2f, 1.f);
    ImGui::TextColored(statusCol, g_game.ready ? "Attached" : "Waiting for game...");
    ImGui::SameLine();
    ImGui::Text("| players: %d", g_game.playerCount);
    ImGui::Separator();

    if (ImGui::BeginTabBar("tabs")) {
        if (ImGui::BeginTabItem("ESP")) {
            ImGui::Checkbox("Enable ESP", &g_esp.enabled);
            ImGui::Checkbox("Boxes", &g_esp.box);
            ImGui::Checkbox("Names", &g_esp.name);
            ImGui::Checkbox("Health bar", &g_esp.health);
            ImGui::Checkbox("Distance", &g_esp.distance);
            ImGui::Checkbox("Weapon", &g_esp.weapon);
            ImGui::Checkbox("Team check", &g_esp.teamCheck);
            ImGui::SliderFloat("Max distance", &g_esp.maxDistance, 10.f, 500.f, "%.0fm");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Aimbot")) {
            ImGui::Checkbox("Enable aimbot", &g_cheat.aimbot);
            ImGui::SliderFloat("FOV", &g_cheat.aimFov, 1.f, 30.f, "%.1f deg");
            ImGui::SliderFloat("Smoothness", &g_cheat.aimSmooth, 1.f, 15.f, "%.1f");
            ImGui::RadioButton("Head", &g_cheat.aimBone, 0); ImGui::SameLine();
            ImGui::RadioButton("Chest", &g_cheat.aimBone, 1);
            ImGui::Checkbox("Visible only", &g_cheat.aimVisibleOnly);
            ImGui::Checkbox("Target teammates", &g_cheat.aimTeammates);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Player")) {
            ImGui::Checkbox("FOV changer", &g_cheat.fovChanger);
            ImGui::SliderFloat("FOV value", &g_cheat.fovValue, 50.f, 150.f, "%.0f");
            ImGui::TextDisabled("(camera FOV from game: %.1f)", g_game.camera.vfov);
            ImGui::Separator();
            ImGui::TextDisabled("Speed/fly disabled by default (anti-cheat fields");
            ImGui::TextDisabled(" lastTickPosition are server-checked).");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Info")) {
            ImGui::Text("Oxide Survival Island");
            ImGui::Text("Offsets: Dump14 (updated)");
            ImGui::Text("Mode: external (root), no memory scan");
            ImGui::Separator();
            ImGui::Text("libil2cpp base: 0x%llx", (unsigned long long)g_il2cppBase);
            ImGui::Text("localPlayer:    0x%llx", (unsigned long long)g_game.localPlayer);
            ImGui::Text("cam yaw/pitch:  %.1f / %.1f", g_game.camera.yaw, g_game.camera.pitch);
            ImGui::Text("cam pos: %.1f %.1f %.1f",
                        g_game.camera.pos.x, g_game.camera.pos.y, g_game.camera.pos.z);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}
