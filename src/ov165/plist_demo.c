#include "app/pokelist.h"
#include "constants/battle.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/net.h"
#include "gfl/particle.h"
#include "gfl/sound.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/rtc.h"
#include "pml/move.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/musical_savedata.h"
#include "save/pokedex_savedata.h"
#include "save/save_control.h"
#include "save/shortcut.h"
#include "system/rtc.h"
#include "system/wipe.h"

// The form changes that items make on the party list, with their particles, and the checks for them. Named after
// the ROM's string "plist_demo.c"

static void PokeListDemo_CreateParticles(PokeListWork *wk, int slot, int type, int plate);
static void PokeListDemo_Setup(PokeListWork *wk);
static void PokeListDemo_FreeParticles(PokeListWork *wk);
static BOOL PokeListDemo_UpdateForm(PokeListWork *wk);
static BOOL PokeListDemo_UpdateFusion(PokeListWork *wk);
static void PokeListDemo_Fuse(PokeListWork *wk, int form, int kyuremPos, int partnerPos);
static void PokeListDemo_Separate(PokeListWork *wk, PartyPkm *kyurem);
static void PokeListDemo_ChangeKyuremMoves(PartyPkm *pkm, u8 from, u8 to);

// The moves that change with Kyurem's form: Normal, White (fused with Reshiram) and Black (fused with Zekrom)
static const u16 sKyuremMoves[2][3] = {
    { MOVE_SCARY_FACE, MOVE_FUSION_FLARE, MOVE_FUSION_BOLT },
    { MOVE_GLACIATE, MOVE_ICE_BURN, MOVE_FREEZE_SHOCK },
};

// The particle file of each effect, and how many emitters it has
static const u8 sParticleResources[8][2] = {
    { 0x1b, 3 }, { 0x20, 2 }, { 0x1e, 4 }, { 0x1f, 3 }, { 0x22, 3 }, { 0x1c, 6 }, { 0x1d, 6 }, { 0x21, 3 },
};

void PokeListDemo_Start(PokeListWork *wk) {
    wk->demoTimer = 0;
    GFL_BGSysSetBGEnabled(0, FALSE);
    PokeList_ReleaseBG0(wk);
    PokeList_Init3D(wk);
    PokeListDemo_Setup(wk);
    wk->state = 15;
    wk->demoStep = 0;
}

void PokeListDemo_End(PokeListWork *wk) {
    void (*doneFunc)(PokeListWork *wk) = PokeList_MessageDoneExit;

    PokeListDemo_FreeParticles(wk);
    PokeList_Exit3D(wk);
    PokeList_CreateBG0(wk);
    PokeListMessage_LoadFrame(wk, wk->message);
    switch (wk->demo) {
    case 1:
    case 2:
        if (wk->param->mode == 0) {
            doneFunc = PokeList_MessageDoneSelect;
        }
    case 3:
    case 4:
    case 5:
    case 6:
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
        PokeList_ShowMessage(wk, 0xb2, TRUE, doneFunc);
        PokeListMessage_FreeWordSet(wk, wk->message);
        break;
    default:
        wk->state = 19;
        break;
    }
}

void PokeListDemo_Update(PokeListWork *wk) {
    BOOL done = FALSE;

    switch (wk->demo) {
    case 1:
    case 2:
    case 3:
    case 6:
        done = PokeListDemo_UpdateForm(wk);
        break;
    case 4:
    case 5:
        done = PokeListDemo_UpdateFusion(wk);
        break;
    }
    if (done) {
        wk->state = 16;
    }
    Particle_UpdateAll();
    GFL_G3DSysReset();
    GFL_G3DSysMtxViewFlush();
    Particle_DrawAll();
    GFL_G3DSysReqSwapBuffers();
}

