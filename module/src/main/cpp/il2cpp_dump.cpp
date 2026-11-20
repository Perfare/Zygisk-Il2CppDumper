#include "il2cpp_dump.h"
#include "log.h"
#include "xdl.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <unistd.h>
#include <dlfcn.h>

// ===================== VERIFIED OFFSETS (FF THUONG - HEADLOCK V3) =====================
static constexpr uintptr_t OFF_CAMERA_GET_MAIN   = 0x9F8FD00;
static constexpr uintptr_t OFF_CAMERA_W2S        = 0x9F8F984;
static constexpr uintptr_t OFF_COMP_GET_TF       = 0x9FD75D0;
static constexpr uintptr_t OFF_TF_GET_POS        = 0x9FE5A68;
static constexpr uintptr_t OFF_QUAT_LOOKROT      = 0x9FCD630;
static constexpr uintptr_t OFF_SCREEN_GET_W      = 0x9F9D1D4;
static constexpr uintptr_t OFF_SCREEN_GET_H      = 0x9F9D1FC;

static constexpr uintptr_t OFF_CURRENT_MATCH     = 0x71BCD10;
static constexpr uintptr_t OFF_IS_TEAMMATE       = 0x71BD8EC;
static constexpr uintptr_t OFF_PLAYER_IS_DIEING  = 0x6F157C0;
static constexpr uintptr_t OFF_PLAYER_SET_AIM    = 0x6F2C794;
static constexpr uintptr_t OFF_PLAYER_GET_HEAD   = 0x6FAE37C;
static constexpr uintptr_t OFF_ENTITY_IS_DEAD    = 0x716CC10;

static constexpr uintptr_t OFF_MATCH_LOCAL       = 0xD8;
static constexpr uintptr_t OFF_HOOK_RVA          = 0x6F82148;
static constexpr const char* HOOK_NS             = "COW.GamePlay";
static constexpr const char* HOOK_CLASS          = "Player";
static constexpr const char* HOOK_METHOD         = "LateUpdate";

static void* g_il2cpp_handle = nullptr;

#if defined(__aarch64__)

struct Vector3 { float x, y, z; };
struct Quaternion { float x, y, z, w; };

struct AimConfig {
    int   enable = 1;        // 1 = bat auto hit dau, 0 = tat
    float fov    = 9999.0f;  // 9999 = thay dich o bat ky dau tren man hinh la hit ngay
    float head_y = 0.03f;    // tinh chinh do cao tam dau
};

static AimConfig g_cfg;
static uintptr_t g_base     = 0;
static uintptr_t g_dict_off = 0;
static uint64_t  g_frame    = 0;

using FnVoidVoid   = void* (*)(const void*);
using FnVoidPtr    = void* (*)(void*, const void*);
using FnBoolPtr    = bool  (*)(void*, const void*);
using FnIntVoid    = int   (*)(const void*);
using FnVec3Ptr    = Vector3 (*)(void*, const void*);
using FnW2S        = Vector3 (*)(void*, Vector3, const void*);
using FnLookRot    = Quaternion (*)(Vector3, const void*);
using FnSetAim     = void (*)(void*, Quaternion, bool, const void*);
using FnUpdateHook = void (*)(void*, const void*);

static FnVoidVoid   fn_Camera_get_main = nullptr;
static FnW2S        fn_Camera_W2S      = nullptr;
static FnVoidPtr    fn_Comp_get_tf     = nullptr;
static FnVec3Ptr    fn_Tf_get_pos      = nullptr;
static FnLookRot    fn_Quat_LookRot    = nullptr;
static FnIntVoid    fn_Screen_W        = nullptr;
static FnIntVoid    fn_Screen_H        = nullptr;
static FnVoidVoid   fn_CurrentMatch    = nullptr;
static FnBoolPtr    fn_IsTeammate      = nullptr;
static FnBoolPtr    fn_IsDieing        = nullptr;
static FnBoolPtr    fn_IsDead          = nullptr;
static FnVoidPtr    fn_GetHeadTF       = nullptr;
static FnSetAim     fn_SetAimRotation  = nullptr;
static FnUpdateHook orig_UpdateHook    = nullptr;

