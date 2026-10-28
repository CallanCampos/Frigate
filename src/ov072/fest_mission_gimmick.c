#include "types.h"
#include "constants/pokemon.h"
#include "field/fest_mission_gimmick.h"
#include "field/fest_mission_people.h"
#include "field/festival.h"
#include "field/field_actor.h"
#include "field/zone.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/mi.h"
#include "pml/personal.h"
#include "pml/poke_party.h"

static BOOL FesMissionGimmick_IsInList(u16 value, const u16 *list, int count);
static u16 FesMissionGimmick_RandomNotInList(u16 max, const u16 *list, int count);
static BOOL FesMissionGimmick_DeleteBlockedPeople(FesGimmick *gimmick, FesMissionWork *work);
static BOOL FesMissionGimmick_IsGridPlaceBlocked(FesGimmick *gimmick, FesMissionWork *work, int index,
                                                 const VecFx32 *position);
static BOOL FesMissionGimmick_IsRailPlaceBlocked(FesGimmick *gimmick, FesMissionWork *work, int index,
                                                 const RailPosition *position);
static void FesMissionGimmick_DeletePersonActor(FesMissionWork *work, int index);

void FesMissionGimmick_LoadActorResource(FesGimmick *gimmick, FesMissionWork *work) {
    FldActSys_LoadStaticBlact(gimmick->actors, FesMissionPeople_GetObjCode(work, gimmick->mission));
}

void FesMissionGimmick_RemoveBlockedPeople(FesGimmick *gimmick, FesMissionWork *work) {
    if (work->actorCount != 0) {
        FesMissionGimmick_DeleteBlockedPeople(gimmick, work);
    }
}

void FesMissionGimmick_RemovePerson(FesGimmick *gimmick, FesMissionWork *work, u16 zoneId, int index) {
    if (work->actorCount != 0) {
        FesMissionPeople_GetZone(work, zoneId)->removed[index] = TRUE;
        SetActorHidden(work->actors[index], TRUE);
        FesMissionGimmick_DeletePersonActor(work, index);
    }
}

BOOL FesMissionGimmick_End(FesGimmick *gimmick, FesMissionWork *work) {
    BOOL result = TRUE;
    int i;

    if (work->placeCount == 0) {
        result = FALSE;
    }
    if (work->actorCount != 0) {
        for (i = 0; i < 4; i++) {
            FesMissionGimmick_DeletePersonActor(work, i);
        }
    }
    MI_CpuClear32(work, sizeof(FesMissionWork));
    return result;
}

void FesMissionGimmick_BoostPartyLevels(FesGimmick *gimmick, PokeParty *party) {
    int count;
    int bonus;
    int i;
    PartyPkm *pkm;
    int level;
    u32 species;

    if (gimmick->mission->info.target == 6) {
        count = PokeParty_GetPkmCount(party);
        bonus = LinkFestival_GetTotalScore(gimmick->festival);
        if (bonus > 30) {
            bonus = 30;
        }
        for (i = 0; i < count; i++) {
            pkm = PokeParty_GetPkm(party, i);
            level = bonus + PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
            if (level > 100) {
                level = 100;
            }
            species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
            PokeParty_SetParam(pkm, PKM_PARAM_EXP,
                               PML_UtilGetPkmLvExp(species, PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL), level));
            PokeParty_RecalcStats(pkm);
        }
    }
}

void FesMissionGimmick_GetItemExchangeInfo(FesGimmick *gimmick, FieldActor *actor, u16 arg0, u16 arg1, u16 *out0,
                                           u16 *out1) {
    const FesMissionAnswers *answers;

    if (gimmick->mission->info.target != 2) {
        *out0 = 0x11;
        *out1 = 0x1a;
        return;
    }
    answers = &FesMissionPeople_Answers[gimmick->mission->info.unk0_20];
    *out0 = answers->values[GetActorUserParam(actor, 0)][arg0];
    *out1 = answers->values[GetActorUserParam(actor, 1)][arg1];
}

void FesMissionGimmick_GetItemSaleInfo(FesGimmick *gimmick, FieldActor *actor, u16 *item, u16 *price) {
    u8 onSale = FesMissionPeople_IsOnSale(gimmick->mission);
    int value;

    if (gimmick->mission->info.target != 3) {
        *item = 0x11;
        *price = 300;
        return;
    }
    *item = GetActorUserParam(actor, 0);
    value = GetActorUserParam(actor, 2);
    if (onSale) {
        // In tenths of the price: 80% to 490%
        u32 rate = GFL_RandomLC(42) + 8;
        value = value / 10 * rate;
    }
    if (value > 59800) {
        value = 59800;
    }
    *price = value;
}