// Starts particle effect type in particle slot 0 or 1, over the plate's Pokémon
static void PokeListDemo_CreateParticles(PokeListWork *wk, int slot, int type, int plate) {
    const u8 *res = sParticleResources[type];
    void *resource = Particle_ReadResource(0x4b, res[0], wk->heapId);
    ClActorPos pos;
    VecFx32 vec;
    u8 i;
    u8 count;

    wk->particleHeaps[slot] = allocConfigDSSoftwareFeature(wk->heapId, 0x5000, "plist_demo.c", 196);
    wk->particles[slot] = Particle_CreateSystem(wk->particleHeaps[slot], 0x5000, FALSE, wk->heapId);
    Particle_SetResource(wk->particles[slot], resource, FALSE, GFL_VBlankGetTCBMgr());
    PokeListPlate_GetCursorPos(wk, wk->plates[plate], &pos);
    pos.x -= 40;
    pos.y += 4;
    vec.x = FX32_CONST(pos.x) / 38;
    vec.y = FX32_CONST(pos.y) / 38;
    vec.z = 64 * FX32_ONE;
    count = res[1];
    for (i = 0; i < count; i++) {
        Particle_CreateEmitter(wk->particles[slot], i, &vec);
    }
}

static void PokeListDemo_Setup(PokeListWork *wk) {
    int i;

    for (i = 0; i < 2; i++) {
        wk->particleHeaps[i] = NULL;
        wk->particles[i] = NULL;
    }
    switch (wk->demo) {
    case 1:
    case 2:
        PokeListDemo_CreateParticles(wk, 0, 0, wk->cursorPos);
        wk->demoLength = 65;
        wk->demoSe = SEQ_SE_SYS_04;
        break;
    case 3:
        PokeListDemo_CreateParticles(wk, 0, 1, wk->cursorPos);
        wk->demoLength = 38;
        wk->demoSe = SEQ_SE_SYS_05;
        break;
    case 4: {
        u8 form = wk->demoForm;

        PokeListDemo_CreateParticles(wk, 0, 2, wk->selectPos);
        PokeListDemo_CreateParticles(wk, 1, form + 3, wk->cursorPos);
        wk->demoLength = 110;
        if (form != 0) {
            wk->demoSe = SEQ_SE_SW_KYUREM_01;
        } else {
            wk->demoSe = SEQ_SE_SW_KYUREM_02;
        }
        break;
    }
    case 5: {
        int zekrom = FALSE;

        if (PokeParty_GetParam(ReshZekSave_GetPkm(wk->param->reshZek), PKM_PARAM_SPECIES, NULL) == SPECIES_ZEKROM) {
            zekrom = TRUE;
        }
        PokeListDemo_CreateParticles(wk, 0, zekrom + 5, wk->cursorPos);
        wk->demoLength = 130;
        wk->demoSe = SEQ_SE_SW_KYUREM_03;
        break;
    }
    case 6:
        PokeListDemo_CreateParticles(wk, 0, 7, wk->cursorPos);
        wk->demoLength = 25;
        wk->demoSe = SEQ_SE_SW_TOROS_FORM;
        break;
    }
}

static void PokeListDemo_FreeParticles(PokeListWork *wk) {
    int i;

    for (i = 0; i < 2; i++) {
        if (wk->particles[i] != NULL) {
            Particle_DeleteSystem(wk->particles[i]);
            GFL_NetFreeAligned32(wk->particleHeaps[i]);
            wk->particleHeaps[i] = NULL;
            wk->particles[i] = NULL;
        }
    }
}