static inline bool IsValidPtr(const void* p) {
    uintptr_t a = reinterpret_cast<uintptr_t>(p);
    return a > 0x100000000ULL && a < 0x7FFFFFFFFFULL && (a & 0x3ULL) == 0;
}

static uintptr_t GetLibBase(const char* name) {
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;
    char line[512];
    uintptr_t best = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, name)) {
            uintptr_t s = 0;
            if (sscanf(line, "%lx-", &s) == 1) {
                if (best == 0 || s < best) best = s;
            }
        }
    }
    fclose(fp);
    return best;
}

static void* ResolveSym(void* xdlHandle, const char* name) {
    void* sym = nullptr;
    if (g_il2cpp_handle) sym = dlsym(g_il2cpp_handle, name);
    if (!sym && xdlHandle) sym = xdl_sym(xdlHandle, name, nullptr);
    if (!sym && xdlHandle) sym = xdl_dsym(xdlHandle, name, nullptr);
    if (!sym) sym = dlsym(RTLD_DEFAULT, name);
    return sym;
}

static void ReloadConfig() {
    FILE* fp = fopen("/data/local/tmp/ff_aim.cfg", "r");
    if (!fp) return;
    char buf[128];
    while (fgets(buf, sizeof(buf), fp)) {
        int iv = 0; float fv = 0.0f;
        if (sscanf(buf, "enable=%d", &iv) == 1) g_cfg.enable = iv;
        else if (sscanf(buf, "fov=%f", &fv) == 1 && fv >= 50.0f) g_cfg.fov = fv;
        else if (sscanf(buf, "head_y=%f", &fv) == 1) g_cfg.head_y = fv;
    }
    fclose(fp);
}

static uintptr_t DetectPlayersDictOffset(void* match, void* localPlayer) {
    static const uintptr_t kCandidates[] = {
        0x118, 0x120, 0x128, 0x130, 0x138, 0x140, 0x148, 0x150,
        0x98,  0xB8,  0x168, 0x170, 0x178, 0x180, 0x188, 0x190,
        0x198, 0x1A0, 0x1A8, 0x450, 0x460, 0x470, 0x478
    };
    void* playerKlass = *reinterpret_cast<void**>(localPlayer);
    if (!IsValidPtr(playerKlass)) return 0;

    for (uintptr_t off : kCandidates) {
        void* dict = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(match) + off);
        if (!IsValidPtr(dict)) continue;

        void* entries = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(dict) + 0x18);
        int32_t count = *reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(dict) + 0x20);
        if (!IsValidPtr(entries) || count <= 0 || count > 120) continue;

        uintptr_t arrBase = reinterpret_cast<uintptr_t>(entries) + 0x20;
        int checkN = (count < 8) ? count : 8;
        for (int i = 0; i < checkN; ++i) {
            uintptr_t eAddr = arrBase + static_cast<uintptr_t>(i) * 0x18;
            int32_t hash = *reinterpret_cast<int32_t*>(eAddr);
            if (hash < 0) continue;
            void* val = *reinterpret_cast<void**>(eAddr + 0x10);
            if (IsValidPtr(val) && (val == localPlayer || *reinterpret_cast<void**>(val) == playerKlass)) {
                LOGI("[FFAim] PlayersDict OK @ 0x%lx (count=%d)", (unsigned long)off, count);
                return off;
            }
        }
    }
    return 0;
}

