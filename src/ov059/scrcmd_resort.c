#include "types.h"
#include "constants/version.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/script_text_banks.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_script.h"
#include "field/resort.h"
#include "field/scrcmd_resort.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/fx.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "save/join_avenue.h"
#include "save/medal_box.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/resort_binary.h"
#include "system/resort_layout.h"
#include "system/resort_work.h"
#include "system/vm.h"
#include "system/wordset.h"

// The script plugin of the Join Avenue (plugin 8), commands from 1000. Its command table is in overlay 58, so that
// the shops' overlay 60 can take this overlay's place

// The page of records that func_ov059_021e79a4 shows, while the script waits
typedef struct {
    void *window;
    s16 page;
    u16 state;
    u16 *var;
} ResortRecordsWork;

// By version: Black 2's row, then White 2's
static const u16 sUnk7db4[2][8] = {
    { 161, 171, 181, 191, 201, 211, 221, 231 },
    { 241, 251, 261, 271, 281, 291, 301, 311 },
};

static ResortPeople *func_ov059_021e7b98(Field *field);
static u16 func_ov059_021e7bc8(FieldScriptEnv *env);
static void *func_ov059_021e7bd8(Field *field);
static int func_ov059_021e7c08(int value, int others, int change);

BOOL func_ov059_021e58c0(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ResortPerson *person = ResortSys_GetScriptPerson(env, func_ov059_021e7b98(GSYS_GetField(gsys)));

    func_ov137_021f10dc(person, 27, 1);
    func_ov137_021f0f84(person, 1);
    if (checkForMidnight(GSYS_GetGameData(gsys)) && func_ov137_021f10f4(person, 0) == 0) {
        func_0203640c(func_ov137_021f0fa8(person), 3, -1);
    }
    return FALSE;
}

BOOL func_ov059_021e591c(VM *vm, FieldScriptEnv *env) {
    ResortPerson *person =
        ResortSys_GetScriptPerson(env, func_ov059_021e7b98(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))));

    if (person != NULL) {
        func_ov137_021f10dc(person, 27, 0);
        func_ov137_021f0f84(person, 0);
    }
    return FALSE;
}

// Sets a variable to one of 65 things about the avenue and the person the script is about
BOOL func_ov059_021e5950(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    u32 which = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);
    ResortSys *sys = ResortSys_Get(field);
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    JoinAvenueOccupants *occupants = ResortSys_GetOccupants(sys);
    ResortPeople *people = func_ov059_021e7b98(field);
    void *shops = ResortSys_GetShopData(sys);
    void *table = ResortSys_GetRanks(sys);
    ResortPerson *person = ResortSys_GetScriptPerson(env, people);
    ResortPersonData *data = NULL;
    GameData *gameData = GSYS_GetGameData(gsys);
    void *unk28;
    int i;

    GetGameDataPlayerInfo(gameData);
    SaveControl_GetMedalBox(GameData_GetSaveControl(gameData));
    unk28 = ResortSys_GetTexts(sys);
    if (person != NULL) {
        data = func_ov137_021f1110(person);
    }
    switch (which) {
    case 0:
        *var = (u16)func_ov137_021f10e8(person, 0, NULL) + 80;
        break;
    case 1: {
        JoinAvenueRecord *a = func_020388c0(occupants);
        u32 b = JoinAvenue_GetParam(info, 12, 0);

        *var = a == NULL && b != 0 ? TRUE : FALSE;
        break;
    }
    case 2: {
        u16 a = func_ov137_021f10e8(person, 19, NULL);
        u16 b = func_ov137_021f10e8(person, 14, NULL);
        u16 c = func_ov137_021f10e8(person, 15, NULL);
        u16 d = func_ov137_021f10e8(person, 18, NULL);
        u16 id1 = func_020394b0(a, b, c, d, info, shops);
        u16 id2 = func_020363e0(func_ov137_021f0fa8(person), 0);
        const u16 *row1 = ResortShopData_GetShop(shops, id1);
        const u16 *row2 = ResortShopData_GetShop(shops, id2);
        u16 kind1 = ResortShopData_GetShopParam(row1, 0);
        u16 kind2 = ResortShopData_GetShopParam(row2, 0);
        u32 unk25 = func_ov137_021f10e8(person, 37, NULL);
        u32 unk13 = JoinAvenue_GetParam(info, 13, 0);

        if (kind1 == 5 && unk13 == 0) {
            unk25 = 1;
        }
        *var = kind1 != kind2 && unk25 == 0 ? TRUE : FALSE;
        break;
    }
    case 3:
        if (func_ov137_021f10e8(person, 20, NULL) == 0) {
            *var = func_ov137_021f1878(data, ResortSys_GetUnk24Table(sys), ResortSys_GetRanks(sys), info);
            func_ov137_021f10dc(person, 20, 1);
        } else {
            *var = 0;
        }
        break;
    case 6:
        *var = func_02038868(occupants) == 0 ? TRUE : FALSE;
        break;
    case 7:
        *var = func_ov137_021f10e8(person, 32, NULL);
        break;
    case 8:
        *var = func_ov137_021f10e8(person, 31, NULL);
        break;
    case 9:
        *var = func_02036434(func_02038470(func_ov137_021f1980(data)), 0);
        break;
    case 10:
        *var = func_02038868(occupants) == 8 ? TRUE : FALSE;
        break;
    case 11: {
        u16 id = func_020363e0(func_ov137_021f0fa8(person), 0);

        *var = ResortShopData_GetShopParam(ResortShopData_GetShop(shops, id), 0);
        break;
    }
    case 15: {
        BOOL flag = func_02036e4c(func_ov137_021f0f58(person), 4);
        JoinAvenuePersonParam param;

        *var = 0;
        if (flag) {
            for (param = 80; param <= 83; param++) {
                if (func_ov137_021f10e8(person, param, NULL) != 0) {
                    (*var)++;
                }
            }
        }
        break;
    }
    case 13: {
        BOOL flag = func_02036e4c(func_ov137_021f0f58(person), 0);
        u32 unk29 = func_ov137_021f10e8(person, 41, NULL);

        *var = flag && unk29 != 255 ? TRUE : FALSE;
        break;
    }
    case 16: {
        u32 a = func_ov137_021f10e8(person, 17, NULL);
        u32 b = JoinAvenue_GetParam(info, 2, 0);

        if (a == 0) {
            *var = 2;
        } else if (a <= b) {
            *var = 0;
        } else {
            *var = 1;
        }
        break;
    }
    case 17:
        *var = ResortSys_HasShopKind(sys, gameData, 0);
        break;
    case 18:
        *var = ResortSys_HasShopKind(sys, gameData, 1);
        break;
    case 19:
        *var = ResortSys_HasShopKind(sys, gameData, 3);
        break;
    case 20:
        *var = ResortSys_HasShopKind(sys, gameData, 2);
        break;
    case 21:
        *var = ResortSys_HasShopKind(sys, gameData, 4);
        break;
    case 22:
        *var = ResortSys_HasShopKind(sys, gameData, 5);
        break;
    case 23:
        *var = ResortSys_HasShopKind(sys, gameData, 6);
        break;
    case 24:
        *var = ResortSys_HasShopKind(sys, gameData, 7);
        break;
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
        which -= 25;
        *var = JoinAvenuePerson_IsEmpty(func_02038860(occupants, which)) == FALSE ? TRUE : FALSE;
        break;
    case 33:
        *var = JoinAvenue_GetParam(info, 11, 0);
        break;
    case 34:
        *var = JoinAvenue_GetParam(info, 10, 0);
        break;
    case 35: {
        u32 value = 0;

        for (i = 0; i < 4; i++) {
            void *record = func_0203888c(occupants, i);

            if (!func_020384e0(record) && func_020385a8(record, 31, NULL) == 0) {
                value = func_020385a8(record, 2, NULL);
                break;
            }
        }
        *var = value;
        break;
    }
    case 36: {
        u32 value = 0;

        for (i = 0; i < 4; i++) {
            void *record = func_0203888c(occupants, i);

            if (!func_020384e0(record) && func_020385a8(record, 31, NULL) == 2) {
                value = func_020385a8(record, 2, NULL);
                break;
            }
        }
        *var = value;
        break;
    }
    case 37:
        for (i = 0; i < 4; i++) {
            void *record = func_0203888c(occupants, i);

            if (!func_020384e0(record) && func_020385a8(record, 31, NULL) == 0) {
                *var = i;
                break;
            }
        }
        break;
    case 38:
        for (i = 0; i < 4; i++) {
            void *record = func_0203888c(occupants, i);

            if (!func_020384e0(record) && func_020385a8(record, 31, NULL) == 2) {
                *var = i;
                break;
            }
        }
        break;
    case 39:
        *var = ResortSys_GetTrainerTypeText(unk28, data, 10);
        break;
    case 40:
        *var = func_ov137_021f198c(data);
        break;
    case 41:
        *var = func_ov137_021f10f4(person, 0);
        break;
    case 44:
    case 45:
    case 46:
    case 47:
    case 48:
    case 49:
    case 50:
    case 51:
        which -= 44;
        *var = (u16)joinAveTextHandler(func_02038860(occupants, which), 0, NULL) + 80;
        break;
    case 52:
    case 53:
    case 54:
    case 55:
    case 56:
    case 57:
    case 58:
    case 59:
    {
        u16 id;

        which -= 52;
        id = func_020363e0(func_02038470(func_02038860(occupants, which)), 0);
        *var = ResortShopData_GetShopParam(ResortShopData_GetShop(shops, id), 5);
        break;
    }
    case 60: {
        u16 kind = func_ov137_021f1984(data);
        u16 unk1f = func_ov137_021f1968(data, 31, NULL);

        *var = kind == 1 && unk1f == 5 ? TRUE : FALSE;
        break;
    }
    case 61:
        *var = ResortBinary_Get(table, ResortBinary_FindRange(table, JoinAvenue_GetParam(info, 2, 0)), 11);
        break;
    case 62:
        *var = JoinAvenue_GetParam(info, 14, 0);
        break;
    case 63:
        *var = ResortBinary_Get(table, ResortBinary_FindRange(table, JoinAvenue_GetParam(info, 2, 0)), 6) - 1753;
        break;
    case 64:
        *var = func_02038868(occupants);
        break;
    }
    return FALSE;
}

