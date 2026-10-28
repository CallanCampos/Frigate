#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/script_text_banks.h"
#include "constants/version.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_msgbg.h"
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

// The page of records that Scrcmd_Resort_ShowRecords shows, while the script waits
typedef struct {
    MoneyWin *window;
    s16 page;
    u16 state;
    u16 *var;
} ResortRecordsWork;

// By version: Black 2's row, then White 2's
static const u16 sUnk7db4[2][8] = {
    { 161, 171, 181, 191, 201, 211, 221, 231 },
    { 241, 251, 261, 271, 281, 291, 301, 311 },
};

static ResortPeople *ResortCmd_GetPeople(Field *field);
static u16 ResortCmd_GetActorUID(FieldScriptEnv *env);
static ResortNPC *ResortCmd_GetNPC(Field *field);
static int ResortCmd_ApplyEffortChange(int value, int others, int change);

BOOL Scrcmd_Resort_StartTalk(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ResortPerson *person = ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(GSYS_GetField(gsys)));

    ResortPerson_SetParam(person, JOIN_AVE_PARAM_UNK_27, 1);
    ResortPerson_SetTalking(person, 1);
    if (checkForMidnight(GSYS_GetGameData(gsys)) && ResortPerson_GetAttr(person, 0) == 0) {
        JoinAvenueShop_SetField(ResortPerson_GetShop(person), 3, -1);
    }
    return FALSE;
}

BOOL Scrcmd_Resort_EndTalk(VM *vm, FieldScriptEnv *env) {
    ResortPerson *person =
        ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))));

    if (person != NULL) {
        ResortPerson_SetParam(person, JOIN_AVE_PARAM_UNK_27, 0);
        ResortPerson_SetTalking(person, 0);
    }
    return FALSE;
}

