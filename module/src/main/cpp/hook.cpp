#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>
#include <sys/mman.h>
#include "log.h"
#include "xdl.h"

#if defined(__aarch64__)
extern "C" void A64HookFunction(void *symbol, void *replace, void **result);
#endif

// ---------------------------------------------------------
// Config
//   ds_hooks.txt : one "name=0" per line disables that hook group (read once
//                  at startup, restart the game after editing).
//                  Missing file or unlisted name = enabled.
//   sk_cd.txt    : cooldown factor 0.0 - 1.0 (re-read every second).
//   Names: hp shield delaydead mall gem energy cooldown items
//          seasoncoin autoattack crit shop goldarmor
//          unlocksave fly fullauto
//   fullauto replaces autoattack: when fullauto installs, the old
//   autoattack hooks are not installed.
// ---------------------------------------------------------
#define CFG_DIR "/storage/emulated/0/Android/data/com.ChillyRoom.DungeonShooter/files/"

// ---------------------------------------------------------
// File log: everything LOGI/LOGE prints is also appended to two files
// (truncated on every game start): CFG_DIR/ds_log.txt and the game's
// internal data dir /data/data/<pkg>/ds_log.txt. Read from Termux with:
//   su -c 'cat /data/data/com.ChillyRoom.DungeonShooter/ds_log.txt'
// ---------------------------------------------------------
#include <cstdarg>
#if __has_include(<android/log.h>)
#include <android/log.h>
#define DS_HAVE_ALOG 1
#endif

#define LOG_PATH_EXT CFG_DIR "ds_log.txt"
#define LOG_PATH_INT "/data/data/com.ChillyRoom.DungeonShooter/ds_log.txt"

static void ds_log_file(const char *path, int err, const char *fmt, va_list ap) {
    FILE *f = fopen(path, "a");
    if (!f) return;
    fputs(err ? "E " : "I ", f);
    vfprintf(f, fmt, ap);
    fputc('\n', f);
    fclose(f);
}

static void ds_log(int err, const char *fmt, ...) {
    va_list ap;
#ifdef DS_HAVE_ALOG
    va_start(ap, fmt);
    __android_log_vprint(err ? ANDROID_LOG_ERROR : ANDROID_LOG_INFO, "DSHook", fmt, ap);
    va_end(ap);
#endif
    va_start(ap, fmt);
    ds_log_file(LOG_PATH_EXT, err, fmt, ap);
    va_end(ap);
    va_start(ap, fmt);
    ds_log_file(LOG_PATH_INT, err, fmt, ap);
    va_end(ap);
}
#undef LOGI
#undef LOGE
#define LOGI(...) ds_log(0, __VA_ARGS__)
#define LOGE(...) ds_log(1, __VA_ARGS__)

static std::set<std::string> g_off;
static std::map<std::string, int> g_val;

static void load_cfg() {
    FILE *f = fopen(CFG_DIR "ds_hooks.txt", "r");
    if (!f) { LOGE("cfg: cannot open %sds_hooks.txt (all hooks enabled)", CFG_DIR); return; }
    char k[64];
    int v;
    while (fscanf(f, " %63[^=]=%d", k, &v) == 2) {
        g_val[k] = v;
        if (!v) { g_off.insert(k); LOGI("cfg: %s disabled", k); }
    }
    fclose(f);
}

static bool on(const char *k) { return g_off.find(k) == g_off.end(); }

static float get_cd_factor() {
    static float s_factor = 0.15f;
    static time_t s_last_check = 0;
    time_t now = time(nullptr);
    if (now != s_last_check) {
        s_last_check = now;
        FILE *f = fopen(CFG_DIR "sk_cd.txt", "r");
        if (f) {
            float val = 0.0f;
            if (fscanf(f, "%f", &val) == 1 && val >= 0.0f && val <= 1.0f)
                s_factor = val;
            fclose(f);
        }
    }
    return s_factor;
}

// ---------------------------------------------------------
// Field offsets. Each default is the value from the current dump; the
// installer overwrites it with the offset looked up BY NAME at runtime
// (il2cpp_class_get_field_from_name + il2cpp_field_get_offset), so a game
// update that shifts a field cannot silently corrupt memory. If the lookup
// fails, the default is kept and an E line is logged.
// ---------------------------------------------------------
static int g_off_itemprice  = 0x24;   // MallItem.itemPrice
static int g_off_finalprice = 0x28;   // MallItem.finalPrice
static int g_off_maxenergy  = 0x178;  // RoleAttributePlayer.max_energy
static int g_off_energy     = 0x17c;  // RoleAttributePlayer._energy
static int g_off_critical   = 0x40;   // DamageTrigger.critical
static int g_off_camp       = 0x64;   // DamageTrigger.camp
static int g_off_curcoin    = 0x10;   // CoinChangeEvent.curCoinVal

// ---------------------------------------------------------
// 1. HurtHp Hooks (God Mode)
// ---------------------------------------------------------
typedef void (*HurtHp_t)(void *self, int damage, void *src, void *mi);
static HurtHp_t g_orig[3];
static thread_local int g_depth = 0;

template <int N>
static void hurt_hook(void *self, int damage, void *src, void *mi) {
    static int logged = 0;
    if (logged < 40) { logged++; LOGI("HurtHp[%d] damage=%d depth=%d", N, damage, g_depth); }
    int d = (g_depth == 0 && damage > 0) ? -damage : damage;
    g_depth++;
    g_orig[N](self, d, src, mi);
    g_depth--;
}
static void *const g_repl[3] = {(void *) hurt_hook<0>, (void *) hurt_hook<1>, (void *) hurt_hook<2>};

// ---------------------------------------------------------
// 2. VoidShield Hooks (Infinite Shield + No Instadeath)
// ---------------------------------------------------------
typedef bool (*Consume_t)(void *self, void *ev, void *mi);
static Consume_t g_consume_orig;

typedef void (*SaveShield_t)(void *self, void *mi);
static SaveShield_t g_saveshield = nullptr;
static int g_shield_off = 0x18;   // _shieldCount, resolved by name at install
static void *g_saveshield_mi = nullptr;

static bool consume_hook(void *self, void *ev, void *mi) {
    static int logged = 0;
    int before = self ? *(int *)((char *)self + g_shield_off) : 0;
    bool r = g_consume_orig(self, ev, mi);
    if (self && before > 0) {
        int after = *(int *)((char *)self + g_shield_off);
        *(int *)((char *)self + g_shield_off) = before;
        if (after != before && g_saveshield) g_saveshield(self, g_saveshield_mi);
        if (logged < 20) { logged++; LOGI("VoidShield consume: count %d -> %d, restored to %d (saved=%d)", before, after, before, (int) (g_saveshield != nullptr)); }
    }
    return r;
}

typedef void (*AddDead_t)(void *self, void *src, void *mi);
static AddDead_t g_adddead_orig;

static void adddead_hook(void *self, void *src, void *mi) {
    static int logged = 0;
    if (logged < 20) { logged++; LOGI("AddDelayDeadBuff blocked"); }
}

// ---------------------------------------------------------
// 3. Mall Hooks (Free In-Game Purchases)
// ---------------------------------------------------------
struct R16 { uint64_t a, b; };
typedef R16 (*BuyItem_t)(void *self, void *mallItem, void *onDone, void *mi);
static BuyItem_t g_buyitem_orig;

static R16 buyitem_hook(void *self, void *mallItem, void *onDone, void *mi) {
    static int logged = 0;
    if (logged < 20) { logged++; LOGI("BuyItem hit! Forcing price to 0."); }
    if (mallItem) {
        *(int *)((char *)mallItem + g_off_itemprice) = 0;
        *(int *)((char *)mallItem + g_off_finalprice) = 0;
    }
    return g_buyitem_orig(self, mallItem, onDone, mi);
}

typedef bool (*HasEnough_t)(void *mallItem, void *mi);
static HasEnough_t g_hasenough_orig;

static bool hasenough_hook(void *mallItem, void *mi) {
    static int logged = 0;
    if (logged < 20) { logged++; LOGI("HasEnoughCurrency bypassed -> true"); }
    return true;
}

// ---------------------------------------------------------
// 4. RGSaveManager Hooks (Free Gem Purchases Outside Dungeon)
// ---------------------------------------------------------
typedef bool (*ConsumeGem_t)(void *self, int amount, bool unk1, bool unk2, void *mi);
static ConsumeGem_t g_consumegem_orig;

static bool consumegem_hook(void *self, int amount, bool unk1, bool unk2, void *mi) {
    static int logged = 0;
    if (logged < 20) { logged++; LOGI("ConsumeGem intercepted! Original cost: %d -> Forcing 0", amount); }
    g_consumegem_orig(self, 0, unk1, unk2, mi);
    return true;
}

// ---------------------------------------------------------
// 5. Energy Hook (RoleAttributePlayer.set_energy)
// ---------------------------------------------------------
static const int ENERGY_REFILL = 100;

