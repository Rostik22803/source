#include "game.h"
#include "proc.h"
#include "offsets.h"

#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>

GameState      g_game;
CheatSettings  g_cheat;

static std::vector<Player> s_players;

// ---------------------------------------------------------------------------
//  Резолв классов — ТОЛЬКО по статическим RVA из script.json.
//  il2cpp_base + TYPEINFO_RVA -> Il2CppClass* ; klass + 0xB8 -> static_fields.
//  НИКАКОГО СКАНА ПАМЯТИ.
// ---------------------------------------------------------------------------
bool game_resolve() {
    if (!g_il2cppBase) {
        g_il2cppBase = proc_module_base(off::MODULE);
        if (!g_il2cppBase) { g_game.attached = false; g_game.ready = false; return false; }
        g_game.attached = true;
    }

    uint64_t klass = rd<uint64_t>(g_il2cppBase + off::PLAYERMANAGER_TYPEINFO_RVA);
    if (!valid_ptr(klass)) { g_game.ready = false; return false; }
    g_game.klassPlayerManager = klass;

    uint64_t statics = rd<uint64_t>(klass + off::CLASS_STATIC_FIELDS);
    if (!valid_ptr(statics)) { g_game.ready = false; return false; }
    g_game.pmStaticFields = statics;

    g_game.ready = true;
    return true;
}

// Список игроков из static clientPlayerList (обычный List<PlayerManager>).
static uint64_t get_client_list() {
    if (!g_game.pmStaticFields) return 0;
    uint64_t list = rd<uint64_t>(g_game.pmStaticFields + off::PM_STAT_CLIENT_LIST);
    return valid_ptr(list) ? list : 0;
}

static bool is_local(uint64_t pm) {
    uint64_t nb = rd<uint64_t>(pm + off::NB_NET_IDENTITY);
    if (!valid_ptr(nb)) return false;
    return rd<uint8_t>(nb + off::NI_IS_LOCAL_PLAYER) != 0;
}

// Позиция игрока: приоритет — интерполированная позиция мотора (рисуемая
// каждый кадр), fallback — server lastTickPosition.
static Vec3 read_position(uint64_t pm, bool local, Vec3 *headOut, Vec3 *chestOut) {
    Vec3 pos = rd<Vec3>(pm + off::PM_LAST_TICK_POS);

    // KCC: у локального — KCC (0x68 motor), у remote — SingleKcc через
    // InterfaceReference<My> (kccReference @0xB0 -> объект внутри).
    uint64_t kcc = 0;
    uint64_t motor = 0;

    // kccReference имеет тип InterfaceReference<My> (struct): само поле PM+0xB0
    // это НЕ указатель на бокс, а начало структуры; реализация My лежит по
    // смещению +0x10 от начала структуры (проверенные смещения для этого билда).
    uint64_t impl = rd<uint64_t>(pm + off::PM_KCC_REFERENCE + 0x10);
    if (valid_ptr(impl)) {
        uint64_t m = rd<uint64_t>(impl + off::SKCC_MOTOR); // SingleKcc @0x70
        if (valid_ptr(m)) { kcc = impl; motor = m; }
        else {
            // на части клиентов это KCC (motor @0x68)
            m = rd<uint64_t>(impl + off::KCC_MOTOR);
            if (valid_ptr(m)) { kcc = impl; motor = m; }
        }
    }

    float capsuleH = off::BOX_HEIGHT;
    if (valid_ptr(motor)) {
        Vec3 interp = rd<Vec3>(motor + off::MOTOR_INTERP_POS);
        if (!interp.zero()) pos = interp;
        capsuleH = rd<float>(motor + off::MOTOR_CAPSULE_HEIGHT);
        if (capsuleH < 0.5f || capsuleH > 3.0f) capsuleH = off::BOX_HEIGHT;

        // голова: лаг-компенсированные хитбоксы из HitBoxRecorderRoot.
        uint64_t rec = valid_ptr(kcc) ? rd<uint64_t>(kcc + off::SKCC_RECORDER) : 0;
        if (valid_ptr(rec)) {
            uint64_t records = rd<uint64_t>(rec + off::HBR_RECORDS);
            if (valid_ptr(records)) {
                int cnt = rd<int32_t>(records + off::ARRAY_COUNT);
                if (cnt > 0 && cnt < 256) {
                    // records[0].. — Yg { HitBox* @0x10, worldPos @0x34 }.
                    // Ищем head (m_HitArea == 0) и chest (m_HitArea == 1).
                    for (int i = 0; i < cnt; i++) {
                        uint64_t slot = records + off::ARRAY_DATA + 0x18 * i;
                        uint64_t yg = rd<uint64_t>(slot);
                        if (!valid_ptr(yg)) continue;
                        uint64_t hb = rd<uint64_t>(yg + off::YG_HITBOX);
                        Vec3 wpos = rd<Vec3>(yg + off::YG_WORLDPOS);
                        if (!valid_ptr(hb)) continue;
                        int area = rd<int32_t>(hb + off::HB_HITAREA);
                        if (area == 0 && headOut)  { *headOut = wpos; }
                        if (area == 1 && chestOut) { *chestOut = wpos; }
                    }
                }
            }
        }
    }

    if (headOut && headOut->zero())  *headOut  = Vec3(pos.x, pos.y + capsuleH, pos.z);
    if (chestOut && chestOut->zero()) *chestOut = Vec3(pos.x, pos.y + capsuleH * 0.6f, pos.z);
    return pos;
}