// Sets a variable to one of 65 things about the avenue and the person the script is about
BOOL Scrcmd_Resort_GetValue(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    u32 which = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);
    ResortSys *sys = ResortSys_Get(field);
    JoinAvenueInfo *info = ResortSys_GetInfo(sys);
    JoinAvenueOccupants *occupants = ResortSys_GetOccupants(sys);
    ResortPeople *people = ResortCmd_GetPeople(field);
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
        data = ResortPerson_GetData(person);
    }
    switch (which) {
    case 0:
        *var = (u16)ResortPerson_GetParam(person, JOIN_AVE_PARAM_SLOT, NULL) + 80;
        break;
    case 1: {
        JoinAvenueRecord *a = JoinAvenueOccupants_GetEmptyRecord(occupants);
        u32 b = JoinAvenue_GetParam(info, JOIN_AVE_INFO_UNLOCKED_5, 0);

        *var = a == NULL && b != 0 ? TRUE : FALSE;
        break;
    }
    case 2: {
        u16 a = ResortPerson_GetParam(person, JOIN_AVE_PARAM_SHOP_ID, NULL);
        u16 b = ResortPerson_GetParam(person, JOIN_AVE_PARAM_TRAINER_ID, NULL);
        u16 c = ResortPerson_GetParam(person, JOIN_AVE_PARAM_VERSION, NULL);
        u16 d = ResortPerson_GetParam(person, JOIN_AVE_PARAM_UNK_18, NULL);
        u16 id1 = JoinAvenue_ChooseShop(a, b, c, d, info, shops);
        u16 id2 = JoinAvenueShop_GetField(ResortPerson_GetShop(person), 0);
        const u16 *row1 = ResortShopData_GetShop(shops, id1);
        const u16 *row2 = ResortShopData_GetShop(shops, id2);
        u16 kind1 = ResortShopData_GetShopParam(row1, 0);
        u16 kind2 = ResortShopData_GetShopParam(row2, 0);
        u32 unk25 = ResortPerson_GetParam(person, JOIN_AVE_PARAM_UNK_37, NULL);
        u32 unk13 = JoinAvenue_GetParam(info, JOIN_AVE_INFO_UNLOCKED_6, 0);

        if (kind1 == 5 && unk13 == 0) {
            unk25 = 1;
        }
        *var = kind1 != kind2 && unk25 == 0 ? TRUE : FALSE;
        break;
    }
    case 3:
        if (ResortPerson_GetParam(person, JOIN_AVE_PARAM_UNK_20, NULL) == 0) {
            *var = ResortData_RaffleShopItem(data, ResortSys_GetUnk24Table(sys), ResortSys_GetRanks(sys), info);
            ResortPerson_SetParam(person, JOIN_AVE_PARAM_UNK_20, 1);
        } else {
            *var = 0;
        }
        break;
    case 6:
        *var = JoinAvenueOccupants_CountPeople(occupants) == 0 ? TRUE : FALSE;
        break;
    case 7:
        *var = ResortPerson_GetParam(person, JOIN_AVE_PARAM_UNK_32, NULL);
        break;
    case 8:
        *var = ResortPerson_GetParam(person, JOIN_AVE_PARAM_SHOP_KIND, NULL);
        break;
    case 9:
        *var = JoinAvenueShop_GetFlag(JoinAvenuePerson_GetShop(ResortData_GetPerson(data)), 0);
        break;
    case 10:
        *var = JoinAvenueOccupants_CountPeople(occupants) == 8 ? TRUE : FALSE;
        break;
    case 11: {
        u16 id = JoinAvenueShop_GetField(ResortPerson_GetShop(person), 0);

        *var = ResortShopData_GetShopParam(ResortShopData_GetShop(shops, id), 0);
        break;
    }
    case 15: {
        BOOL flag = JoinAvenuePerson_HasExtraData(ResortPerson_GetJoinAvenuePerson(person), 4);
        JoinAvenuePersonParam param;

        *var = 0;
        if (flag) {
            for (param = 80; param <= 83; param++) {
                if (ResortPerson_GetParam(person, param, NULL) != 0) {
                    (*var)++;
                }
            }
        }
        break;
    }
    case 13: {
        BOOL flag = JoinAvenuePerson_HasExtraData(ResortPerson_GetJoinAvenuePerson(person), 0);
        u32 unk29 = ResortPerson_GetParam(person, JOIN_AVE_PARAM_LAST_MEDAL, NULL);

        *var = flag && unk29 != 255 ? TRUE : FALSE;
        break;
    }
    case 16: {
        u32 a = ResortPerson_GetParam(person, JOIN_AVE_PARAM_RANK, NULL);
        u32 b = JoinAvenue_GetParam(info, JOIN_AVE_INFO_RANK, 0);

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
        *var = JoinAvenuePerson_IsEmpty(JoinAvenueOccupants_GetPerson(occupants, which)) == FALSE ? TRUE : FALSE;
        break;
    case 33:
        *var = JoinAvenue_GetParam(info, JOIN_AVE_INFO_UNLOCKED_4, 0);
        break;
    case 34:
        *var = JoinAvenue_GetParam(info, JOIN_AVE_INFO_UNLOCKED_3, 0);
        break;
    case 35: {
        u32 value = 0;

        for (i = 0; i < 4; i++) {
            void *record = JoinAvenueOccupants_GetRecord(occupants, i);

            if (!JoinAvenueRecord_IsEmpty(record) &&
                JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_SHOP_KIND, NULL) == 0) {
                value = JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_GENDER, NULL);
                break;
            }
        }
        *var = value;
        break;
    }
    case 36: {
        u32 value = 0;

        for (i = 0; i < 4; i++) {
            void *record = JoinAvenueOccupants_GetRecord(occupants, i);

            if (!JoinAvenueRecord_IsEmpty(record) &&
                JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_SHOP_KIND, NULL) == 2) {
                value = JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_GENDER, NULL);
                break;
            }
        }
        *var = value;
        break;
    }
    case 37:
        for (i = 0; i < 4; i++) {
            void *record = JoinAvenueOccupants_GetRecord(occupants, i);

            if (!JoinAvenueRecord_IsEmpty(record) &&
                JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_SHOP_KIND, NULL) == 0) {
                *var = i;
                break;
            }
        }
        break;
    case 38:
        for (i = 0; i < 4; i++) {
            void *record = JoinAvenueOccupants_GetRecord(occupants, i);

            if (!JoinAvenueRecord_IsEmpty(record) &&
                JoinAvenueRecord_GetParam(record, JOIN_AVE_PARAM_SHOP_KIND, NULL) == 2) {
                *var = i;
                break;
            }
        }
        break;
    case 39:
        *var = ResortSys_GetTrainerTypeText(unk28, data, 10);
        break;
    case 40:
        *var = ResortData_GetIndex(data);
        break;
    case 41:
        *var = ResortPerson_GetAttr(person, 0);
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
        *var = (u16)joinAveTextHandler(JoinAvenueOccupants_GetPerson(occupants, which), JOIN_AVE_PARAM_SLOT, NULL) + 80;
        break;
    case 52:
    case 53:
    case 54:
    case 55:
    case 56:
    case 57:
    case 58:
    case 59: {
        u16 id;

        which -= 52;
        id = JoinAvenueShop_GetField(JoinAvenuePerson_GetShop(JoinAvenueOccupants_GetPerson(occupants, which)), 0);
        *var = ResortShopData_GetShopParam(ResortShopData_GetShop(shops, id), 5);
        break;
    }
    case 60: {
        u16 kind = ResortData_GetType(data);
        u16 unk1f = ResortData_GetParam(data, JOIN_AVE_PARAM_SHOP_KIND, NULL);

        *var = kind == 1 && unk1f == 5 ? TRUE : FALSE;
        break;
    }
    case 61:
        *var = ResortBinary_Get(table, ResortBinary_FindRange(table, JoinAvenue_GetParam(info, JOIN_AVE_INFO_RANK, 0)),
                                11);
        break;
    case 62:
        *var = JoinAvenue_GetParam(info, JOIN_AVE_INFO_UNLOCKED_7, 0);
        break;
    case 63:
        *var = ResortBinary_Get(table, ResortBinary_FindRange(table, JoinAvenue_GetParam(info, JOIN_AVE_INFO_RANK, 0)),
                                6) -
               1753;
        break;
    case 64:
        *var = JoinAvenueOccupants_CountPeople(occupants);
        break;
    }
    return FALSE;
}