typedef void (*SetEnergy_t)(void *self, int value, void *mi);
static SetEnergy_t g_setenergy_orig;

static void setenergy_hook(void *self, int value, void *mi) {
    static int logged = 0;
    g_setenergy_orig(self, value, mi);
    if (!self) return;

    int *p_max = (int *)((char *)self + g_off_maxenergy);
    int *p_cur = (int *)((char *)self + g_off_energy);
    int max = *p_max;
    int cur = *p_cur;
    if (max > 0 && cur <= ENERGY_REFILL) {
        *p_cur = max;
        if (logged < 20) { logged++; LOGI("energy: refilled %d -> %d", cur, max); }
    }
}

// ---------------------------------------------------------
// 6. Skill Cooldown Hooks (SkillInfo)
// ---------------------------------------------------------
static thread_local bool g_cd_applied = false;

typedef float (*GetFinalCd_t)(void *self, void *mi);
static GetFinalCd_t g_getfinalcd_orig;

static float getfinalcd_hook(void *self, void *mi) {
    static int logged = 0;
    g_cd_applied = false;
    float raw = g_getfinalcd_orig(self, mi);
    float factor = get_cd_factor();
    float result = g_cd_applied ? raw : raw * factor;
    if (logged < 20) { logged++; LOGI("SkillInfo.get_FinalCooldown: %.2f -> %.2f (factor=%.2f, inner=%d)", raw, result, factor, (int) g_cd_applied); }
    g_cd_applied = false;
    return result;
}

typedef float (*CalcFinalCd_t)(void *self, float cd, void *mi);
static CalcFinalCd_t g_calcfinalcd_orig;

static float calcfinalcd_hook(void *self, float cd, void *mi) {
    static int logged = 0;
    float raw = g_calcfinalcd_orig(self, cd, mi);
    float factor = get_cd_factor();
    float result = raw * factor;
    g_cd_applied = true;
    if (logged < 20) { logged++; LOGI("SkillInfo.CalculateFinalCooldown: %.2f -> %.2f (factor=%.2f)", raw, result, factor); }
    return result;
}

// ---------------------------------------------------------
// 7. No Change: Materials, Seeds, Vouchers, Fish Chips (ItemData)
// ---------------------------------------------------------
static void consumemat_hook(void *self) {
    static int logged = 0;
    if (logged < 20) { logged++; LOGI("ConsumeMaterial blocked"); }
}

typedef void (*ConsumeSeed_t)(void *self, void *itemName, void *mi);
static ConsumeSeed_t g_consumeseed_orig;
static void consumeseed_hook(void *self, void *itemName, void *mi) {
    static int logged = 0;
    if (logged < 20) { logged++; LOGI("ConsumeSeed blocked"); }
}

typedef void (*ConsumeTicket_t)(void *self, void *itemName, int count, void *mi);
static ConsumeTicket_t g_consumeticket_orig;
static void consumeticket_hook(void *self, void *itemName, int count, void *mi) {
    static int logged = 0;
    if (logged < 20) { logged++; LOGI("ConsumeTokenTicket blocked: count=%d", count); }
}

typedef void (*ConsumeFishCount_t)(void *self, int count, int obtainType, bool save, void *mi);
static ConsumeFishCount_t g_consumefishcount_orig;
static void consumefishcount_hook(void *self, int count, int obtainType, bool save, void *mi) {
    static int logged = 0;
    if (logged < 20) { logged++; LOGI("ConsumeFishCount blocked: count=%d", count); }
}

// ---------------------------------------------------------
// 9. Season Coin: No Deduct + Always Affordable
// ---------------------------------------------------------
typedef void (*DoConsumeSeasonCoin_t)(void *self, void *mi);
static DoConsumeSeasonCoin_t g_doconsumeseasoncoin_orig;
static void doconsumeseasoncoin_hook(void *self, void *mi) {
    static int logged = 0;
    if (logged < 20) { logged++; LOGI("DoConsumeSeasonCoin blocked: balance preserved"); }
}

typedef void (*DoConsumeGem2_t)(void *self, void *mi);
static DoConsumeGem2_t g_doconsumegem2_orig;
static void doconsumegem2_hook(void *self, void *mi) {
    static int logged = 0;
    if (logged < 20) { logged++; LOGI("DoConsumeGem (dialog) blocked"); }
}

typedef bool (*IsAffordable_t)(void *self, void *mi);
static IsAffordable_t g_isaffordable_orig;
static bool isaffordable_hook(void *self, void *mi) {
    static int logged = 0;
    if (logged < 20) { logged++; LOGI("IsAffordable bypassed -> true"); }
    return true;
}

// ---------------------------------------------------------
// 10. get_AutoAttack: true while in battle (RGWeapon)
//   OLD design (flag forcing). Superseded by section 14c (fullauto); only
//   installed when fullauto is off or failed to install.
// ---------------------------------------------------------
typedef bool (*GetAutoAttack_t)(void *self, void *mi);
static GetAutoAttack_t g_autoattack_orig[2];

typedef bool (*InBattle_t)(void *mi);
static InBattle_t g_inbattle = nullptr;
static void *g_inbattle_mi = nullptr;

template <int N>
static bool autoattack_hook(void *self, void *mi) {
    static int logged = 0;
    bool orig = g_autoattack_orig[N] ? g_autoattack_orig[N](self, mi) : false;
    bool r = g_inbattle ? (orig || g_inbattle(g_inbattle_mi)) : true;
    if (logged < 10) { logged++; LOGI("autoattack[%d] orig=%d -> %d", N, orig, r); }
    return r;
}

typedef bool (*HasTags_t)(void *self, void *mi);
static HasTags_t g_hastags_orig;

static bool hastags_hook(void *self, void *mi) {
    static int logged = 0;
    bool orig = g_hastags_orig ? g_hastags_orig(self, mi) : false;
    bool r = orig || (g_inbattle && g_inbattle(g_inbattle_mi));
    if (logged < 10) { logged++; LOGI("HasAutoAttackTags orig=%d -> %d", orig, r); }
    return r;
}

typedef void (*M12SetAttack_t)(void *self, bool v, bool hand, void *mi);
static M12SetAttack_t g_m12setattack_orig;

static void m12setattack_hook(void *self, bool v, bool hand, void *mi) {
    static int logged = 0;
    bool inb = g_inbattle && g_inbattle(g_inbattle_mi);
    if (logged < 20) { logged++; LOGI("M12.SetAttack v=%d hand=%d inbattle=%d", v, hand, inb); }
    g_m12setattack_orig(self, inb ? true : v, hand, mi);
}

// ---------------------------------------------------------
// 11. Always Crit
// ---------------------------------------------------------
static const int CAMP_CHARACTER = 1;

typedef void (*HandleCritical_t)(void *self, void *mi);
static HandleCritical_t g_handlecrit_orig;

static void handlecrit_hook(void *self, void *mi) {
    static int logged = 0;
    if (self) {
        int camp = *(int *)((char *)self + g_off_camp);
        if (camp == CAMP_CHARACTER) {
            int *p_crit = (int *)((char *)self + g_off_critical);
            if (logged < 20) { logged++; LOGI("HandleCritical: camp=%d critical %d -> 10000", camp, *p_crit); }
            *p_crit = 10000;
        }
    }
    g_handlecrit_orig(self, mi);
}

struct V3 { float x, y, z; };
typedef bool (*DealDamage_t)(void *carrier, int damage, bool isCrit, V3 repel, void *target,
                             void *sourceObj, void *weapon, int sourceCamp, int damageType,
                             int elementalType, int subType, void *mi);
static DealDamage_t g_dealdamage_orig;

static bool dealdamage_hook(void *carrier, int damage, bool isCrit, V3 repel, void *target,
                            void *sourceObj, void *weapon, int sourceCamp, int damageType,
                            int elementalType, int subType, void *mi) {
    static int logged = 0;
    bool crit = (sourceCamp == CAMP_CHARACTER) ? true : isCrit;
    if (logged < 20) { logged++; LOGI("DealDamage: camp=%d isCrit=%d -> %d dmg=%d", sourceCamp, isCrit, crit, damage); }
    return g_dealdamage_orig(carrier, damage, crit, repel, target, sourceObj, weapon,
                             sourceCamp, damageType, elementalType, subType, mi);
}

// ---------------------------------------------------------
// 12. Free Shopping (not tied to the buff)
// ---------------------------------------------------------
static const int BUFF_EXPERTSHOP = 10;

struct PriceName { const char *cls; const char *m; };
static const PriceName k_sites[] = {
    {"ItemSellerDoctor",       "GetItemValue"},
    {"ItemSellPlaceAttrbute",  "GetItemValue"},
    {"ItemSellPlaceBallBuff",  "GetItemValue"},
    {"ObjectBuffman",          "Start"},
    {"ObjectSlotInGame",       "Start"},
    {"ObjectVendingMachine",   "Start"},
    {"RGContainer",            "GetItemValue"},
    {"RGContainer",            "CalculateItemValue"},
    {"TalkMercenaryCoinRobot", "GetItemValue"},
    {"TalkStatue",             "Start"},
    {"ContainerItemBase",      "GetItemValue"},
    {"GunCoin",                "DoConsume"},
};
static uintptr_t g_pr_lo[16], g_pr_hi[16];
static int g_pr_n = 0;