static std::string read_weapon(uint64_t pm) {
    uint64_t fp = rd<uint64_t>(pm + off::PM_FPMANAGER);
    if (!valid_ptr(fp)) return "";
    uint64_t weapon = rd<uint64_t>(fp + off::FP_CURRENT_WEAPON);
    if (!valid_ptr(weapon)) return "";
    uint64_t item = rd<uint64_t>(weapon + off::FPOBJ_ITEM);
    if (!valid_ptr(item)) return "";
    uint64_t data = rd<uint64_t>(item + off::ITEM_DATA);
    if (!valid_ptr(data)) return "";
    uint64_t shortName = rd<uint64_t>(data + off::ITEMDATA_SHORTNAME);
    std::string s = rd_string(shortName, 48);
    return s;
}

void game_update() {
    s_players.clear();
    g_game.playerCount = 0;

    if (!g_game.ready) {
        if (!game_resolve()) return;
    }

    uint64_t list = get_client_list();
    if (!list) return;

    int size = rd<int32_t>(list + off::LIST_SIZE);
    if (size <= 0 || size > off::LIST_COUNT_MAX) return;
    uint64_t items = rd<uint64_t>(list + off::LIST_ITEMS);
    if (!valid_ptr(items)) return;

    uint64_t localPm = 0;

    for (int i = 0; i < size; i++) {
        uint64_t pm = rd<uint64_t>(items + off::ARRAY_DATA + 0x8 * i);
        if (!valid_ptr(pm)) continue;

        Player p;
        p.pm = pm;

        // живость: vitals.m_MaxHealth в разумных пределах + не респавн.
        uint64_t vitals = rd<uint64_t>(pm + off::PM_VITALS);
        if (valid_ptr(vitals)) {
            float hp = rd<float>(vitals + off::VITALS_MAX_HEALTH);
            if (hp >= off::HP_MIN && hp <= off::HP_MAX) {
                p.health = hp;
                p.alive = rd<uint8_t>(pm + off::PM_RESPAWNING) == 0;
            }
        }

        p.local = is_local(pm);
        if (p.local) localPm = pm;

        p.pos = read_position(pm, p.local, &p.headPos, &p.chestPos);
        p.inAir = rd<uint8_t>(pm + off::PM_IS_IN_AIR) != 0;

        uint64_t nick = rd<uint64_t>(pm + off::PM_NICKLABEL);
        if (valid_ptr(nick)) {
            p.teammate = rd<uint8_t>(nick + off::WK_IS_TEAMMATE) != 0;
            p.friend_  = rd<uint8_t>(nick + off::WK_IS_FRIEND)   != 0;
            uint64_t txt = rd<uint64_t>(nick + off::WK_NICKNAME);
            if (valid_ptr(txt)) {
                uint64_t str = rd<uint64_t>(txt + off::UITEXT_M_TEXT);
                p.name = rd_string(str, 48);
            }
        }

        p.userId = rd_string(rd<uint64_t>(pm + off::PM_USER_ID), 40);
        p.weapon = read_weapon(pm);

        s_players.push_back(p);
    }

    g_game.localPlayer = localPm;
    g_game.playerCount = (int)s_players.size();

    // --- камера локального игрока ---
    if (valid_ptr(localPm)) {
        // Базис берём из НАТИВНОГО worldCameraRoot Transform (мировой
        // кватернион в matrixData) — не зависит от рандомизации полей
        // MouseLook. worldCameraRoot держит yaw камеры.
        uint64_t camRoot = rd<uint64_t>(localPm + off::PM_WORLD_CAM_ROOT);
        bool haveBasis = false;
        Vec3 camPos;
        if (valid_ptr(camRoot)) {
            uint64_t native = rd<uint64_t>(camRoot + off::TR_NATIVE_PTR);
            if (valid_ptr(native)) {
                uint64_t matrix = rd<uint64_t>(native + off::TR_INT_MATRIXPTR);
                if (valid_ptr(matrix)) {
                    camPos = rd<Vec3>(matrix + off::TR_MATRIX_WORLDPOS);
                    Quat q = rd<Quat>(matrix + off::TR_MATRIX_ROT);
                    if (!camPos.zero()) g_game.camera.pos = camPos;
                    float yaw = 0, pitch = 0;
                    quat_to_yaw_pitch(q, yaw, pitch);
                    // worldCameraRoot = yaw-пивот (pitch=0); питч камеры
                    // лежит на дочернем m_LookRoot. Пробуем и его.
                    g_game.camera.yaw = yaw;
                    g_game.camera.pitch = pitch;
                    haveBasis = true;

                    // pitch — из m_LookRoot (дочерний Transform) если доступен.
                    uint64_t ml = rd<uint64_t>(localPm + off::PM_MOUSELOOK);
                    if (valid_ptr(ml)) {
                        uint64_t lookRoot = rd<uint64_t>(ml + off::ML_LOOKROOT);
                        if (valid_ptr(lookRoot)) {
                            uint64_t lrNative = rd<uint64_t>(lookRoot + off::TR_NATIVE_PTR);
                            if (valid_ptr(lrNative)) {
                                uint64_t lrMatrix = rd<uint64_t>(lrNative + off::TR_INT_MATRIXPTR);
                                if (valid_ptr(lrMatrix)) {
                                    Quat lq = rd<Quat>(lrMatrix + off::TR_MATRIX_ROT);
                                    float ly = 0, lp = 0;
                                    quat_to_yaw_pitch(lq, ly, lp);
                                    g_game.camera.pitch = lp; // питч от дочернего пивота
                                }
                            }
                        }
                    }
                }
            }
        }
        // fallback — управляемые поля MouseLook (pitch 0x60, аккумулятор 0x88).
        if (!haveBasis) {
            uint64_t ml = rd<uint64_t>(localPm + off::PM_MOUSELOOK);
            if (valid_ptr(ml)) {
                g_game.camera.pitch = rd<float>(ml + off::ML_PITCH);
                g_game.camera.yaw   = rd<float>(ml + off::ML_ANGLES_YAW);
            }
        }
        // позиция глаз: если нативный трансформ не дал — из позиции игрока.
        if (g_game.camera.pos.zero()) {
            for (auto &p : s_players) {
                if (p.pm == localPm) {
                    g_game.camera.pos = Vec3(p.pos.x, p.headPos.y - 0.15f, p.pos.z);
                    break;
                }
            }
        }
        // FOV — живой, из FPManager.
        uint64_t fp = rd<uint64_t>(localPm + off::PM_FPMANAGER);
        if (valid_ptr(fp)) {
            float cur  = rd<float>(fp + off::FP_FOV_CURRENT);
            float base = rd<float>(fp + off::FP_FOV_BASE);
            float offv = rd<float>(fp + off::FP_FOV_OFFSET);
            float fov = (base > 1.f && base < 170.f) ? base : cur;
            if (offv == offv && fabsf(offv) < 50.f) fov = cur - offv;
            if (fov > 5.f && fov < 160.f) g_game.camera.vfov = fov;
        }
    }

    // дистанции
    for (auto &p : s_players) {
        p.distance = p.pos.dist(g_game.camera.pos);
    }

    game_apply();
}

