// The Union Room's field side (the file's name is from its allocations' strings): it boots the communication and runs
// each frame from the field. The net commands are union_receive.c's and the commands that run as field events are
// union_subproc.c's: the ROM lays out each of the three with an assert file name ("") of its own, which MWCC shares
// across one file
#include "field/union_main.h"
#include "types.h"
#include "app/p_status.h"
#include "app/pokelist.h"
#include "app/pokemon_trade.h"
#include "constants/zones.h"
#include "demo/shinka_demo.h"
#include "field/colosseum.h"
#include "field/event_colosseum_battle.h"
#include "field/event_mapchange.h"
#include "field/field.h"
#include "field/field_controller.h"
#include "field/field_event.h"
#include "field/field_sound.h"
#include "field/union_app.h"
#include "field/union_comm.h"
#include "field/union_msg.h"
#include "field/union_oneself.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_sync.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "pml/poke_party.h"
#include "save/join_avenue.h"
#include "save/player_info.h"
#include "save/pokedex.h"
#include "save/save_control.h"
#include "save/wifi_list.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

void UnionMain_Boot(GameSystem *gsys) {
    GameCommSys *game_comm = GSYS_GetGameCommSystem(gsys);
    GameData *gameData = GSYS_GetGameData(gsys);
    UnionBootParam *param;

    GFL_ASSERT(GameCommSys_BootCheck(game_comm) == GAME_COMM_NO_NULL);
    param = GFL_HeapAllocate(5, sizeof(UnionBootParam), TRUE, "union_main.c", 55);
    param->playerInfo = GetGameDataPlayerInfo(gameData);
    param->comm = game_comm;
    param->gameData = gameData;
    param->gsys = gsys;
    GameCommSys_Boot(game_comm, GAME_COMM_NO_UNION, param);
}

void UnionMain_Update(GameCommSys *comm, Field *field) {
    UnionSystem *unisys;
    u8 activity;

    if (GameCommSys_BootCheck(comm) == GAME_COMM_NO_UNION) {
        unisys = GameCommSys_GetWork(comm);
        GFL_ASSERT(unisys != NULL);
        if (func_02016b14(Field_GetGameSystem(field)) == TRUE) {
            return;
        }
        if (GSYS_CheckNowEvent(unisys->param->gsys) == TRUE) {
            return;
        }
        if (UnionMain_IsFieldReady(unisys) == TRUE) {
            activity = unisys->situ.activity;
            if (activity >= 4 && activity <= 0x17) {
                if (UnionCommand_IsActive(unisys)) {
                    return;
                }
                UnionOneself_RunStep(unisys, field);
                return;
            }
            switch (activity) {
            case 0:
            case 1:
                if (UnionCommand_IsActive(unisys)) {
                    return;
                }
                UnionOneself_CheckEntries(unisys);
                UnionOneself_RunStep(unisys, field);
                UnionOneself_UpdateActors(unisys, unisys->param->gameData, field);
                break;
            }
        }
    }
}

BOOL UnionMain_IsFieldReady(UnionSystem *unisys) {
    if (unisys->fieldActive == TRUE && GSYS_CheckField(unisys->param->gsys) == TRUE &&
        Field_CheckMapLoadFinished(GSYS_GetField(unisys->param->gsys)) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

void UnionMain_OnFieldIn(void *param, void *work, Field *field) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    unisys->fieldActive = TRUE;
    if (unisys->updateDisabled == TRUE) {
        FieldmapCtrlGrid_SetUpdateDisable(field, TRUE);
    }
    if (GetZoneIsUnionRoom(Field_GetPlayerStateZoneID(field)) == TRUE) {
        UnionOneself_UpdateMarks(unisys, GSYS_GetGameData(unisys->param->gsys), field);
    }
    if (colosseum != NULL && colosseum->unkC8_0 == TRUE) {
        func_ov012_021615a4(colosseum->commPlayer);
    }
}

void UnionMain_OnFieldOut(void *param, void *work, Field *field) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL) {
        if (colosseum->unkC9 == 1) {
            Colosseum_Free(colosseum, unisys->param->gsys);
            unisys->colosseum_sys = NULL;
        } else if (colosseum->unkC8_0 == TRUE) {
            func_ov012_02161544(colosseum->commPlayer);
        }
    }
    unisys->fieldActive = FALSE;
}

u32 func_ov028_021710a4(UnionSystem *unisys) {
    return unisys->unk3630;
}

u32 func_ov028_021710b0(UnionSystem *unisys) {
    return unisys->unk3634;
}

void func_ov028_021710bc(UnionSystem *unisys, u8 value) {
    unisys->situ.unk02 = value;
}

u8 func_ov028_021710c8(UnionSystem *unisys) {
    return unisys->situ.unk02;
}

BOOL UnionMain_IsIdle(GameSystem *gsys) {
    UnionSystem *unisys = GameCommSys_GetWork(GSYS_GetGameCommSystem(gsys));

    if (unisys == NULL) {
        return FALSE;
    }
    if ((unisys->situ.step == 0 || unisys->situ.step == 0x2a) && unisys->updateDisabled == 0) {
        return TRUE;
    }
    return FALSE;
}