static void RunInstantHeadMagnet(void* callerInstance) {
    if ((++g_frame % 90) == 1) ReloadConfig();
    if (!g_cfg.enable) return;

    void* match = fn_CurrentMatch(nullptr);
    if (!IsValidPtr(match)) return;

    void* localPlayer = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(match) + OFF_MATCH_LOCAL);
    if (!IsValidPtr(localPlayer) || callerInstance != localPlayer) return;

    if (fn_IsDead(localPlayer, nullptr) || fn_IsDieing(localPlayer, nullptr)) return;

    if (g_dict_off == 0) {
        g_dict_off = DetectPlayersDictOffset(match, localPlayer);
        if (g_dict_off == 0) return;
    }

    void* dict = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(match) + g_dict_off);
    if (!IsValidPtr(dict)) { g_dict_off = 0; return; }

    void* entries = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(dict) + 0x18);
    int32_t count = *reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(dict) + 0x20);
    if (!IsValidPtr(entries) || count <= 0) return;
    if (count > 120) count = 120;

    void* cam = fn_Camera_get_main(nullptr);
    if (!IsValidPtr(cam)) return;
    void* camTf = fn_Comp_get_tf(cam, nullptr);
    if (!IsValidPtr(camTf)) return;

    Vector3 camPos = fn_Tf_get_pos(camTf, nullptr);

    float sw = (fn_Screen_W ? static_cast<float>(fn_Screen_W(nullptr)) : 1280.0f);
    float sh = (fn_Screen_H ? static_cast<float>(fn_Screen_H(nullptr)) : 720.0f);
    if (sw < 100.0f || sh < 100.0f) { sw = 1280.0f; sh = 720.0f; }
    float cx = sw * 0.5f;
    float cy = sh * 0.5f;

    uintptr_t arrBase = reinterpret_cast<uintptr_t>(entries) + 0x20;
    void*   bestEnemy = nullptr;
    Vector3 bestHead  = {0, 0, 0};
    float   bestDist  = g_cfg.fov;

    for (int32_t i = 0; i < count; ++i) {
        uintptr_t entryAddr = arrBase + static_cast<uintptr_t>(i) * 0x18;
        int32_t hashCode = *reinterpret_cast<int32_t*>(entryAddr);
        if (hashCode < 0) continue;

        void* enemy = *reinterpret_cast<void**>(entryAddr + 0x10);
        if (!IsValidPtr(enemy) || enemy == localPlayer) continue;
        if (fn_IsTeammate(enemy, nullptr)) continue;
        if (fn_IsDead(enemy, nullptr) || fn_IsDieing(enemy, nullptr)) continue;

        void* headTf = fn_GetHeadTF(enemy, nullptr);
        if (!IsValidPtr(headTf)) continue;

        Vector3 headPos = fn_Tf_get_pos(headTf, nullptr);
        headPos.y += g_cfg.head_y;

        Vector3 sp = fn_Camera_W2S(cam, headPos, nullptr);
        // Chi hit dich dang hien truoc mat trong khung hinh
        if (sp.z <= 0.1f || sp.x < 0.0f || sp.x > sw || sp.y < 0.0f || sp.y > sh) continue;

        float dx = sp.x - cx;
        float dy = sp.y - cy;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist < bestDist) {
            bestDist  = dist;
            bestEnemy = enemy;
            bestHead  = headPos;
        }
    }

    if (!bestEnemy) return;

    Vector3 dir = { bestHead.x - camPos.x, bestHead.y - camPos.y, bestHead.z - camPos.z };
    float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len < 0.01f) return;
    dir.x /= len; dir.y /= len; dir.z /= len;

    // HIT CUNG TUC THI VAO DAU (KHONG SLERP, EP GOC = TRUE)
    Quaternion snapRot = fn_Quat_LookRot(dir, nullptr);
    fn_SetAimRotation(localPlayer, snapRot, true, nullptr);
}

static void Hook_MainLoop(void* instance, const void* method) {
    if (orig_UpdateHook) orig_UpdateHook(instance, method);
    RunInstantHeadMagnet(instance);
}