BOOL Scrcmd_Resort_GetScriptValue(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    u16 a0 = ScriptReadAny(vm, env);
    u16 *var = ScriptReadVar(vm, env);
    ResortPeople *people = ResortCmd_GetPeople(field);
    HeapID heapId = Field_GetHeapID(field);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    ResortSys *sys = ResortSys_Get(field);
    ResortPersonData *data = NULL;
    GameData *gameData = GSYS_GetGameData(gsys);
    ResortPerson *person = ResortSys_GetScriptPerson(env, people);

    if (person != NULL) {
        data = ResortPerson_GetData(person);
    }
    *var = ResortSys_GetScriptValue(a0, sys, data, gameData, wordSet, heapId);
    return FALSE;
}

BOOL Scrcmd_Resort_MakeData(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    u16 mode = VM_Read16(vm);
    u16 *var = ScriptReadVar(vm, env);
    ResortSys *sys = ResortSys_Get(field);
    ResortPerson *person;
    ResortPersonData *data;

    ResortSys_GetSlots(sys);
    person = ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(field));
    data = ResortPerson_GetData(person);
    switch (mode) {
    case 2: {
        VecFx32 pos;
        u16 x;
        u16 z;
        u16 dir;
        FieldPlayer *player;

        *var = ResortSys_MakeData(sys, data, 0);
        player = Field_GetPlayer(field);
        ResortPerson_MoveToHome(person);
        ResortPerson_GetFacingPos(person, &x, &z, &dir);
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
    ResortPerson_SetParam(person, JOIN_AVE_PARAM_UNK_37, 1);
    return FALSE;
}

BOOL Scrcmd_Resort_SetWord(VM *vm, FieldScriptEnv *env) {
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
    ResortPerson *person = ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(field));

    ResortSys_SetScriptWord(a0, a1, a2, a3, sys, person, wordSet, GSYS_GetGameData(gsys), heapId);
    return FALSE;
}

BOOL Scrcmd_Resort_RemovePerson(VM *vm, FieldScriptEnv *env) {
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
    people = ResortCmd_GetPeople(field);
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
        data = ResortPerson_GetData(person);
        break;
    case 0:
    case 1:
    case 2:
    case 3:
        data = ResortDatas_GetByScript(unk30, which, index);
        person = ResortPeople_FindByData(people, data);
        break;
    }
    if (person != NULL) {
        if (Field_CheckGimmickWorkPassword(field, 1)) {
            ResortField_RemoveWalker(field, person);
        }
        ResortPeople_Delete(people, person);
    }
    if (data != NULL) {
        if (mode == 0) {
            ResortData_Clear(data);
        } else {
            ResortData_SetParam(data, JOIN_AVE_PARAM_SLOT, 0);
            ResortData_SetParam(data, JOIN_AVE_PARAM_ROW, 0);
        }
    }
    ResortSlots_Fill(unk34);
    return FALSE;
}

BOOL Scrcmd_Resort_SaveShopId(VM *vm, FieldScriptEnv *env) {
    ResortPerson *person;
    u16 value;

    FieldScriptEnv_GetScriptWork(env);
    person = ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))));
    value = JoinAvenueShop_GetField(ResortPerson_GetShop(person), 0);
    ResortPerson_SetParam(person, JOIN_AVE_PARAM_SHOP_ID, value);
    return FALSE;
}