BOOL func_ov059_021e5eb0(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    u16 a0 = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);
    ResortPeople *people = func_ov059_021e7b98(field);
    HeapID heapId = Field_GetHeapID(field);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    ResortSys *sys = ResortSys_Get(field);
    ResortPersonData *data = NULL;
    GameData *gameData = GSYS_GetGameData(gsys);
    ResortPerson *person = ResortSys_GetScriptPerson(env, people);

    if (person != NULL) {
        data = func_ov137_021f1110(person);
    }
    *var = ResortSys_GetScriptValue(a0, sys, data, gameData, wordSet, heapId);
    return FALSE;
}

BOOL func_ov059_021e5f38(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    u16 mode = VM_Read16(vm);
    u16 *var = ScriptReadVar(vm, env);
    ResortSys *sys = ResortSys_Get(field);
    ResortPerson *person;
    ResortPersonData *data;

    ResortSys_GetSlots(sys);
    person = ResortSys_GetScriptPerson(env, func_ov059_021e7b98(field));
    data = func_ov137_021f1110(person);
    switch (mode) {
    case 2: {
        VecFx32 pos;
        u16 x;
        u16 z;
        u16 dir;
        FieldPlayer *player;

        *var = ResortSys_MakeData(sys, data, 0);
        player = Field_GetPlayer(field);
        func_ov137_021f0fb8(person);
        func_ov137_021f1020(person, &x, &z, &dir);
        x = x <= 11 ? 11 : 19;
        FieldPlayer_GetWPos(player, &pos);
        pos.x = x * (16 * FX32_ONE) + 8 * FX32_ONE;
        pos.z = z * (16 * FX32_ONE) + 8 * FX32_ONE;
        FieldPlayer_SetWPos(player, &pos);
        FieldPlayer_SetDirection(player, dir);
        break;
    }
    case 0:
        *var = ResortSys_MakeData(sys, data, 0);
        break;
    case 1:
        *var = ResortSys_MakeData(sys, data, 3);
        break;
    }
    func_ov137_021f10dc(person, 37, 1);
    return FALSE;
}

BOOL func_ov059_021e6014(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    HeapID heapId = Field_GetHeapID(field);
    u16 a0 = VM_Read16(vm);
    u16 a1 = ScriptReadAny(vm, env);
    u16 a2 = ScriptReadAny(vm, env);
    u16 a3 = VM_Read16(vm);
    ResortSys *sys = ResortSys_Get(field);
    ResortPerson *person = ResortSys_GetScriptPerson(env, func_ov059_021e7b98(field));

    ResortSys_SetScriptWord(a0, a1, a2, a3, sys, person, wordSet, GSYS_GetGameData(gsys), heapId);
    return FALSE;
}

