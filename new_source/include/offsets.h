#ifndef NEW_SOURCE_OFFSETS_H
#define NEW_SOURCE_OFFSETS_H

#include <cstdint>

// ============================================================================
//  Oxide: Survival Island — оффсеты
//  Обновлено по Il2CppDumper output: Dump14 (com.catsbit.oxidesurvivalisland)
//  Движок: Unity 6000.x il2cpp, сеть Mirror.
//
//  ВАЖНО: НИКАКОГО СКАНА ПАМЯТИ.
//  Адреса классов берутся НАПРЯМУЮ из .data: il2cpp_base + <XXX_TYPEINFO_RVA>
//  это указатель на Il2CppClass (Il2CppDumper -> script.json / TypeInfoPointers).
//  static_fields лежат в Il2CppClass+0xB8. Никаких name-scan по куче/анонам,
//  никаких metadataRegistration->types и XOR-расшифровок — всё статично.
// ============================================================================

namespace off {

// --- Цель ---
static constexpr const char *PACKAGE = "com.catsbit.oxidesurvivalisland";
static constexpr const char *MODULE  = "libil2cpp.so";

// --- TypeInfo RVA (script.json -> TypeInfoPointers, Dump14) -----------------
// base + RVA -> Il2CppClass* (заполняется рантаймом il2cpp при инициализации).
static constexpr uint64_t PLAYERMANAGER_TYPEINFO_RVA  = 0xD18B560; // Oxide.PlayerManager
static constexpr uint64_t BUILDINGPIECE_TYPEINFO_RVA  = 0xD183B18; // Oxide.Building.BuildingPiece
static constexpr uint64_t PLAYERVITALS_TYPEINFO_RVA   = 0xD18B5C0; // Oxide.PlayerVitals
static constexpr uint64_t CAMERA_TYPEINFO_RVA         = 0xD183D38; // UnityEngine.Camera
static constexpr uint64_t NETWORKCLIENT_TYPEINFO_RVA  = 0xD18A820; // Mirror.NetworkClient

// script.json TypeInfoPointers (Address decimal):
//   PlayerManager 219723104 / BuildingPiece 219691800 / PlayerVitals 219723200
//   Camera 219692344 / NetworkClient 219719712

// --- Il2CppClass (runtime layout, il2cpp v29, без изменений в этом билде) ---
static constexpr uint64_t CLASS_NAME           = 0x10; // const char* name
static constexpr uint64_t CLASS_NAMESPACE      = 0x18; // const char* namespaze
static constexpr uint64_t CLASS_STATIC_FIELDS  = 0xB8; // void* static_fields

// --- PlayerManager static fields (static_fields + X) ------------------------
//   static aO<PlayerManager> sleepingPlayerList;  // 0x0  (кастомный список)
//   static aO<PlayerManager> activePlayerList;    // 0x8  (кастомный список)
//   static List<PlayerManager> clientPlayerList;  // 0x10 <-- обычный List<T>,
//                                                  //       заполнен на КЛИЕНТЕ,
//                                                  //       его и используем
static constexpr uint64_t PM_STAT_CLIENT_LIST  = 0x10; // List<PlayerManager> clientPlayerList

// --- generic List<T> ---
static constexpr uint64_t LIST_ITEMS = 0x10; // T[] _items
static constexpr uint64_t LIST_SIZE  = 0x18; // int _size

// --- Il2CppArray ---
static constexpr uint64_t ARRAY_COUNT = 0x18;
static constexpr uint64_t ARRAY_DATA  = 0x20;

// --- Il2CppString (managed: klass, monitor + int length + char16 first_char) -
static constexpr uint64_t STRING_LENGTH = 0x10; // int32
static constexpr uint64_t STRING_DATA   = 0x14; // UTF-16 chars

// --- PlayerManager instance fields (dump.cs TDI 9016, сверено) --------------
static constexpr uint64_t PM_WORLD_CAM_ROOT    = 0x68;  // Transform worldCameraRoot
static constexpr uint64_t PM_MOUSELOOK         = 0x70;  // MouseLook mouseLook
static constexpr uint64_t PM_RAYCASTMANAGER    = 0x88;  // RaycastManager raycastManager
static constexpr uint64_t PM_FPMANAGER         = 0x90;  // FPManager fpManager
static constexpr uint64_t PM_KCC_REFERENCE     = 0xB0;  // InterfaceReference<My> kccReference
static constexpr uint64_t PM_VITALS            = 0xC8;  // PlayerVitals vitals
static constexpr uint64_t PM_TEAM              = 0x120; // fml team
static constexpr uint64_t PM_NICKLABEL         = 0x130; // wK nicklabel
static constexpr uint64_t PM_CHARACTER_MODEL   = 0x150; // GameObject characterModel
static constexpr uint64_t PM_ANIMATOR          = 0x190; // Animator animator
static constexpr uint64_t PM_LOOK_ANGLE        = 0x1A8; // float lookAngle
static constexpr uint64_t PM_IS_IN_AIR         = 0x1B8; // bool isInAir
static constexpr uint64_t PM_LAST_TICK_POS     = 0x1C8; // Vector3 lastTickPosition
static constexpr uint64_t PM_LAST_SAVED_POS    = 0x1D4; // Vector3 lastSavedPosition
static constexpr uint64_t PM_RESPAWNING        = 0x208; // bool respawning
static constexpr uint64_t PM_PLAYER_FLAGS      = 0x250; // PlayerFlags playerFlags
static constexpr uint64_t PM_PRIME             = 0x254; // bool prime
static constexpr uint64_t PM_USER_ID           = 0x278; // string userID (SyncVar)
static constexpr uint64_t PM_TEAM_NAME         = 0x280; // string teamName (SyncVar)

// --- Mirror.NetworkBehaviour (TDI 24710) ---
static constexpr uint64_t NB_NET_IDENTITY      = 0x40;  // NetworkIdentity netIdentity
// --- Mirror.NetworkIdentity (TDI 24736) ---
static constexpr uint64_t NI_IS_LOCAL_PLAYER   = 0x4A;  // bool isLocalPlayer
static constexpr uint64_t NI_IS_OWNED          = 0x4B;  // bool isOwned
static constexpr uint64_t NI_NET_ID            = 0x58;  // uint netId
static constexpr uint64_t NI_BEHAVIOURS        = 0x80;  // NetworkBehaviour[]

// --- MouseLook (TDI 8651) ---------------------------------------------------
// Раскладка в этом билде ИЗМЕНИЛАСЬ относительно прошлого дампа:
//   0x60 float  Lbz — PITCH (градусы, вниз +); игра ставит m_LookRoot
//        localRotation = Euler(Lbz.x …) — как и раньше.
//   0x88 Vector2 <LbJ>k__BackingField — аккумулятор углов (x = pitch?,
//        y = yaw). Раньше yaw был отдельным float @0x64 — теперь это
//        выровненный паддинг перед ссылкой LbF@0x68, поля там НЕТ.
static constexpr uint64_t ML_LOOKROOT      = 0x28; // Transform m_LookRoot
static constexpr uint64_t ML_INVERT        = 0x30; // bool m_Invert
static constexpr uint64_t ML_SENSITIVITY   = 0x34; // float m_Sensitivity
static constexpr uint64_t ML_PITCH         = 0x60; // float Lbz — PITCH (вниз +)
static constexpr uint64_t ML_ANGLES_BACK   = 0x88; // Vector2 LbJ (x,y углы камеры)
static constexpr uint64_t ML_ANGLES_PITCH  = 0x88; // Vector2.x
static constexpr uint64_t ML_ANGLES_YAW    = 0x8C; // Vector2.y

// --- RaycastManager (TDI 8767) ---
static constexpr uint64_t RM_PLAYER         = 0x20; // PlayerManager player
static constexpr uint64_t RM_WORLD_CAMERA   = 0x30; // Camera m_WorldCamera
static constexpr uint64_t RM_RAY_LENGTH     = 0x38; // float m_RayLength
static constexpr uint64_t RM_AIM_RAY_LENGTH = 0x3C; // float m_AimRayLength
static constexpr uint64_t RM_LAYER_MASK     = 0x48; // LayerMask m_LayerMask
static constexpr uint64_t RM_AIM_LAYER_MASK = 0x4C; // LayerMask m_AimLayerMask

// --- FPManager (TDI 8733) ---
static constexpr uint64_t FP_WORLD_CAMERA   = 0x20; // Camera m_WorldCamera
static constexpr uint64_t FP_CURRENT_WEAPON = 0x50; // FPObject _currentWeapon
static constexpr uint64_t FP_FOV_CURRENT    = 0xA0; // float LtU — текущий (интерпол.) верт. FOV
static constexpr uint64_t FP_FOV_BASE       = 0xAC; // float <Ltq> — базовый верт. FOV
static constexpr uint64_t FP_FOV_OFFSET     = 0xB4; // float LtB — FOV-оффсет (отдача)

// --- FPObject (TDI 8740) ---
static constexpr uint64_t FPOBJ_ITEM        = 0x40; // Item <Ltl>k__BackingField
static constexpr uint64_t FPOBJ_NORMAL_FOV  = 0x80; // int normalFOV
static constexpr uint64_t FPOBJ_AIM_FOV     = 0x84; // int aimFOV

// --- Item (TDI 8943) / ItemData (TDI 8948) ----------------------------------
static constexpr uint64_t ITEM_DATA         = 0x20; // ItemData <LIN>k__BackingField (было 0x28!)
static constexpr uint64_t ITEMDATA_NAME     = 0x18; // string m_Name
static constexpr uint64_t ITEMDATA_SHORTNAME= 0x20; // string m_ShortName

// --- Vitals: GenericVitals (TDI 8664) <- EntityVitals (8662) <- PlayerVitals (8673)
static constexpr uint64_t VITALS_MAX_HEALTH = 0x88; // float m_MaxHealth (GenericVitals)
static constexpr uint64_t VITALS_PROTECTION = 0x90; // ProtectionValues m_Protection

// --- nicklabel wK (TDI 428) ---
static constexpr uint64_t WK_NICKNAME       = 0x38; // UI.Text nickname
static constexpr uint64_t WK_PLAYERID       = 0x40; // UI.Text playerId
static constexpr uint64_t WK_IS_FRIEND      = 0x90; // bool isFriend
static constexpr uint64_t WK_IS_TEAMMATE    = 0x91; // bool isTeammate
static constexpr uint64_t UITEXT_M_TEXT     = 0xE0; // string m_Text (UnityEngine.UI.Text)

// --- KCC (TDI 3889) / SingleKcc (TDI 2103) ----------------------------------
static constexpr uint64_t KCC_MOTOR         = 0x68; // KinematicCharacterMotor <Motor> (KCC)
static constexpr uint64_t KCC_RECORDER      = 0x70; // HitBoxRecorderRoot hitBoxRecorderRoot (KCC)
static constexpr uint64_t KCC_PLAYER        = 0x78; // PlayerManager player (KCC)
static constexpr uint64_t KCC_HEAD          = 0x88; // Transform head (KCC)
static constexpr uint64_t KCC_LOOK_HEIGHT   = 0x90; // float lookHeightOffset
static constexpr uint64_t KCC_NORMAL_HEIGHT = 0xA0; // float normalHeight
static constexpr uint64_t KCC_CROUCH_HEIGHT = 0xA4; // float m_CrouchHeight
static constexpr uint64_t SKCC_MOTOR        = 0x70; // KinematicCharacterMotor <Motor> (SingleKcc)
static constexpr uint64_t SKCC_RECORDER     = 0x78; // HitBoxRecorderRoot (SingleKcc)
static constexpr uint64_t SKCC_PLAYER       = 0x80; // PlayerManager player (SingleKcc)
static constexpr uint64_t SKCC_HEAD         = 0x90; // Transform head (SingleKcc)
static constexpr uint64_t SKCC_NORMAL_HEIGHT= 0xA8; // float normalHeight (SingleKcc)
static constexpr uint64_t SKCC_CROUCH_HEIGHT= 0xAC; // float m_CrouchHeight (SingleKcc)
static constexpr uint64_t SKCC_CHARANIM     = 0xC0; // CharacterAnimation FVv (SingleKcc)

// --- KinematicCharacterMotor (TDI 10057) ---
static constexpr uint64_t MOTOR_TRANSFORM      = 0x118; // Transform HWq
static constexpr uint64_t MOTOR_TICK_POS       = 0x1E0; // Vector3 InitialTickPosition
static constexpr uint64_t MOTOR_INTERP_POS     = 0x1EC; // Vector3 LastInterpolatedPosition
static constexpr uint64_t MOTOR_BASE_VELOCITY  = 0x210; // Vector3 BaseVelocity
static constexpr uint64_t MOTOR_CAPSULE_RADIUS = 0x30;  // float CapsuleRadius
static constexpr uint64_t MOTOR_CAPSULE_HEIGHT = 0x34;  // float CapsuleHeight
static constexpr uint64_t MOTOR_CAPSULE_YOFF   = 0x38;  // float CapsuleYOffset

// --- HitBoxRecorderRoot (TDI 4026) / hitbox lag-comp записи Yg (TDI 4023) ---
static constexpr uint64_t HBR_HITBOXES     = 0x68; // HitBox[] hitBoxes
static constexpr uint64_t HBR_RECORDS      = 0x80; // Yg[] ZeG (lag-comp записи)
static constexpr uint64_t YG_HITBOX        = 0x10; // HitBox ZPp
static constexpr uint64_t YG_WORLDPOS      = 0x34; // Vector3 ZPr (живая мировая позиция кости)
static constexpr uint64_t YG_WORLDROT      = 0x40; // Quaternion ZPl
// --- HitBox (TDI 8669) ---
static constexpr uint64_t HB_HITAREA       = 0x68; // HitArea m_HitArea (0 head,1 chest,2 leg,3 foot,4 hand)
static constexpr uint64_t HB_ID            = 0x6C; // int id

// --- PlayerModelInfo (TDI 3928) ---
static constexpr uint64_t PMI_HEAD         = 0x20; // Transform head
static constexpr uint64_t PMI_RIGHT_HAND   = 0x28; // Transform rightWeaponHolder
static constexpr uint64_t PMI_LEFT_HAND    = 0x30; // Transform leftWeaponHolder
static constexpr uint64_t PMI_BODY         = 0x40; // Transform body
static constexpr uint64_t PMI_CHAR_ANIM    = 0x60; // CharacterAnimation characterAnimation

// --- BuildingPiece (TDI 9420) static + instance ------------------------------
static constexpr uint64_t BP_STAT_SAVELIST   = 0x0;  // static HashSet<BuildingPiece> saveList
static constexpr uint64_t BP_STAT_SAVELOOKUP = 0x8;  // static Dictionary<int,BuildingPiece> saveLookup
static constexpr uint64_t BP_PIECE_NAME      = 0x80; // string m_PieceName
static constexpr uint64_t BP_CORRECT_POS     = 0x8C; // Vector3 m_CorrectPosition
static constexpr uint64_t BP_BOUNDS          = 0xD0; // Bounds m_Bounds
static constexpr uint64_t BP_ADD_BOUNDS      = 0xF8; // Bounds[] additionalBounds
static constexpr uint64_t BP_GRADE           = 0x190;// BuildingGrade.Enum m_Grade
static constexpr uint64_t BP_GRADE_HOLDER    = 0x198;// Transform gradeHolder
static constexpr uint64_t BP_HEALTH          = 0x380;// float health
static constexpr uint64_t BP_MAXHEALTH       = 0x384;// float maxHealth
static constexpr uint64_t BP_ID              = 0x38C;// int id

// --- Mono HashSet<T> (как в прошлом билде — Slot[] + _count) ----------------
static constexpr uint64_t HASHSET_SLOTS        = 0x18; // Slot[] _slots
static constexpr uint64_t HASHSET_COUNT        = 0x20; // int _count
static constexpr uint64_t HASHSET_LASTINDEX    = 0x24; // int _lastIndex
static constexpr uint64_t HASHSET_SLOT_STRIDE  = 0x10;
static constexpr uint64_t HASHSET_SLOT_VALUE   = 0x8;

// --- NetworkClient static fields (TDI 24725) --------------------------------
static constexpr uint64_t NC_STAT_SPAWNED   = 0x28; // static Dictionary<uint,NetworkIdentity> spawned

// --- UnityEngine.Object -> нативный объект ---
static constexpr uint64_t OBJ_CACHED_PTR    = 0x10; // m_CachedPtr

// --- Нативный Transform (Unity internal, не из dump.cs) ---------------------
// managed Transform +0x10 -> native Transform; native +0x28 -> TransformAccess
// (matrixData); +0x90 world pos, +0xA0 world quaternion, +0xB0 scale.
static constexpr uint64_t TR_NATIVE_PTR      = 0x10;
static constexpr uint64_t TR_INT_MATRIXPTR   = 0x28;
static constexpr uint64_t TR_MATRIX_WORLDPOS = 0x90;
static constexpr uint64_t TR_MATRIX_ROT      = 0xA0;

// --- Камера / ESP константы ---
static constexpr float DEFAULT_VFOV = 60.0f;   // верт. FOV fallback
static constexpr float EYE_HEIGHT   = 1.6f;
static constexpr float PLAYER_TOP   = 1.85f;
static constexpr float BOX_HEIGHT   = 1.85f;
static constexpr float HP_MIN       = 1.0f;
static constexpr float HP_MAX       = 200.0f;
static constexpr float ADS_FOV_RATIO = 0.90f;   // kFov < base*0.9 => целится

static constexpr int   LIST_COUNT_MAX = 200;    // защита от мусора в размере списка

} // namespace off

#endif // NEW_SOURCE_OFFSETS_H
