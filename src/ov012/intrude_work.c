#include "types.h"
#include "constants/version.h"
#include "field/intrude_work.h"
#include "gfl/net.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_system.h"

BOOL func_ov012_021535dc(GameSystem *gsys) {
    GameCommSys *commSys = GSYS_GetGameCommSystem(gsys);

    if (GameData_IsForceSeasonSync(GSYS_GetGameData(gsys)) == TRUE && func_0202be08(commSys)) {
        return FALSE;
    }
    return TRUE;
}

void *func_ov012_02153608(GameCommSys *commSys) {
    void *work = GameCommSys_GetWork(commSys);

    if (GFL_NetErrCheck() || GameCommSys_BootCheck(commSys) != 2 || GameCommSys_IsTransitioning(commSys) == TRUE || work == NULL) {
        return NULL;
    }
    return work;
}

u32 getGameOrigin(GameCommSys *commSys) {
#ifdef BLACK2
    return VERSION_BLACK2;
#else
    return VERSION_WHITE2;
#endif
}

u8 getSeasonFromPlayerData(GameCommSys *commSys) {
    return GameData_GetSeason(getBasePlayerBlk(commSys));
}

u32 func_ov012_0215364c(GameCommSys *commSys, GameData *gameData) {
    return 0;
}

u32 func_ov012_02153650(GameCommSys *commSys) {
    return 0;
}

void func_ov012_02153654(GameCommSys *commSys) {
}

u32 func_ov012_02153658(GameCommSys *commSys) {
    return 0;
}

u32 func_ov012_0215365c(void) {
    return 0;
}

u32 func_ov012_02153660(void) {
    return 0;
}

u32 func_ov012_02153664(GameCommSys *commSys) {
    return 0;
}

void func_ov012_02153668(GameCommSys *commSys) {
}