BOOL Scrcmd_Resort_RaffleShopItem(VM *vm, FieldScriptEnv *env) {
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
    person = ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(field));
    data = ResortPerson_GetData(person);
    *var2 = 0;
    row = ResortShopData_GetGoods(shops, ResortShopData_GetPersonShop(shops, ResortPerson_GetJoinAvenuePerson(person)),
                                  0);
    id = ResortShopData_GetGoodsParam(row, 6);
    ResortShopData_GetGoodsParam(row, 5);
    prize = join_ave_raffle_shop(table, id, ResortData_Roll(data, index + 5, ResortBinary_Get(table, id, 0)));
    *var2 = prize + 1;
    if (prize != 10) {
        *var1 = ResortBinary_Get(table, id, prize * 2 + 3);
    }
    return FALSE;
}

// Sets a Pokémon's friendship
BOOL Scrcmd_Resort_SetFriendship(VM *vm, FieldScriptEnv *env) {
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
BOOL Scrcmd_Resort_RaiseLevel(VM *vm, FieldScriptEnv *env) {
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
        JoinAvenue_RecordActivity(9);
        PokeParty_SetParam(pkm, PKM_PARAM_EXP, exp);
        PokeParty_RecalcStats(pkm);
        *var = TRUE;
    } else {
        *var = FALSE;
    }
    return FALSE;
}

// Raises or, for stats from 6, lowers one of a Pokémon's effort values
BOOL Scrcmd_Resort_ChangeEffortValue(VM *vm, FieldScriptEnv *env) {
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
    value = ResortCmd_ApplyEffortChange(current, total - current, change);
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
BOOL Scrcmd_Resort_ShortenEggSteps(VM *vm, FieldScriptEnv *env) {
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
    PersonalData *personal;
    u32 cycles;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    field = GSYS_GetField(gsys);
    sys = ResortSys_Get(field);
    index = ScriptReadAny(vm, env);
    slot = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    person = ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(field));
    data = ResortPerson_GetData(person);
    rate = 0;
    shops = ResortSys_GetShopData(sys);
    table = ResortSys_GetUnk24Table(sys);
    row = ResortShopData_GetGoods(shops, ResortShopData_GetPersonShop(shops, ResortPerson_GetJoinAvenuePerson(person)),
                                  index);
    id = ResortShopData_GetGoodsParam(row, 6);
    ResortShopData_GetGoodsParam(row, 5);
    prize = join_ave_raffle_shop(table, id, ResortData_Roll(data, index + 5, ResortBinary_Get(table, id, 0)));
    if (prize != 10) {
        rate = ResortBinary_Get(table, id, prize * 2 + 3);
    }
    pkm = PokeParty_GetPkm(GameData_GetParty(GSYS_GetGameData(gsys)), slot);
    species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    personal = PML_PersonalLoad(species, form, Field_GetHeapID(field));
    cycles = PML_PersonalGetParam(personal, PERSONAL_HATCH_CYCLES);
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

BOOL Scrcmd_Resort_SetShopFlag(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    JoinAvenueShop_SetFlag(JoinAvenuePerson_GetShop(ResortPerson_GetJoinAvenuePerson(ResortSys_GetScriptPerson(
                               env, ResortCmd_GetPeople(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)))))),
                           0, TRUE);
    return FALSE;
}

// Takes ten times the amount from the player's money, and sets a flag of the person
BOOL Scrcmd_Resort_PayAndSetShopFlag(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    Field *field = GSYS_GetField(gsys);
    ResortPeople *people = ResortCmd_GetPeople(field);
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
    JoinAvenueShop_SetFlag(JoinAvenuePerson_GetShop(ResortPerson_GetJoinAvenuePerson(person)), bit, TRUE);
    return FALSE;
}

BOOL Scrcmd_Resort_GetPersonVisit(VM *vm, FieldScriptEnv *env) {
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
    person = ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(field));
    sys = ResortSys_Get(field);
    occupants = ResortSys_GetOccupants(sys);
    Field_GetHeapID(field);
    index = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    var2 = ScriptReadVar(vm, env);
    var3 = ScriptReadVar(vm, env);
    var4 = ScriptReadVar(vm, env);
    *var = ResortSys_GetPersonVisit(ResortPerson_GetData(person), JoinAvenueOccupants_GetPerson(occupants, index), sys,
                                    var2, var3, var4);
    return FALSE;
}

BOOL Scrcmd_Resort_AddShopExperience(VM *vm, FieldScriptEnv *env) {
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
    ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(field));
    sys = ResortSys_Get(field);
    occupants = ResortSys_GetOccupants(sys);
    index = ScriptReadAny(vm, env);
    a2 = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    var2 = ScriptReadVar(vm, env);
    *var = ResortSys_AddShopExperience(JoinAvenueOccupants_GetPerson(occupants, index), sys, a2, var2);
    return FALSE;
}