BOOL func_ov059_021e60a4(VM *vm, FieldScriptEnv *env) {
    Field *field;
    ResortSys *sys;
    ResortPeople *people;
    void *unk30;
    void *unk34;
    u16 mode;
    int which;
    u16 index;
    ResortPerson *person;
    ResortPersonData *data;

    FieldScriptEnv_GetScriptWork(env);
    field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    sys = ResortSys_Get(field);
    people = func_ov059_021e7b98(field);
    unk30 = ResortSys_GetDatas(sys);
    ResortSys_GetOccupants(sys);
    unk34 = ResortSys_GetSlots(sys);
    mode = ScriptReadAny(vm, env);
    which = ScriptReadAny(vm, env);
    index = ScriptReadAny(vm, env);
    person = NULL;
    data = NULL;
    switch (which) {
    case 255:
        person = ResortSys_GetScriptPerson(env, people);
        data = func_ov137_021f1110(person);
        break;
    case 0:
    case 1:
    case 2:
    case 3:
        data = func_ov137_021f1ba8(unk30, which, index);
        person = func_ov137_021f163c(people, data);
        break;
    }
    if (person != NULL) {
        if (Field_CheckGimmickWorkPassword(field, 1)) {
            func_ov137_021eeed4(field, person);
        }
        func_ov137_021f15ac(people, person);
    }
    if (data != NULL) {
        if (mode == 0) {
            func_ov137_021f1950(data);
        } else {
            func_ov137_021f1974(data, 0, 0);
            func_ov137_021f1974(data, 1, 0);
        }
    }
    func_ov137_021f1748(unk34);
    return FALSE;
}

BOOL func_ov059_021e619c(VM *vm, FieldScriptEnv *env) {
    ResortPerson *person;
    u16 value;

    FieldScriptEnv_GetScriptWork(env);
    person = ResortSys_GetScriptPerson(env, func_ov059_021e7b98(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))));
    value = func_020363e0(func_ov137_021f0fa8(person), 0);
    func_ov137_021f10dc(person, 19, value);
    return FALSE;
}

BOOL func_ov059_021e61d8(VM *vm, FieldScriptEnv *env) {
    Field *field;
    ResortSys *sys;
    void *shops;
    void *table;
    u16 index;
    u16 *var1;
    u16 *var2;
    ResortPerson *person;
    ResortPersonData *data;
    const u16 *row;
    u16 id;
    u32 prize;

    FieldScriptEnv_GetScriptWork(env);
    field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    sys = ResortSys_Get(field);
    shops = ResortSys_GetShopData(sys);
    table = ResortSys_GetUnk24Table(sys);
    index = ScriptReadAny(vm, env);
    var1 = ScriptReadVar(vm, env);
    var2 = ScriptReadVar(vm, env);
    person = ResortSys_GetScriptPerson(env, func_ov059_021e7b98(field));
    data = func_ov137_021f1110(person);
    *var2 = 0;
    row = ResortShopData_GetGoods(shops, ResortShopData_GetPersonShop(shops, func_ov137_021f0f58(person)), 0);
    id = ResortShopData_GetGoodsParam(row, 6);
    ResortShopData_GetGoodsParam(row, 5);
    prize = join_ave_raffle_shop(table, id, func_ov137_021f1990(data, index + 5, ResortBinary_Get(table, id, 0)));
    *var2 = prize + 1;
    if (prize != 10) {
        *var1 = ResortBinary_Get(table, id, prize * 2 + 3);
    }
    return FALSE;
}

// Sets a Pokémon's friendship
BOOL func_ov059_021e62a8(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    u16 slot;
    u16 value;
    u16 *var;
    PartyPkm *pkm;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    GSYS_GetField(gsys);
    slot = ScriptReadAny(vm, env);
    value = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    pkm = PokeParty_GetPkm(GameData_GetParty(GSYS_GetGameData(gsys)), slot);
    if (PokeParty_GetParam(pkm, PKM_PARAM_HAPPINESS, NULL) == 255) {
        *var = FALSE;
    } else {
        setPkmBattleData(pkm, PKM_PARAM_HAPPINESS, value);
        *var = TRUE;
    }
    return FALSE;
}

// Raises a Pokémon's level, up to 100
BOOL func_ov059_021e6318(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    u16 slot;
    s16 levels;
    u16 *var;
    PartyPkm *pkm;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    GSYS_GetField(gsys);
    slot = ScriptReadAny(vm, env);
    levels = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    pkm = PokeParty_GetPkm(GameData_GetParty(GSYS_GetGameData(gsys)), slot);
    if (PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL) < 100) {
        u32 level = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
        u32 species;
        u32 exp;

        level += levels;
        if (level > 100) {
            level = 100;
        }
        species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        exp = PML_UtilGetPkmLvExp(species, PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL), level);
        func_02038bc8(9);
        PokeParty_SetParam(pkm, PKM_PARAM_EXP, exp);
        PokeParty_RecalcStats(pkm);
        *var = TRUE;
    } else {
        *var = FALSE;
    }
    return FALSE;
}

// Raises or, for stats from 6, lowers one of a Pokémon's effort values
BOOL func_ov059_021e63d4(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    u16 slot;
    u16 stat;
    s32 change;
    u16 *var;
    PartyPkm *pkm;
    int total;
    int current;
    int i;
    int value;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    GSYS_GetField(gsys);
    slot = ScriptReadAny(vm, env);
    stat = VM_Read16(vm);
    change = (s16)ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    pkm = PokeParty_GetPkm(GameData_GetParty(GSYS_GetGameData(gsys)), slot);
    if (stat >= 6) {
        stat -= 6;
        change *= -1;
    }
    total = 0;
    for (i = 0; i < 6; i++) {
        total += PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + i, NULL);
        if (i == stat) {
            current = PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + i, NULL);
        }
    }
    value = func_ov059_021e7c08(current, total - current, change);
    if (value != -1) {
        PokeParty_SetParam(pkm, PKM_PARAM_EV_HP + stat, value);
        PokeParty_RecalcStats(pkm);
        *var = TRUE;
    } else {
        *var = FALSE;
    }
    return FALSE;
}