void FesMissionGimmick_MakePokemonQuiz(FesGimmick *gimmick, u16 *count, u16 *answer, u16 *answerChoice) {
    u16 picked[10];
    u8 order[10];
    FesMissionQuiz *quiz = &gimmick->quiz;
    FesMissionWork *work = &gimmick->work->people;
    u32 max;
    const u16 *species;
    u8 total;
    int i;
    u8 temp;
    u8 j;

    MI_CpuFill8(quiz, 0, sizeof(FesMissionQuiz));
    quiz->count = LinkFestival_GetScore(gimmick->festival);
    if (quiz->count < 3) {
        quiz->count = 3;
    } else if (quiz->count > 10) {
        quiz->count = 10;
    }

    total = quiz->count;
    if (total < 5) {
        total = 5;
    }
    max = work->speciesCount / 2;
    species = work->species;
    for (i = 0; i < total; i++) {
        order[i] = i;
        picked[i] = FesMissionGimmick_RandomNotInList(max, picked, i);
        quiz->species[i] = species[picked[i]];
    }

    quiz->answer = GFL_RandomLC(quiz->count);
    for (i = 0; i < total; i++) {
        temp = order[i];
        j = GFL_RandomLC(total);
        order[i] = order[j];
        order[j] = temp;
    }

    quiz->answerChoice = 0xff;
    for (i = 0; i < 5; i++) {
        quiz->choices[i] = quiz->species[order[i]];
        if (order[i] == quiz->answer) {
            quiz->answerChoice = i;
        }
    }
    if (quiz->answerChoice == 0xff) {
        quiz->answerChoice = GFL_RandomLC(5);
        quiz->choices[quiz->answerChoice] = quiz->species[quiz->answer];
    }

    *count = quiz->count;
    *answer = quiz->answer;
    *answerChoice = quiz->answerChoice;
}

u16 FesMissionGimmick_GetQuizSpecies(FesGimmick *gimmick, u8 index) {
    return gimmick->quiz.species[index];
}

u16 FesMissionGimmick_GetQuizChoice(FesGimmick *gimmick, u8 index) {
    return gimmick->quiz.choices[index];
}

static BOOL FesMissionGimmick_IsInList(u16 value, const u16 *list, int count) {
    int i;

    for (i = 0; i < count; i++) {
        if (value == list[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

static u16 FesMissionGimmick_RandomNotInList(u16 max, const u16 *list, int count) {
    u16 value;

    do {
        value = GFL_RandomLC(max);
    } while (FesMissionGimmick_IsInList(value, list, count));
    return value;
}

static BOOL FesMissionGimmick_DeleteBlockedPeople(FesGimmick *gimmick, FesMissionWork *work) {
    FesMissionZone *zone = FesMissionPeople_GetZone(work, work->zoneId);
    int i;

    for (i = 0; i < work->actorCount; i++) {
        const ZoneBGEntity *place = &work->places[work->placeIndex[i]];

        if (zone->removed[i] == FALSE && work->actors[i] != NULL) {
            if (place->isRail == FALSE) {
                VecFx32 position;

                GetBGEntityCenterPosGrid(place, &position);
                if (FesMissionGimmick_IsGridPlaceBlocked(gimmick, work, i, &position)) {
                    zone->removed[i] = TRUE;
                    FesMissionGimmick_DeletePersonActor(work, i);
                }
            } else {
                RailPosition position;

                GetBGEntityPosRail(place, &position);
                if (FesMissionGimmick_IsRailPlaceBlocked(gimmick, work, i, &position)) {
                    zone->removed[i] = TRUE;
                    FesMissionGimmick_DeletePersonActor(work, i);
                }
            }
        }
    }
    return TRUE;
}

static BOOL FesMissionGimmick_IsGridPlaceBlocked(FesGimmick *gimmick, FesMissionWork *work, int index,
                                                 const VecFx32 *position) {
    BOOL result;

    if (FindActorByGPos_(gimmick->actors, (position->x >> 4) / FX32_ONE, (position->z >> 4) / FX32_ONE, position->y,
                         FX32_CONST(20), FALSE, work->actors[index]) != NULL) {
        return TRUE;
    }
    result = FALSE;
    if (GetTriggerSCRIDAtPosGrid(gimmick->eventData, gimmick->eventWork, position, 8) != 0xffff) {
        result = TRUE;
    }
    return result;
}

static BOOL FesMissionGimmick_IsRailPlaceBlocked(FesGimmick *gimmick, FesMissionWork *work, int index,
                                                 const RailPosition *position) {
    BOOL result = FALSE;

    if (FindActorByRailPosExcluding(gimmick->actors, position, FALSE, work->actors[index]) != NULL) {
        return TRUE;
    }
    if (GetTriggerSCRIDAtPosRail(gimmick->eventData, gimmick->eventWork, position) != 0xffff) {
        result = TRUE;
    }
    return result;
}

static void FesMissionGimmick_DeletePersonActor(FesMissionWork *work, int index) {
    if (work->actors[index] != NULL) {
        DeleteActor(work->actors[index]);
        work->actors[index] = NULL;
    }
}