BOOL Scrcmd_Resort_AddPopularity(VM *vm, FieldScriptEnv *env) {
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
    ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(field));
    sys = ResortSys_Get(field);
    ResortSys_GetOccupants(sys);
    a2 = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    *var = ResortSys_AddPopularity(sys, gameData, a2, ScriptReadVar(vm, env));
    return FALSE;
}

BOOL Scrcmd_Resort_SetOccupantsLocked(VM *vm, FieldScriptEnv *env) {
    JoinAvenueOccupants *occupants;

    FieldScriptEnv_GetScriptWork(env);
    occupants = ResortSys_GetOccupants(ResortSys_Get(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))));
    JoinAvenueOccupants_SetLocked(occupants, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL Scrcmd_Resort_HideActors(VM *vm, FieldScriptEnv *env) {
    Field *field;
    ResortPeople *people;
    void *unk;
    ResortPerson *person;

    FieldScriptEnv_GetScriptWork(env);
    field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    people = ResortCmd_GetPeople(field);
    unk = ResortCmd_GetNPC(field);
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
        ResortPeople_ApplyVisibility(people);
        ResortNPC_SetVisible(unk, 1);
        break;
    }
    return FALSE;
}

BOOL Scrcmd_Resort_Visit(VM *vm, FieldScriptEnv *env) {
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
    data = ResortPerson_GetData(ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(field)));
    unk38 = ResortSys_GetWork(sys);
    unk28 = ResortSys_GetTexts(sys);
    unk30 = ResortSys_GetDatas(sys);
    a2 = ScriptReadAny(vm, env);
    mode = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    var2 = ScriptReadVar(vm, env);
    kind = ResortData_GetType(data);
    unk1f = ResortData_GetParam(data, JOIN_AVE_PARAM_SHOP_KIND, NULL);
    flag = FALSE;
    if (kind == 1 && unk1f == 5) {
        flag = TRUE;
    }
    switch (mode) {
    case 0: {
        ResortPersonData *other = ResortDatas_Get(unk30, 0, a2);
        u16 unk = ResortWork_Get(ResortSys_GetWork(sys), RESORT_WORK_VISITS);
        u32 score;
        u32 value;
        u32 limit;

        ResortSys_GetOccupants(sys);
        score = ResortData_RollPair(other, data, 27, 100);
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
        *var = (a2 * (((u16)ResortWork_Get(unk38, RESORT_WORK_VISITS) + 1) * (FX32_ONE / 4) + FX32_ONE)) >> FX32_SHIFT;
        break;
    case 2:
        *var = ResortData_Roll(data, 26, 100) < 85 ? TRUE : FALSE;
        break;
    case 3: {
        u16 unk2 = ResortData_GetParam(data, JOIN_AVE_PARAM_GENDER, NULL);

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
        ResortSys_ChooseVisitGroup(sys, ResortDatas_Get(unk30, 0, a2), data);
        break;
    case 5:
        ResortSys_MarkMet(sys, ResortDatas_Get(unk30, 0, a2), a2);
        break;
    }
    return FALSE;
}

BOOL Scrcmd_Resort_CheckRank100Unlock(VM *vm, FieldScriptEnv *env) {
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
    if (JoinAvenue_GetParam(info, JOIN_AVE_INFO_RANK, 0) >= 100) {
        u32 result = ResortSys_GetUnlockForRank(sys, gameData, 100);

        if (result != 0 && result != 8) {
            JoinAvenueInfo_SetParam(info, result + JOIN_AVE_INFO_UNLOCK_PENDING_1 - 1, 1);
            *var = TRUE;
        }
    }
    return FALSE;
}

BOOL Scrcmd_Resort_NameEntry(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 a0 = VM_Read16(vm);
    u16 a3 = VM_Read16(vm);

    ScriptWork_CallEvent(work, JoinAvenueNameEntry_Create(a0, gsys, 0, a3));
    return TRUE;
}

BOOL Scrcmd_Resort_MarkRaffled(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    ResortPerson_SetParam(ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(GSYS_GetField(gsys))),
                          JOIN_AVE_PARAM_UNK_20, 1);
    return FALSE;
}

BOOL Scrcmd_Resort_GetNPCValue(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    void *unk;
    u16 id;
    u16 mode;
    u16 *var;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    unk = ResortCmd_GetNPC(GSYS_GetField(gsys));
    id = ScriptReadAny(vm, env);
    mode = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    switch (mode) {
    case 0:
        *var = ResortCmd_GetActorUID(env);
        break;
    case 1:
        *var = ResortCmd_GetActorUID(env) - 148;
        break;
    case 2:
        *var = ResortNPC_GetTextEntry(unk, id - 148, 5);
        break;
    case 3:
        *var = ResortNPC_GetTextEntry(unk, id - 148, 6);
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
        *var = ResortNPC_GetTextEntry(unk, id - 148, mode + 7);
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
        *var = ResortNPC_GetTextEntry(unk, id - 148, mode + 7) + 59;
        break;
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
        mode -= 28;
        *var = ResortNPC_GetTextEntry(unk, id - 148, mode);
        break;
    case 33:
        *var = ResortNPC_GetTextRow(unk, id - 148);
        break;
    }
    return FALSE;
}

