# Oxide Simple — новый сурс (без скана памяти)

Простой внешний чит для **Oxide: Survival Island** (`com.catsbit.oxidesurvivalisland`,
Unity il2cpp / Mirror) под arm64 Android с root.

Сделан с нуля на основе прошлого проекта (`LastROot-main`), но **без сканера
памяти** и с **обновлёнными оффсетами под свежий дамп (Dump14)**.

## Что изменено

1. **Убран скан памяти.** В прошлом сурсе на старте был `ox_scanClassByName` —
   перебор анонимных/rw-регионов игры в `/proc/pid/maps` с поиском `Il2CppClass`
   по строковому имени (сотни МБ чтений, таймауты, зависания, ловится
   анти-читом). В этом сурсе классы берутся **напрямую по статическим RVA**:

   ```
   il2cpp_base + PLAYERMANAGER_TYPEINFO_RVA  -> Il2CppClass*
   klass       + CLASS_STATIC_FIELDS (0xB8)  -> static_fields
   statics     + 0x10                        -> List<PlayerManager> clientPlayerList
   ```

   RVA взяты из `script.json -> TypeInfoPointers` (Il2CppDumper, Dump14).

2. **Оффсеты обновлены под Dump14.** Все field-оффсеты сверены с `dump.cs`.
   Ключевые отличия от прошлого билда:
   - `PlayerManager.clientPlayerList` — новый статический обычный
     `List<PlayerManager>` по `static_fields + 0x10` (на клиенте всегда
     заполнен). Старый `activePlayerList` стал обфусцированным `aO<T>`.
   - `MouseLook`: pitch по-прежнему `float @0x60`, но **yaw больше не
     `@0x64`** — там теперь паддинг. Аккумулятор углов — `Vector2 @0x88`.
     Базис камеры для ESP читается с **нативного Transform**
     (`worldCameraRoot` / `m_LookRoot` -> matrixData кватернион) — это не
     зависит от перестановки managed-полей.
   - `Item.data` переехал `0x28 -> 0x20`.
   - `KCC/SingleKcc`: мотор/рекордер `SingleKcc @0x70/0x78`, голова `@0x90`.
   - TypeInfo RVA (PlayerManager `0xD18B560`, BuildingPiece `0xD183B18`,
     PlayerVitals `0xD18B5C0`, Camera `0xD183D38`, NetworkClient
     `0xD18A820`).

## Возможности

- **ESP**: боксы, ники, HP-бар, дистанция, оружие, team-check.
- **Aimbot**: FOV, плавность, кость (голова/корпус), проверка перед нами.
- **FOV changer.**
- Меню — простое ImGui (табы ESP / Aimbot / Player / Info).

Никаких Lua, скриптов, json, ассетов и прочего балласта — только то, что нужно.

## Структура

```
include/
  offsets.h   — все RVA и field-оффсеты (Dump14)
  proc.h      — процесс игры: pid, база модуля, rpm/wpm (process_vm_readv)
  game.h      — модели данных (Player, камера, настройки)
  math.h      — Vec3/Quat, W2S, углы прицеливания
  menu.h
  Android_draw/, Android_touch/, native_surface/, ImGui/ — рендер-стек (как было)
src/
  main.cpp    — точка входа: ожидание игры, overlay, поток данных, рендер
  proc.cpp
  game.cpp    — резолв классов по RVA, кэш игроков, камера, применение читов
  menu.cpp    — ImGui меню + ESP-отрисовка
  Android_draw/draw.cpp, Android_touch/TouchHelperA.cpp — EGL/ImGui/тач
  ImGui/      — imgui + android/opengl3 бэкенды
Android.mk / Application.mk
```

## Сборка

Нужен Android NDK (r23+):

```bash
cd new_source
ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=Android.mk NDK_APPLICATION_MK=Application.mk
# бинарь: libs/arm64-v8a/liboxsimple.so  (это исполняемый файл)
adb push libs/arm64-v8a/liboxsimple.so /data/local/tmp/oxsimple
adb shell su -c "chmod 755 /data/local/tmp/oxsimple && /data/local/tmp/oxsimple"
```

Запускать как root. Бинарь ждёт запуска игры и появления оверлея.

> Примечание: поля прицела MouseLook в этом билде частично обфусцированы —
> базис ESP берётся с нативного Transform (точно), а запись углов аимбота
> дублируется в `pitch @0x60` и аккумулятор `Vector2 @0x88`. Если на
> конкретном билде yaw не подхватится — проверить в IDA сеттер
> `MouseLook.LbJ` (Vector2) и уточнить компоненту.