// Shortens the steps an egg of the party needs to hatch, by the shop's rate
BOOL func_ov059_021e64a0(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;
    ResortSys *sys;
    u16 index;
    u16 slot;
    u16 *var;
    ResortPerson *person;
    ResortPersonData *data;
    u32 rate;
    void *shops;
    void *table;
    const u16 *row;
    u16 id;
    u32 prize;
    PartyPkm *pkm;
    u32 species;
    u32 form;
    void *personal;
    u32 cycles;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    field = GSYS_GetField(gsys);
    sys = ResortSys_Get(field);
    index = ScriptReadAny(vm, env);
    slot = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    person = ResortSys_GetScriptPerson(env, func_ov059_021e7b98(field));
    data = func_ov137_021f1110(person);
    rate = 0;
    shops = ResortSys_GetShopData(sys);
    table = ResortSys_GetUnk24Table(sys);
    row = ResortShopData_GetGoods(shops, ResortShopData_GetPersonShop(shops, func_ov137_021f0f58(person)), index);
    id = ResortShopData_GetGoodsParam(row, 6);
    ResortShopData_GetGoodsParam(row, 5);
    prize = join_ave_raffle_shop(table, id, func_ov137_021f1990(data, index + 5, ResortBinary_Get(table, id, 0)));
    if (prize != 10) {
        rate = ResortBinary_Get(table, id, prize * 2 + 3);
    }
    pkm = PokeParty_GetPkm(GameData_GetParty(GSYS_GetGameData(gsys)), slot);
    species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    personal = PML_PersonalLoad(species, form, Field_GetHeapID(field));
    cycles = PML_PersonalGetParam(personal, 21);
    PML_PersonalFree(personal);
    if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == TRUE) {
        int steps = PokeParty_GetParam(pkm, PKM_PARAM_HAPPINESS, NULL);
        u32 left = 0;

        steps = steps - (int)(cycles * rate) / 100;
        if (steps > 0) {
            left = steps;
        }
        PokeParty_SetParam(pkm, PKM_PARAM_HAPPINESS, left);
        *var = left;
    } else {
        *var = 200;
    }
    return FALSE;
}

BOOL func_ov059_021e65fc(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    func_02036448(func_02038470(func_ov137_021f0f58(ResortSys_GetScriptPerson(
                      env, func_ov059_021e7b98(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)))))),
                  0, TRUE);
    return FALSE;
}

// Takes ten times the amount from the player's money, and sets a flag of the person
BOOL func_ov059_021e6630(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    Field *field = GSYS_GetField(gsys);
    ResortPeople *people = func_ov059_021e7b98(field);
    ResortPerson *person = ResortSys_GetScriptPerson(env, people);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    u16 amount = ScriptReadAny(vm, env);
    u16 bit = ScriptReadAny(vm, env);
    u32 cost = amount * 10;
    GameRecords *records;
    TrainerCardSave *card;

    card = getTrainerCardDataBlkAddress(gameData);
    subCashFromTotal(card, cost);
    records = GameData_GetRecords(gameData);
    RecordAdd(records, 0x16, cost);
    RecordAddOne(records, 0x15);
    WordSetNumber(wordSet, 1, cost, 6, 0, 1);
    func_02036448(func_02038470(func_ov137_021f0f58(person)), bit, TRUE);
    return FALSE;
}

BOOL func_ov059_021e66d8(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;
    ResortPerson *person;
    ResortSys *sys;
    JoinAvenueOccupants *occupants;
    u16 index;
    u16 *var;
    u16 *var2;
    u16 *var3;
    u16 *var4;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    person = ResortSys_GetScriptPerson(env, func_ov059_021e7b98(field));
    sys = ResortSys_Get(field);
    occupants = ResortSys_GetOccupants(sys);
    Field_GetHeapID(field);
    index = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    var2 = ScriptReadVar(vm, env);
    var3 = ScriptReadVar(vm, env);
    var4 = ScriptReadVar(vm, env);
    *var = ResortSys_GetPersonVisit(func_ov137_021f1110(person), func_02038860(occupants, index), sys, var2, var3, var4);
    return FALSE;
}

BOOL func_ov059_021e6778(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;
    ResortSys *sys;
    JoinAvenueOccupants *occupants;
    u16 index;
    u16 a2;
    u16 *var;
    u16 *var2;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    ResortSys_GetScriptPerson(env, func_ov059_021e7b98(field));
    sys = ResortSys_Get(field);
    occupants = ResortSys_GetOccupants(sys);
    index = ScriptReadAny(vm, env);
    a2 = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    var2 = ScriptReadVar(vm, env);
    *var = ResortSys_AddShopExperience(func_02038860(occupants, index), sys, a2, var2);
    return FALSE;
}

BOOL func_ov059_021e67f8(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    ResortSys *sys;
    u16 a2;
    u16 *var;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    ResortSys_GetScriptPerson(env, func_ov059_021e7b98(field));
    sys = ResortSys_Get(field);
    ResortSys_GetOccupants(sys);
    a2 = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    *var = ResortSys_AddPopularity(sys, gameData, a2, ScriptReadVar(vm, env));
    return FALSE;
}

