#include "battle/field_handlers.h"
#include "types.h"
#include "battle/btl_event.h"
#include "constants/battle.h"
#include "constants/types.h"

// One entry of the field effect table: an effect and the function that returns its handlers
typedef struct FieldEffectEventAddEntry {
    u32 effect;
    const BattleEventHandlerEntry *(*eventAdd)(u32 *numHandlers);
} FieldEffectEventAddEntry;

static const BattleEventHandlerEntry *EventAddFieldWeather(u32 *numHandlers);
static void HandlerFieldWeather(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFieldTrickRoom(u32 *numHandlers);
static void HandlerFieldTrickRoom(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFieldGravity(u32 *numHandlers);
static void HandlerFieldGravityAccuracy(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerFieldGravityGrounded(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFieldWaterSport(u32 *numHandlers);
static void HandlerFieldWaterSport(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFieldMudSport(u32 *numHandlers);
static void HandlerFieldMudSport(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFieldWonderRoom(u32 *numHandlers);
static void HandlerFieldWonderRoom(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFieldImprison(u32 *numHandlers);
static const BattleEventHandlerEntry *EventAddFieldMagicRoom(u32 *numHandlers);
static void HandlerFieldMagicRoom(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);

// Each field effect and the function that returns its handlers
static const FieldEffectEventAddEntry sFieldEffectEventAddTable[] = {
    { FIELD_CONDITION_WEATHER, EventAddFieldWeather },        { FIELD_CONDITION_TRICK_ROOM, EventAddFieldTrickRoom },
    { FIELD_CONDITION_GRAVITY, EventAddFieldGravity },        { FIELD_CONDITION_IMPRISON, EventAddFieldImprison },
    { FIELD_CONDITION_WATER_SPORT, EventAddFieldWaterSport }, { FIELD_CONDITION_MUD_SPORT, EventAddFieldMudSport },
    { FIELD_CONDITION_WONDER_ROOM, EventAddFieldWonderRoom }, { FIELD_CONDITION_MAGIC_ROOM, EventAddFieldMagicRoom },
};

BattleEventItem *FieldEffectEventAdd(u32 effect, u8 monId) {
    u32 i;
    u32 numHandlers;
    const BattleEventHandlerEntry *handlers;

    for (i = 0; i < NELEMS(sFieldEffectEventAddTable); i++) {
        if (effect == sFieldEffectEventAddTable[i].effect) {
            handlers = sFieldEffectEventAddTable[i].eventAdd(&numHandlers);
            return BattleEvent_AddItem(BTL_EVENT_FACTOR_FIELD, effect, 3, 0, monId, handlers, numHandlers);
        }
    }
    return NULL;
}

void FieldEffectEventRemove(BattleEventItem *item) {
    BattleEventItem_Remove(item);
}

static const BattleEventHandlerEntry sHandlersFieldWeather[] = {
    { BTL_EVENT_CALC_DAMAGE, HandlerFieldWeather },
};

static const BattleEventHandlerEntry *EventAddFieldWeather(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersFieldWeather);
    return sHandlersFieldWeather;
}

static void HandlerFieldWeather(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
}

static const BattleEventHandlerEntry sHandlersFieldTrickRoom[] = {
    { BTL_EVENT_CALC_SPEED, HandlerFieldTrickRoom },
};

static const BattleEventHandlerEntry *EventAddFieldTrickRoom(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersFieldTrickRoom);
    return sHandlersFieldTrickRoom;
}

// Reverses the turn order
static void HandlerFieldTrickRoom(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleEventVar_RewriteValue(BTL_EVAR_TRICK_ROOM_FLAG, TRUE);
}

static const BattleEventHandlerEntry sHandlersFieldGravity[] = {
    { BTL_EVENT_GET_MOVE_ACCURACY, HandlerFieldGravityAccuracy },
    { BTL_EVENT_CHECK_TYPE_EFFECTIVENESS, HandlerFieldGravityGrounded },
};

static const BattleEventHandlerEntry *EventAddFieldGravity(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersFieldGravity);
    return sHandlersFieldGravity;
}

// Raises accuracy by 5/3 (the ratio is in 4096ths)
static void HandlerFieldGravityAccuracy(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleEventVar_MulValue(BTL_EVAR_RATIO, 0x1ab8);
}

// Lets Ground moves hit Pokémon in the air
static void HandlerFieldGravityGrounded(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(BTL_EVAR_MOVE_TYPE) == TYPE_GROUND) {
        BattleEventVar_RewriteValue(0x4b, TRUE);
    }
}

static const BattleEventHandlerEntry sHandlersFieldWaterSport[] = {
    { BTL_EVENT_MOVE_POWER, HandlerFieldWaterSport },
};

static const BattleEventHandlerEntry *EventAddFieldWaterSport(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersFieldWaterSport);
    return sHandlersFieldWaterSport;
}

// Weakens Fire moves to 0x548/4096 (about a third)
static void HandlerFieldWaterSport(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(BTL_EVAR_MOVE_TYPE) == TYPE_FIRE) {
        BattleEventVar_MulValue(BTL_EVAR_POWER_RATIO, 0x548);
    }
}

static const BattleEventHandlerEntry sHandlersFieldMudSport[] = {
    { BTL_EVENT_MOVE_POWER, HandlerFieldMudSport },
};

static const BattleEventHandlerEntry *EventAddFieldMudSport(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersFieldMudSport);
    return sHandlersFieldMudSport;
}

// Weakens Electric moves to 0x548/4096 (about a third)
static void HandlerFieldMudSport(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(BTL_EVAR_MOVE_TYPE) == TYPE_ELECTRIC) {
        BattleEventVar_MulValue(BTL_EVAR_POWER_RATIO, 0x548);
    }
}

static const BattleEventHandlerEntry sHandlersFieldWonderRoom[] = {
    { BTL_EVENT_DECIDE_DEFENSE_STAT, HandlerFieldWonderRoom },
};

static const BattleEventHandlerEntry *EventAddFieldWonderRoom(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersFieldWonderRoom);
    return sHandlersFieldWonderRoom;
}

// Swaps Defense and Special Defense
static void HandlerFieldWonderRoom(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleEventVar_RewriteValue(BTL_EVAR_DEFENSE_SWAP_COUNT, BattleEventVar_GetValue(BTL_EVAR_DEFENSE_SWAP_COUNT) + 1);
}

// Imprison does its work elsewhere and shares Magic Room's empty handler
static const BattleEventHandlerEntry sHandlersFieldImprison[] = {
    { 0, HandlerFieldMagicRoom },
};

static const BattleEventHandlerEntry *EventAddFieldImprison(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersFieldImprison);
    return sHandlersFieldImprison;
}

static const BattleEventHandlerEntry sHandlersFieldMagicRoom[] = {
    { 0, HandlerFieldMagicRoom },
};

static const BattleEventHandlerEntry *EventAddFieldMagicRoom(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersFieldMagicRoom);
    return sHandlersFieldMagicRoom;
}

static void HandlerFieldMagicRoom(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
}