const std::vector<Player> &game_players() { return s_players; }

// ---------------------------------------------------------------------------
//  Применение чит-функций
// ---------------------------------------------------------------------------
void game_apply() {
    uint64_t local = g_game.localPlayer;
    if (!valid_ptr(local)) return;

    // --- FOV changer ---
    if (g_cheat.fovChanger) {
        uint64_t fp = rd<uint64_t>(local + off::PM_FPMANAGER);
        if (valid_ptr(fp)) {
            float v = g_cheat.fovValue;
            wr<float>(fp + off::FP_FOV_BASE, v);
            wr<float>(fp + off::FP_FOV_CURRENT, v);
        }
    }

    // --- Aimbot ---
    if (g_cheat.aimbot) {
        uint64_t ml = rd<uint64_t>(local + off::PM_MOUSELOOK);
        if (!valid_ptr(ml)) return;

        Vec3 eye = g_game.camera.pos;

        float bestFovPx = g_cheat.aimFov; // в угловых градусах
        Vec3  bestPoint;
        bool  have = false;

        for (const auto &p : s_players) {
            if (p.pm == local || !p.alive) continue;
            if (p.teammate && !g_cheat.aimTeammates) continue;

            Vec3 point = (g_cheat.aimBone == 0) ? p.headPos : p.chestPos;

            float tp, ty;
            angles_to_target(eye, point, tp, ty);
            float dPitch = tp - g_game.camera.pitch;
            float dYaw   = normalize_angle(ty - g_game.camera.yaw);
            float ang = sqrtf(dPitch * dPitch + dYaw * dYaw);
            if (ang > bestFovPx) continue;

            // visible-only: грубая проверка — цель перед нами (по углу).
            if (g_cheat.aimVisibleOnly) {
                Vec3 fwd = g_game.camera.forward();
                Vec3 dir = point - eye;
                float dl = dir.length();
                if (dl > 0.1f) {
                    float dotv = fwd.dot(dir) / dl;
                    if (dotv < 0.6f) continue; // вне конуса видимости
                }
            }

            bestFovPx = ang;
            bestPoint = point;
            have = true;
        }

        if (have) {
            float targetPitch, targetYaw;
            angles_to_target(eye, bestPoint, targetPitch, targetYaw);
            float curPitch = rd<float>(ml + off::ML_PITCH);
            float curYaw   = rd<float>(ml + off::ML_ANGLES_YAW);
            if (!std::isfinite(curPitch)) curPitch = g_game.camera.pitch;
            if (!std::isfinite(curYaw))   curYaw   = g_game.camera.yaw;
            float smooth = g_cheat.aimSmooth < 1.f ? 1.f : g_cheat.aimSmooth;

            float nextPitch = curPitch + (targetPitch - curPitch) / smooth;
            float nextYaw   = curYaw + normalize_angle(targetYaw - curYaw) / smooth;
            if (nextPitch > 80.f)  nextPitch = 80.f;
            if (nextPitch < -80.f) nextPitch = -80.f;
            nextYaw = normalize_angle(nextYaw);

            // Письмо углов: pitch-поле @0x60 (из него игра строит поворот
            // m_LookRoot) + аккумулятор-аксессор Vector2 @0x88 (x=pitch, y=yaw).
            wr<float>(ml + off::ML_PITCH, nextPitch);
            wr<float>(ml + off::ML_ANGLES_PITCH, nextPitch);
            wr<float>(ml + off::ML_ANGLES_YAW, nextYaw);
        }
    }
}