BOOL func_ov059_021e6868(VM *vm, FieldScriptEnv *env) {
    JoinAvenueOccupants *occupants;

    FieldScriptEnv_GetScriptWork(env);
    occupants = ResortSys_GetOccupants(ResortSys_Get(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))));
    func_02038a0c(occupants, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL func_ov059_021e689c(VM *vm, FieldScriptEnv *env) {
    Field *field;
    ResortPeople *people;
    void *unk;
    ResortPerson *person;

    FieldScriptEnv_GetScriptWork(env);
    field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    people = func_ov059_021e7b98(field);
    unk = func_ov059_021e7bd8(field);
    person = ResortSys_GetScriptPerson(env, people);
    switch (ScriptReadAny(vm, env)) {
    case 0:
        ResortSys_HideOverlappingActors(people, unk, field, person, 0);
        break;
    case 1:
        ResortSys_HideOverlappingActors(people, unk, field, NULL, 0);
        break;
    case 3:
        ResortSys_HideOverlappingActors(people, unk, field, NULL, 1);
        break;
    case 4:
        ResortSys_HideOverlappingActors(people, unk, field, NULL, 2);
        break;
    case 2:
        func_ov137_021f152c(people);
        func_ov137_021f1d04(unk, 1);
        break;
    }
    return FALSE;
}

BOOL func_ov059_021e6934(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;
    ResortSys *sys;
    ResortPersonData *data;
    ResortWork *unk38;
    void *unk28;
    void *unk30;
    u16 a2;
    u16 mode;
    u16 *var;
    u16 *var2;
    u16 kind;
    u16 unk1f;
    BOOL flag;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    field = GSYS_GetField(gsys);
    sys = ResortSys_Get(field);
    ResortSys_GetInfo(sys);
    data = func_ov137_021f1110(ResortSys_GetScriptPerson(env, func_ov059_021e7b98(field)));
    unk38 = ResortSys_GetWork(sys);
    unk28 = ResortSys_GetTexts(sys);
    unk30 = ResortSys_GetDatas(sys);
    a2 = ScriptReadAny(vm, env);
    mode = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    var2 = ScriptReadVar(vm, env);
    kind = func_ov137_021f1984(data);
    unk1f = func_ov137_021f1968(data, 31, NULL);
    flag = FALSE;
    if (kind == 1 && unk1f == 5) {
        flag = TRUE;
    }
    switch (mode) {
    case 0: {
        ResortPersonData *other = func_ov137_021f1b94(unk30, 0, a2);
        u16 unk = ResortWork_Get(ResortSys_GetWork(sys), 10);
        u32 score;
        u32 value;
        u32 limit;

        ResortSys_GetOccupants(sys);
        score = func_ov137_021f199c(other, data, 27, 100);
        if (checkForMidnight(GSYS_GetGameData(gsys))) {
            *var = 1;
            return FALSE;
        }
        if (flag) {
            *var = 1;
            return FALSE;
        }
        if (!ResortSys_IsFull(sys)) {
            *var = 1;
            return FALSE;
        }
        if (!ResortSys_PickPersonToMeet(sys, other, data, &value)) {
            *var = 1;
            return FALSE;
        }
        if (unk == 0) {
            limit = ResortSys_GetVisitPoints(sys, other);
        } else {
            limit = ResortSys_GetVisitScore(sys, other);
        }
        if (score < limit) {
            *var = 0;
        } else if (score < limit + limit / 2) {
            *var = 2;
        } else {
            *var = 1;
        }
        *var2 = value;
        return FALSE;
    }
    case 1:
        *var = (a2 * (((u16)ResortWork_Get(unk38, 10) + 1) * (FX32_ONE / 4) + FX32_ONE)) >> FX32_SHIFT;
        break;
    case 2:
        *var = func_ov137_021f1990(data, 26, 100) < 85 ? TRUE : FALSE;
        break;
    case 3: {
        u16 unk2 = func_ov137_021f1968(data, 2, NULL);

        if (a2 != 0) {
            if (flag) {
                *var = ResortSys_GetTrainerTypeText(unk28, data, 15);
            } else {
                *var = 253;
            }
        } else if (flag) {
            *var = ResortSys_GetTrainerTypeText(unk28, data, 16);
        } else if (unk2 == 0) {
            *var = 256;
        } else {
            *var = 257;
        }
        break;
    }
    case 4:
        ResortSys_ChooseVisitGroup(sys, func_ov137_021f1b94(unk30, 0, a2), data);
        break;
    case 5:
        ResortSys_MarkMet(sys, func_ov137_021f1b94(unk30, 0, a2), a2);
        break;
    }
    return FALSE;
}

BOOL func_ov059_021e6b68(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    GameData *gameData;
    ResortSys *sys;
    JoinAvenueInfo *info;
    u16 *var;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    sys = ResortSys_Get(GSYS_GetField(gsys));
    info = ResortSys_GetInfo(sys);
    var = ScriptReadVar(vm, env);
    *var = FALSE;
    if (JoinAvenue_GetParam(info, 2, 0) >= 100) {
        u32 result = ResortSys_GetUnlockForRank(sys, gameData, 100);

        if (result != 0 && result != 8) {
            func_02039064(info, result + 15, 1);
            *var = TRUE;
        }
    }
    return FALSE;
}

BOOL func_ov059_021e6bd4(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 a0 = VM_Read16(vm);
    u16 a3 = VM_Read16(vm);

    ScriptWork_CallEvent(work, JoinAvenueNameEntry_Create(a0, gsys, 0, a3));
    return TRUE;
}

BOOL func_ov059_021e6c10(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    func_ov137_021f10dc(ResortSys_GetScriptPerson(env, func_ov059_021e7b98(GSYS_GetField(gsys))), 20, 1);
    return FALSE;
}

BOOL func_ov059_021e6c48(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    void *unk;
    u16 id;
    u16 mode;
    u16 *var;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    unk = func_ov059_021e7bd8(GSYS_GetField(gsys));
    id = ScriptReadAny(vm, env);
    mode = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    switch (mode) {
    case 0:
        *var = func_ov059_021e7bc8(env);
        break;
    case 1:
        *var = func_ov059_021e7bc8(env) - 148;
        break;
    case 2:
        *var = func_ov137_021f1d60(unk, id - 148, 5);
        break;
    case 3:
        *var = func_ov137_021f1d60(unk, id - 148, 6);
        break;
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        mode -= 4;
        *var = func_ov137_021f1d60(unk, id - 148, mode + 7);
        break;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
        mode -= 16;
        *var = func_ov137_021f1d60(unk, id - 148, mode + 7) + 59;
        break;
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
        mode -= 28;
        *var = func_ov137_021f1d60(unk, id - 148, mode);
        break;
    case 33:
        *var = func_ov137_021f1f00(unk, id - 148);
        break;
    }
    return FALSE;
}

BOOL func_ov059_021e6d50(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work;
    GameSystem *gsys;
    Field *field;
    u16 param;
    u16 n;
    u16 *var;
    JoinAvenueOccupants *occupants;
    int count;
    HeapID heapId;
    int i;
    JoinAvenuePerson *person;

    work = FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    param = ScriptReadAny(vm, env);
    n = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    occupants = getAddressOfBeginningOfOccupants(SaveControl_GetJoinAvenue(GameData_GetSaveControl(GSYS_GetGameData(gsys))));
    count = 0;
    heapId = Field_GetHeapID(field);
    *var = 0;
    for (i = 7; i >= 0; i--) {
        person = func_02038860(occupants, i);
        if (!JoinAvenuePerson_IsEmpty(person) && joinAveTextHandler(person, param + 64, NULL) != 0) {
            count++;
        }
    }
    if (count == 0) {
        return FALSE;
    }
    if (n >= count) {
        return FALSE;
    }
    i = 7;
    while (TRUE) {
        person = func_02038860(occupants, i);
        if (!JoinAvenuePerson_IsEmpty(person)) {
            u32 value = joinAveTextHandler(person, param + 64, NULL);

            if (value != 0) {
                if (n == 0) {
                    WordSet *wordSet = ScriptWork_GetWordSet(work);
                    StrBuf *strbuf = GFL_StrBufCreate(8, HEAPID_TAIL(heapId));
                    u32 unk2 = joinAveTextHandler(person, 2, NULL);
                    u16 text[64];

                    joinAveTextHandler(person, JOIN_AVE_PARAM_NAME, text);
                    GFL_StrBufLoadString(strbuf, text);
                    func_0202437c(wordSet, 0, strbuf, unk2, 1, 2);
                    GFL_StrBufFree(strbuf);
                    strbuf = GFL_StrBufCreate(9, HEAPID_TAIL(heapId));
                    joinAveTextHandler(person, 88, text);
                    GFL_StrBufLoadString(strbuf, text);
                    func_0202437c(wordSet, 1, strbuf, 2, 1, 2);
                    GFL_StrBufFree(strbuf);
                    strbuf = GFL_StrBufCreate(9, HEAPID_TAIL(heapId));
                    joinAveTextHandler(person, 89, text);
                    GFL_StrBufLoadString(strbuf, text);
                    func_0202437c(wordSet, 2, strbuf, 2, 1, 2);
                    GFL_StrBufFree(strbuf);
                    *var = value;
                    break;
                }
                n--;
            }
        }
        if (--i < 0) {
            i = 7;
        }
    }
    return FALSE;
}

BOOL func_ov059_021e6f14(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field;
    u16 id;
    u16 column;
    u16 index;
    WordSet *wordSet;
    HeapID heapId;
    void *unk;
    MsgData *msgData;
    StrBuf *strbuf;

    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    id = ScriptReadAny(vm, env);
    column = ScriptReadAny(vm, env);
    index = VM_Read16(vm);
    wordSet = ScriptWork_GetWordSet(work);
    heapId = Field_GetHeapID(field);
    unk = func_ov059_021e7bd8(field);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
    strbuf = GFL_MsgDataLoadStrbufNew(msgData, func_ov137_021f1d60(unk, id, column + 7));
    func_0202437c(wordSet, index, strbuf, 2, 1, 2);
    GFL_StrBufFree(strbuf);
    GFL_MsgDataFree(msgData);
    return FALSE;
}

// Puts things about one of the avenue's people in the script's WordSet, and sets a variable to the message that
// tells them
BOOL func_ov059_021e6fc8(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work;
    GameSystem *gsys;
    Field *field;
    JoinAvenueOccupants *occupants;
    WordSet *wordSet;
    HeapID heapId;
    u16 index;
    u16 value;
    BOOL found;
    u16 number;
    enum { MODE_0 } mode;
    MsgData *msgData;
    BOOL any;
    u16 *var;
    JoinAvenuePerson *person;
    u16 date;
    u16 a;
    u16 b;
    u32 total;
    JoinAvenuePersonParam param;
    int i;
    u32 n;
    u32 count;
    StrBuf *strbuf;

    work = FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    func_ov059_021e7bd8(field);
    occupants = getAddressOfBeginningOfOccupants(SaveControl_GetJoinAvenue(GameData_GetSaveControl(GSYS_GetGameData(gsys))));
    wordSet = ScriptWork_GetWordSet(work);
    heapId = Field_GetHeapID(field);
    index = ScriptReadAny(vm, env);
    mode = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    person = func_02038860(occupants, index);
    switch (mode) {
    case 0:
        date = joinAveTextHandler(person, 44, NULL);
        if (date == 0) {
            *var = 1725;
            break;
        }
        WordSetNumber(wordSet, 1, (date >> 9) & 0x7f, 2, 2, 1);
        WordSetNumber(wordSet, 2, (date >> 5) & 0xf, 2, 0, 1);
        WordSetNumber(wordSet, 3, date & 0x1f, 2, 0, 1);
        *var = 1724;
        break;
    case 1:
        date = joinAveTextHandler(person, 45, NULL);
        if (date == 0) {
            *var = 1727;
            break;
        }
        WordSetNumber(wordSet, 1, (date >> 9) & 0x7f, 2, 2, 1);
        WordSetNumber(wordSet, 2, (date >> 5) & 0xf, 2, 0, 1);
        WordSetNumber(wordSet, 3, date & 0x1f, 2, 0, 1);
        *var = 1726;
        break;
    case 2:
        a = joinAveTextHandler(person, 10, NULL);
        b = joinAveTextHandler(person, 11, NULL);
        if (a == 0 && b == 0) {
            *var = 1729;
            break;
        }
        WordSetNumber(wordSet, 1, a, 3, 0, 1);
        WordSetNumber(wordSet, 2, b, 3, 0, 1);
        *var = 1728;
        break;
    case 3:
        value = joinAveTextHandler(person, 46, NULL);
        if (value == 0) {
            *var = 1731;
            break;
        }
        WordSetNumber(wordSet, 1, value, 3, 0, 1);
        *var = 1730;
        break;
    case 5:
        value = joinAveTextHandler(person, 47, NULL);
        if (value == 0) {
            *var = 1733;
            break;
        }
        WordSet_LoadSpeciesName(wordSet, 1, value);
        *var = 1732;
        break;
    case 6:
        a = joinAveTextHandler(person, JOIN_AVE_PARAM_COUNTRY, NULL);
        b = joinAveTextHandler(person, JOIN_AVE_PARAM_AREA, NULL);
        if (a == 0 && b == 0) {
            *var = 1737;
        } else if (b == 0) {
            loadCountryToStrbuf(wordSet, 1, a);
            *var = 1736;
        } else {
            loadCountryToStrbuf(wordSet, 1, a);
            loadCountryAreaToStrbuf(wordSet, 2, a, b);
            *var = 1735;
        }
        break;
    case 7:
        value = joinAveTextHandler(person, JOIN_AVE_PARAM_JOB, NULL);
        if (value == 0) {
            *var = 1739;
            break;
        }
        loadJobAnswerToStrbuf(wordSet, 1, value);
        *var = 1738;
        break;
    case 8:
        value = joinAveTextHandler(person, JOIN_AVE_PARAM_HOBBY, NULL);
        if (value == 0) {
            *var = 1741;
            break;
        }
        loadHobbyNameToStrbuf(wordSet, 1, value);
        *var = 1740;
        break;
    case 9:
        number = joinAveTextHandler(person, 17, NULL);
        found = FALSE;
        if (number != 0) {
            if (number == 1) {
                for (param = 48; param <= 55; param++) {
                    if (joinAveTextHandler(person, param, NULL) != 0) {
                        found = TRUE;
                        break;
                    }
                }
            } else {
                found = TRUE;
            }
        }
        if (!found) {
            *var = 1743;
            break;
        }
        WordSetNumber(wordSet, 1, number, 3, 0, 1);
        *var = 1742;
        break;
    case 10:
        total = 0;
        for (i = 0; i < 8; i++) {
            total += joinAveTextHandler(person, i + 48, NULL);
        }
        any = FALSE;
        if (total != 0) {
            any = TRUE;
        }
        *var = any;
        break;
    case 11:
        count = 0;
        for (i = 0; i < 8; i++) {
            if (joinAveTextHandler(person, i + 48, NULL) != 0) {
                count++;
            }
        }
        *var = count;
        break;
    case 12:
        total = 0;
        for (i = 0; i < 8; i++) {
            total += joinAveTextHandler(person, i + 48, NULL);
        }
        WordSetNumber(wordSet, 2, total, 1, 0, 1);
        *var = 1745;
        break;
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
        mode -= 13;
        msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
        n = 0;
        for (i = 0; i < 8; i++) {
            u32 target = joinAveTextHandler(person, i + 48, NULL);
            int wanted = mode + 1;

            if (target != 0 && wanted == ++n) {
                strbuf = GFL_MsgDataLoadStrbufNew(msgData, i + 280);
                func_0202437c(wordSet, 1, strbuf, 2, 1, 2);
                WordSetNumber(wordSet, 2, target, 1, 0, 1);
                GFL_StrBufFree(strbuf);
                break;
            }
        }
        GFL_MsgDataFree(msgData);
        if (mode + 1 < n) {
            *var = 1747;
        } else {
            *var = 1746;
        }
        break;
    }
    return FALSE;
}

BOOL func_ov059_021e742c(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;
    void *shops;
    ResortPerson *person;
    const u16 *row;
    u16 index;
    u16 mode;
    u16 *var;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    shops = ResortSys_GetShopData(ResortSys_Get(field));
    person = ResortSys_GetScriptPerson(env, func_ov059_021e7b98(field));
    row = ResortShopData_GetPersonShop(shops, func_ov137_021f0f58(person));
    index = ScriptReadAny(vm, env);
    mode = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    if (mode == 0) {
        const u16 *item = ResortShopData_GetGoods(shops, row, index);

        if (func_ov137_021f10e8(person, 2, NULL) == 0) {
            *var = ResortShopData_GetGoodsParam(item, 8);
        } else {
            *var = ResortShopData_GetGoodsParam(item, 9);
        }
    }
    return FALSE;
}

// Sets a field of the player's own entry or the avenue's info
BOOL func_ov059_021e74cc(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    Field *field = GSYS_GetField(gsys);
    u32 param = ScriptReadAny(vm, env);
    u16 value = ScriptReadAny(vm, env);
    JoinAvenueSave *joinAvenue = SaveControl_GetJoinAvenue(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
    JoinAvenuePerson *own = func_02038a18(getAddressOfBeginningOfOccupants(joinAvenue));
    JoinAvenueInfo *info = JoinAvenue_GetInfo(joinAvenue);

    switch (param) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        JoinAvenuePerson_SetParam(own, (u16)param + 64, value);
        break;
    case 16:
        JoinAvenuePerson_SetParam(own, 19, sUnk7db4[VERSION_BLACK2 - GAME_VERSION][value]);
        break;
    case 17:
        func_02039064(info, 5, value);
        break;
    case 18:
        func_02039064(info, 7, value);
        if (value == 0) {
            func_ov137_021eef24(field);
        }
        break;
    case 21:
        func_02039064(info, value + 15, 0);
        func_02039064(info, value + 7, 1);
        break;
    case 22:
        func_02039064(info, 25, value);
        break;
    case 23:
        if (Field_CheckGimmickWorkPassword(field, 1)) {
            func_ov137_021eef4c(field);
        }
        break;
    case 24:
        if (Field_CheckGimmickWorkPassword(field, 1)) {
            func_ov137_021eef68(field, value);
        }
        break;
    case 26:
        ScriptWork_SetParentActor(work, FindFieldActor(GameData_GetMMSys(gameData), value));
        break;
    }
    return FALSE;
}

// Sets a variable to a field of the player's own entry or the avenue's info
BOOL func_ov059_021e7608(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    u32 param;
    u16 *var;
    JoinAvenueSave *joinAvenue;
    JoinAvenueOccupants *occupants;
    JoinAvenueInfo *info;
    JoinAvenuePerson *own;
    ResortSys *sys;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    param = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    joinAvenue = SaveControl_GetJoinAvenue(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
    occupants = getAddressOfBeginningOfOccupants(joinAvenue);
    info = JoinAvenue_GetInfo(joinAvenue);
    own = func_02038a18(occupants);
    sys = ResortSys_Get(field);
    switch (param) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        *var = joinAveTextHandler(own, (u16)param + 64, NULL);
        break;
    case 16:
        *var = func_02038a20(own, GetGameDataPlayerInfo(gameData));
        break;
    case 19:
        *var = ResortSys_GetInfoFlag1b(sys);
        break;
    case 20:
        *var = ResortSys_GetUnlock(sys, gameData);
        break;
    case 22:
        *var = JoinAvenue_GetParam(info, 25, 0);
        break;
    case 25:
        *var = JoinAvenue_GetParam(info, 13, 0);
        break;
    case 17:
        *var = JoinAvenue_GetParam(info, 5, 0);
        break;
    case 27:
        *var = ResortSys_IsFull(sys);
        break;
    case 28:
        *var = ResortSys_AreAllShopsMaxLevel(sys);
        break;
    }
    return FALSE;
}

BOOL func_ov059_021e7710(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    func_ov137_021eeee8(field, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL func_ov059_021e7748(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;
    u16 a1;
    u16 *var;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    a1 = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    *var = func_ov137_021eeefc(field, a1);
    return FALSE;
}

BOOL func_ov059_021e778c(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    func_ov137_021eef10(field, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL func_ov059_021e77c4(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    ResortSys *sys;
    HeapID heapId;
    PlayerInfo *playerInfo;
    u16 a2;
    u16 a3;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    gameData = FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    sys = ResortSys_Get(field);
    heapId = Field_GetHeapID(field);
    playerInfo = GetGameDataPlayerInfo(gameData);
    a2 = ScriptReadAny(vm, env);
    a3 = ScriptReadAny(vm, env);
    ResortSys_AddEntryOrRecord(sys, playerInfo, a2, a3, heapId);
    if (a3 <= 1) {
        func_ov137_021eef3c(field);
    }
    return FALSE;
}

static BOOL func_ov059_021e7834(VM *vm, void *data) {
    FieldScriptEnv *env = data;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    Field *field = GSYS_GetField(gsys);
    ResortSys *sys = ResortSys_Get(field);
    void **ptr = ScriptWork_GetUserHeapPtr(work);
    HeapID heapId = Field_GetHeapID(field);
    ResortRecordsWork *wk = *ptr;
    void *msgBGSys = Field_GetMsgBGSys(field);
    WordSet *wordSet = ScriptWork_GetWordSet(work);

    switch (wk->state) {
    case 0:
        wk->window = ResortRecords_CreateWindow(msgBGSys, heapId);
        wk->state++;
        break;
    case 1:
        ResortWork_UpdateRecords(func_02017b84(gameData), GameData_GetSaveControl(gameData));
        ResortRecords_PrintPage(wk->window, ResortSys_GetOccupants(sys), msgBGSys, wordSet, gameData, wk->page, heapId);
        wk->state++;
        break;
    case 2:
        if (func_ov036_02187c70(wk->window)) {
            GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(func_ov036_02187c9c(wk->window)));
            wk->state++;
        }
        break;
    case 3:
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            GFL_SndSEPlay(1361);
            wk->state = 4;
        } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_RIGHT) {
            GFL_SndSEPlay(1352);
            wk->page++;
            wk->page %= 9;
            wk->state = 1;
        } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_LEFT) {
            GFL_SndSEPlay(1352);
            wk->page--;
            wk->page = wk->page < 0 ? 8 : wk->page;
            wk->state = 1;
        }
        break;
    case 4:
        func_ov036_02187c7c(wk->window);
        GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(func_ov036_02187c9c(wk->window)));
        func_ov036_02187c1c(wk->window);
        wk->state++;
        break;
    case 5:
        *wk->var = wk->page;
        GFL_HeapFree(wk);
        *ptr = NULL;
        return TRUE;
    }
    return FALSE;
}