static bool InstallMethodInfoHook() {
    using FnDomainGet   = void* (*)();
    using FnGetAsms     = void** (*)(void*, size_t*);
    using FnAsmGetImg   = void* (*)(void*);
    using FnClsFromName = void* (*)(void*, const char*, const char*);
    using FnGetMethod   = void* (*)(void*, const char*, int);

    void* xdlH   = xdl_open("libil2cpp.so", XDL_DEFAULT);
    auto fDomain = reinterpret_cast<FnDomainGet>(ResolveSym(xdlH, "il2cpp_domain_get"));
    auto fAsms   = reinterpret_cast<FnGetAsms>(ResolveSym(xdlH, "il2cpp_domain_get_assemblies"));
    auto fImg    = reinterpret_cast<FnAsmGetImg>(ResolveSym(xdlH, "il2cpp_assembly_get_image"));
    auto fCls    = reinterpret_cast<FnClsFromName>(ResolveSym(xdlH, "il2cpp_class_from_name"));
    auto fMtd    = reinterpret_cast<FnGetMethod>(ResolveSym(xdlH, "il2cpp_class_get_method_from_name"));

    if (!fDomain || !fAsms || !fImg || !fCls || !fMtd) {
        if (xdlH) xdl_close(xdlH);
        return false;
    }

    void* domain = fDomain();
    size_t n = 0;
    void** asms = fAsms(domain, &n);
    uintptr_t targetAddr = g_base + OFF_HOOK_RVA;

    for (size_t i = 0; i < n; ++i) {
        void* img = fImg(asms[i]);
        if (!img) continue;
        void* klass = fCls(img, HOOK_NS, HOOK_CLASS);
        if (!klass) continue;
        void* methodInfo = fMtd(klass, HOOK_METHOD, 0);
        if (!methodInfo) continue;

        void** slots = reinterpret_cast<void**>(methodInfo);
        for (int k = 0; k < 4; ++k) {
            uintptr_t sVal = reinterpret_cast<uintptr_t>(slots[k]);
            if (sVal == targetAddr || (k == 0 && sVal > g_base && sVal < g_base + 0x20000000ULL)) {
                orig_UpdateHook = reinterpret_cast<FnUpdateHook>(slots[k]);
                slots[k] = reinterpret_cast<void*>(&Hook_MainLoop);
                LOGI("[FFAim] HOOK THANH CONG (HEADLOCK V3 - INSTANT SNAP)");
                if (xdlH) xdl_close(xdlH);
                return true;
            }
        }
    }
    if (xdlH) xdl_close(xdlH);
    return false;
}

#endif

void il2cpp_api_init(void *handle) { g_il2cpp_handle = handle; }

void il2cpp_dump(const char *outDir) {
    (void)outDir;
#if defined(__aarch64__)
    LOGI("[FFAim] Khoi dong HEADLOCK V3...");
    g_base = GetLibBase("libil2cpp.so");
    if (!g_base) return;

    fn_Camera_get_main = reinterpret_cast<FnVoidVoid>(g_base + OFF_CAMERA_GET_MAIN);
    fn_Camera_W2S      = reinterpret_cast<FnW2S>(g_base + OFF_CAMERA_W2S);
    fn_Comp_get_tf     = reinterpret_cast<FnVoidPtr>(g_base + OFF_COMP_GET_TF);
    fn_Tf_get_pos      = reinterpret_cast<FnVec3Ptr>(g_base + OFF_TF_GET_POS);
    fn_Quat_LookRot    = reinterpret_cast<FnLookRot>(g_base + OFF_QUAT_LOOKROT);
    fn_Screen_W        = reinterpret_cast<FnIntVoid>(g_base + OFF_SCREEN_GET_W);
    fn_Screen_H        = reinterpret_cast<FnIntVoid>(g_base + OFF_SCREEN_GET_H);

    fn_CurrentMatch    = reinterpret_cast<FnVoidVoid>(g_base + OFF_CURRENT_MATCH);
    fn_IsTeammate      = reinterpret_cast<FnBoolPtr>(g_base + OFF_IS_TEAMMATE);
    fn_IsDieing        = reinterpret_cast<FnBoolPtr>(g_base + OFF_PLAYER_IS_DIEING);
    fn_IsDead          = reinterpret_cast<FnBoolPtr>(g_base + OFF_ENTITY_IS_DEAD);
    fn_GetHeadTF       = reinterpret_cast<FnVoidPtr>(g_base + OFF_PLAYER_GET_HEAD);
    fn_SetAimRotation  = reinterpret_cast<FnSetAim>(g_base + OFF_PLAYER_SET_AIM);

    ReloadConfig();
    for (int retry = 0; retry < 20; ++retry) {
        if (InstallMethodInfoHook()) break;
        sleep(1);
    }
#endif
}