// A form change on one plate: the effect, the new form, then the cry
static BOOL PokeListDemo_UpdateForm(PokeListWork *wk) {
    switch (wk->demoStep) {
    case 0:
        GFL_BGSysSetBGEnabled(0, TRUE);
        GFL_SndSEPlay(wk->demoSe);
        wk->demoStep++;
    case 1:
        wk->demoTimer++;
        if (wk->demoTimer > wk->demoLength) {
            PokeListPlate_SetPkm(wk, wk->plates[wk->cursorPos], wk->pkm, 0);
            wk->demoStep++;
        }
        break;
    case 2:
        if (Particle_GetEmitterCount(wk->particles[0]) <= 0 && GFL_SndPlayerIsActiveAny() == FALSE) {
            wk->voice = PokeVoice_Play(PokeParty_GetParam(wk->pkm, PKM_PARAM_SPECIES, NULL),
                                       PokeParty_GetParam(wk->pkm, PKM_PARAM_FORM, NULL), 64, 0, 0, 0, 0, NULL);
            wk->demoStep++;
        }
        break;
    case 3:
        if (PokeVoice_IsPlaying(wk->voice) == FALSE) {
            return TRUE;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

// Kyurem fusing or separating: the effect, a white wipe while the party changes, then the cry
static BOOL PokeListDemo_UpdateFusion(PokeListWork *wk) {
    int i;

    switch (wk->demoStep) {
    case 0:
        GFL_BGSysSetBGEnabled(0, TRUE);
        GFL_SEPlayKeepVol(wk->demoSe, 1);
        wk->demoStep++;
    case 1:
        wk->demoTimer++;
        if (wk->demoTimer > wk->demoLength) {
            if (wk->demoForm != 2) {
                GFL_SndSEPlay(SEQ_SE_SW_KYUREM_04);
            }
            GFL_WipeSet(WIPE_MODE_MAIN, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0x7fff, 12, 1, wk->heapId);
            wk->demoStep++;
        }
        break;
    case 2: {
        int count;

        if (GFL_WipeIsFinished() == FALSE || GFL_SndPlayerIsActiveAny() == TRUE) {
            break;
        }
        for (i = 0; i < 2; i++) {
            if (wk->particles[i] != NULL && Particle_GetEmitterCount(wk->particles[i]) > 0) {
                // BUG: Meant to wait for the particles, but this leaves only the loop
#ifdef BUGFIX
                return FALSE;
#else
                break;
#endif
            }
        }
        if (wk->demoForm == 2) {
            PokeListDemo_Separate(wk, wk->pkm);
        } else {
            PokeListDemo_Fuse(wk, wk->demoForm, wk->selectPos, wk->cursorPos);
        }
        if (wk->cursorPos < wk->selectPos) {
            wk->selectPos--;
            wk->cursorPos = wk->selectPos;
            wk->pkm = PokeParty_GetPkm(wk->param->party, wk->selectPos);
        }
        count = PokeParty_GetPkmCount(wk->param->party);
        wk->enteredCount = 0;
        for (i = 0; i < POKELIST_PLATE_COUNT; i++) {
            PokeListPlate_ClearSlid(wk, wk->plates[i], 0);
            PokeListPlate_Free(wk, wk->plates[i]);
            if (i < count) {
                wk->plates[i] = PokeListPlate_Create(wk, i, PokeParty_GetPkm(wk->param->party, i));
            } else {
                wk->plates[i] = PokeListPlate_CreateEmpty(wk, i);
            }
            if (PokeListPlate_GetEntry(wk->plates[i]) <= 5) {
                wk->enteredCount++;
            }
        }
        ClActor_SetVisible(wk->cursor, FALSE);
        ClActor_SetVisible(wk->subCursor, FALSE);
        wk->demoStep++;
        break;
    }
    case 3: {
        int printing;
        PartyPkm *pkm;

        // BUG: Only the last plate is checked
#ifdef BUGFIX
        printing = FALSE;
        for (i = 0; i < POKELIST_PLATE_COUNT; i++) {
            printing |= PokeListPlate_IsPrinting(wk->plates[i]);
        }
#else
        for (i = 0; i < POKELIST_PLATE_COUNT; i++) {
            printing = PokeListPlate_IsPrinting(wk->plates[i]);
        }
#endif
        if (printing > 0) {
            break;
        }
        pkm = PokeParty_GetPkm(wk->param->party, wk->selectPos);
        wk->voice = PokeVoice_Play(PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL),
                                   PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL), 64, 0, 0, 0, 0, NULL);
        wk->demoStep++;
        break;
    }
    case 4:
        if (PokeVoice_IsPlaying(wk->voice) == FALSE) {
            GFL_WipeSet(WIPE_MODE_MAIN, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, 0x7fff, 12, 1, wk->heapId);
            wk->demoStep++;
        }
        break;
    case 5:
    default:
        if (GFL_WipeIsFinished() == TRUE) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// The Griseous Orb given to a Giratina in its Altered Forme. Like the other checks but the Reveal Glass's, this one
// checks wk->pkm, not pkm
BOOL PokeListDemo_CanBecomeOrigin(PokeListWork *wk, PartyPkm *pkm) {
    BOOL result = FALSE;

    if (PokeParty_GetParam(wk->pkm, PKM_PARAM_SPECIES, NULL) != SPECIES_GIRATINA) {
        return result;
    }
    if (PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL) != ITEM_GRISEOUS_ORB) {
        return result;
    }
    if (PokeParty_GetParam(wk->pkm, PKM_PARAM_FORM, NULL) == 0) {
        result = TRUE;
    }
    return result;
}

void PokeListDemo_SetOrigin(PokeListWork *wk, PartyPkm *pkm) {
    PokeParty_ChangeForme(pkm, 1);
    addPkmToDex(wk->param->pokedex, pkm);
}

// The Griseous Orb taken from a Giratina in its Origin Forme
BOOL PokeListDemo_CanBecomeAltered(PokeListWork *wk, PartyPkm *pkm) {
    BOOL result = FALSE;

    if (PokeParty_GetParam(wk->pkm, PKM_PARAM_ITEM, NULL) == ITEM_GRISEOUS_ORB) {
        return result;
    }
    if (PokeParty_GetParam(wk->pkm, PKM_PARAM_SPECIES, NULL) != SPECIES_GIRATINA) {
        return result;
    }
    if (PokeParty_GetParam(wk->pkm, PKM_PARAM_FORM, NULL) == 1) {
        result = TRUE;
    }
    return result;
}

void PokeListDemo_SetAltered(PokeListWork *wk, PartyPkm *pkm) {
    PokeParty_ChangeForme(pkm, 0);
    addPkmToDex(wk->param->pokedex, pkm);
}

// The Gracidea, used on a Shaymin in its Land Forme that has met the player in a fateful encounter, in the day
BOOL PokeListDemo_CanBecomeSky(PokeListWork *wk, PartyPkm *pkm) {
    BOOL result = FALSE;
    u8 start;
    u8 end;
    RTCTime time;

    if (PokeParty_GetParam(wk->pkm, PKM_PARAM_SPECIES, NULL) != SPECIES_SHAYMIN) {
        return result;
    }
    if (PokeParty_GetParam(wk->pkm, PKM_PARAM_FORM, NULL) != 0) {
        return result;
    }
    if (PokeParty_GetParam(wk->pkm, PKM_PARAM_FATEFUL_ENCOUNTER, NULL) != TRUE) {
        return result;
    }
    // BUG: The status condition is a number, not the bits of Diamond and Pearl, where 0x20 was freeze, so a frozen
    // Shaymin changes too
#ifdef BUGFIX
    if (GetStatusCond(wk->pkm) == CONDITION_FREEZE)
#else
    if (GetStatusCond(wk->pkm) & 0x20)
#endif
    {
        return result;
    }
    start = GetLightChangeHoursForSeasons(wk->param->season, 0);
    end = GetLightChangeHoursForSeasons(wk->param->season, 3);
    RTC_GetCachedTime(&time);
    if (time.hour < start || time.hour >= end) {
        return FALSE;
    }
    return TRUE;
}

void PokeListDemo_SetSky(PokeListWork *wk, PartyPkm *pkm) {
    PokeParty_ChangeForme(pkm, 1);
    addPkmToDex(wk->param->pokedex, pkm);
}

u32 PokeListDemo_CheckFuse(PokeListWork *wk, PartyPkm *pkm) {
    u32 result = 0;

    if (PokeParty_GetParam(wk->pkm, PKM_PARAM_SPECIES, NULL) != SPECIES_KYUREM) {
        return 2;
    }
    if (PokeParty_GetParam(wk->pkm, PKM_PARAM_FORM, NULL) != 0) {
        return 4;
    }
    if (PokeParty_GetParam(wk->pkm, PKM_PARAM_HP, NULL) == 0) {
        result = 3;
    }
    return result;
}

u32 PokeListDemo_CheckSeparate(PokeListWork *wk, PartyPkm *pkm) {
    u32 result = 5;

    if (PokeParty_GetParam(wk->pkm, PKM_PARAM_SPECIES, NULL) != SPECIES_KYUREM) {
        return 2;
    }
    if (PokeParty_GetParam(wk->pkm, PKM_PARAM_FORM, NULL) == 0) {
        return 4;
    }
    if (PokeParty_GetPkmCount(wk->param->party) != 6) {
        result = 1;
    }
    return result;
}

// Kyurem takes form 1 (White) or 2 (Black) from the partner, which leaves the party for the save
static void PokeListDemo_Fuse(PokeListWork *wk, int form, int kyuremPos, int partnerPos) {
    PartyPkm *kyurem = PokeParty_GetPkm(wk->param->party, kyuremPos);

    ReshZekSave_SetPkm(wk->param->reshZek, PokeParty_GetPkm(wk->param->party, partnerPos));
    PokeListDemo_ChangeKyuremMoves(kyurem, 0, form + 1);
    // PokeParty_ChangeForme takes a u16
    PokeParty_ChangeForme(kyurem, form + 1);
    addPkmToDex(wk->param->pokedex, kyurem);
    PokeParty_RemovePkm(wk->param->party, partnerPos);
    BagSave_SwitchOwnedDNASplicers(wk->param->bag, 0);
    ShortcutSave_SwitchDNASplicers(wk->param->shortcut, 7);
}

static void PokeListDemo_Separate(PokeListWork *wk, PartyPkm *kyurem) {
    PokeListDemo_ChangeKyuremMoves(kyurem, PokeParty_GetParam(kyurem, PKM_PARAM_FORM, NULL), 0);
    PokeParty_ChangeForme(kyurem, 0);
    PokeParty_AddPkm(wk->param->party, ReshZekSave_GetPkm(wk->param->reshZek));
    BagSave_SwitchOwnedDNASplicers(wk->param->bag, 1);
    ShortcutSave_SwitchDNASplicers(wk->param->shortcut, 8);
}

// Replaces the moves of Kyurem's old form with those of its new one, keeping the PP within the new maximum
static void PokeListDemo_ChangeKyuremMoves(PartyPkm *pkm, u8 from, u8 to) {
    int i;
    int j;

    for (i = 0; i < 4; i++) {
        u16 move = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i, NULL);
        u16 pp = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_PP + i, NULL);

        if (move == MOVE_NONE) {
            continue;
        }
        for (j = 0; j < 2; j++) {
            if (move == sKyuremMoves[j][from]) {
                u32 maxPP;

                move = sKyuremMoves[j][to];
                maxPP = PML_MoveGetMaxPP(move, PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_PP_UP + i, NULL));
                PokeParty_SetParam(pkm, PKM_PARAM_MOVE1 + i, move);
                if (maxPP < pp) {
                    PokeParty_SetParam(pkm, PKM_PARAM_MOVE1_PP + i, maxPP);
                }
                break;
            }
        }
    }
}

// The Reveal Glass, used on Tornadus, Thundurus or Landorus
BOOL PokeListDemo_CanBecomeTherian(PokeListWork *wk, PartyPkm *pkm) {
    BOOL result = FALSE;
    u16 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);

    if (species != SPECIES_TORNADUS && species != SPECIES_THUNDURUS && species != SPECIES_LANDORUS) {
        return result;
    }
    return TRUE;
}

void PokeListDemo_ToggleTherian(PokeListWork *wk, PartyPkm *pkm) {
    u16 form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);

    PokeParty_ChangeForme(pkm, form ^ 1);
    addPkmToDex(wk->param->pokedex, pkm);
}