// Shows the avenue's records, from the page, and sets a variable to the page it closes on
BOOL func_ov059_021e79a4(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field;
    HeapID heapId;
    void **ptr;
    u16 page;
    u16 *var;
    ResortRecordsWork *wk;

    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    ResortSys_GetSlots(ResortSys_Get(field));
    heapId = Field_GetHeapID(field);
    ptr = ScriptWork_GetUserHeapPtr(work);
    page = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    wk = GFL_HeapAllocate(heapId, sizeof(ResortRecordsWork), TRUE, "scrcmd_resort.c", 2716);
    wk->page = page;
    wk->var = var;
    *ptr = wk;
    VM_SetNativeCallback(vm, func_ov059_021e7834);
    return TRUE;
}

BOOL func_ov059_021e7a28(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;
    ResortSys *sys;
    ResortPeople *people;
    void *unk30;
    u16 zoneId;
    u32 index;
    u16 *var;
    ResortPersonSource key;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    sys = ResortSys_Get(field);
    people = func_ov059_021e7b98(field);
    unk30 = ResortSys_GetDatas(sys);
    zoneId = Field_GetPlayerStateZoneID(field);
    index = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    sys_memset(&key, 0, sizeof(key));
    switch (index) {
    case 0:
    case 1:
    case 2:
    case 3:
        key.data = func_ov137_021f1b94(unk30, 1, index);
        key.zone = func_02039518(zoneId);
        *var = (u16)func_ov137_021f10e8(func_ov137_021f14a8(people, &key), 0, NULL) + 80;
        break;
    }
    return FALSE;
}