BOOL Scrcmd_Resort_SetOccupantWords(VM *vm, FieldScriptEnv *env) {
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
    occupants =
        getAddressOfBeginningOfOccupants(SaveControl_GetJoinAvenue(GameData_GetSaveControl(GSYS_GetGameData(gsys))));
    count = 0;
    heapId = Field_GetHeapID(field);
    *var = 0;
    for (i = 7; i >= 0; i--) {
        person = JoinAvenueOccupants_GetPerson(occupants, i);
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
        person = JoinAvenueOccupants_GetPerson(occupants, i);
        if (!JoinAvenuePerson_IsEmpty(person)) {
            u32 value = joinAveTextHandler(person, param + 64, NULL);

            if (value != 0) {
                if (n == 0) {
                    WordSet *wordSet = ScriptWork_GetWordSet(work);
                    StrBuf *strbuf = GFL_StrBufCreate(8, HEAPID_TAIL(heapId));
                    u32 unk2 = joinAveTextHandler(person, JOIN_AVE_PARAM_GENDER, NULL);
                    u16 text[64];

                    joinAveTextHandler(person, JOIN_AVE_PARAM_NAME, text);
                    GFL_StrBufLoadString(strbuf, text);
                    GFL_WordSetSetStrbuf(wordSet, 0, strbuf, unk2, 1, 2);
                    GFL_StrBufFree(strbuf);
                    strbuf = GFL_StrBufCreate(9, HEAPID_TAIL(heapId));
                    joinAveTextHandler(person, JOIN_AVE_PARAM_MESSAGE1, text);
                    GFL_StrBufLoadString(strbuf, text);
                    GFL_WordSetSetStrbuf(wordSet, 1, strbuf, 2, 1, 2);
                    GFL_StrBufFree(strbuf);
                    strbuf = GFL_StrBufCreate(9, HEAPID_TAIL(heapId));
                    joinAveTextHandler(person, JOIN_AVE_PARAM_MESSAGE2, text);
                    GFL_StrBufLoadString(strbuf, text);
                    GFL_WordSetSetStrbuf(wordSet, 2, strbuf, 2, 1, 2);
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

BOOL Scrcmd_Resort_SetNPCWord(VM *vm, FieldScriptEnv *env) {
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
    unk = ResortCmd_GetNPC(field);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685, HEAPID_TAIL(heapId));
    strbuf = GFL_MsgDataLoadStrbufNew(msgData, ResortNPC_GetTextEntry(unk, id, column + 7));
    GFL_WordSetSetStrbuf(wordSet, index, strbuf, 2, 1, 2);
    GFL_StrBufFree(strbuf);
    GFL_MsgDataFree(msgData);
    return FALSE;
}

// Puts things about one of the avenue's people in the script's WordSet, and sets a variable to the message that
// tells them
BOOL Scrcmd_Resort_DescribeOccupant(VM *vm, FieldScriptEnv *env) {
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
    ResortCmd_GetNPC(field);
    occupants =
        getAddressOfBeginningOfOccupants(SaveControl_GetJoinAvenue(GameData_GetSaveControl(GSYS_GetGameData(gsys))));
    wordSet = ScriptWork_GetWordSet(work);
    heapId = Field_GetHeapID(field);
    index = ScriptReadAny(vm, env);
    mode = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    person = JoinAvenueOccupants_GetPerson(occupants, index);
    switch (mode) {
    case 0:
        date = joinAveTextHandler(person, JOIN_AVE_PARAM_START_DATE, NULL);
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
        date = joinAveTextHandler(person, JOIN_AVE_PARAM_HALL_OF_FAME_DATE, NULL);
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
        a = joinAveTextHandler(person, JOIN_AVE_PARAM_PLAY_HOURS, NULL);
        b = joinAveTextHandler(person, JOIN_AVE_PARAM_PLAY_MINUTES, NULL);
        if (a == 0 && b == 0) {
            *var = 1729;
            break;
        }
        WordSetNumber(wordSet, 1, a, 3, 0, 1);
        WordSetNumber(wordSet, 2, b, 3, 0, 1);
        *var = 1728;
        break;
    case 3:
        value = joinAveTextHandler(person, JOIN_AVE_PARAM_DEX_CAUGHT, NULL);
        if (value == 0) {
            *var = 1731;
            break;
        }
        WordSetNumber(wordSet, 1, value, 3, 0, 1);
        *var = 1730;
        break;
    case 5:
        value = joinAveTextHandler(person, JOIN_AVE_PARAM_SPECIES, NULL);
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
        number = joinAveTextHandler(person, JOIN_AVE_PARAM_RANK, NULL);
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
                GFL_WordSetSetStrbuf(wordSet, 1, strbuf, 2, 1, 2);
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

BOOL Scrcmd_Resort_GetGoodsValue(VM *vm, FieldScriptEnv *env) {
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
    person = ResortSys_GetScriptPerson(env, ResortCmd_GetPeople(field));
    row = ResortShopData_GetPersonShop(shops, ResortPerson_GetJoinAvenuePerson(person));
    index = ScriptReadAny(vm, env);
    mode = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    if (mode == 0) {
        const u16 *item = ResortShopData_GetGoods(shops, row, index);

        if (ResortPerson_GetParam(person, JOIN_AVE_PARAM_GENDER, NULL) == 0) {
            *var = ResortShopData_GetGoodsParam(item, 8);
        } else {
            *var = ResortShopData_GetGoodsParam(item, 9);
        }
    }
    return FALSE;
}

// Sets a field of the player's own entry or the avenue's info
BOOL Scrcmd_Resort_SetOwnValue(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    Field *field = GSYS_GetField(gsys);
    u32 param = ScriptReadAny(vm, env);
    u16 value = ScriptReadAny(vm, env);
    JoinAvenueSave *joinAvenue = SaveControl_GetJoinAvenue(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
    JoinAvenuePerson *own = JoinAvenueOccupants_GetPlayerPerson(getAddressOfBeginningOfOccupants(joinAvenue));
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
        JoinAvenuePerson_SetParam(own, JOIN_AVE_PARAM_SHOP_ID, sUnk7db4[VERSION_BLACK2 - GAME_VERSION][value]);
        break;
    case 17:
        JoinAvenueInfo_SetParam(info, JOIN_AVE_INFO_SEASON, value);
        break;
    case 18:
        JoinAvenueInfo_SetParam(info, JOIN_AVE_INFO_UNOPENED, value);
        if (value == 0) {
            ResortField_RefreshPeople(field);
        }
        break;
    case 21:
        JoinAvenueInfo_SetParam(info, value + JOIN_AVE_INFO_UNLOCK_PENDING_1 - 1, 0);
        JoinAvenueInfo_SetParam(info, value + JOIN_AVE_INFO_UNLOCKED_1 - 1, 1);
        break;
    case 22:
        JoinAvenueInfo_SetParam(info, JOIN_AVE_INFO_UNK_DC_13, value);
        break;
    case 23:
        if (Field_CheckGimmickWorkPassword(field, 1)) {
            ResortField_EnterEventMode(field);
        }
        break;
    case 24:
        if (Field_CheckGimmickWorkPassword(field, 1)) {
            ResortField_SetBubblesActive(field, value);
        }
        break;
    case 26:
        ScriptWork_SetParentActor(work, FindFieldActor(GameData_GetMMSys(gameData), value));
        break;
    }
    return FALSE;
}

// Sets a variable to a field of the player's own entry or the avenue's info
BOOL Scrcmd_Resort_GetOwnValue(VM *vm, FieldScriptEnv *env) {
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
    own = JoinAvenueOccupants_GetPlayerPerson(occupants);
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
        *var = JoinAvenuePerson_GetShopId(own, GetGameDataPlayerInfo(gameData));
        break;
    case 19:
        *var = ResortSys_GetInfoFlag1b(sys);
        break;
    case 20:
        *var = ResortSys_GetUnlock(sys, gameData);
        break;
    case 22:
        *var = JoinAvenue_GetParam(info, JOIN_AVE_INFO_UNK_DC_13, 0);
        break;
    case 25:
        *var = JoinAvenue_GetParam(info, JOIN_AVE_INFO_UNLOCKED_6, 0);
        break;
    case 17:
        *var = JoinAvenue_GetParam(info, JOIN_AVE_INFO_SEASON, 0);
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

BOOL Scrcmd_Resort_StartSceneAnim(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    ResortField_StartSceneAnim(field, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL Scrcmd_Resort_IsSceneAnimDone(VM *vm, FieldScriptEnv *env) {
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
    *var = ResortField_IsSceneAnimDone(field, a1);
    return FALSE;
}

BOOL Scrcmd_Resort_EndSceneAnim(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    Field *field;

    FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(gsys);
    ResortField_EndSceneAnim(field, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL Scrcmd_Resort_AddEntryOrRecord(VM *vm, FieldScriptEnv *env) {
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
        ResortField_MarkPeopleChanged(field);
    }
    return FALSE;
}

static BOOL ResortCmd_UpdateRecords(VM *vm, void *data) {
    FieldScriptEnv *env = data;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    Field *field = GSYS_GetField(gsys);
    ResortSys *sys = ResortSys_Get(field);
    void **ptr = ScriptWork_GetUserHeapPtr(work);
    HeapID heapId = Field_GetHeapID(field);
    ResortRecordsWork *wk = *ptr;
    FieldMsgBG *msgBGSys = Field_GetMsgBGSys(field);
    WordSet *wordSet = ScriptWork_GetWordSet(work);

    switch (wk->state) {
    case 0:
        wk->window = ResortRecords_CreateWindow(msgBGSys, heapId);
        wk->state++;
        break;
    case 1:
        ResortWork_UpdateRecords(GameData_GetResortWork(gameData), GameData_GetSaveControl(gameData));
        ResortRecords_PrintPage(wk->window, ResortSys_GetOccupants(sys), msgBGSys, wordSet, gameData, wk->page, heapId);
        wk->state++;
        break;
    case 2:
        if (MsgBGPrinter_IsMsgWinTransDone(wk->window)) {
            GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(MsgBGPrinter_GetMsgWinBmpWin(wk->window)));
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
        MsgBGPrinter_ClearMsgWin(wk->window);
        GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(MsgBGPrinter_GetMsgWinBmpWin(wk->window)));
        MsgBGPrinter_DeleteMsgWin(wk->window);
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
BOOL Scrcmd_Resort_ShowRecords(VM *vm, FieldScriptEnv *env) {
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
    VM_SetNativeCallback(vm, ResortCmd_UpdateRecords);
    return TRUE;
}

BOOL Scrcmd_Resort_AddRecordPerson(VM *vm, FieldScriptEnv *env) {
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
    people = ResortCmd_GetPeople(field);
    unk30 = ResortSys_GetDatas(sys);
    zoneId = Field_GetPlayerStateZoneID(field);
    index = ScriptReadAny(vm, env);
    var = ScriptReadVar(vm, env);
    MI_CpuFill8(&key, 0, sizeof(key));
    switch (index) {
    case 0:
    case 1:
    case 2:
    case 3:
        key.data = ResortDatas_Get(unk30, 1, index);
        key.zone = ResortLayout_GetZoneIndex(zoneId);
        *var = (u16)ResortPerson_GetParam(ResortPeople_AddAtEmptySlot(people, &key), JOIN_AVE_PARAM_SLOT, NULL) + 80;
        break;
    }
    return FALSE;
}

BOOL Scrcmd_Resort_RecreatePeople(VM *vm, FieldScriptEnv *env) {
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
    people = ResortCmd_GetPeople(field);
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
        ResortPeople_Recreate(people, ResortPeople_FindByJoinAvenuePerson(
                                          people, ResortData_GetPerson(ResortDatas_Get(unk30, 0, index))));
        break;
    case 8: {
        u32 i;

        for (i = 0; i < ResortPeople_GetCount(people); i++) {
            ResortPerson *person = ResortPeople_GetByIndex(people, i);

            if (person != NULL && ResortPerson_GetAttr(person, 0) == 3) {
                ResortPeople_Recreate(people, person);
            }
        }
        break;
    }
    }
    return FALSE;
}

static ResortPeople *ResortCmd_GetPeople(Field *field) {
    ResortPeople *people = NULL;

    if (Field_CheckGimmickWorkPassword(field, 0)) {
        return ResortField0_GetPeople(field);
    }
    if (Field_CheckGimmickWorkPassword(field, 1)) {
        people = ResortField_GetPeople(field);
    }
    return people;
}

// The UID of the script's actor
static u16 ResortCmd_GetActorUID(FieldScriptEnv *env) {
    return GetActorUID(ScriptWork_GetParentActor(FieldScriptEnv_GetScriptWork(env)));
}

static ResortNPC *ResortCmd_GetNPC(Field *field) {
    void *unk;

    if (Field_CheckGimmickWorkPassword(field, 1)) {
        return ResortField_GetNPC(field);
    }
    unk = NULL;
    if (Field_CheckGimmickWorkPassword(field, 0)) {
        unk = ResortField0_GetNPC(field);
    }
    return unk;
}

// Changes an effort value, keeping it to 252 and the total to 510, or returns -1 if it can't change
static int ResortCmd_ApplyEffortChange(int value, int others, int change) {
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