static uintptr_t bl_target(uintptr_t at) {
    uint32_t ins = *(volatile uint32_t *) at;
    if ((ins & 0xFC000000u) != 0x94000000u) return 0;
    int32_t imm = (int32_t)((ins & 0x03FFFFFFu) << 6) >> 6;
    return at + (intptr_t) imm * 4;
}

static bool patch_u32(uintptr_t at, uint32_t val) {
    uintptr_t pg = at & ~(uintptr_t) 4095;
    if (mprotect((void *) pg, 4096, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) return false;
    *(volatile uint32_t *) at = val;
    __builtin___clear_cache((char *) at, (char *) at + 4);
    mprotect((void *) pg, 4096, PROT_READ | PROT_EXEC);
    return *(volatile uint32_t *) at == val;
}

static bool patch_clamp(uintptr_t rate_call, uintptr_t *where) {
    for (uintptr_t a = rate_call + 8; a < rate_call + 0x100; a += 4) {
        uint32_t ins  = *(volatile uint32_t *) a;
        uint32_t prev = *(volatile uint32_t *) (a - 4);
        if ((ins & 0xFFFFFC00u) == 0x1A9FC400u &&
            (prev & 0xFFFFFC1Fu) == 0x7100041Fu &&
            ((prev >> 5) & 31) == ((ins >> 5) & 31)) {
            uint32_t rn = (ins >> 5) & 31, rd = ins & 31;
            *where = a;
            return patch_u32(a, 0x2A0003E0u | (rn << 16) | rd);
        }
    }
    return false;
}

typedef float (*RateFn_t)(float discount, void *mi);
static RateFn_t g_ratefn_orig;

static float ratefn_hook(float discount, void *mi) {
    static int logged = 0;
    if (logged < 20) { logged++; LOGI("CalBuffExpertShopExtraRate(%.2f) -> 0.00", discount); }
    return 0.0f;
}

typedef bool (*HasBuff_t)(void *self, int buff, void *mi);
static HasBuff_t g_hasbuff_orig;

static int g_site_hits[16];

__attribute__((noinline))
static bool hasbuff_hook(void *self, int buff, void *mi) {
    bool r = g_hasbuff_orig(self, buff, mi);
    if (buff != BUFF_EXPERTSHOP || r) return r;
    uintptr_t ra = (uintptr_t) __builtin_return_address(0);
    for (int i = 0; i < g_pr_n; i++) {
        if (ra >= g_pr_lo[i] && ra < g_pr_hi[i]) {
            if (g_site_hits[i] < 3) { g_site_hits[i]++; LOGI("HasBuff(10) -> true at price site %d (ra=%p)", i, (void *) ra); }
            return true;
        }
    }
    return r;
}

// ---------------------------------------------------------
// 13. Golden Armor+
// ---------------------------------------------------------
static const int GOLD_STEP = 10;
static volatile int g_coin = 0;

static int gold_bonus() { int c = g_coin; return c > 0 ? c / GOLD_STEP : 0; }

typedef void (*OnCoinChange_t)(void *self, void *ev, void *mi);
static OnCoinChange_t g_oncoin_orig;

static void oncoin_hook(void *self, void *ev, void *mi) {
    static int logged = 0;
    if (ev) {
        int cur = *(int *)((char *)ev + g_off_curcoin);
        if (cur >= 0) g_coin = cur;
        if (logged < 20) { logged++; LOGI("OnCoinChange: curCoinVal=%d (+%d)", cur, gold_bonus()); }
    }
    g_oncoin_orig(self, ev, mi);
}

typedef void (*UpdateArmorInt_t)(void *self, int coinVal, void *mi);
static UpdateArmorInt_t g_updarmor_orig;

static void updarmor_hook(void *self, int coinVal, void *mi) {
    static int logged = 0;
    if (coinVal >= 0) g_coin = coinVal;
    if (logged < 20) { logged++; LOGI("UpdateArmor(coinVal=%d)", coinVal); }
    g_updarmor_orig(self, coinVal, mi);
}

typedef int (*GetInt_t)(void *self, void *mi);
static GetInt_t g_getmaxhp_orig;
static GetInt_t g_getmaxarmor_orig;

static int getmaxhp_hook(void *self, void *mi) {
    static int logged = 0;
    int r = g_getmaxhp_orig(self, mi);
    int b = gold_bonus();
    if (b > 0 && logged < 20) { logged++; LOGI("max_hp %d -> %d", r, r + b); }
    return r + b;
}

static int getmaxarmor_hook(void *self, void *mi) {
    static int logged = 0;
    int r = g_getmaxarmor_orig(self, mi);
    int b = gold_bonus();
    if (r <= 0 || b <= 0) return r;
    if (logged < 20) { logged++; LOGI("max_armor %d -> %d", r, r + b); }
    return r + b;
}

// ---------------------------------------------------------
// 14. Unlock heroes, skins, skills by the game's own setters (unlocksave)
//   Once per launch, right before the first ChooseHeroModel.OnShow, the game's
//   DataUtil.SetHeroUnlock / SetSkinUnlock / SetSkillUnlock are called for every
//   hero, skin and skill that the game's own getter (DataUtil.GetHeroUnlock /
//   GetSkinUnlock / GetSkillUnlock) does not already report as unlocked, then
//   RGSaveManager.SaveGameData() (only if something was set). A getter that is
//   missing or does not return Boolean is not used: that group then calls the
//   setter for every entry.
//   While the bulk call runs, g_bulk silences the two telemetry calls
//   (CommonStatistics.ReportClickUnlock, YearReviewManager.RecordDataIncrementInt)
//   and MapManagerHall's HeroUnlockEvent handler (it can start guides).
// ---------------------------------------------------------
static int g_off_charlist   = 0x20;
static int g_off_skinlist   = 0x28;
static int g_off_skillcount = 0x24;

static const int SKIN_HIDDEN_A = -3;
static const int SKIN_HIDDEN_B = -8;

typedef void *(*GetInst_t)(void *mi);
static GetInst_t g_getinst = nullptr;
static void *g_getinst_mi = nullptr;

typedef void (*SaveGameData_t)(void *self, void *mi);
static SaveGameData_t g_savegame = nullptr;
static void *g_savegame_mi = nullptr;

typedef void (*SetHeroUnlock_t)(int hero, bool unlock, void *mi);
typedef void (*SetSkinUnlock_t)(int hero, int skin, bool unlock, bool needTrack, void *mi);
typedef void (*SetSkillUnlock_t)(int hero, int skill, bool unlock, bool ignoreTrack, void *mi);
static SetHeroUnlock_t  g_sethero  = nullptr;  static void *g_sethero_mi  = nullptr;
static SetSkinUnlock_t  g_setskin  = nullptr;  static void *g_setskin_mi  = nullptr;
static SetSkillUnlock_t g_setskill = nullptr;  static void *g_setskill_mi = nullptr;

typedef bool (*GetHeroUnlock_t)(int hero, void *mi);
typedef bool (*GetSkinUnlock_t)(int hero, int skin, void *mi);
typedef bool (*GetSkillUnlock_t)(int hero, int skill, void *mi);
static GetHeroUnlock_t  g_gethero  = nullptr;  static void *g_gethero_mi  = nullptr;
static GetSkinUnlock_t  g_getskin  = nullptr;  static void *g_getskin_mi  = nullptr;
static GetSkillUnlock_t g_getskill = nullptr;  static void *g_getskill_mi = nullptr;

// Skill count the hero select screen uses: RGController.get_skills() calls
// SkillConfigLoader.GetSkillInfoList(hero), a plain indexer on the static
// dictionary _skillInfoDict (it THROWS if the key is missing). So the key is
// checked first with the dictionary's own ContainsKey, and the list size is read
// only for present keys. Any missing piece -> -1 (bound stays the save skillCount).
typedef void *(*SLGetList_t)(int id, void *mi);
typedef bool (*DictHasKey_t)(void *dict, int key, void *mi);
static SLGetList_t g_sl_getlist = nullptr;  static void *g_sl_getlist_mi = nullptr;
static void *g_sl_dictfield = nullptr;
static void (*g_sl_static_get)(void *, void *) = nullptr;
static void *(*g_sl_obj_class)(void *) = nullptr;
static void *(*g_sl_cls_method)(void *, const char *, int) = nullptr;
static void *(*g_sl_cls_field)(void *, const char *) = nullptr;
static int (*g_sl_fld_off)(void *) = nullptr;
static int g_sl_size_off = -1;

static int skill_list_count(int h) {
    static DictHasKey_t has = nullptr;
    static void *has_mi = nullptr;
    static bool tried = false;
    static bool nodict_logged = false;
    if (!g_sl_getlist || !g_sl_dictfield || !g_sl_static_get || !g_sl_obj_class || !g_sl_cls_method) return -1;
    void *dict = nullptr;
    g_sl_static_get(g_sl_dictfield, &dict);
    if (!dict) {
        if (!nodict_logged) { nodict_logged = true; LOGE("unlocksave: SkillConfigLoader._skillInfoDict is null, list bound unavailable"); }
        return -1;
    }
    if (!tried) {
        tried = true;
        void *k = g_sl_obj_class(dict);
        void *mi = k ? g_sl_cls_method(k, "ContainsKey", 1) : nullptr;
        if (mi && *(void **) mi) { has = (DictHasKey_t) *(void **) mi; has_mi = mi; }
        LOGI("unlocksave: skill list: dict=%p ContainsKey=%p GetSkillInfoList=%p", dict, (void *) has, (void *) g_sl_getlist);
        if (!has) LOGE("unlocksave: Dictionary.ContainsKey not callable, GetSkillInfoList is NOT used (it throws on a missing key)");
    }
    if (!has) return -1;
    if (!has(dict, h, has_mi)) return 0;
    void *list = g_sl_getlist(h, g_sl_getlist_mi);
    if (!list) return -1;
    if (g_sl_size_off < 0) {
        void *lk = g_sl_obj_class(list);
        void *f = (lk && g_sl_cls_field) ? g_sl_cls_field(lk, "_size") : nullptr;
        int off = (f && g_sl_fld_off) ? g_sl_fld_off(f) : -1;
        g_sl_size_off = (off > 0 && off < 0x40) ? off : 0x18;
        LOGI("unlocksave: List._size offset 0x%x (%s)", g_sl_size_off, (off > 0 && off < 0x40) ? "by name" : "fallback");
    }
    int n = *(int *) ((char *) list + g_sl_size_off);
    return (n >= 0 && n <= 64) ? n : -1;
}

static volatile bool g_bulk = false;
struct BulkGuard {
    BulkGuard() { g_bulk = true; }
    ~BulkGuard() { g_bulk = false; }
};

typedef void (*HallHeroEv_t)(void *self, void *ev, void *mi);
static HallHeroEv_t g_hallhero_orig;
static void hallhero_hook(void *self, void *ev, void *mi) {
    if (g_bulk) return;
    g_hallhero_orig(self, ev, mi);
}

typedef void (*ReportUnlock_t)(int a, int b, int c, int d, int e, void *mi);
static ReportUnlock_t g_reportunlock_orig;
static void reportunlock_hook(int a, int b, int c, int d, int e, void *mi) {
    if (g_bulk) return;
    g_reportunlock_orig(a, b, c, d, e, mi);
}

typedef void (*RecordInc_t)(void *name, int count, void *mi);
static RecordInc_t g_recordinc_orig;
static void recordinc_hook(void *name, int count, void *mi) {
    if (g_bulk) return;
    g_recordinc_orig(name, count, mi);
}

static bool g_unlock_done = false;
static bool g_unlock_running = false;

static void do_unlock_all() {
    if (g_unlock_done || g_unlock_running) return;
    char *inst = g_getinst ? (char *) g_getinst(g_getinst_mi) : nullptr;
    char *arr = inst ? *(char **) (inst + g_off_charlist) : nullptr;
    int n = arr ? *(int *) (arr + 0x18) : 0;
    if (n <= 0 || n > 4096) return;   // save not loaded yet: retry on the next OnShow
    g_unlock_running = true;
    g_unlock_done = true;
    int nh = 0, ns = 0, nk = 0;       // set
    int sh = 0, ss = 0, sk = 0;       // skipped (already unlocked)
    int n_diff = 0;
    {
        BulkGuard guard;
        for (int h = 0; h < n; h++) {
            char *e = *(char **) (arr + 0x20 + (size_t) h * 8);
            if (!e) continue;
            if (g_gethero && g_gethero(h, g_gethero_mi)) {
                sh++;
            } else {
                g_sethero(h, true, g_sethero_mi);
                nh++;
            }
            char *sl = *(char **) (e + g_off_skinlist);
            int len = sl ? *(int *) (sl + 0x18) : 0;
            if (len > 0 && len <= 4096) {
                for (int k = 0; k < len; k++) {
                    int v = *(int *) (sl + 0x20 + (size_t) k * 4);
                    if (v == SKIN_HIDDEN_A || v == SKIN_HIDDEN_B) continue;
                    if (g_getskin && g_getskin(h, k, g_getskin_mi)) { ss++; continue; }
                    g_setskin(h, k, true, false, g_setskin_mi);
                    ns++;
                }
            }
            int sc = *(int *) (e + g_off_skillcount);
            {
                int cl = skill_list_count(h);
                if (cl >= 0 && cl != sc && n_diff < 40) { n_diff++; LOGI("unlocksave: hero %d save skillCount=%d, game skill list=%d", h, sc, cl); }
                if (cl > sc) sc = cl;
            }
            if (sc > 0 && sc <= 64) {
                for (int k = 0; k < sc; k++) {
                    if (g_getskill && g_getskill(h, k, g_getskill_mi)) { sk++; continue; }
                    g_setskill(h, k, true, true, g_setskill_mi);
                    nk++;
                }
            }
        }
    }
    int saved = 0;
    if ((nh + ns + nk) > 0 && g_savegame) { g_savegame(inst, g_savegame_mi); saved = 1; }
    LOGI("unlocksave: heroes=%d skins=%d skills=%d skipped=%d/%d/%d saved=%d", nh, ns, nk, sh, ss, sk, saved);
    g_unlock_running = false;
}

typedef void (*OnShow_t)(void *self, void *extra, void *mi);
static OnShow_t g_onshow_orig;
static void onshow_hook(void *self, void *extra, void *mi) {
    do_unlock_all();
    g_onshow_orig(self, extra, mi);
}

// ---------------------------------------------------------
// 14b. Permanent flight on the local player (fly)
//   The game's own RGController.SetFlyable(flag, true, false, false, false)
//   (the call FlyMountController.DoFlyAfterMount and UIWindowSandbox.OnClick_Fly
//   make) is called with our own flag string after RGController.OnRoomEnter and
//   RGController.OnStartGame (both dungeon-only), only if IsLocalPlayer() and
//   only while RGBaseController.isFly is false. The flag is a key in the hero's
//   _flyFlags HashSet, so it does not clash with skill or mount flags. The hero
//   is destroyed and rebuilt on a level change (OnEnterNextLevel), so this runs
//   again every level.
// ---------------------------------------------------------
static int g_off_isfly = 0x102;   // RGBaseController.isFly

typedef void (*SetFlyable_t)(void *self, void *flag, bool isFlyable, bool changeSortingOrder,
                             bool ignoreLandFromAir, bool keepRunAnimation, void *mi);
typedef bool (*IsLocalPlayer_t)(void *self, void *mi);
static SetFlyable_t    g_setflyable = nullptr;  static void *g_setflyable_mi = nullptr;
static IsLocalPlayer_t g_islocal    = nullptr;  static void *g_islocal_mi    = nullptr;
static void *g_flyflag = nullptr;

static void fly_apply(void *self, int site) {
    static int n_skip[2], n_app[2];
    if (!self || !g_setflyable || !g_islocal || !g_flyflag) return;
    if (!g_islocal(self, g_islocal_mi)) return;
    bool before = *(volatile uint8_t *) ((char *) self + g_off_isfly) != 0;
    if (before) {
        if (n_skip[site] < 5) { n_skip[site]++; LOGI("fly: site %d, local player already flying, skipped", site); }
        return;
    }
    g_setflyable(self, g_flyflag, true, false, false, false, g_setflyable_mi);
    bool after = *(volatile uint8_t *) ((char *) self + g_off_isfly) != 0;
    if (n_app[site] < 5) { n_app[site]++; LOGI("fly: site %d applied, isFly 0 -> %d", site, (int) after); }
}

typedef void (*FlyEv_t)(void *self, void *ev, void *mi);
static FlyEv_t g_flyroom_orig;
static FlyEv_t g_flystart_orig;

static void flyroom_hook(void *self, void *ev, void *mi) {
    g_flyroom_orig(self, ev, mi);
    fly_apply(self, 0);
}

static void flystart_hook(void *self, void *ev, void *mi) {
    g_flystart_orig(self, ev, mi);
    fly_apply(self, 1);
}

// ---------------------------------------------------------
// 14c. Fully automatic attack (fullauto): copy of the game's own
//   "Fully Automatic Device" (WeaponAffixLevelAuto.Update, decoded from live
//   code, no guessing). The device registers a per-frame command that does:
//       X = weapon.controller.target_obj          (Transform, the current target)
//       if (X != null && !weapon.IsAutoAttack(tag)) weapon.SetAutoAttack(1.0f, 9999999f, tag)
//       if (X == null &&  weapon.IsAutoAttack(tag)) weapon.StopAutoAttack(tag)
//   SetAutoAttack starts RGWeapon.AutoAttacking as a coroutine on the weapon's
//   controller; it keeps the attack held while the tag is in the weapon's
//   _autoAttackTags set, and ends when StopAutoAttack removes the tag.
//   Here the same is done from a hook on RGController.FixedUpdate (every hero
//   controller override calls it), for the local player's front weapon only.
//   Extra (not in the device): when the front weapon of a controller changes,
//   the tag is removed from the previous front weapon (so it does not keep
//   firing from the back) and from the new one (clears a stale tag).
//   The target check is UnityEngine.Object.m_CachedPtr != 0 (what op_Inequality
//   reads), offsets resolved by name.
// ---------------------------------------------------------
static const int IL2CPP_TYPE_VOID = 0x01;

static int g_off_hand   = 0xa8;   // RGBaseController.hand (RGHand)
static int g_off_target = 0x38;   // RGBaseController.target_obj (Transform)
static int g_off_front  = 0x28;   // RGBaseHand.front_weapon (RGWeapon)
static int g_off_cached = 0x10;   // UnityEngine.Object.m_CachedPtr

typedef bool (*IsAuto_t)(void *weapon, void *tag, void *mi);
typedef void (*SetAuto_t)(void *weapon, float speed, float duration, void *tag, void *mi);
typedef void (*StopAuto_t)(void *weapon, void *tag, void *mi);
static IsAuto_t  g_fa_isauto = nullptr;  static void *g_fa_isauto_mi = nullptr;
static SetAuto_t g_fa_set    = nullptr;  static void *g_fa_set_mi    = nullptr;
static StopAuto_t g_fa_stop  = nullptr;  static void *g_fa_stop_mi   = nullptr;
static IsLocalPlayer_t g_fa_islocal = nullptr;  static void *g_fa_islocal_mi = nullptr;
static void *g_fa_tag = nullptr;
static const float FA_DURATION = 9999999.0f;

static uint32_t (*g_gc_new)(void *, bool) = nullptr;
static void *(*g_gc_target)(uint32_t) = nullptr;
static void (*g_gc_free)(uint32_t) = nullptr;

struct FaRec { uintptr_t front; uint32_t h; };
static std::map<uintptr_t, FaRec> g_fa_rec;

static void fa_tick(void *self) {
    static int n_arm = 0, n_stop = 0, n_sw = 0;
    if (!self || !g_fa_isauto || !g_fa_set || !g_fa_stop || !g_fa_islocal || !g_fa_tag) return;
    if (!g_fa_islocal(self, g_fa_islocal_mi)) return;
    char *hand = *(char **) ((char *) self + g_off_hand);
    if (!hand) return;
    char *w = *(char **) (hand + g_off_front);
    if (!w) return;

    if (g_gc_new && g_gc_target && g_gc_free) {
        if (g_fa_rec.size() > 64) {
            for (auto &kv : g_fa_rec)
                if (kv.second.h) g_gc_free(kv.second.h);
            g_fa_rec.clear();
        }
        FaRec &r = g_fa_rec[(uintptr_t) self];
        if (r.front != (uintptr_t) w) {
            if (r.h) {
                void *old = g_gc_target(r.h);
                if (old && old != (void *) w) g_fa_stop(old, g_fa_tag, g_fa_stop_mi);
                g_gc_free(r.h);
                r.h = 0;
            }
            g_fa_stop(w, g_fa_tag, g_fa_stop_mi);
            r.h = g_gc_new(w, false);
            r.front = (uintptr_t) w;
            if (n_sw < 10) { n_sw++; LOGI("fullauto: front weapon %p, previous tag cleared", (void *) w); }
        }
    }

    char *tr = *(char **) ((char *) self + g_off_target);
    bool has = tr && *(void **) (tr + g_off_cached) != nullptr;
    bool armed = g_fa_isauto(w, g_fa_tag, g_fa_isauto_mi);
    if (has && !armed) {
        g_fa_set(w, 1.0f, FA_DURATION, g_fa_tag, g_fa_set_mi);
        if (n_arm < 10) { n_arm++; LOGI("fullauto: target present, SetAutoAttack on weapon %p, IsAutoAttack now %d", (void *) w, (int) g_fa_isauto(w, g_fa_tag, g_fa_isauto_mi)); }
    } else if (!has && armed) {
        g_fa_stop(w, g_fa_tag, g_fa_stop_mi);
        if (n_stop < 10) { n_stop++; LOGI("fullauto: no target, StopAutoAttack on weapon %p", (void *) w); }
    }
}

typedef void (*FixedUpdate_t)(void *self, void *mi);
static FixedUpdate_t g_fa_orig = nullptr;

static void fa_hook(void *self, void *mi) {
    g_fa_orig(self, mi);
    fa_tick(self);
}

// ---------------------------------------------------------
// 15. Installer
// ---------------------------------------------------------
typedef void *(*ClassFromName_t)(void *, const char *, const char *);
typedef void *(*MethodByName_t)(void *, const char *, int);
static ClassFromName_t class_from_name;
static MethodByName_t  method_by_name;

static size_t (*image_class_count)(void *) = nullptr;
static void *(*image_get_class)(void *, size_t) = nullptr;
static const char *(*class_get_name)(void *) = nullptr;
static void *(*class_get_methods)(void *, void **) = nullptr;
static const char *(*method_get_name)(void *) = nullptr;

static const char *(*class_get_ns)(void *) = nullptr;
static void *(*asm_get_image)(void *) = nullptr;
static const char *(*img_get_name)(void *) = nullptr;
static void **g_asms = nullptr;
static size_t g_nasms = 0;

static void *scan_image(void *img, const char *name) {
    if (!image_class_count || !image_get_class || !class_get_name) return nullptr;
    size_t n = image_class_count(img);
    for (size_t i = 0; i < n; i++) {
        void *c = image_get_class(img, i);
        if (c && !strcmp(class_get_name(c), name)) return c;
    }
    return nullptr;
}

static void *find_class(void *img, const char *name) {
    void *k = class_from_name(img, "", name);
    if (k) return k;
    k = scan_image(img, name);
    for (size_t i = 0; !k && i < g_nasms && asm_get_image; i++) {
        void *im = asm_get_image(g_asms[i]);
        if (im && im != img) k = scan_image(im, name);
    }
    if (k) LOGI("find_class: %s found by scan (ns='%s')", name, class_get_ns ? class_get_ns(k) : "?");
    else LOGE("find_class: %s not found in any assembly", name);
    return k;
}

static uintptr_t method_addr(void *img, const char *cls, const char *m, int argc) {
    void *k = find_class(img, cls);
    void *mi = k ? method_by_name(k, m, argc) : nullptr;
    return mi ? (uintptr_t) *(void **) mi : 0;
}

static void hook_all_overloads(void *img, const char *cls, const char *m, void *repl) {
    static void *dummy[16];
    static int nd = 0;
    void *k = find_class(img, cls);
    if (!k || !class_get_methods || !method_get_name) { LOGE("hook: %s overloads unavailable", cls); return; }
    std::set<void *> done;
    void *iter = nullptr;
    while (void *mi = class_get_methods(k, &iter)) {
        if (strcmp(method_get_name(mi), m) != 0) continue;
        void *addr = *(void **) mi;
        if (!addr || !done.insert(addr).second || nd >= 16) continue;
#if defined(__aarch64__)
        A64HookFunction(addr, repl, &dummy[nd++]);
        LOGI("hook: %s.%s overload @%p installed", cls, m, addr);
#endif
    }
}

static void *(*class_get_field)(void *, const char *) = nullptr;
static int (*field_get_offset)(void *) = nullptr;
static void *(*method_ret_type)(void *) = nullptr;
static int (*type_get_type)(void *) = nullptr;
static const int IL2CPP_TYPE_I4 = 0x08;
static const int IL2CPP_TYPE_BOOLEAN = 0x02;

static int resolve_off(void *img, const char *cls, const char *fld, int fallback) {
    void *k = find_class(img, cls);
    void *f = (k && class_get_field && field_get_offset) ? class_get_field(k, fld) : nullptr;
    int off = f ? field_get_offset(f) : -1;
    if (off > 0 && off < 0x1000) {
        if (off == fallback) LOGI("field: %s.%s offset 0x%x (by name)", cls, fld, off);
        else LOGI("field: %s.%s offset 0x%x (by name, CHANGED from old 0x%x)", cls, fld, off, fallback);
        return off;
    }
    LOGE("field: %s.%s not found, keeping fallback 0x%x", cls, fld, fallback);
    return fallback;
}

static bool hook(void *img, const char *ns, const char *cls, const char *m, int argc,
                 void *repl, void **orig, int ret_type = -1) {
    void *k = class_from_name(img, ns, cls);
    if (!k && !*ns) k = find_class(img, cls);
    if (!k) { LOGE("hook: class %s not found", cls); return false; }
    void *mi = method_by_name(k, m, argc);
    void *addr = mi ? *(void **) mi : nullptr;
    if (!addr) { LOGE("hook: %s.%s(%d) not found", cls, m, argc); return false; }
    if (ret_type >= 0) {
        if (!method_ret_type || !type_get_type) { LOGE("hook: %s.%s skipped, return-type API missing", cls, m); return false; }
        int rt = type_get_type(method_ret_type(mi));
        if (rt != ret_type) { LOGE("hook: %s.%s skipped, return type 0x%x != 0x%x", cls, m, rt, ret_type); return false; }
    }
#if defined(__aarch64__)
    A64HookFunction(addr, repl, orig);
    LOGI("hook: %s.%s @%p installed", cls, m, addr);
    return true;
#else
    return false;
#endif
}

static uintptr_t iter_method_addr(void *img, const char *cls, const char *m) {
    void *k = find_class(img, cls);
    if (!k || !class_get_methods || !method_get_name) return 0;
    void *it = nullptr;
    while (void *mi = class_get_methods(k, &it))
        if (!strcmp(method_get_name(mi), m)) return (uintptr_t) *(void **) mi;
    return 0;
}

// Resolve a static Boolean getter; returns the MethodInfo or null (getter then unused).
static void *resolve_bool_getter(void *klass, const char *m, int argc) {
    void *mi = klass ? method_by_name(klass, m, argc) : nullptr;
    if (!mi || !*(void **) mi) { LOGE("unlocksave: getter %s(%d) not found, not used", m, argc); return nullptr; }
    if (!method_ret_type || !type_get_type) { LOGE("unlocksave: getter %s not used, return-type API missing", m); return nullptr; }
    int rt = type_get_type(method_ret_type(mi));
    if (rt != IL2CPP_TYPE_BOOLEAN) { LOGE("unlocksave: getter %s not used, return type 0x%x", m, rt); return nullptr; }
    return mi;
}

// fullauto: resolve an instance method by name and argument count, return type checked.
static void *fa_method(void *klass, const char *cls, const char *m, int argc, int ret_type) {
    void *mi = klass ? method_by_name(klass, m, argc) : nullptr;
    if (!mi || !*(void **) mi) { LOGE("fullauto: %s.%s(%d) not found", cls, m, argc); return nullptr; }
    if (!method_ret_type || !type_get_type) { LOGE("fullauto: %s.%s not used, return-type API missing", cls, m); return nullptr; }
    int rt = type_get_type(method_ret_type(mi));
    if (rt != ret_type) { LOGE("fullauto: %s.%s not used, return type 0x%x != 0x%x", cls, m, rt, ret_type); return nullptr; }
    return mi;
}

// fullauto: UnityEngine.Object.m_CachedPtr offset, by name from the class in namespace UnityEngine.
static int unity_cached_off(int fallback) {
    if (!asm_get_image || !class_get_field || !field_get_offset) return fallback;
    for (size_t i = 0; i < g_nasms; i++) {
        void *im = asm_get_image(g_asms[i]);
        void *k = im ? class_from_name(im, "UnityEngine", "Object") : nullptr;
        if (!k) continue;
        void *f = class_get_field(k, "m_CachedPtr");
        int off = f ? field_get_offset(f) : -1;
        if (off > 0 && off < 0x100) {
            LOGI("field: UnityEngine.Object.m_CachedPtr offset 0x%x (by name, image %s)", off, img_get_name ? img_get_name(im) : "?");
            return off;
        }
    }
    LOGE("field: UnityEngine.Object.m_CachedPtr not found, keeping fallback 0x%x", fallback);
    return fallback;
}

static void install(void *h) {
#define S(n) xdl_sym(h, n, nullptr)
    auto is_vm           = (bool  (*)(void *))              S("il2cpp_is_vm_thread");
    auto domain_get      = (void *(*)())                    S("il2cpp_domain_get");
    auto attach          = (void *(*)(void *))              S("il2cpp_thread_attach");
    auto get_asms        = (void **(*)(void *, size_t *))   S("il2cpp_domain_get_assemblies");
    auto get_image       = (void *(*)(void *))              S("il2cpp_assembly_get_image");
    auto image_name      = (const char *(*)(void *))        S("il2cpp_image_get_name");
    class_from_name      = (ClassFromName_t)                S("il2cpp_class_from_name");
    method_by_name       = (MethodByName_t)                 S("il2cpp_class_get_method_from_name");
    method_ret_type      = (void *(*)(void *))              S("il2cpp_method_get_return_type");
    type_get_type        = (int (*)(void *))                S("il2cpp_type_get_type");
    image_class_count    = (size_t (*)(void *))             S("il2cpp_image_get_class_count");
    image_get_class      = (void *(*)(void *, size_t))      S("il2cpp_image_get_class");
    class_get_name       = (const char *(*)(void *))        S("il2cpp_class_get_name");
    class_get_methods    = (void *(*)(void *, void **))     S("il2cpp_class_get_methods");
    method_get_name      = (const char *(*)(void *))        S("il2cpp_method_get_name");
    class_get_ns         = (const char *(*)(void *))        S("il2cpp_class_get_namespace");
    class_get_field      = (void *(*)(void *, const char *)) S("il2cpp_class_get_field_from_name");
    field_get_offset     = (int (*)(void *))                S("il2cpp_field_get_offset");
    auto string_new      = (void *(*)(const char *))        S("il2cpp_string_new");
    auto gchandle_new    = (uint32_t (*)(void *, bool))     S("il2cpp_gchandle_new");
    g_gc_new             = gchandle_new;
    g_gc_target          = (void *(*)(uint32_t))            S("il2cpp_gchandle_get_target");
    g_gc_free            = (void (*)(uint32_t))             S("il2cpp_gchandle_free");
    g_sl_static_get      = (void (*)(void *, void *))       S("il2cpp_field_static_get_value");
    g_sl_obj_class       = (void *(*)(void *))              S("il2cpp_object_get_class");
    asm_get_image        = get_image;
    img_get_name         = image_name;
#undef S

    if (!domain_get || !attach || !get_asms || !get_image || !image_name || !class_from_name || !method_by_name) {
        LOGE("hook: il2cpp api missing"); return;
    }
    LOGI("install: start");
    load_cfg();

    for (int i = 0; i < 180 && is_vm && !is_vm(nullptr); i++) sleep(1);
    void *domain = domain_get();
    attach(domain);

    { size_t n0 = 0; void **a0 = get_asms(domain, &n0); g_asms = a0; g_nasms = n0; }
    void *image = nullptr;
    for (int i = 0; i < 60 && !image; i++) {
        size_t n = 0;
        void **asms = get_asms(domain, &n);
        for (size_t k = 0; k < n; k++) {
            void *img = get_image(asms[k]);
            if (img && !strcmp(image_name(img), "Assembly-CSharp.dll")) { image = img; break; }
        }
        if (!image) sleep(1);
    }
    if (!image) { LOGE("hook: Assembly-CSharp not found"); return; }
    {
        void *gu = class_from_name(image, "", "GameUtil");
        void *imi = gu ? method_by_name(gu, "InBattleState", 0) : nullptr;
        if (imi && *(void **) imi) {
            g_inbattle_mi = imi;
            g_inbattle = (InBattle_t) *(void **) imi;
            LOGI("InBattleState resolved @%p", (void *) g_inbattle);
        } else LOGE("InBattleState not found, autoattack falls back to always-true");
    }

    if (on("mall")) {
        g_off_itemprice  = resolve_off(image, "MallItem", "itemPrice", g_off_itemprice);
        g_off_finalprice = resolve_off(image, "MallItem", "finalPrice", g_off_finalprice);
    }
    if (on("energy")) {
        g_off_maxenergy = resolve_off(image, "RoleAttributePlayer", "max_energy", g_off_maxenergy);
        g_off_energy    = resolve_off(image, "RoleAttributePlayer", "_energy", g_off_energy);
    }
    if (on("crit")) {
        g_off_critical = resolve_off(image, "DamageTrigger", "critical", g_off_critical);
        g_off_camp     = resolve_off(image, "DamageTrigger", "camp", g_off_camp);
    }
    if (on("goldarmor"))
        g_off_curcoin = resolve_off(image, "CoinChangeEvent", "curCoinVal", g_off_curcoin);

    if (on("unlocksave")) {
        g_off_charlist   = resolve_off(image, "RGSaveManager", "char_list", g_off_charlist);
        g_off_skinlist   = resolve_off(image, "RGCharacterInfo", "skin_list", g_off_skinlist);
        g_off_skillcount = resolve_off(image, "RGCharacterInfo", "skillCount", g_off_skillcount);
        void *sk = find_class(image, "RGSaveManager");
        void *gmi = sk ? method_by_name(sk, "get_Inst", 0) : nullptr;
        void *smi = sk ? method_by_name(sk, "SaveGameData", 0) : nullptr;
        void *dk = find_class(image, "DataUtil");
        void *hmi = dk ? method_by_name(dk, "SetHeroUnlock", 2) : nullptr;
        void *kmi = dk ? method_by_name(dk, "SetSkinUnlock", 4) : nullptr;
        void *lmi = dk ? method_by_name(dk, "SetSkillUnlock", 4) : nullptr;
        if (gmi && *(void **) gmi) { g_getinst = (GetInst_t) *(void **) gmi; g_getinst_mi = gmi; }
        if (smi && *(void **) smi) { g_savegame = (SaveGameData_t) *(void **) smi; g_savegame_mi = smi; }
        if (hmi && *(void **) hmi) { g_sethero  = (SetHeroUnlock_t)  *(void **) hmi; g_sethero_mi  = hmi; }
        if (kmi && *(void **) kmi) { g_setskin  = (SetSkinUnlock_t)  *(void **) kmi; g_setskin_mi  = kmi; }
        if (lmi && *(void **) lmi) { g_setskill = (SetSkillUnlock_t) *(void **) lmi; g_setskill_mi = lmi; }
        LOGI("unlocksave: get_Inst=%p SaveGameData=%p SetHero=%p SetSkin=%p SetSkill=%p",
             (void *) g_getinst, (void *) g_savegame, (void *) g_sethero, (void *) g_setskin, (void *) g_setskill);

        void *ghm = resolve_bool_getter(dk, "GetHeroUnlock", 1);
        void *gkm = resolve_bool_getter(dk, "GetSkinUnlock", 2);
        void *glm = resolve_bool_getter(dk, "GetSkillUnlock", 2);
        if (ghm) { g_gethero  = (GetHeroUnlock_t)  *(void **) ghm; g_gethero_mi  = ghm; }
        if (gkm) { g_getskin  = (GetSkinUnlock_t)  *(void **) gkm; g_getskin_mi  = gkm; }
        if (glm) { g_getskill = (GetSkillUnlock_t) *(void **) glm; g_getskill_mi = glm; }
        {
            void *ck = find_class(image, "SkillConfigLoader");
            void *lmi2 = ck ? method_by_name(ck, "GetSkillInfoList", 1) : nullptr;
            if (lmi2 && *(void **) lmi2) { g_sl_getlist = (SLGetList_t) *(void **) lmi2; g_sl_getlist_mi = lmi2; }
            g_sl_dictfield = (ck && class_get_field) ? class_get_field(ck, "_skillInfoDict") : nullptr;
            g_sl_cls_method = method_by_name;
            g_sl_cls_field  = class_get_field;
            g_sl_fld_off    = field_get_offset;
            LOGI("unlocksave: SkillConfigLoader GetSkillInfoList=%p _skillInfoDict field=%p static_get=%p obj_class=%p",
                 (void *) g_sl_getlist, g_sl_dictfield, (void *) g_sl_static_get, (void *) g_sl_obj_class);
        }
        LOGI("unlocksave: GetHero=%p GetSkin=%p GetSkill=%p (null = group not skipped)",
             (void *) g_gethero, (void *) g_getskin, (void *) g_getskill);

        if (!g_getinst || !g_sethero || !g_setskin || !g_setskill) {
            LOGE("unlocksave: a required function is missing, skipped");
        } else {
            bool t1 = hook(image, "", "CommonStatistics", "ReportClickUnlock", 5,
                           (void *) reportunlock_hook, (void **) &g_reportunlock_orig);
            bool t2 = hook(image, "", "YearReviewManager", "RecordDataIncrementInt", 2,
                           (void *) recordinc_hook, (void **) &g_recordinc_orig);
            bool t3 = hook(image, "", "MapManagerHall", "OnHeroUnlockEvent", 1,
                           (void *) hallhero_hook, (void **) &g_hallhero_orig);
            if (!t3) LOGE("unlocksave: hall handler hook missing, guides may appear after the first unlock");
            uintptr_t os = iter_method_addr(image, "ChooseHeroModel", "OnShow");
            if (!t1 || !t2) {
                LOGE("unlocksave: telemetry hooks missing, skipped (nothing was unlocked)");
            } else if (!os) {
                LOGE("unlocksave: ChooseHeroModel.OnShow not found, skipped");
            } else {
#if defined(__aarch64__)
                A64HookFunction((void *) os, (void *) onshow_hook, (void **) &g_onshow_orig);
                LOGI("unlocksave: OnShow hook installed @%p", (void *) os);
#endif
            }
        }
    }

    if (on("fly")) {
        g_off_isfly = resolve_off(image, "RGBaseController", "isFly", g_off_isfly);
        void *rk = find_class(image, "RGController");
        void *fmi = rk ? method_by_name(rk, "SetFlyable", 5) : nullptr;
        void *lmi = rk ? method_by_name(rk, "IsLocalPlayer", 0) : nullptr;
        bool lbool = false;
        if (lmi && *(void **) lmi && method_ret_type && type_get_type)
            lbool = type_get_type(method_ret_type(lmi)) == IL2CPP_TYPE_BOOLEAN;
        if (fmi && *(void **) fmi) { g_setflyable = (SetFlyable_t) *(void **) fmi; g_setflyable_mi = fmi; }
        if (lmi && *(void **) lmi && lbool) { g_islocal = (IsLocalPlayer_t) *(void **) lmi; g_islocal_mi = lmi; }
        if (string_new && gchandle_new) {
            g_flyflag = string_new("dsfly");
            if (g_flyflag) gchandle_new(g_flyflag, true);
        }
        LOGI("fly: SetFlyable=%p IsLocalPlayer=%p flag=%p isFly off=0x%x",
             (void *) g_setflyable, (void *) g_islocal, g_flyflag, g_off_isfly);
        if (!g_setflyable || !g_islocal || !g_flyflag) {
            LOGE("fly: a required function is missing, skipped");
        } else {
            bool f1 = hook(image, "", "RGController", "OnRoomEnter", 1,
                           (void *) flyroom_hook, (void **) &g_flyroom_orig);
            bool f2 = hook(image, "", "RGController", "OnStartGame", 1,
                           (void *) flystart_hook, (void **) &g_flystart_orig);
            if (!f1 && !f2) LOGE("fly: no trigger hook installed, flight will never be applied");
            else if (!f1 || !f2) LOGE("fly: only one trigger hook installed (OnRoomEnter=%d OnStartGame=%d)", (int) f1, (int) f2);
        }
    }

    bool fa_ok = false;
    if (on("fullauto")) {
        g_off_hand   = resolve_off(image, "RGBaseController", "hand", g_off_hand);
        g_off_target = resolve_off(image, "RGBaseController", "target_obj", g_off_target);
        g_off_front  = resolve_off(image, "RGBaseHand", "front_weapon", g_off_front);
        g_off_cached = unity_cached_off(g_off_cached);
        void *wk = find_class(image, "RGWeapon");
        void *rk = find_class(image, "RGController");
        void *m1 = fa_method(wk, "RGWeapon", "IsAutoAttack", 1, IL2CPP_TYPE_BOOLEAN);
        void *m2 = fa_method(wk, "RGWeapon", "SetAutoAttack", 3, IL2CPP_TYPE_VOID);
        void *m3 = fa_method(wk, "RGWeapon", "StopAutoAttack", 1, IL2CPP_TYPE_VOID);
        void *m4 = fa_method(rk, "RGController", "IsLocalPlayer", 0, IL2CPP_TYPE_BOOLEAN);
        if (m1) { g_fa_isauto = (IsAuto_t)   *(void **) m1; g_fa_isauto_mi = m1; }
        if (m2) { g_fa_set    = (SetAuto_t)  *(void **) m2; g_fa_set_mi    = m2; }
        if (m3) { g_fa_stop   = (StopAuto_t) *(void **) m3; g_fa_stop_mi   = m3; }
        if (m4) { g_fa_islocal = (IsLocalPlayer_t) *(void **) m4; g_fa_islocal_mi = m4; }
        if (string_new && gchandle_new) {
            g_fa_tag = string_new("dsfullauto");
            if (g_fa_tag) gchandle_new(g_fa_tag, true);
        }
        LOGI("fullauto: IsAutoAttack=%p SetAutoAttack=%p StopAutoAttack=%p IsLocalPlayer=%p tag=%p",
             (void *) g_fa_isauto, (void *) g_fa_set, (void *) g_fa_stop, (void *) g_fa_islocal, g_fa_tag);
        LOGI("fullauto: offsets hand=0x%x target_obj=0x%x front_weapon=0x%x m_CachedPtr=0x%x gc=%d/%d/%d",
             g_off_hand, g_off_target, g_off_front, g_off_cached,
             (int) (g_gc_new != nullptr), (int) (g_gc_target != nullptr), (int) (g_gc_free != nullptr));
        if (!g_fa_isauto || !g_fa_set || !g_fa_stop || !g_fa_islocal || !g_fa_tag) {
            LOGE("fullauto: a required function is missing, skipped");
        } else {
            if (!g_gc_new || !g_gc_target || !g_gc_free)
                LOGE("fullauto: gchandle API missing, weapon-switch cleanup disabled");
            fa_ok = hook(image, "", "RGController", "FixedUpdate", 0,
                         (void *) fa_hook, (void **) &g_fa_orig);
            if (!fa_ok) LOGE("fullauto: RGController.FixedUpdate hook failed, falling back to the old autoattack group");
        }
    }

    if (on("hp")) {
        const char *hurtnames[3] = {"RGController", "C12Controller", "C13Controller"};
        std::set<void *> done;
        int slot = 0;
        for (auto nm : hurtnames) {
            void *klass = class_from_name(image, "", nm);
            if (!klass) { LOGE("hook: class %s not found", nm); continue; }
            void *mi = method_by_name(klass, "HurtHp", 2);
            if (!mi) { LOGE("hook: HurtHp not found in %s", nm); continue; }
            void *addr = *(void **) mi;
            if (!addr || !done.insert(addr).second) continue;
#if defined(__aarch64__)
            A64HookFunction(addr, g_repl[slot], (void **) &g_orig[slot]);
            LOGI("hook: %s HurtHp @%p installed", nm, addr);
            slot++;
#endif
        }
    }

    if (on("shield")) {
        void *vk = class_from_name(image, "TalentImpl", "BuffImplVoidShield");
        if (!vk) vk = find_class(image, "BuffImplVoidShield");
        if (vk && class_get_field && field_get_offset) {
            void *fld = class_get_field(vk, "_shieldCount");
            int off = fld ? field_get_offset(fld) : -1;
            if (off > 0 && off < 0x200) { g_shield_off = off; LOGI("shield: _shieldCount offset 0x%x (by name)", off); }
            else LOGE("shield: _shieldCount field not found, using 0x%x", g_shield_off);
        }
        void *smi = vk ? method_by_name(vk, "SaveShieldCount", 0) : nullptr;
        if (smi && *(void **) smi) {
            g_saveshield = (SaveShield_t) *(void **) smi;
            g_saveshield_mi = smi;
            LOGI("shield: SaveShieldCount resolved @%p", (void *) g_saveshield);
        } else LOGE("shield: SaveShieldCount not found, count will not persist");
        hook(image, "TalentImpl", "BuffImplVoidShield", "OnBeforePlayerConsumeVoidShield", 1,
             (void *) consume_hook, (void **) &g_consume_orig);
    }

    if (on("delaydead"))
        hook(image, "TalentImpl", "BuffImplVoidShield", "AddDelayDeadBuff", 1,
             (void *) adddead_hook, (void **) &g_adddead_orig);

    if (on("mall")) {
        hook(image, "RGScript.Data.Mall", "MallData", "BuyItem", 2,
             (void *) buyitem_hook, (void **) &g_buyitem_orig);
        hook(image, "RGScript.Data.Mall", "MallData", "HasEnoughCurrency", 1,
             (void *) hasenough_hook, (void **) &g_hasenough_orig);
    }

    if (on("gem"))
        hook(image, "", "RGSaveManager", "ConsumeGem", 3,
             (void *) consumegem_hook, (void **) &g_consumegem_orig);

    if (on("energy"))
        hook(image, "", "RoleAttributePlayer", "set_energy", 1,
             (void *) setenergy_hook, (void **) &g_setenergy_orig);

    if (on("cooldown")) {
        hook(image, "", "SkillInfo", "get_FinalCooldown", 0,
             (void *) getfinalcd_hook, (void **) &g_getfinalcd_orig);
        hook(image, "", "SkillInfo", "CalculateFinalCooldown", 1,
             (void *) calcfinalcd_hook, (void **) &g_calcfinalcd_orig);
    }

    if (on("items")) {
        hook_all_overloads(image, "ItemData", "ConsumeMaterial", (void *) consumemat_hook);
        hook(image, "", "ItemData", "ConsumeSeed", 1,
             (void *) consumeseed_hook, (void **) &g_consumeseed_orig);
        hook(image, "", "ItemData", "ConsumeTokenTicket", 2,
             (void *) consumeticket_hook, (void **) &g_consumeticket_orig);
        hook(image, "", "ItemData", "ConsumeFishCount", 3,
             (void *) consumefishcount_hook, (void **) &g_consumefishcount_orig);
    }

    if (on("seasoncoin")) {
        hook(image, "RGScript.UI", "WindowBuyOrWatchAdDialog", "DoConsumeSeasonCoin", 0,
             (void *) doconsumeseasoncoin_hook, (void **) &g_doconsumeseasoncoin_orig);
        hook(image, "RGScript.UI", "WindowBuyOrWatchAdDialog", "DoConsumeGem", 0,
             (void *) doconsumegem2_hook, (void **) &g_doconsumegem2_orig);
        hook(image, "RGScript.UI", "WindowBuyOrWatchAdDialog", "IsAffordable", 0,
             (void *) isaffordable_hook, (void **) &g_isaffordable_orig);
    }

    if (on("autoattack") && fa_ok) {
        LOGI("autoattack: old flag hooks not installed (fullauto is active)");
    } else if (on("autoattack")) {
        hook(image, "", "RGWeapon", "get_AutoAttack", 0,
             (void *) autoattack_hook<0>, (void **) &g_autoattack_orig[0]);
        hook(image, "", "RGWeapon", "get__autoAttack", 0,
             (void *) autoattack_hook<1>, (void **) &g_autoattack_orig[1]);
        hook(image, "", "RGWeapon", "get_HasAutoAttackTags", 0,
             (void *) hastags_hook, (void **) &g_hastags_orig);
        hook(image, "", "MythicWeapon12", "SetAttack", 2,
             (void *) m12setattack_hook, (void **) &g_m12setattack_orig);
    }

    if (on("crit")) {
        hook(image, "", "DamageTrigger", "HandleCritical", 0,
             (void *) handlecrit_hook, (void **) &g_handlecrit_orig);
        hook(image, "", "DamageCarrierHelper", "DealDamage", 11,
             (void *) dealdamage_hook, (void **) &g_dealdamage_orig);
    }

    if (on("shop")) {
        uintptr_t rate    = method_addr(image, "DataUtil", "CalBuffExpertShopExtraRate", 1);
        uintptr_t hasbuff = method_addr(image, "BattleData", "HasBuff", 1);
        if (!rate || !hasbuff) {
            LOGE("shop: rate=%p hasbuff=%p, not found, skipped", (void *) rate, (void *) hasbuff);
        } else {
            for (const PriceName &pn : k_sites) {
                uintptr_t fn = method_addr(image, pn.cls, pn.m, 0);
                if (!fn) { LOGE("shop: %s.%s not found", pn.cls, pn.m); continue; }
                uintptr_t off = 0;
                for (uintptr_t o = 4; o < 0x400; o += 4)
                    if (bl_target(fn + o) == rate) { off = o; break; }
                if (!off || g_pr_n >= 16) { LOGE("shop: %s.%s has no rate call, skipped", pn.cls, pn.m); continue; }
                g_pr_lo[g_pr_n] = fn;
                g_pr_hi[g_pr_n] = fn + off;
                g_pr_n++;
                LOGI("shop: %s.%s rate call at +0x%x", pn.cls, pn.m, (unsigned) off);
                uintptr_t at = 0;
                if (patch_clamp(fn + off, &at)) LOGI("shop: %s.%s min-price clamp removed at +0x%x", pn.cls, pn.m, (unsigned) (at - fn));
                else LOGE("shop: %s.%s min-price clamp NOT found/patched", pn.cls, pn.m);
            }
            LOGI("shop: %d price sites found", g_pr_n);
#if defined(__aarch64__)
            if (g_pr_n > 0) {
                A64HookFunction((void *) rate, (void *) ratefn_hook, (void **) &g_ratefn_orig);
                A64HookFunction((void *) hasbuff, (void *) hasbuff_hook, (void **) &g_hasbuff_orig);
                LOGI("shop: hooks installed (rate=%p hasbuff=%p)", (void *) rate, (void *) hasbuff);
            }
#endif
        }
    }

    if (on("goldarmor")) {
        hook(image, "", "RoleAttributePlayer", "OnCoinChange", 1,
             (void *) oncoin_hook, (void **) &g_oncoin_orig);
        hook(image, "", "RoleAttributePlayer", "UpdateArmor", 1,
             (void *) updarmor_hook, (void **) &g_updarmor_orig);
        hook(image, "", "RoleAttributeProxy", "get_max_hp", 0,
             (void *) getmaxhp_hook, (void **) &g_getmaxhp_orig, IL2CPP_TYPE_I4);
        hook(image, "", "RoleAttributeProxy", "get_MaxArmor", 0,
             (void *) getmaxarmor_hook, (void **) &g_getmaxarmor_orig, IL2CPP_TYPE_I4);
    }
}

void hook_start(void *handle) {
    { FILE *f = fopen(LOG_PATH_EXT, "w"); if (f) fclose(f); }
    { FILE *f = fopen(LOG_PATH_INT, "w"); if (f) fclose(f); }
    LOGI("hook_start: module entered, pid=%d", (int) getpid());
    std::thread(install, handle).detach();
}