BOOL func_ov059_021e7ad8(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;
    void *unk30;
    ResortPeople *people;
    u32 index;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    unk30 = ResortSys_GetDatas(ResortSys_Get(field));
    people = func_ov059_021e7b98(field);
    index = ScriptReadAny(vm, env);
    switch (index) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        func_ov137_021f1554(
            people, func_ov137_021f1600(people, func_ov137_021f1980(func_ov137_021f1b94(unk30, 0, index))));
        break;
    case 8: {
        u32 i;

        for (i = 0; i < func_ov137_021f134c(people); i++) {
            ResortPerson *person = func_ov137_021f1634(people, i);

            if (person != NULL && func_ov137_021f10f4(person, 0) == 3) {
                func_ov137_021f1554(people, person);
            }
        }
        break;
    }
    }
    return FALSE;
}

static ResortPeople *func_ov059_021e7b98(Field *field) {
    ResortPeople *people = NULL;

    if (Field_CheckGimmickWorkPassword(field, 0)) {
        return func_ov137_021f0e74(field);
    }
    if (Field_CheckGimmickWorkPassword(field, 1)) {
        people = func_ov137_021eeeac(field);
    }
    return people;
}

// The UID of the script's actor
static u16 func_ov059_021e7bc8(FieldScriptEnv *env) {
    return GetActorUID(ScriptWork_GetParentActor(FieldScriptEnv_GetScriptWork(env)));
}

static void *func_ov059_021e7bd8(Field *field) {
    void *unk;

    if (Field_CheckGimmickWorkPassword(field, 1)) {
        return func_ov137_021eeebc(field);
    }
    unk = NULL;
    if (Field_CheckGimmickWorkPassword(field, 0)) {
        unk = func_ov137_021f0e8c(field);
    }
    return unk;
}

// Changes an effort value, keeping it to 252 and the total to 510, or returns -1 if it can't change
static int func_ov059_021e7c08(int value, int others, int change) {
    if (value == 0 && change < 0) {
        return -1;
    }
    if (value == 255 && change > 0) {
        return -1;
    }
    if (value + others >= 510 && change > 0) {
        return -1;
    }
    if (value >= 252 && change > 0) {
        return -1;
    }
    if (value + change >= 252) {
        change = 252 - value;
    }
    value += change;
    if (value < 0) {
        value = 0;
    }
    if (value + others > 510) {
        value = 510 - others;
    }
    return value;
}
