#include "types.h"
#include "battle/btl_ability.h"
#include "battle/btl_action.h"
#include "battle/btl_calc.h"
#include "battle/btl_event.h"
#include "battle/btl_field.h"
#include "battle/btl_handler.h"
#include "battle/btl_handler_work.h"
#include "battle/btl_item.h"
#include "battle/btl_main.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_server_flow.h"
#include "battle/handler_common.h"
#include "constants/battle.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/species.h"
#include "constants/types.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "pml/waza.h"

// One entry of the item lookup table at the end of the file: an item and the function that returns its handlers
typedef struct ItemEventAddEntry {
    u32 item;
    const BattleEventHandlerEntry *(*eventAdd)(u32 *numHandlers);
} ItemEventAddEntry;

static BattleEventItem *ItemEvent_AddItemCore(BattleMon *mon, u16 itemId);
static BOOL ItemEvent_RollEffectChance(BtlServerFlow *flow, u8 chance);
static u32 CommonGetItemParam(BattleEventItem *item, u32 param);
static fx32 ItemAttackValueToRatio(BattleEventItem *item);
static void ItemEvent_PushRun(BattleEventItem *item, BtlServerFlow *flow, u8 monId);
static void HandlerCheriBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerCherryBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddCheriBerry(u32 *numHandlers);
static void HandlerChestoBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerChestoBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddChestoBerry(u32 *numHandlers);
static void HandlerRawstBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerRawstBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRawstBerry(u32 *numHandlers);
static void HandlerAspearBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerAspearBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddAspearBerry(u32 *numHandlers);
static void HandlerPersimBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerPersimBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPersimBerry(u32 *numHandlers);
static void HandlerPechaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerPechaBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPechaBerry(u32 *numHandlers);
static void HandlerLumBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerLumBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddLumBerry(u32 *numHandlers);
static void CommonStatusReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, u32 condition);
static void CommonUseForStatus(BattleEventItem *item, BtlServerFlow *flow, u8 monId, u32 condition);
static BOOL CommonConditionCodeMatch(BtlServerFlow *flow, u8 monId, u32 condition);
static const BattleEventHandlerEntry *EventAddLeppaBerry(u32 *numHandlers);
static void HandlerLeppaBerryMoveEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerLeppaBerryReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerLeppaBerryGet(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerLeppaBerryPPUsed(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerLeppaBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerLeppaBerryUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static u8 CommonLeppaBerryLastMoveID(BtlServerFlow *flow, u8 monId);
static u8 CommonLeppaBerryEnableMoveID(BtlServerFlow *flow, u8 monId);
static BOOL HandlerLeppaBerryCommon(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work, BOOL anyUsed);
static const BattleEventHandlerEntry *EventAddOranBerry(u32 *numHandlers);
static void HandlerOranBerryReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerOranBerryMemberIn(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerOranBerryCheckActivation(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerOranBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddBerryJuice(u32 *numHandlers);
static const BattleEventHandlerEntry *EventAddSitrusBerry(u32 *numHandlers);
static void HandlerSitrusBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFigyBerry(u32 *numHandlers);
static void HandlerFigyBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerFigyBerryUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddWikiBerry(u32 *numHandlers);
static void HandlerWikiBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerWikiBerryUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMagoBerry(u32 *numHandlers);
static void HandlerMagoBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMagoBerryUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddAguavBerry(u32 *numHandlers);
static void HandlerAguavBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerAguavBerryUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddIapapaBerry(u32 *numHandlers);
static void HandlerIapapaBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerIapapaBerryUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerCommonPinchBerryReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerCommonPinchBerryMemberIn(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerCommonPinchBerryCheckActivation(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void CommonPinchBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work, u32 flavor);
static const BattleEventHandlerEntry *EventAddLiechiBerry(u32 *numHandlers);
static void HandlerLiechiBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddGanlonBerry(u32 *numHandlers);
static void HandlerGanlonBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSalacBerry(u32 *numHandlers);
static void HandlerSalacBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPetayaBerry(u32 *numHandlers);
static void HandlerPetayaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddApicotBerry(u32 *numHandlers);
static void HandlerApicotBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddLansatBerry(u32 *numHandlers);
static void HandlerLansatBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddStarfBerry(u32 *numHandlers);
static void HandlerStarfBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void CommonStatBoostBerry(BtlServerFlow *flow, u8 monId, u32 stat, s8 change);
static const BattleEventHandlerEntry *EventAddEnigmaBerry(u32 *numHandlers);
static void HandlerEnigmaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerEnigmaBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddOccaBerry(u32 *numHandlers);
static void HandlerOccaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPasshoBerry(u32 *numHandlers);
static void HandlerPasshoBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddWacanBerry(u32 *numHandlers);
static void HandlerWacanBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRindoBerry(u32 *numHandlers);
static void HandlerRindoBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddYacheBerry(u32 *numHandlers);
static void HandlerYacheBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddChopleBerry(u32 *numHandlers);
static void HandlerChopleBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddKebiaBerry(u32 *numHandlers);
static void HandlerKebiaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddShucaBerry(u32 *numHandlers);
static void HandlerShucaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddCobaBerry(u32 *numHandlers);
static void HandlerCobaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPayapaBerry(u32 *numHandlers);
static void HandlerPayapaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddTangaBerry(u32 *numHandlers);
static void HandlerTangaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddChartiBerry(u32 *numHandlers);
static void HandlerChartiBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddKasibBerry(u32 *numHandlers);
static void HandlerKasibBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddHabanBerry(u32 *numHandlers);
static void HandlerHabanBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddColburBerry(u32 *numHandlers);
static void HandlerColburBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddBabiriBerry(u32 *numHandlers);
static void HandlerBabiriBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddChilanBerry(u32 *numHandlers);
static void HandlerChilanBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static BOOL CommonResistBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work, u8 type,
                              BOOL anyEffectiveness);
static void HandlerCommonResistBerryDamageAfter(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerPinchReactionCommon(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerPinchReactionMemberIn(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void CommonDamageReact(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void CommonDamageReactCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, u32 ratio, BOOL checkContext);
static BOOL func_ov167_021c369c(BattleEventItem *item, BtlServerFlow *flow, u8 monId, u32 ratio);
static BOOL CommonDamageReactCheckCore(BtlServerFlow *flow, u8 monId, u32 ratio);
static const BattleEventHandlerEntry *EventAddCustapBerry(u32 *numHandlers);
static void HandlerCustapBerryPriorityCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerCustapBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMicleBerry(u32 *numHandlers);
static void HandlerMicleBerryReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMicleBerryActProcEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMicleBerryActionEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMicleBerryMemberIn(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMicleBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMicleBerryUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddJabocaBerry(u32 *numHandlers);
static void HandlerJabocaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRowapBerry(u32 *numHandlers);
static void HandlerRowapBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void CommonJabocaRowapBerryReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work,
                                           u32 category);
static void HandlerJabocaRowapBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddWhiteHerb(u32 *numHandlers);
static void HandlerWhiteHerbReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void func_ov167_021c3a74(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerWhiteHerbTurnCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerWhiteHerbUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerWhiteHerbUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMentalHerb(u32 *numHandlers);
static void HandlerMentalHerbReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMentalHerbUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMentalHerbUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddBrightPowder(u32 *numHandlers);
static void HandlerBrightPowder(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMachoBrace(u32 *numHandlers);
static void HandlerMachoBrace(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddQuickClaw(u32 *numHandlers);
static void HandlerQuickClawPriorityCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerQuickClawUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddLaggingTail(u32 *numHandlers);
static void HandlerLaggingTail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddKingsRock(u32 *numHandlers);
static void HandlerKingsRock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerKingsRockUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRazorFang(u32 *numHandlers);
static const BattleEventHandlerEntry *EventAddWideLens(u32 *numHandlers);
static void HandlerWideLens(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddScopeLens(u32 *numHandlers);
static void HandlerScopeLens(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddLuckyPunch(u32 *numHandlers);
static void HandlerLuckyPunch(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddStick(u32 *numHandlers);
static void HandlerStick(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddZoomLens(u32 *numHandlers);
static void HandlerZoomLens(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddLaxIncense(u32 *numHandlers);
static void HandlerLaxIncense(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMuscleBand(u32 *numHandlers);
static void HandlerMuscleBand(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddWiseGlasses(u32 *numHandlers);
static void HandlerWiseGlasses(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddDeepSeaTooth(u32 *numHandlers);
static void HandlerDeepSeaTooth(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddDeepSeaScale(u32 *numHandlers);
static void HandlerDeepSeaScale(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMetalPowder(u32 *numHandlers);
static void HandlerMetalPowder(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddQuickPowder(u32 *numHandlers);
static void HandlerQuickPowder(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSoulDew(u32 *numHandlers);
static void HandlerSoulDewAttacker(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerSoulDewDefender(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddThickClub(u32 *priority);
static void HandlerThickClub(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddChoiceBand(u32 *priority);
static void HandlerChoiceBandPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddChoiceSpecs(u32 *priority);
static void HandlerChoiceSpecsPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddChoiceScarf(u32 *priority);
static void HandlerChoiceScarf(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerChoiceItemCommonMoveLock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerChoiceItemCommonItemChange(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFocusSash(u32 *priority);
static void HandlerFocusSash(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerFocusSashUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFocusBand(u32 *priority);
static void HandlerFocusBandCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerFocusBand(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerFocusBandUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddExpertBelt(u32 *priority);
static void HandlerExpertBelt(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddLifeOrb(u32 *priority);
static void HandlerLifeOrbReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerLifeOrbPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMetronomeItem(u32 *priority);
static void HandlerMetronomeItem(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddGripClaw(u32 *priority);
static void HandlerGripClaw(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddShellBell(u32 *priority);
static void HandlerShellBell(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddLightClay(u32 *priority);
static void HandlerLightClay(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPowerHerb(u32 *priority);
static void HandlerPowerHerbCheckChargeSkip(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerPowerHerbFixChargeSkip(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerPowerHerbUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddLeftovers(u32 *priority);
static void HandlerLeftoversReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerLeftoversUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddBlackSludge(u32 *priority);
static void HandlerBlackSludge(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddDestinyKnot(u32 *priority);
static void HandlerDestinyKnot(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddStickyBarb(u32 *priority);
static void HandlerStickyBarbDamageReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerStickyBarbTurnCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPowerBracer(u32 *priority);
static const BattleEventHandlerEntry *EventAddPowerBelt(u32 *priority);
static const BattleEventHandlerEntry *EventAddPowerLens(u32 *priority);
static const BattleEventHandlerEntry *EventAddPowerBand(u32 *priority);
static const BattleEventHandlerEntry *EventAddPowerAnklet(u32 *priority);
static const BattleEventHandlerEntry *EventAddPowerWeight(u32 *priority);
static void HandlerPowerItemCalcSpeed(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFlamePlate(u32 *priority);
static const BattleEventHandlerEntry *EventAddSplashPlate(u32 *priority);
static const BattleEventHandlerEntry *EventAddZapPlate(u32 *priority);
static const BattleEventHandlerEntry *EventAddMeadowPlate(u32 *priority);
static const BattleEventHandlerEntry *EventAddIciclePlate(u32 *priority);
static const BattleEventHandlerEntry *EventAddFistPlate(u32 *priority);
static const BattleEventHandlerEntry *EventAddToxicPlate(u32 *priority);
static const BattleEventHandlerEntry *EventAddEarthPlate(u32 *priority);
static const BattleEventHandlerEntry *EventAddSkyPlate(u32 *priority);
static const BattleEventHandlerEntry *EventAddMindPlate(u32 *priority);
static const BattleEventHandlerEntry *EventAddInsectPlate(u32 *priority);
static const BattleEventHandlerEntry *EventAddStonePlate(u32 *priority);
static const BattleEventHandlerEntry *EventAddSpookyPlate(u32 *priority);
static const BattleEventHandlerEntry *EventAddDracoPlate(u32 *priority);
static const BattleEventHandlerEntry *EventAddDreadPlate(u32 *priority);
static const BattleEventHandlerEntry *EventAddIronPlate(u32 *priority);
static const BattleEventHandlerEntry *func_ov167_021c4b8c(u32 *priority);
static void HandlerPlate(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSmokeBall(u32 *priority);
static void HandlerSmokeBall(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerSmokeBallMessage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddAmuletCoin(u32 *priority);
static void HandlerAmuletCoin(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void func_ov167_021c4c48(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddGriseousOrb(u32 *priority);
static void HandlerGriseousOrb(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddIcyRock(u32 *priority);
static void HandlerIcyRock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSmoothRock(u32 *priority);
static void HandlerSmoothRock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddHeatRock(u32 *priority);
static void HandlerHeatRock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddDampRock(u32 *priority);
static void HandlerDampRock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void CommonWeatherMoveIncreaseTurns(BattleEventItem *item, BtlServerFlow *flow, u8 monId, u8 weather);
static const BattleEventHandlerEntry *EventAddLightBall(u32 *priority);
static void HandlerLightBall(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerLightBallUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddToxicOrb(u32 *priority);
static void HandlerToxicOrb(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerToxicOrbUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFlameOrb(u32 *priority);
static void HandlerFlameOrb(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerFlameOrbUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSilverPowder(u32 *priority);
static void HandlerSilverPowder(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSoftSand(u32 *priority);
static void HandlerSoftSand(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddHardStone(u32 *priority);
static void HandlerHardStone(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMiracleSeed(u32 *priority);
static void HandlerMiracleSeed(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddBlackGlasses(u32 *priority);
static void HandlerBlackGlasses(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddBlackBelt(u32 *priority);
static void HandlerBlackBelt(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMagnet(u32 *priority);
static void HandlerMagnet(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMetalCoat(u32 *priority);
static void HandlerMetalCoat(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMysticWater(u32 *priority);
static void HandlerMysticWater(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSharpBeak(u32 *priority);
static void HandlerSharpBeak(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPoisonBarb(u32 *priority);
static void HandlerPoisonBarb(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerPoisonBarbUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddNeverMeltIce(u32 *priority);
static void HandlerNeverMeltIce(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSpellTag(u32 *priority);
static void HandlerSpellTag(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddTwistedSpoon(u32 *priority);
static void HandlerTwistedSpoon(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddCharcoal(u32 *priority);
static void HandlerCharcoal(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddDragonFang(u32 *priority);
static void HandlerDragonFang(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSilkScarf(u32 *priority);
static void HandlerSilkScarf(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddOddIncense(u32 *priority);
static void HandlerOddIncense(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRockIncense(u32 *priority);
static void HandlerRockIncense(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddWaveIncense(u32 *priority);
static void HandlerWaveIncense(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSeaIncense(u32 *priority);
static void HandlerSeaIncense(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRoseIncense(u32 *priority);
static void HandlerRoseIncense(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void CommonTypeBoostingItem(BattleEventItem *item, BtlServerFlow *flow, u8 monId, u8 type);
static const BattleEventHandlerEntry *EventAddLustrousOrb(u32 *priority);
static void HandlerLustrousOrb(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddAdamantOrb(u32 *priority);
static void HandlerAdamantOrb(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddIronBall(u32 *priority);
static void HandlerIronBallCalcSpeed(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerIronBallCheckFly(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFloatStone(u32 *priority);
static void HandlerFloatStone(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddEviolite(u32 *priority);
static void HandlerEviolite(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRockyHelmet(u32 *priority);
static void HandlerRockyHelmet(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddAirBalloon(u32 *priority);
static void HandlerAirBalloonMemberIn(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerAirBalloonCheckFlying(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerAirBalloonDamageReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRedCard(u32 *priority);
static void HandlerRedCard(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRingTarget(u32 *priority);
static void HandlerRingTarget(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddBindingBand(u32 *priority);
static void HandlerBindingBand(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddAbsorbBulb(u32 *priority);
static void HandlerAbsorbBulbDamageReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerAbsorbBulbUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddCellBattery(u32 *priority);
static void HandlerCellBatteryDamageReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerCellBatteryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddEjectButton(u32 *priority);
static void HandlerEjectButtonReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerEjectButtonUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFireGem(u32 *priority);
static void HandlerFireGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerFireGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddWaterGem(u32 *priority);
static void HandlerWaterGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerWaterGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddElectricGem(u32 *priority);
static void HandlerElectricGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerElectricGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddGrassGem(u32 *priority);
static void HandlerGrassGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerGrassGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddIceGem(u32 *priority);
static void HandlerIceGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerIceGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFightingGem(u32 *priority);
static void HandlerFightingGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerFightingGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPoisonGem(u32 *priority);
static void HandlerPoisonGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerPoisonGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddGroundGem(u32 *priority);
static void HandlerGroundGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerGroundGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFlyingGem(u32 *priority);
static void HandlerFlyingGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerFlyingGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPsychicGem(u32 *priority);
static void HandlerPsychicGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerPsychicGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddBugGem(u32 *priority);
static void HandlerBugGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerBugGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRockGem(u32 *priority);
static void HandlerRockGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerRockGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddGhostGem(u32 *priority);
static void HandlerGhostGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerGhostGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddDragonGem(u32 *priority);
static void HandlerDragonGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerDragonGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddDarkGem(u32 *priority);
static void HandlerDarkGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerDarkGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSteelGem(u32 *priority);
static void HandlerSteelGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerSteelGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddNormalGem(u32 *priority);
static void HandlerNormalGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerNormalGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void CommonGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work, u8 type);
static void CommonGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work, u8 type);
static void HandlerGemEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);

static const ItemEventAddEntry sItemEventAddTable[] = {
    { ITEM_CHERI_BERRY, EventAddCheriBerry },
    { ITEM_CHESTO_BERRY, EventAddChestoBerry },
    { ITEM_RAWST_BERRY, EventAddRawstBerry },
    { ITEM_ASPEAR_BERRY, EventAddAspearBerry },
    { ITEM_PERSIM_BERRY, EventAddPersimBerry },
    { ITEM_PECHA_BERRY, EventAddPechaBerry },
    { ITEM_LUM_BERRY, EventAddLumBerry },
    { ITEM_LEPPA_BERRY, EventAddLeppaBerry },
    { ITEM_ORAN_BERRY, EventAddOranBerry },
    { ITEM_BERRY_JUICE, EventAddBerryJuice },
    { ITEM_SITRUS_BERRY, EventAddSitrusBerry },
    { ITEM_FIGY_BERRY, EventAddFigyBerry },
    { ITEM_WIKI_BERRY, EventAddWikiBerry },
    { ITEM_MAGO_BERRY, EventAddMagoBerry },
    { ITEM_AGUAV_BERRY, EventAddAguavBerry },
    { ITEM_IAPAPA_BERRY, EventAddIapapaBerry },
    { ITEM_LIECHI_BERRY, EventAddLiechiBerry },
    { ITEM_GANLON_BERRY, EventAddGanlonBerry },
    { ITEM_SALAC_BERRY, EventAddSalacBerry },
    { ITEM_PETAYA_BERRY, EventAddPetayaBerry },
    { ITEM_APICOT_BERRY, EventAddApicotBerry },
    { ITEM_LANSAT_BERRY, EventAddLansatBerry },
    { ITEM_STARF_BERRY, EventAddStarfBerry },
    { ITEM_ENIGMA_BERRY, EventAddEnigmaBerry },
    { ITEM_OCCA_BERRY, EventAddOccaBerry },
    { ITEM_PASSHO_BERRY, EventAddPasshoBerry },
    { ITEM_WACAN_BERRY, EventAddWacanBerry },
    { ITEM_RINDO_BERRY, EventAddRindoBerry },
    { ITEM_YACHE_BERRY, EventAddYacheBerry },
    { ITEM_CHOPLE_BERRY, EventAddChopleBerry },
    { ITEM_KEBIA_BERRY, EventAddKebiaBerry },
    { ITEM_SHUCA_BERRY, EventAddShucaBerry },
    { ITEM_COBA_BERRY, EventAddCobaBerry },
    { ITEM_PAYAPA_BERRY, EventAddPayapaBerry },
    { ITEM_TANGA_BERRY, EventAddTangaBerry },
    { ITEM_CHARTI_BERRY, EventAddChartiBerry },
    { ITEM_KASIB_BERRY, EventAddKasibBerry },
    { ITEM_HABAN_BERRY, EventAddHabanBerry },
    { ITEM_COLBUR_BERRY, EventAddColburBerry },
    { ITEM_BABIRI_BERRY, EventAddBabiriBerry },
    { ITEM_CHILAN_BERRY, EventAddChilanBerry },
    { ITEM_CUSTAP_BERRY, EventAddCustapBerry },
    { ITEM_MICLE_BERRY, EventAddMicleBerry },
    { ITEM_JABOCA_BERRY, EventAddJabocaBerry },
    { ITEM_ROWAP_BERRY, EventAddRowapBerry },
    { ITEM_WHITE_HERB, EventAddWhiteHerb },
    { ITEM_MENTAL_HERB, EventAddMentalHerb },
    { ITEM_BRIGHT_POWDER, EventAddBrightPowder },
    { ITEM_MACHO_BRACE, EventAddMachoBrace },
    { ITEM_QUICK_CLAW, EventAddQuickClaw },
    { ITEM_LAGGING_TAIL, EventAddLaggingTail },
    { ITEM_FULL_INCENSE, EventAddLaggingTail },
    { ITEM_KINGS_ROCK, EventAddKingsRock },
    { ITEM_RAZOR_CLAW, EventAddScopeLens },
    { ITEM_WIDE_LENS, EventAddWideLens },
    { ITEM_SCOPE_LENS, EventAddScopeLens },
    { ITEM_ZOOM_LENS, EventAddZoomLens },
    { ITEM_LAX_INCENSE, EventAddLaxIncense },
    { ITEM_MUSCLE_BAND, EventAddMuscleBand },
    { ITEM_WISE_GLASSES, EventAddWiseGlasses },
    { ITEM_DEEP_SEA_TOOTH, EventAddDeepSeaTooth },
    { ITEM_DEEP_SEA_SCALE, EventAddDeepSeaScale },
    { ITEM_METAL_POWDER, EventAddMetalPowder },
    { ITEM_QUICK_POWDER, EventAddQuickPowder },
    { ITEM_LIGHT_BALL, EventAddLightBall },
    { ITEM_LUCKY_PUNCH, EventAddLuckyPunch },
    { ITEM_STICK, EventAddStick },
    { ITEM_SOUL_DEW, EventAddSoulDew },
    { ITEM_THICK_CLUB, EventAddThickClub },
    { ITEM_CHOICE_BAND, EventAddChoiceBand },
    { ITEM_BLACK_SLUDGE, EventAddBlackSludge },
    { ITEM_CHOICE_SPECS, EventAddChoiceSpecs },
    { ITEM_CHOICE_SCARF, EventAddChoiceScarf },
    { ITEM_SILVER_POWDER, EventAddSilverPowder },
    { ITEM_SOFT_SAND, EventAddSoftSand },
    { ITEM_HARD_STONE, EventAddHardStone },
    { ITEM_MIRACLE_SEED, EventAddMiracleSeed },
    { ITEM_BLACK_GLASSES, EventAddBlackGlasses },
    { ITEM_BLACK_BELT, EventAddBlackBelt },
    { ITEM_MAGNET, EventAddMagnet },
    { ITEM_METAL_COAT, EventAddMetalCoat },
    { ITEM_MYSTIC_WATER, EventAddMysticWater },
    { ITEM_SHARP_BEAK, EventAddSharpBeak },
    { ITEM_RAZOR_FANG, EventAddRazorFang },
    { ITEM_POISON_BARB, EventAddPoisonBarb },
    { ITEM_NEVER_MELT_ICE, EventAddNeverMeltIce },
    { ITEM_SPELL_TAG, EventAddSpellTag },
    { ITEM_TWISTED_SPOON, EventAddTwistedSpoon },
    { ITEM_CHARCOAL, EventAddCharcoal },
    { ITEM_DRAGON_FANG, EventAddDragonFang },
    { ITEM_SILK_SCARF, EventAddSilkScarf },
    { ITEM_ODD_INCENSE, EventAddOddIncense },
    { ITEM_ROCK_INCENSE, EventAddRockIncense },
    { ITEM_WAVE_INCENSE, EventAddWaveIncense },
    { ITEM_SEA_INCENSE, EventAddSeaIncense },
    { ITEM_ROSE_INCENSE, EventAddRoseIncense },
    { ITEM_FOCUS_SASH, EventAddFocusSash },
    { ITEM_FOCUS_BAND, EventAddFocusBand },
    { ITEM_EXPERT_BELT, EventAddExpertBelt },
    { ITEM_LIFE_ORB, EventAddLifeOrb },
    { ITEM_METRONOME, EventAddMetronomeItem },
    { ITEM_GRIP_CLAW, EventAddGripClaw },
    { ITEM_SHELL_BELL, EventAddShellBell },
    { ITEM_LIGHT_CLAY, EventAddLightClay },
    { ITEM_POWER_HERB, EventAddPowerHerb },
    { ITEM_LEFTOVERS, EventAddLeftovers },
    { ITEM_TOXIC_ORB, EventAddToxicOrb },
    { ITEM_FLAME_ORB, EventAddFlameOrb },
    { ITEM_LUSTROUS_ORB, EventAddLustrousOrb },
    { ITEM_ADAMANT_ORB, EventAddAdamantOrb },
    { ITEM_IRON_BALL, EventAddIronBall },
    { ITEM_DESTINY_KNOT, EventAddDestinyKnot },
    { ITEM_ICY_ROCK, EventAddIcyRock },
    { ITEM_SMOOTH_ROCK, EventAddSmoothRock },
    { ITEM_HEAT_ROCK, EventAddHeatRock },
    { ITEM_DAMP_ROCK, EventAddDampRock },
    { ITEM_STICKY_BARB, EventAddStickyBarb },
    { ITEM_POWER_BRACER, EventAddPowerBracer },
    { ITEM_POWER_BELT, EventAddPowerBelt },
    { ITEM_POWER_LENS, EventAddPowerLens },
    { ITEM_POWER_BAND, EventAddPowerBand },
    { ITEM_POWER_ANKLET, EventAddPowerAnklet },
    { ITEM_POWER_WEIGHT, EventAddPowerWeight },
    { ITEM_FLAME_PLATE, EventAddFlamePlate },
    { ITEM_SPLASH_PLATE, EventAddSplashPlate },
    { ITEM_ZAP_PLATE, EventAddZapPlate },
    { ITEM_MEADOW_PLATE, EventAddMeadowPlate },
    { ITEM_ICICLE_PLATE, EventAddIciclePlate },
    { ITEM_FIST_PLATE, EventAddFistPlate },
    { ITEM_TOXIC_PLATE, EventAddToxicPlate },
    { ITEM_EARTH_PLATE, EventAddEarthPlate },
    { ITEM_SKY_PLATE, EventAddSkyPlate },
    { ITEM_MIND_PLATE, EventAddMindPlate },
    { ITEM_INSECT_PLATE, EventAddInsectPlate },
    { ITEM_STONE_PLATE, EventAddStonePlate },
    { ITEM_SPOOKY_PLATE, EventAddSpookyPlate },
    { ITEM_DRACO_PLATE, EventAddDracoPlate },
    { ITEM_DREAD_PLATE, EventAddDreadPlate },
    { ITEM_IRON_PLATE, EventAddIronPlate },
    { ITEM_BIG_ROOT, func_ov167_021c4b8c },
    { ITEM_SMOKE_BALL, EventAddSmokeBall },
    { ITEM_AMULET_COIN, EventAddAmuletCoin },
    { ITEM_LUCK_INCENSE, EventAddAmuletCoin },
    { ITEM_GRISEOUS_ORB, EventAddGriseousOrb },
    { ITEM_FLOAT_STONE, EventAddFloatStone },
    { ITEM_EVIOLITE, EventAddEviolite },
    { ITEM_ROCKY_HELMET, EventAddRockyHelmet },
    { ITEM_AIR_BALLOON, EventAddAirBalloon },
    { ITEM_RED_CARD, EventAddRedCard },
    { ITEM_RING_TARGET, EventAddRingTarget },
    { ITEM_BINDING_BAND, EventAddBindingBand },
    { ITEM_ABSORB_BULB, EventAddAbsorbBulb },
    { ITEM_CELL_BATTERY, EventAddCellBattery },
    { ITEM_EJECT_BUTTON, EventAddEjectButton },
    { ITEM_FIRE_GEM, EventAddFireGem },
    { ITEM_WATER_GEM, EventAddWaterGem },
    { ITEM_ELECTRIC_GEM, EventAddElectricGem },
    { ITEM_GRASS_GEM, EventAddGrassGem },
    { ITEM_ICE_GEM, EventAddIceGem },
    { ITEM_FIGHTING_GEM, EventAddFightingGem },
    { ITEM_POISON_GEM, EventAddPoisonGem },
    { ITEM_GROUND_GEM, EventAddGroundGem },
    { ITEM_FLYING_GEM, EventAddFlyingGem },
    { ITEM_PSYCHIC_GEM, EventAddPsychicGem },
    { ITEM_BUG_GEM, EventAddBugGem },
    { ITEM_ROCK_GEM, EventAddRockGem },
    { ITEM_GHOST_GEM, EventAddGhostGem },
    { ITEM_DRAGON_GEM, EventAddDragonGem },
    { ITEM_DARK_GEM, EventAddDarkGem },
    { ITEM_STEEL_GEM, EventAddSteelGem },
    { ITEM_NORMAL_GEM, EventAddNormalGem },
    { ITEM_NONE, NULL },
};

// Function names from swan.

static BattleEventItem *ItemEvent_AddItemCore(BattleMon *mon, u16 itemId) {
    u32 i;
    u16 subPriority;
    u8 monId;
    const BattleEventHandlerEntry *handlers;
    u32 numHandlers;

    for (i = 0; i < 0xac; i++) {
        if (itemId == sItemEventAddTable[i].item) {
            subPriority = RawBattleMonStat(mon, 12);
            monId = GetMonID(mon);
            handlers = sItemEventAddTable[i].eventAdd(&numHandlers);
            return BattleEvent_AddItem(5, itemId, 6, subPriority, monId, handlers, numHandlers);
        }
    }
    return NULL;
}

void ItemEvent_RemoveItem(BattleMon *mon) {
    BattleEventItem *item;
    BattleEventItem *next;

    item = BattleEvent_SeekItem(5, GetMonID(mon));
    while (item != NULL) {
        next = BattleEvent_GetNextItem(item);
        if (BattleEventItem_GetWorkValue(item, 6) == 0) {
            BattleEventItem_Remove(item);
        }
        item = next;
    }
}

BattleEventItem *ItemEvent_AddItem(BattleMon *mon) {
    u32 itemId;

    itemId = GetBattleMonHeldItem(mon);
    if (itemId != 0) {
        return ItemEvent_AddItemCore(mon, itemId);
    }
    return NULL;
}

BattleEventItem *ItemEvent_TempAdd(BattleMon *mon, u16 itemId) {
    BattleEventItem *item;

    if (itemId != 0) {
        item = ItemEvent_AddItemCore(mon, itemId);
        if (item != NULL) {
            BattleEventItem_SetTempItemFlag(item);
            BattleEventItem_SetWorkValue(item, 6, 1);
            return item;
        }
    }
    return NULL;
}

void func_ov167_021c27c4(void *temp) {
    BattleEventItem_Remove(temp);
}

void ItemEvent_ItemRotationSleep(BattleMon *mon) {
    BattleEvent_ItemRotationSleep(GetMonID(mon), 5);
}

void ItemEvent_ItemRotationWake(BattleMon *mon) {
    if (!BattleEvent_ItemRotationWake(GetMonID(mon), 5)) {
        ItemEvent_AddItem(mon);
    }
}

static BOOL ItemEvent_RollEffectChance(BtlServerFlow *flow, u8 chance) {
    if (RollEffectChance(chance)) {
        return TRUE;
    }
    if (func_ov167_021abdf8(flow, 2)) {
        return TRUE;
    }
    return FALSE;
}

static u32 CommonGetItemParam(BattleEventItem *item, u32 param) {
    return ItemGetParam(BattleEventItem_GetSubID(item), param);
}

static fx32 ItemAttackValueToRatio(BattleEventItem *item) {
    s32 value;

    value = CommonGetItemParam(item, 2) + 100;
    return FX32_CONST(value) / 100;
}

static void ItemEvent_PushRun(BattleEventItem *item, BtlServerFlow *flow, u8 monId) {
    BattleEventItem_SetRecallEnable(item);
    BattleHandler_PushRun(flow, 0, monId);
}

static void HandlerCheriBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonStatusReaction(item, flow, monId, 1);
}

static void HandlerCherryBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonUseForStatus(item, flow, monId, 1);
}

static const BattleEventHandlerEntry sHandlersCheriBerry[] = {
    { 0x55, HandlerCheriBerry },     { 0x91, HandlerCheriBerry },     { 0x2, HandlerCheriBerry },
    { 0x72, HandlerCherryBerryUse }, { 0x73, HandlerCherryBerryUse },
};

static const BattleEventHandlerEntry *EventAddCheriBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersCheriBerry);
    return sHandlersCheriBerry;
}

static void HandlerChestoBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonStatusReaction(item, flow, monId, 2);
}

static void HandlerChestoBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonUseForStatus(item, flow, monId, 2);
}

static const BattleEventHandlerEntry sHandlersChestoBerry[] = {
    { 0x55, HandlerChestoBerry },    { 0x91, HandlerChestoBerry },    { 0x2, HandlerChestoBerry },
    { 0x72, HandlerChestoBerryUse }, { 0x73, HandlerChestoBerryUse },
};

static const BattleEventHandlerEntry *EventAddChestoBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersChestoBerry);
    return sHandlersChestoBerry;
}

static void HandlerRawstBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonStatusReaction(item, flow, monId, 4);
}

static void HandlerRawstBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonUseForStatus(item, flow, monId, 4);
}

static const BattleEventHandlerEntry sHandlersRawstBerry[] = {
    { 0x55, HandlerRawstBerry },    { 0x91, HandlerRawstBerry },    { 0x2, HandlerRawstBerry },
    { 0x72, HandlerRawstBerryUse }, { 0x73, HandlerRawstBerryUse },
};

static const BattleEventHandlerEntry *EventAddRawstBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersRawstBerry);
    return sHandlersRawstBerry;
}

static void HandlerAspearBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonStatusReaction(item, flow, monId, 3);
}

static void HandlerAspearBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonUseForStatus(item, flow, monId, 3);
}

static const BattleEventHandlerEntry sHandlersAspearBerry[] = {
    { 0x55, HandlerAspearBerry },    { 0x91, HandlerAspearBerry },    { 0x2, HandlerAspearBerry },
    { 0x72, HandlerAspearBerryUse }, { 0x73, HandlerAspearBerryUse },
};

static const BattleEventHandlerEntry *EventAddAspearBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersAspearBerry);
    return sHandlersAspearBerry;
}

static void HandlerPersimBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonStatusReaction(item, flow, monId, 6);
}

static void HandlerPersimBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonUseForStatus(item, flow, monId, 6);
}

static const BattleEventHandlerEntry sHandlersPersimBerry[] = {
    { 0x55, HandlerPersimBerry },    { 0x91, HandlerPersimBerry },    { 0x2, HandlerPersimBerry },
    { 0x72, HandlerPersimBerryUse }, { 0x73, HandlerPersimBerryUse },
};

static const BattleEventHandlerEntry *EventAddPersimBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersPersimBerry);
    return sHandlersPersimBerry;
}

static void HandlerPechaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonStatusReaction(item, flow, monId, 5);
}

static void HandlerPechaBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonUseForStatus(item, flow, monId, 5);
}

static const BattleEventHandlerEntry sHandlersPechaBerry[] = {
    { 0x55, HandlerPechaBerry },    { 0x91, HandlerPechaBerry },    { 0x2, HandlerPechaBerry },
    { 0x72, HandlerPechaBerryUse }, { 0x73, HandlerPechaBerryUse },
};

static const BattleEventHandlerEntry *EventAddPechaBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersPechaBerry);
    return sHandlersPechaBerry;
}

static void HandlerLumBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonStatusReaction(item, flow, monId, 0x25);
}

static void HandlerLumBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonUseForStatus(item, flow, monId, 0x25);
}

static const BattleEventHandlerEntry sHandlersLumBerry[] = {
    { 0x55, HandlerLumBerry },    { 0x91, HandlerLumBerry },    { 0x2, HandlerLumBerry },
    { 0x72, HandlerLumBerryUse }, { 0x73, HandlerLumBerryUse },
};

static const BattleEventHandlerEntry *EventAddLumBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersLumBerry);
    return sHandlersLumBerry;
}

static void CommonStatusReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, u32 condition) {
    s32 context;

    if (BattleEventVar_GetValue(2) == monId) {
        if (!func_ov167_021bcfa0(0x2e, &context) || context == 3 || context == 0) {
            if (CommonConditionCodeMatch(flow, monId, condition)) {
                ItemEvent_PushRun(item, flow, monId);
            }
        }
    }
}

static void CommonUseForStatus(BattleEventItem *item, BtlServerFlow *flow, u8 monId, u32 condition) {
    BattleHandlerCureConditionParam *param;

    if (BattleEventVar_GetValue(2) == monId && CommonConditionCodeMatch(flow, monId, condition)) {
        param = BattleHandler_PushWork(flow, 0xb, monId);
        param->condition = condition;
        param->count = 1;
        param->monIds[0] = monId;
        BattleHandler_PopWork(flow, param);
    }
}

static BOOL CommonConditionCodeMatch(BtlServerFlow *flow, u8 monId, u32 condition) {
    BattleMon *mon;

    mon = GetBattleMon(flow, monId);
    switch (condition) {
    case 0x25:
        if (GetBattleMonStatus(mon) != 0 || CheckCondition(mon, 6)) {
            return TRUE;
        }
        break;
    case 0x24:
        if (GetBattleMonStatus(mon) != 0) {
            return TRUE;
        }
        return FALSE;
    case 0x26:
        if (func_ov167_021bd624(mon)) {
            return TRUE;
        }
        return FALSE;
    default:
        return CheckCondition(mon, condition);
    }
    return FALSE;
}

static const BattleEventHandlerEntry sHandlersLeppaBerry[] = {
    { 0x4, HandlerLeppaBerryMoveEnd },  { 0x91, HandlerLeppaBerryReaction }, { 0x55, HandlerLeppaBerryGet },
    { 0x9d, HandlerLeppaBerryGet },     { 0x4f, HandlerLeppaBerryPPUsed },   { 0x72, HandlerLeppaBerryUse },
    { 0x73, HandlerLeppaBerryUseTemp },
};

static const BattleEventHandlerEntry *EventAddLeppaBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersLeppaBerry);
    return sHandlersLeppaBerry;
}

static void HandlerLeppaBerryMoveEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        if (CommonLeppaBerryLastMoveID(flow, monId) != 4) {
            ItemEvent_PushRun(item, flow, monId);
        }
    }
}

static void HandlerLeppaBerryReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 context;

    if (BattleEventVar_GetValue(2) == monId) {
        context = BattleEventVar_GetValue(0x2e);
        if (context == 2 || context == 0) {
            if (CommonLeppaBerryEnableMoveID(flow, monId) != 4) {
                ItemEvent_PushRun(item, flow, monId);
            }
        }
    }
}

static void HandlerLeppaBerryGet(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        if (CommonLeppaBerryEnableMoveID(flow, monId) != 4) {
            ItemEvent_PushRun(item, flow, monId);
        }
    }
}

static void HandlerLeppaBerryPPUsed(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        if (CommonLeppaBerryLastMoveID(flow, monId) != 4) {
            BattleEventVar_RewriteValue(0x51, 1);
        }
    }
}

static void HandlerLeppaBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        HandlerLeppaBerryCommon(item, flow, monId, work, FALSE);
    }
}

static void HandlerLeppaBerryUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (work[6] != 0) {
        HandlerLeppaBerryCommon(item, flow, monId, work, TRUE);
    }
}

static u8 CommonLeppaBerryLastMoveID(BtlServerFlow *flow, u8 monId) {
    BattleMon *mon;
    u8 moveIndex;

    mon = GetBattleMon(flow, monId);
    moveIndex = func_ov167_021baf78(mon, GetPreviousMoveUsed(mon));
    if (moveIndex != 4 && GetMovePP(mon, moveIndex) == 0) {
        return moveIndex;
    }
    return 4;
}

static u8 CommonLeppaBerryEnableMoveID(BtlServerFlow *flow, u8 monId) {
    BattleMon *mon;
    u32 count;
    u32 i;

    mon = GetBattleMon(flow, monId);
    count = GetBattleMonMoveCount(mon);
    for (i = 0; i < count; i++) {
        if (GetMovePP(mon, i) == 0) {
            return i;
        }
    }
    return 4;
}

static BOOL HandlerLeppaBerryCommon(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work, BOOL anyUsed) {
    BattleMon *mon;
    BattleHandlerPPParam *param;
    u32 count;
    u8 moveIndex;
    u32 i;

    mon = GetBattleMon(flow, monId);
    moveIndex = CommonLeppaBerryLastMoveID(flow, monId);
    if (moveIndex == 4) {
        moveIndex = CommonLeppaBerryEnableMoveID(flow, monId);
        if (moveIndex == 4 && anyUsed) {
            count = GetBattleMonMoveCount(mon);
            for (i = 0; i < count; i++) {
                if (GetMovePPUsed(mon, i) != 0) {
                    moveIndex = i;
                    break;
                }
            }
        }
    }
    if (moveIndex != 4) {
        param = BattleHandler_PushWork(flow, 9, monId);
        param->amount = 10;
        param->monIndex = monId;
        param->moveIndex = moveIndex;
        param->currentMoves = 1;
        BattleHandler_StrSetup(&param->string, 2, 0x38f);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
        BattleHandler_AddArg(&param->string, MoveGetID(mon, moveIndex));
        BattleHandler_PopWork(flow, param);
        return TRUE;
    }
    return FALSE;
}

static const BattleEventHandlerEntry sHandlersOranBerry[] = {
    { 0x91, HandlerOranBerryReaction }, { 0x55, HandlerOranBerryMemberIn }, { 0x58, HandlerOranBerryCheckActivation },
    { 0x72, HandlerOranBerryUse },      { 0x73, HandlerOranBerryUse },
};

static const BattleEventHandlerEntry *EventAddOranBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersOranBerry);
    return sHandlersOranBerry;
}

static void HandlerOranBerryReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (!CheckCondition(GetBattleMon(flow, monId), 0xf)) {
        CommonDamageReactCheck(item, flow, monId, 2, TRUE);
    }
}

static void HandlerOranBerryMemberIn(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (!CheckCondition(GetBattleMon(flow, monId), 0xf)) {
        CommonDamageReactCheck(item, flow, monId, 2, FALSE);
    }
}

static void HandlerOranBerryCheckActivation(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (!CheckCondition(GetBattleMon(flow, monId), 0xf) && CommonDamageReactCheckCore(flow, monId, 2)) {
        ItemEvent_PushRun(item, flow, monId);
    }
}

static void HandlerOranBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerRecoverHPParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        param = BattleHandler_PushWork(flow, 5, monId);
        param->targetIndex = monId;
        param->amount = CommonGetItemParam(item, 2);
        BattleHandler_StrSetup(&param->string, 2, 0x38c);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersBerryJuice[] = {
    { 0x91, HandlerOranBerryReaction },
    { 0x55, HandlerOranBerryMemberIn },
    { 0x58, HandlerOranBerryCheckActivation },
    { 0x72, HandlerOranBerryUse },
};

static const BattleEventHandlerEntry *EventAddBerryJuice(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersBerryJuice);
    return sHandlersBerryJuice;
}

static const BattleEventHandlerEntry sHandlersSitrusBerry[] = {
    { 0x91, HandlerOranBerryReaction }, { 0x55, HandlerOranBerryMemberIn }, { 0x58, HandlerOranBerryCheckActivation },
    { 0x72, HandlerSitrusBerryUse },    { 0x73, HandlerSitrusBerryUse },
};

static const BattleEventHandlerEntry *EventAddSitrusBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersSitrusBerry);
    return sHandlersSitrusBerry;
}

static void HandlerSitrusBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerRecoverHPParam *param;
    u32 maxHp;
    u8 ratio;

    if (BattleEventVar_GetValue(2) == monId) {
        maxHp = GetBattleMonStat(GetBattleMon(flow, monId), 0xe);
        ratio = CommonGetItemParam(item, 2);
        param = BattleHandler_PushWork(flow, 5, monId);
        param->targetIndex = monId;
        param->amount = maxHp * ratio / 100;
        BattleHandler_StrSetup(&param->string, 2, 0x38c);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersFigyBerry[] = {
    { 0x91, HandlerCommonPinchBerryReaction },
    { 0x55, HandlerCommonPinchBerryMemberIn },
    { 0x58, HandlerCommonPinchBerryCheckActivation },
    { 0x72, HandlerFigyBerryUse },
    { 0x73, HandlerFigyBerryUseTemp },
};

static const BattleEventHandlerEntry *EventAddFigyBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersFigyBerry);
    return sHandlersFigyBerry;
}

static void HandlerFigyBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonPinchBerry(item, flow, monId, work, 0);
}

static void HandlerFigyBerryUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (work[6] != 0) {
        CommonPinchBerry(item, flow, monId, work, 0);
    }
}

static const BattleEventHandlerEntry sHandlersWikiBerry[] = {
    { 0x91, HandlerCommonPinchBerryReaction },
    { 0x55, HandlerCommonPinchBerryMemberIn },
    { 0x58, HandlerCommonPinchBerryCheckActivation },
    { 0x72, HandlerWikiBerryUse },
    { 0x73, HandlerWikiBerryUseTemp },
};

static const BattleEventHandlerEntry *EventAddWikiBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersWikiBerry);
    return sHandlersWikiBerry;
}

static void HandlerWikiBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonPinchBerry(item, flow, monId, work, 1);
}

static void HandlerWikiBerryUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (work[6] != 0) {
        CommonPinchBerry(item, flow, monId, work, 1);
    }
}

static const BattleEventHandlerEntry sHandlersMagoBerry[] = {
    { 0x91, HandlerCommonPinchBerryReaction },
    { 0x55, HandlerCommonPinchBerryMemberIn },
    { 0x58, HandlerCommonPinchBerryCheckActivation },
    { 0x72, HandlerMagoBerryUse },
    { 0x73, HandlerMagoBerryUseTemp },
};

static const BattleEventHandlerEntry *EventAddMagoBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersMagoBerry);
    return sHandlersMagoBerry;
}

static void HandlerMagoBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonPinchBerry(item, flow, monId, work, 2);
}

static void HandlerMagoBerryUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (work[6] != 0) {
        CommonPinchBerry(item, flow, monId, work, 2);
    }
}

static const BattleEventHandlerEntry sHandlersAguavBerry[] = {
    { 0x91, HandlerCommonPinchBerryReaction },
    { 0x55, HandlerCommonPinchBerryMemberIn },
    { 0x58, HandlerCommonPinchBerryCheckActivation },
    { 0x72, HandlerAguavBerryUse },
    { 0x73, HandlerAguavBerryUseTemp },
};

static const BattleEventHandlerEntry *EventAddAguavBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersAguavBerry);
    return sHandlersAguavBerry;
}

static void HandlerAguavBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonPinchBerry(item, flow, monId, work, 3);
}

static void HandlerAguavBerryUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (work[6] != 0) {
        CommonPinchBerry(item, flow, monId, work, 3);
    }
}

static const BattleEventHandlerEntry sHandlersIapapaBerry[] = {
    { 0x91, HandlerCommonPinchBerryReaction },
    { 0x55, HandlerCommonPinchBerryMemberIn },
    { 0x58, HandlerCommonPinchBerryCheckActivation },
    { 0x72, HandlerIapapaBerryUse },
    { 0x73, HandlerIapapaBerryUseTemp },
};

static const BattleEventHandlerEntry *EventAddIapapaBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersIapapaBerry);
    return sHandlersIapapaBerry;
}

static void HandlerIapapaBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonPinchBerry(item, flow, monId, work, 4);
}

static void HandlerIapapaBerryUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (work[6] != 0) {
        CommonPinchBerry(item, flow, monId, work, 4);
    }
}

static void HandlerCommonPinchBerryReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (work[0] == 0 && !CheckCondition(GetBattleMon(flow, monId), 0xf)) {
        CommonDamageReactCheck(item, flow, monId, 2, TRUE);
    }
}

static void HandlerCommonPinchBerryMemberIn(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (work[0] == 0 && !CheckCondition(GetBattleMon(flow, monId), 0xf)) {
        CommonDamageReactCheck(item, flow, monId, 2, FALSE);
    }
}

static void HandlerCommonPinchBerryCheckActivation(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (work[0] == 0 && !CheckCondition(GetBattleMon(flow, monId), 0xf) && CommonDamageReactCheckCore(flow, monId, 2)) {
        ItemEvent_PushRun(item, flow, monId);
    }
}

static void CommonPinchBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work, u32 flavor) {
    BattleMon *mon;
    BattleHandlerRecoverHPParam *param;
    BattleHandlerAddConditionParam *confuse;

    if (work[0] == 0 && BattleEventVar_GetValue(2) == monId) {
        mon = GetBattleMon(flow, monId);
        param = BattleHandler_PushWork(flow, 5, monId);
        param->targetIndex = monId;
        param->amount = DivideMaxHPZeroCheck(mon, CommonGetItemParam(item, 2));
        BattleHandler_StrSetup(&param->string, 2, 0x38c);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
        BattleHandler_PopWork(flow, param);
        if (doesNatureAffectStat(GetSrcData(mon), flavor) == -1) {
            confuse = BattleHandler_PushWork(flow, 0xc, monId);
            confuse->targetIndex = monId;
            confuse->condition = 6;
            func_ov167_021bd5d4(6, mon, &confuse->value);
            BattleHandler_PopWork(flow, confuse);
        }
        work[0] = 1;
    }
}

static const BattleEventHandlerEntry sHandlersLiechiBerry[] = {
    { 0x91, HandlerPinchReactionCommon }, { 0x55, HandlerPinchReactionMemberIn }, { 0x58, CommonDamageReact },
    { 0x72, HandlerLiechiBerry },         { 0x73, HandlerLiechiBerry },
};

static const BattleEventHandlerEntry *EventAddLiechiBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersLiechiBerry);
    return sHandlersLiechiBerry;
}

static void HandlerLiechiBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonStatBoostBerry(flow, monId, 1, 1);
}

static const BattleEventHandlerEntry sHandlersGanlonBerry[] = {
    { 0x91, HandlerPinchReactionCommon }, { 0x55, HandlerPinchReactionMemberIn }, { 0x58, CommonDamageReact },
    { 0x72, HandlerGanlonBerry },         { 0x73, HandlerGanlonBerry },
};

static const BattleEventHandlerEntry *EventAddGanlonBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersGanlonBerry);
    return sHandlersGanlonBerry;
}

static void HandlerGanlonBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonStatBoostBerry(flow, monId, 2, 1);
}

static const BattleEventHandlerEntry sHandlersSalacBerry[] = {
    { 0x91, HandlerPinchReactionCommon }, { 0x55, HandlerPinchReactionMemberIn },
    { 0x58, CommonDamageReact },          { 0x72, HandlerSalacBerry },
    { 0x73, HandlerSalacBerry },
};

static const BattleEventHandlerEntry *EventAddSalacBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersSalacBerry);
    return sHandlersSalacBerry;
}

static void HandlerSalacBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonStatBoostBerry(flow, monId, 5, 1);
}

static const BattleEventHandlerEntry sHandlersPetayaBerry[] = {
    { 0x91, HandlerPinchReactionCommon }, { 0x55, HandlerPinchReactionMemberIn }, { 0x58, CommonDamageReact },
    { 0x72, HandlerPetayaBerry },         { 0x73, HandlerPetayaBerry },
};

static const BattleEventHandlerEntry *EventAddPetayaBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersPetayaBerry);
    return sHandlersPetayaBerry;
}

static void HandlerPetayaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonStatBoostBerry(flow, monId, 3, 1);
}

static const BattleEventHandlerEntry sHandlersApicotBerry[] = {
    { 0x91, HandlerPinchReactionCommon }, { 0x55, HandlerPinchReactionMemberIn }, { 0x58, CommonDamageReact },
    { 0x72, HandlerApicotBerry },         { 0x73, HandlerApicotBerry },
};

static const BattleEventHandlerEntry *EventAddApicotBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersApicotBerry);
    return sHandlersApicotBerry;
}

static void HandlerApicotBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonStatBoostBerry(flow, monId, 4, 1);
}

static const BattleEventHandlerEntry sHandlersLansatBerry[] = {
    { 0x91, HandlerPinchReactionCommon }, { 0x55, HandlerPinchReactionMemberIn }, { 0x58, CommonDamageReact },
    { 0x72, HandlerLansatBerry },         { 0x73, HandlerLansatBerry },
};

static const BattleEventHandlerEntry *EventAddLansatBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersLansatBerry);
    return sHandlersLansatBerry;
}

static void HandlerLansatBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerFlagParam *flag;
    BattleHandlerMessageParam *message;

    if (BattleEventVar_GetValue(2) == monId && !GetAdditionalConditionFlag(GetBattleMon(flow, monId), 9)) {
        flag = BattleHandler_PushWork(flow, 0x17, monId);
        flag->monIndex = monId;
        flag->flag = 9;
        BattleHandler_PopWork(flow, flag);
        message = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&message->string, 2, 0x3e9);
        BattleHandler_AddArg(&message->string, monId);
        BattleHandler_AddArg(&message->string, BattleEventItem_GetSubID(item));
        BattleHandler_PopWork(flow, message);
    }
}

static const BattleEventHandlerEntry sHandlersStarfBerry[] = {
    { 0x91, HandlerPinchReactionCommon }, { 0x55, HandlerPinchReactionMemberIn },
    { 0x58, CommonDamageReact },          { 0x72, HandlerStarfBerry },
    { 0x73, HandlerStarfBerry },
};

static const BattleEventHandlerEntry *EventAddStarfBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersStarfBerry);
    return sHandlersStarfBerry;
}

static const u8 sStarfBerryStats[] = { 1, 2, 3, 4, 5 };

static void HandlerStarfBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u8 *stats;
    u32 i;
    u32 count;
    u8 stat;

    mon = GetBattleMon(flow, monId);
    stats = func_ov167_021abc60(flow, 5);
    for (i = 0, count = 0; i < 5; i++) {
        stat = sStarfBerryStats[i];
        if (IsStatChangeValid(mon, stat, 1)) {
            stats[count++] = stat;
        }
    }
    if (count != 0) {
        CommonStatBoostBerry(flow, monId, stats[BattleRandom(count)], 2);
    }
}

static void CommonStatBoostBerry(BtlServerFlow *flow, u8 monId, u32 stat, s8 change) {
    BattleHandlerStatChangeParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        param = BattleHandler_PushWork(flow, 0xe, monId);
        param->stat = stat;
        param->change = change;
        param->count = 1;
        param->monIds[0] = monId;
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersEnigmaBerry[] = {
    { 0x4b, HandlerEnigmaBerry },
    { 0x72, HandlerEnigmaBerryUse },
};

static const BattleEventHandlerEntry *EventAddEnigmaBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersEnigmaBerry);
    return sHandlersEnigmaBerry;
}

static void HandlerEnigmaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(4) == monId && BattleEventVar_GetValue(0x46) == 0 &&
        (s32)BattleEventVar_GetValue(0x38) > 3 && !CheckCondition(GetBattleMon(flow, monId), 0xf)) {
        ItemEvent_PushRun(item, flow, monId);
    }
}

static void HandlerEnigmaBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u8 ratio;
    BattleHandlerRecoverHPParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        mon = GetBattleMon(flow, monId);
        ratio = CommonGetItemParam(item, 2);
        param = BattleHandler_PushWork(flow, 5, monId);
        param->targetIndex = monId;
        param->amount = DivideMaxHPZeroCheck(mon, ratio);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersOccaBerry[] = {
    { 0x47, HandlerOccaBerry },
    { 0x44, HandlerCommonResistBerryDamageAfter },
};

static const BattleEventHandlerEntry *EventAddOccaBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersOccaBerry);
    return sHandlersOccaBerry;
}

static void HandlerOccaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonResistBerry(item, flow, monId, work, 9, FALSE);
}

static const BattleEventHandlerEntry sHandlersPasshoBerry[] = {
    { 0x47, HandlerPasshoBerry },
    { 0x44, HandlerCommonResistBerryDamageAfter },
};

static const BattleEventHandlerEntry *EventAddPasshoBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersPasshoBerry);
    return sHandlersPasshoBerry;
}

static void HandlerPasshoBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonResistBerry(item, flow, monId, work, 10, FALSE);
}

static const BattleEventHandlerEntry sHandlersWacanBerry[] = {
    { 0x47, HandlerWacanBerry },
    { 0x44, HandlerCommonResistBerryDamageAfter },
};

static const BattleEventHandlerEntry *EventAddWacanBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersWacanBerry);
    return sHandlersWacanBerry;
}

static void HandlerWacanBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonResistBerry(item, flow, monId, work, 12, FALSE);
}

static const BattleEventHandlerEntry sHandlersRindoBerry[] = {
    { 0x47, HandlerRindoBerry },
    { 0x44, HandlerCommonResistBerryDamageAfter },
};

static const BattleEventHandlerEntry *EventAddRindoBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersRindoBerry);
    return sHandlersRindoBerry;
}

static void HandlerRindoBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonResistBerry(item, flow, monId, work, 11, FALSE);
}

static const BattleEventHandlerEntry sHandlersYacheBerry[] = {
    { 0x47, HandlerYacheBerry },
    { 0x44, HandlerCommonResistBerryDamageAfter },
};

static const BattleEventHandlerEntry *EventAddYacheBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersYacheBerry);
    return sHandlersYacheBerry;
}

static void HandlerYacheBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonResistBerry(item, flow, monId, work, 14, FALSE);
}

static const BattleEventHandlerEntry sHandlersChopleBerry[] = {
    { 0x47, HandlerChopleBerry },
    { 0x44, HandlerCommonResistBerryDamageAfter },
};

static const BattleEventHandlerEntry *EventAddChopleBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersChopleBerry);
    return sHandlersChopleBerry;
}

static void HandlerChopleBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonResistBerry(item, flow, monId, work, 1, FALSE);
}

static const BattleEventHandlerEntry sHandlersKebiaBerry[] = {
    { 0x47, HandlerKebiaBerry },
    { 0x44, HandlerCommonResistBerryDamageAfter },
};

static const BattleEventHandlerEntry *EventAddKebiaBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersKebiaBerry);
    return sHandlersKebiaBerry;
}

static void HandlerKebiaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonResistBerry(item, flow, monId, work, 3, FALSE);
}

static const BattleEventHandlerEntry sHandlersShucaBerry[] = {
    { 0x47, HandlerShucaBerry },
    { 0x44, HandlerCommonResistBerryDamageAfter },
};

static const BattleEventHandlerEntry *EventAddShucaBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersShucaBerry);
    return sHandlersShucaBerry;
}

static void HandlerShucaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonResistBerry(item, flow, monId, work, 4, FALSE);
}

static const BattleEventHandlerEntry sHandlersCobaBerry[] = {
    { 0x47, HandlerCobaBerry },
    { 0x44, HandlerCommonResistBerryDamageAfter },
};

static const BattleEventHandlerEntry *EventAddCobaBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersCobaBerry);
    return sHandlersCobaBerry;
}

static void HandlerCobaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonResistBerry(item, flow, monId, work, 2, FALSE);
}

static const BattleEventHandlerEntry sHandlersPayapaBerry[] = {
    { 0x47, HandlerPayapaBerry },
    { 0x44, HandlerCommonResistBerryDamageAfter },
};

static const BattleEventHandlerEntry *EventAddPayapaBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersPayapaBerry);
    return sHandlersPayapaBerry;
}

static void HandlerPayapaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonResistBerry(item, flow, monId, work, 13, FALSE);
}

static const BattleEventHandlerEntry sHandlersTangaBerry[] = {
    { 0x47, HandlerTangaBerry },
    { 0x44, HandlerCommonResistBerryDamageAfter },
};

static const BattleEventHandlerEntry *EventAddTangaBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersTangaBerry);
    return sHandlersTangaBerry;
}

static void HandlerTangaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonResistBerry(item, flow, monId, work, 6, FALSE);
}

static const BattleEventHandlerEntry sHandlersChartiBerry[] = {
    { 0x47, HandlerChartiBerry },
    { 0x44, HandlerCommonResistBerryDamageAfter },
};

static const BattleEventHandlerEntry *EventAddChartiBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersChartiBerry);
    return sHandlersChartiBerry;
}

static void HandlerChartiBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonResistBerry(item, flow, monId, work, 5, FALSE);
}

static const BattleEventHandlerEntry sHandlersKasibBerry[] = {
    { 0x47, HandlerKasibBerry },
    { 0x44, HandlerCommonResistBerryDamageAfter },
};

static const BattleEventHandlerEntry *EventAddKasibBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersKasibBerry);
    return sHandlersKasibBerry;
}

static void HandlerKasibBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonResistBerry(item, flow, monId, work, 7, FALSE);
}

static const BattleEventHandlerEntry sHandlersHabanBerry[] = {
    { 0x47, HandlerHabanBerry },
    { 0x44, HandlerCommonResistBerryDamageAfter },
};

static const BattleEventHandlerEntry *EventAddHabanBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersHabanBerry);
    return sHandlersHabanBerry;
}

static void HandlerHabanBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonResistBerry(item, flow, monId, work, 15, FALSE);
}

static const BattleEventHandlerEntry sHandlersColburBerry[] = {
    { 0x47, HandlerColburBerry },
    { 0x44, HandlerCommonResistBerryDamageAfter },
};

static const BattleEventHandlerEntry *EventAddColburBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersColburBerry);
    return sHandlersColburBerry;
}

static void HandlerColburBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonResistBerry(item, flow, monId, work, 16, FALSE);
}

static const BattleEventHandlerEntry sHandlersBabiriBerry[] = {
    { 0x47, HandlerBabiriBerry },
    { 0x44, HandlerCommonResistBerryDamageAfter },
};

static const BattleEventHandlerEntry *EventAddBabiriBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersBabiriBerry);
    return sHandlersBabiriBerry;
}

static void HandlerBabiriBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonResistBerry(item, flow, monId, work, 8, FALSE);
}

static const BattleEventHandlerEntry sHandlersChilanBerry[] = {
    { 0x47, HandlerChilanBerry },
    { 0x44, HandlerCommonResistBerryDamageAfter },
};

static const BattleEventHandlerEntry *EventAddChilanBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersChilanBerry);
    return sHandlersChilanBerry;
}

static void HandlerChilanBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonResistBerry(item, flow, monId, work, 0, TRUE);
}

static BOOL CommonResistBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work, u8 type,
                              BOOL anyEffectiveness) {
    if (BattleEventVar_GetValue(4) == monId && BattleEventVar_GetValue(0x16) == type) {
        if (anyEffectiveness || (s32)BattleEventVar_GetValue(0x38) > 3) {
            if (!IsSubstituteActive(GetBattleMon(flow, monId))) {
                BattleEventVar_MulValue(0x35, FX32_CONST(0.5));
                if (!func_ov167_021aba04(flow)) {
                    work[0] = 1;
                }
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void HandlerCommonResistBerryDamageAfter(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerConsumeItemParam *param;

    if (work[0] != 0 && func_ov167_021cde38(monId)) {
        param = BattleHandler_PushWork(flow, 0x23, monId);
        BattleHandler_StrSetup(&param->string, 2, 0xdb);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
        BattleHandler_PopWork(flow, param);
    }
}

static void HandlerPinchReactionCommon(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;

    if (BattleEventVar_GetValue(2) == monId) {
        mon = GetBattleMon(flow, monId);
        CommonDamageReactCheck(item, flow, monId, CommonGetItemParam(item, 2), TRUE);
    }
}

static void HandlerPinchReactionMemberIn(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;

    if (BattleEventVar_GetValue(2) == monId) {
        mon = GetBattleMon(flow, monId);
        CommonDamageReactCheck(item, flow, monId, CommonGetItemParam(item, 2), FALSE);
    }
}

static void CommonDamageReact(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;

    mon = GetBattleMon(flow, monId);
    if (CommonDamageReactCheckCore(flow, monId, CommonGetItemParam(item, 2))) {
        ItemEvent_PushRun(item, flow, monId);
    }
}

static void CommonDamageReactCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, u32 ratio, BOOL checkContext) {
    BOOL react;

    if (BattleEventVar_GetValue(2) == monId) {
        if (checkContext) {
            react = func_ov167_021c369c(item, flow, monId, ratio);
        } else {
            react = CommonDamageReactCheckCore(flow, monId, ratio);
        }
        if (react) {
            ItemEvent_PushRun(item, flow, monId);
        }
    }
}

static BOOL func_ov167_021c369c(BattleEventItem *item, BtlServerFlow *flow, u8 monId, u32 ratio) {
    u8 context;

    if (BattleEventVar_GetValue(2) == monId) {
        context = BattleEventVar_GetValue(0x2e);
        if (context == 0 || context == 1) {
            return CommonDamageReactCheckCore(flow, monId, ratio);
        }
    }
    return FALSE;
}

static BOOL CommonDamageReactCheckCore(BtlServerFlow *flow, u8 monId, u32 ratio) {
    BattleMon *mon;
    u32 hp;

    mon = GetBattleMon(flow, monId);
    if (GetBattleMonStat(mon, 0x11) == 0x52 && ratio > 2) {
        ratio /= 2;
    }
    if (ratio == 0) {
        ratio = 1;
    }
    if (GetBattleMonStat(mon, 0xe) > 1) {
        hp = GetBattleMonStat(mon, 0xd);
        if (hp <= DivideMaxHp(mon, ratio)) {
            return TRUE;
        }
    }
    return FALSE;
}

static const BattleEventHandlerEntry sHandlersCustapBerry[] = {
    { 0xf, HandlerCustapBerryPriorityCheck },
    { 0x72, HandlerCustapBerryUse },
};

static const BattleEventHandlerEntry *EventAddCustapBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersCustapBerry);
    return sHandlersCustapBerry;
}

static void HandlerCustapBerryPriorityCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId && CommonDamageReactCheckCore(flow, monId, 4) &&
        BattleEventVar_RewriteValue(0x11, 2)) {
        ItemEvent_PushRun(item, flow, monId);
    }
}

static void HandlerCustapBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        param = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&param->string, 2, 0x401);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
        BattleHandler_PopWork(flow, param);
        work[0] = 0;
    }
}

static const BattleEventHandlerEntry sHandlersMicleBerry[] = {
    { 0x91, HandlerMicleBerryReaction }, { 0x55, HandlerMicleBerryActProcEnd }, { 0x2, HandlerMicleBerryActionEnd },
    { 0x58, HandlerMicleBerryMemberIn }, { 0x72, HandlerMicleBerryUse },        { 0x73, HandlerMicleBerryUseTemp },
};

static const BattleEventHandlerEntry *EventAddMicleBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersMicleBerry);
    return sHandlersMicleBerry;
}

static void HandlerMicleBerryReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u8 ratio;

    if (BattleEventVar_GetValue(2) == monId) {
        mon = GetBattleMon(flow, monId);
        ratio = CommonGetItemParam(item, 2);
        if (func_ov167_021c369c(item, flow, monId, ratio)) {
            ItemEvent_PushRun(item, flow, monId);
        }
    }
}

static void HandlerMicleBerryActProcEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        HandlerMicleBerryMemberIn(item, flow, monId, work);
    }
}

static void HandlerMicleBerryActionEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(0xc) != 6) {
        HandlerMicleBerryActProcEnd(item, flow, monId, work);
    }
}

static void HandlerMicleBerryMemberIn(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u8 ratio;

    mon = GetBattleMon(flow, monId);
    ratio = CommonGetItemParam(item, 2);
    if (CommonDamageReactCheckCore(flow, monId, ratio)) {
        ItemEvent_PushRun(item, flow, monId);
    }
}

static void HandlerMicleBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        mon = GetBattleMon(flow, monId);
        param = BattleHandler_PushWork(flow, 0xc, monId);
        param->targetIndex = monId;
        param->condition = 0x22;
        param->value = MakeConditionParamPermanent(0x78);
        BattleHandler_StrSetup(&param->string, 2, 0x404);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
        BattleHandler_PopWork(flow, param);
    }
}

static void HandlerMicleBerryUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (work[6] != 0) {
        HandlerMicleBerryUse(item, flow, monId, work);
    }
}

static const BattleEventHandlerEntry sHandlersJabocaBerry[] = {
    { 0x4b, HandlerJabocaBerry },
    { 0x72, HandlerJabocaRowapBerryUse },
};

static const BattleEventHandlerEntry *EventAddJabocaBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersJabocaBerry);
    return sHandlersJabocaBerry;
}

static void HandlerJabocaBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonJabocaRowapBerryReaction(item, flow, monId, work, 1);
}

static const BattleEventHandlerEntry sHandlersRowapBerry[] = {
    { 0x4b, HandlerRowapBerry },
    { 0x72, HandlerJabocaRowapBerryUse },
};

static const BattleEventHandlerEntry *EventAddRowapBerry(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersRowapBerry);
    return sHandlersRowapBerry;
}

static void HandlerRowapBerry(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonJabocaRowapBerryReaction(item, flow, monId, work, 2);
}

static void CommonJabocaRowapBerryReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work,
                                           u32 category) {
    BattleHandlerUseHeldItemParam *param;

    if (BattleEventVar_GetValue(4) == monId && BattleEventVar_GetValue(0x46) == 0 &&
        !func_ov169_0689ca54(BattleEventVar_GetValue(0x12)) && BattleEventVar_GetValue(0x1a) == category) {
        BattleEventItem_SetRecallEnable(item);
        work[0] = BattleEventVar_GetValue(3) + 1;
        param = BattleHandler_PushWork(flow, 0, monId);
        param->allowFainted = 1;
        BattleHandler_PopWork(flow, param);
    }
}

static void HandlerJabocaRowapBerryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 attackerId;
    BattleMon *attacker;
    u8 ratio;
    BattleHandlerDamageParam *param;

    if (BattleEventVar_GetValue(2) == monId && work[0] != 0) {
        attackerId = work[0] - 1;
        attacker = GetBattleMon(flow, attackerId);
        if (!IsFainted(attacker)) {
            ratio = CommonGetItemParam(item, 2);
            param = BattleHandler_PushWork(flow, 7, monId);
            param->targetIndex = attackerId;
            param->amount = DivideMaxHPZeroCheck(attacker, ratio);
            BattleHandler_StrSetup(&param->string, 2, 0x407);
            BattleHandler_AddArg(&param->string, param->targetIndex);
            BattleHandler_AddArg(&param->string, monId);
            BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
            BattleHandler_PopWork(flow, param);
        }
    }
}

static const BattleEventHandlerEntry sHandlersWhiteHerb[] = {
    { 0x57, HandlerWhiteHerbReaction },  { 0x2, func_ov167_021c3a74 },  { 0x58, HandlerWhiteHerbReaction },
    { 0x77, HandlerWhiteHerbTurnCheck }, { 0x72, HandlerWhiteHerbUse }, { 0x73, HandlerWhiteHerbUseTemp },
};

static const BattleEventHandlerEntry *EventAddWhiteHerb(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersWhiteHerb);
    return sHandlersWhiteHerb;
}

static void HandlerWhiteHerbReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (AreStatsLowered(GetBattleMon(flow, monId))) {
        ItemEvent_PushRun(item, flow, monId);
    }
}

static void func_ov167_021c3a74(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(0xc) != 6) {
        HandlerWhiteHerbReaction(item, flow, monId, work);
    }
}

static void HandlerWhiteHerbTurnCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId && AreStatsLowered(GetBattleMon(flow, monId))) {
        ItemEvent_PushRun(item, flow, monId);
    }
}

static void HandlerWhiteHerbUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerRecoverStatStageParam *recover;
    BattleHandlerMessageParam *message;

    if (BattleEventVar_GetValue(2) == monId) {
        recover = BattleHandler_PushWork(flow, 0x12, monId);
        recover->monIndex = monId;
        BattleHandler_PopWork(flow, recover);
        message = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&message->string, 2, 0x3f2);
        BattleHandler_AddArg(&message->string, monId);
        BattleHandler_AddArg(&message->string, BattleEventItem_GetSubID(item));
        BattleHandler_PopWork(flow, message);
    }
}

static void HandlerWhiteHerbUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (work[6] != 0) {
        HandlerWhiteHerbUse(item, flow, monId, work);
    }
}

static const BattleEventHandlerEntry sHandlersMentalHerb[] = {
    { 0x91, HandlerMentalHerbReaction },
    { 0x55, HandlerMentalHerbReaction },
    { 0x72, HandlerMentalHerbUse },
    { 0x73, HandlerMentalHerbUseTemp },
};

static const BattleEventHandlerEntry *EventAddMentalHerb(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersMentalHerb);
    return sHandlersMentalHerb;
}

static void HandlerMentalHerbReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId && func_ov167_021bd624(GetBattleMon(flow, monId))) {
        ItemEvent_PushRun(item, flow, monId);
    }
}

static void HandlerMentalHerbUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonUseForStatus(item, flow, monId, 0x26);
}

static void HandlerMentalHerbUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (work[6] != 0) {
        HandlerMentalHerbUse(item, flow, monId, work);
    }
}

static const BattleEventHandlerEntry sHandlersBrightPowder[] = {
    { 0x34, HandlerBrightPowder },
};

static const BattleEventHandlerEntry *EventAddBrightPowder(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersBrightPowder);
    return sHandlersBrightPowder;
}

static void HandlerBrightPowder(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 ratio;

    if (BattleEventVar_GetValue(4) == monId) {
        ratio = 100 - CommonGetItemParam(item, 2);
        BattleEventVar_MulValue(0x35, FX32_CONST(ratio) / 100);
    }
}

static const BattleEventHandlerEntry sHandlersMachoBrace[] = {
    { 0x13, HandlerMachoBrace },
};

static const BattleEventHandlerEntry *EventAddMachoBrace(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersMachoBrace);
    return sHandlersMachoBrace;
}

static void HandlerMachoBrace(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        BattleEventVar_MulValue(0x35, FX32_CONST(0.5));
    }
}

static const BattleEventHandlerEntry sHandlersQuickClaw[] = {
    { 0xf, HandlerQuickClawPriorityCheck },
    { 0x72, HandlerQuickClawUse },
};

static const BattleEventHandlerEntry *EventAddQuickClaw(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersQuickClaw);
    return sHandlersQuickClaw;
}

static void HandlerQuickClawPriorityCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 chance;

    if (BattleEventVar_GetValue(2) == monId) {
        chance = CommonGetItemParam(item, 2);
        if (ItemEvent_RollEffectChance(flow, chance) && BattleEventVar_RewriteValue(0x11, 2)) {
            ItemEvent_PushRun(item, flow, monId);
        }
    }
}

static void HandlerQuickClawUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        param = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&param->string, 2, 0x401);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
        BattleHandler_PopWork(flow, param);
        work[0] = 0;
    }
}

static const BattleEventHandlerEntry sHandlersLaggingTail[] = {
    { 0xf, HandlerLaggingTail },
};

static const BattleEventHandlerEntry *EventAddLaggingTail(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersLaggingTail);
    return sHandlersLaggingTail;
}

static void HandlerLaggingTail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        BattleEventVar_RewriteValue(0x11, 0);
    }
}

static const BattleEventHandlerEntry sHandlersKingsRock[] = {
    { 0x6c, HandlerKingsRock },
    { 0x73, HandlerKingsRockUseTemp },
};

static const BattleEventHandlerEntry *EventAddKingsRock(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersKingsRock);
    return sHandlersKingsRock;
}

static void HandlerKingsRock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 chance;

    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x25) == 0) {
        chance = CommonGetItemParam(item, 2);
        BattleEventVar_RewriteValue(0x26, chance);
    }
}

static void HandlerKingsRockUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerFlinchParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        param = BattleHandler_PushWork(flow, 0x2b, monId);
        param->monIndex = monId;
        param->flag = 100;
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersRazorFang[] = {
    { 0x6c, HandlerKingsRock },
    { 0x73, HandlerKingsRockUseTemp },
};

static const BattleEventHandlerEntry *EventAddRazorFang(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersRazorFang);
    return sHandlersRazorFang;
}

static const BattleEventHandlerEntry sHandlersWideLens[] = {
    { 0x34, HandlerWideLens },
};

static const BattleEventHandlerEntry *EventAddWideLens(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersWideLens);
    return sHandlersWideLens;
}

static void HandlerWideLens(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_MulValue(0x35, ItemAttackValueToRatio(item));
    }
}

static const BattleEventHandlerEntry sHandlersScopeLens[] = {
    { 0x36, HandlerScopeLens },
};

static const BattleEventHandlerEntry *EventAddScopeLens(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersScopeLens);
    return sHandlersScopeLens;
}

static void HandlerScopeLens(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 rank;

    if (BattleEventVar_GetValue(3) == monId) {
        rank = BattleEventVar_GetValue(0x2c);
        BattleEventVar_RewriteValue(0x2c, (u8)(rank + 1));
    }
}

static const BattleEventHandlerEntry sHandlersLuckyPunch[] = {
    { 0x36, HandlerLuckyPunch },
};

static const BattleEventHandlerEntry *EventAddLuckyPunch(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersLuckyPunch);
    return sHandlersLuckyPunch;
}

static void HandlerLuckyPunch(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 rank;

    if (BattleEventVar_GetValue(3) == monId && GetBattleMonSpecies(GetBattleMon(flow, monId)) == 0x71) {
        rank = BattleEventVar_GetValue(0x2c);
        BattleEventVar_RewriteValue(0x2c, (u8)(rank + 2));
    }
}

static const BattleEventHandlerEntry sHandlersStick[] = {
    { 0x36, HandlerStick },
};

static const BattleEventHandlerEntry *EventAddStick(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersStick);
    return sHandlersStick;
}

static void HandlerStick(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 rank;

    if (BattleEventVar_GetValue(3) == monId && GetBattleMonSpecies(GetBattleMon(flow, monId)) == 0x53) {
        rank = BattleEventVar_GetValue(0x2c);
        BattleEventVar_RewriteValue(0x2c, (u8)(rank + 2));
    }
}

static const BattleEventHandlerEntry sHandlersZoomLens[] = {
    { 0x34, HandlerZoomLens },
};

static const BattleEventHandlerEntry *EventAddZoomLens(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersZoomLens);
    return sHandlersZoomLens;
}

static void HandlerZoomLens(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(4);
        if (GetTurnFlag(GetBattleMon(flow, targetId), 1)) {
            BattleEventVar_MulValue(0x35, ItemAttackValueToRatio(item));
        }
    }
}

static const BattleEventHandlerEntry sHandlersLaxIncense[] = {
    { 0x34, HandlerLaxIncense },
};

static const BattleEventHandlerEntry *EventAddLaxIncense(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersLaxIncense);
    return sHandlersLaxIncense;
}

static void HandlerLaxIncense(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u16 ratio;

    if (BattleEventVar_GetValue(4) == monId) {
        ratio = 100 - CommonGetItemParam(item, 2);
        BattleEventVar_MulValue(0x35, FX32_CONST(ratio) / 100);
    }
}

static const BattleEventHandlerEntry sHandlersMuscleBand[] = {
    { 0x38, HandlerMuscleBand },
};

static const BattleEventHandlerEntry *EventAddMuscleBand(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersMuscleBand);
    return sHandlersMuscleBand;
}

static void HandlerMuscleBand(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && PML_MoveGetCategory(BattleEventVar_GetValue(0x12)) == 1) {
        BattleEventVar_MulValue(0x31, ItemAttackValueToRatio(item));
    }
}

static const BattleEventHandlerEntry sHandlersWiseGlasses[] = {
    { 0x38, HandlerWiseGlasses },
};

static const BattleEventHandlerEntry *EventAddWiseGlasses(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersWiseGlasses);
    return sHandlersWiseGlasses;
}

static void HandlerWiseGlasses(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && PML_MoveGetCategory(BattleEventVar_GetValue(0x12)) == 2) {
        BattleEventVar_MulValue(0x31, ItemAttackValueToRatio(item));
    }
}

static const BattleEventHandlerEntry sHandlersDeepSeaTooth[] = {
    { 0x3b, HandlerDeepSeaTooth },
};

static const BattleEventHandlerEntry *EventAddDeepSeaTooth(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersDeepSeaTooth);
    return sHandlersDeepSeaTooth;
}

static void HandlerDeepSeaTooth(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && GetBattleMonSpecies(GetBattleMon(flow, monId)) == 0x16e &&
        PML_MoveGetCategory(BattleEventVar_GetValue(0x12)) == 2) {
        BattleEventVar_MulValue(0x35, FX32_CONST(2));
    }
}

static const BattleEventHandlerEntry sHandlersDeepSeaScale[] = {
    { 0x3c, HandlerDeepSeaScale },
};

static const BattleEventHandlerEntry *EventAddDeepSeaScale(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersDeepSeaScale);
    return sHandlersDeepSeaScale;
}

static void HandlerDeepSeaScale(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(4) == monId && GetBattleMonSpecies(GetBattleMon(flow, monId)) == 0x16e &&
        BattleEventVar_GetValue(0x1a) == 2) {
        BattleEventVar_MulValue(0x35, FX32_CONST(2));
    }
}

static const BattleEventHandlerEntry sHandlersMetalPowder[] = {
    { 0x3c, HandlerMetalPowder },
};

static const BattleEventHandlerEntry *EventAddMetalPowder(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersMetalPowder);
    return sHandlersMetalPowder;
}

static void HandlerMetalPowder(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;

    if (BattleEventVar_GetValue(4) == monId) {
        mon = GetBattleMon(flow, monId);
        if (GetBattleMonSpecies(mon) == 0x84 && !TransformCheck(mon) && BattleEventVar_GetValue(0x1a) == 1) {
            BattleEventVar_MulValue(0x35, FX32_CONST(2));
        }
    }
}

static const BattleEventHandlerEntry sHandlersQuickPowder[] = {
    { 0x13, HandlerQuickPowder },
};

static const BattleEventHandlerEntry *EventAddQuickPowder(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersQuickPowder);
    return sHandlersQuickPowder;
}

static void HandlerQuickPowder(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;

    if (BattleEventVar_GetValue(2) == monId) {
        mon = GetBattleMon(flow, monId);
        if (GetBattleMonSpecies(mon) == 0x84 && !TransformCheck(mon)) {
            BattleEventVar_MulValue(0x35, FX32_CONST(2));
        }
    }
}

static const BattleEventHandlerEntry sHandlersSoulDew[] = {
    { 0x3b, HandlerSoulDewAttacker },
    { 0x3c, HandlerSoulDewDefender },
};

static const BattleEventHandlerEntry *EventAddSoulDew(u32 *numHandlers) {
    *numHandlers = NELEMS(sHandlersSoulDew);
    return sHandlersSoulDew;
}

static void HandlerSoulDewAttacker(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 species;

    if (BattleEventVar_GetValue(3) == monId) {
        species = GetBattleMonSpecies(GetBattleMon(flow, monId));
        if ((species == 0x17c || species == 0x17d) && BattleEventVar_GetValue(0x1a) == 2) {
            BattleEventVar_MulValue(0x35, FX32_CONST(1.5));
        }
    }
}

static void HandlerSoulDewDefender(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 species;

    if (BattleEventVar_GetValue(4) == monId) {
        species = GetBattleMonSpecies(GetBattleMon(flow, monId));
        if ((species == 0x17c || species == 0x17d) && BattleEventVar_GetValue(0x1a) == 2) {
            BattleEventVar_MulValue(0x35, FX32_CONST(1.5));
        }
    }
}

static const BattleEventHandlerEntry sHandlersThickClub[] = {
    { 0x3b, HandlerThickClub },
};

static const BattleEventHandlerEntry *EventAddThickClub(u32 *priority) {
    *priority = NELEMS(sHandlersThickClub);
    return sHandlersThickClub;
}

static void HandlerThickClub(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 species;

    if (BattleEventVar_GetValue(3) == monId) {
        species = GetBattleMonSpecies(GetBattleMon(flow, monId));
        if (species == SPECIES_CUBONE || species == SPECIES_MAROWAK) {
            if (PML_MoveGetCategory(BattleEventVar_GetValue(0x12)) == MOVE_CATEGORY_PHYSICAL) {
                BattleEventVar_MulValue(0x35, FX32_CONST(2));
            }
        }
    }
}

static const BattleEventHandlerEntry sHandlersChoiceBand[] = {
    { 0x22, HandlerChoiceItemCommonMoveLock },
    { 0x9c, HandlerChoiceItemCommonItemChange },
    { 0x3b, HandlerChoiceBandPower },
};

static const BattleEventHandlerEntry *EventAddChoiceBand(u32 *priority) {
    *priority = NELEMS(sHandlersChoiceBand);
    return sHandlersChoiceBand;
}

static void HandlerChoiceBandPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        if (PML_MoveGetCategory(BattleEventVar_GetValue(0x12)) == MOVE_CATEGORY_PHYSICAL) {
            BattleEventVar_MulValue(0x35, FX32_CONST(1.5));
        }
    }
}

static const BattleEventHandlerEntry sHandlersChoiceSpecs[] = {
    { 0x22, HandlerChoiceItemCommonMoveLock },
    { 0x9c, HandlerChoiceItemCommonItemChange },
    { 0x3b, HandlerChoiceSpecsPower },
};

static const BattleEventHandlerEntry *EventAddChoiceSpecs(u32 *priority) {
    *priority = NELEMS(sHandlersChoiceSpecs);
    return sHandlersChoiceSpecs;
}

static void HandlerChoiceSpecsPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        if (PML_MoveGetCategory(BattleEventVar_GetValue(0x12)) == MOVE_CATEGORY_SPECIAL) {
            BattleEventVar_MulValue(0x35, FX32_CONST(1.5));
        }
    }
}

static const BattleEventHandlerEntry sHandlersChoiceScarf[] = {
    { 0x22, HandlerChoiceItemCommonMoveLock },
    { 0x9c, HandlerChoiceItemCommonItemChange },
    { 0x13, HandlerChoiceScarf },
};

static const BattleEventHandlerEntry *EventAddChoiceScarf(u32 *priority) {
    *priority = NELEMS(sHandlersChoiceScarf);
    return sHandlersChoiceScarf;
}

static void HandlerChoiceScarf(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        BattleEventVar_MulValue(0x35, FX32_CONST(1.5));
    }
}

// Locks the holder into the move it used
static void HandlerChoiceItemCommonMoveLock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u16 move;
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        move = BattleEventVar_GetValue(0x12);
        if (move != MOVE_STRUGGLE) {
            param = BattleHandler_PushWork(flow, 0xc, monId);
            param->targetIndex = monId;
            param->condition = 0x1b;
            param->value = MakeConditionParamPermanent(move);
            param->overwrite = 2;
            BattleHandler_PopWork(flow, param);
        }
    }
}

// Ends the move lock when the item changes
static void HandlerChoiceItemCommonItemChange(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerCureConditionParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        param = BattleHandler_PushWork(flow, 0xb, monId);
        param->condition = 0x1b;
        param->monIds[0] = monId;
        param->count = 1;
        param->useString = 1;
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersFocusSash[] = {
    { 0x74, HandlerFocusSash },
    { 0x75, HandlerFocusSashUse },
};

static const BattleEventHandlerEntry *EventAddFocusSash(u32 *priority) {
    *priority = NELEMS(sHandlersFocusSash);
    return sHandlersFocusSash;
}

static void HandlerFocusSash(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(4) == monId) {
        if (IsMonFullHP(GetBattleMon(flow, monId))) {
            *work = BattleEventVar_RewriteValue(0x3a, 3);
        } else {
            *work = 0;
        }
    }
}

static void HandlerFocusSashUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerConsumeItemParam *param;

    if (BattleEventVar_GetValue(2) == monId && *work) {
        param = BattleHandler_PushWork(flow, 0x23, monId);
        BattleHandler_StrSetup(&param->string, 2, 0x3ef);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
        BattleHandler_PopWork(flow, param);
        *work = 0;
    }
}

static const BattleEventHandlerEntry sHandlersFocusBand[] = {
    { 0x74, HandlerFocusBandCheck },
    { 0x75, HandlerFocusBand },
    { 0x72, HandlerFocusBandUse },
};

static const BattleEventHandlerEntry *EventAddFocusBand(u32 *priority) {
    *priority = NELEMS(sHandlersFocusBand);
    return sHandlersFocusBand;
}

static void HandlerFocusBandCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 chance;
    if (BattleEventVar_GetValue(4) == monId) {
        chance = CommonGetItemParam(item, 2);
        if (ItemEvent_RollEffectChance(flow, chance)) {
            *work = BattleEventVar_RewriteValue(0x3a, 3);
        } else {
            *work = 0;
        }
    }
}

static void HandlerFocusBand(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId && *work) {
        ItemEvent_PushRun(item, flow, monId);
        *work = 0;
    }
}

static void HandlerFocusBandUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        param = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&param->string, 2, 0x3ef);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersExpertBelt[] = {
    { 0x47, HandlerExpertBelt },
};

static const BattleEventHandlerEntry *EventAddExpertBelt(u32 *priority) {
    *priority = NELEMS(sHandlersExpertBelt);
    return sHandlersExpertBelt;
}

static void HandlerExpertBelt(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        if ((s32)BattleEventVar_GetValue(0x38) > TYPE_EFFECTIVENESS_NORMAL) {
            BattleEventVar_MulValue(0x35, ItemAttackValueToRatio(item));
        }
    }
}

static const BattleEventHandlerEntry sHandlersLifeOrb[] = {
    { 0x85, HandlerLifeOrbReaction },
    { 0x47, HandlerLifeOrbPower },
};

static const BattleEventHandlerEntry *EventAddLifeOrb(u32 *priority) {
    *priority = NELEMS(sHandlersLifeOrb);
    return sHandlersLifeOrb;
}

static void HandlerLifeOrbReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    BattleHandlerDamageParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        param = BattleHandler_PushWork(flow, 7, monId);
        param->targetIndex = monId;
        param->amount = DivideMaxHPZeroCheck(mon, 10);
        BattleHandler_StrSetup(&param->string, 2, 0x3f5);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_PopWork(flow, param);
    }
}

static void HandlerLifeOrbPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_MulValue(0x35, ItemAttackValueToRatio(item));
    }
}

static const BattleEventHandlerEntry sHandlersMetronomeItem[] = {
    { 0x47, HandlerMetronomeItem },
};

static const BattleEventHandlerEntry *EventAddMetronomeItem(u32 *priority) {
    *priority = NELEMS(sHandlersMetronomeItem);
    return sHandlersMetronomeItem;
}

static void HandlerMetronomeItem(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u16 count;
    u16 move;
    u16 ratio;

    if (BattleEventVar_GetValue(3) == monId) {
        if (!func_ov169_0689ca54(BattleEventVar_GetValue(0x12))) {
            mon = GetBattleMon(flow, monId);
            count = GetConsecutiveMoveCount(mon);
            if (count >= 1) {
                move = BattleEventVar_GetValue(0x12);
                if (move == GetPreviousMoveID(mon)) {
                    ratio = 100 + CommonGetItemParam(item, 2) * count;
                    if (ratio > 200) {
                        ratio = 200;
                    }
                    BattleEventVar_MulValue(0x35, FX32_CONST(ratio) / 100);
                }
            }
        }
    }
}

static const BattleEventHandlerEntry sHandlersGripClaw[] = {
    { 0x62, HandlerGripClaw },
};

static const BattleEventHandlerEntry *EventAddGripClaw(u32 *priority) {
    *priority = NELEMS(sHandlersGripClaw);
    return sHandlersGripClaw;
}

static void HandlerGripClaw(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleCondition condition;

    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x1d) == 8) {
        condition.raw = BattleEventVar_GetValue(0x1e);
        SetTurns(&condition, 8);
        BattleEventVar_RewriteValue(0x1e, condition.raw);
    }
}

static const BattleEventHandlerEntry sHandlersShellBell[] = {
    { 0x85, HandlerShellBell },
};

static const BattleEventHandlerEntry *EventAddShellBell(u32 *priority) {
    *priority = NELEMS(sHandlersShellBell);
    return sHandlersShellBell;
}

static void HandlerShellBell(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 damage;
    u32 amount;
    BattleHandlerRecoverHPParam *param;

    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x4d) == 0) {
        damage = BattleEventVar_GetValue(0x32);
        if (damage != 0) {
            amount = damage / CommonGetItemParam(item, 2);
            if (amount == 0) {
                amount = 1;
            }
            param = BattleHandler_PushWork(flow, 5, monId);
            param->targetIndex = monId;
            param->amount = amount;
            BattleHandler_StrSetup(&param->string, 2, 0x392);
            BattleHandler_AddArg(&param->string, monId);
            BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
            BattleHandler_PopWork(flow, param);
        }
    }
}

static const BattleEventHandlerEntry sHandlersLightClay[] = {
    { 0x9f, HandlerLightClay },
};

static const BattleEventHandlerEntry *EventAddLightClay(u32 *priority) {
    *priority = NELEMS(sHandlersLightClay);
    return sHandlersLightClay;
}

static void HandlerLightClay(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleCondition condition;
    u8 turns;

    if (BattleEventVar_GetValue(2) == monId && BattleEventVar_GetValue(0x53) <= 1) {
        condition.raw = BattleEventVar_GetValue(0x1e);
        turns = CommonGetItemParam(item, 2);
        IncrementTurn(&condition, turns);
        BattleEventVar_RewriteValue(0x1e, condition.raw);
    }
}

static const BattleEventHandlerEntry sHandlersPowerHerb[] = {
    { 0x94, HandlerPowerHerbCheckChargeSkip },
    { 0x97, HandlerPowerHerbFixChargeSkip },
    { 0x72, HandlerPowerHerbUse },
};

static const BattleEventHandlerEntry *EventAddPowerHerb(u32 *priority) {
    *priority = NELEMS(sHandlersPowerHerb);
    return sHandlersPowerHerb;
}

static void HandlerPowerHerbCheckChargeSkip(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x12) != MOVE_SKY_DROP) {
        if (BattleEventVar_RewriteValue(0x51, 1)) {
            *work = 1;
        }
    }
}

static void HandlerPowerHerbFixChargeSkip(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && *work) {
        ItemEvent_PushRun(item, flow, monId);
    }
}

static void HandlerPowerHerbUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        param = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&param->string, 2, 0x3fe);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersLeftovers[] = {
    { 0x76, HandlerLeftoversReaction },
    { 0x72, HandlerLeftoversUse },
};

static const BattleEventHandlerEntry *EventAddLeftovers(u32 *priority) {
    *priority = NELEMS(sHandlersLeftovers);
    return sHandlersLeftovers;
}

static void HandlerLeftoversReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerUseHeldItemParam *param;

    if (BattleEventVar_GetValue(2) == monId && !IsMonFullHP(GetBattleMon(flow, monId))) {
        param = BattleHandler_PushWork(flow, 0, monId);
        param->checkFullHp = 1;
        BattleEventItem_SetRecallEnable(item);
        BattleHandler_PopWork(flow, param);
    }
}

static void HandlerLeftoversUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    BattleHandlerRecoverHPParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        mon = GetBattleMon(flow, monId);
        if (!IsMonFullHP(mon)) {
            param = BattleHandler_PushWork(flow, 5, monId);
            param->targetIndex = monId;
            param->amount = DivideMaxHPZeroCheck(mon, 16);
            BattleHandler_StrSetup(&param->string, 2, 0x392);
            BattleHandler_AddArg(&param->string, monId);
            BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
            BattleHandler_PopWork(flow, param);
        }
    }
}

static const BattleEventHandlerEntry sHandlersBlackSludge[] = {
    { 0x76, HandlerBlackSludge },
};

static const BattleEventHandlerEntry *EventAddBlackSludge(u32 *priority) {
    *priority = NELEMS(sHandlersBlackSludge);
    return sHandlersBlackSludge;
}

static void HandlerBlackSludge(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    BattleHandlerRecoverHPParam *recoverParam;
    BattleHandlerDamageParam *damageParam;

    if (BattleEventVar_GetValue(2) == monId && !func_ov167_021abf14(flow)) {
        mon = GetBattleMon(flow, monId);
        if (func_ov167_021ce564(GetPokeType(mon), TYPE_POISON)) {
            recoverParam = BattleHandler_PushWork(flow, 5, monId);
            recoverParam->targetIndex = monId;
            recoverParam->amount = DivideMaxHPZeroCheck(mon, CommonGetItemParam(item, 2));
            BattleHandler_StrSetup(&recoverParam->string, 2, 0x392);
            BattleHandler_AddArg(&recoverParam->string, monId);
            BattleHandler_AddArg(&recoverParam->string, BattleEventItem_GetSubID(item));
            BattleHandler_PopWork(flow, recoverParam);
        } else {
            damageParam = BattleHandler_PushWork(flow, 7, monId);
            damageParam->targetIndex = monId;
            damageParam->amount = DivideMaxHPZeroCheck(mon, 8);
            damageParam->showViewEffect = 1;
            damageParam->effect = 0x25f;
            damageParam->effectArg1 = func_ov167_021abb50(flow, monId);
            damageParam->effectArg2 = 6;
            BattleHandler_StrSetup(&damageParam->string, 2, 0x40e);
            BattleHandler_AddArg(&damageParam->string, monId);
            BattleHandler_AddArg(&damageParam->string, BattleEventItem_GetSubID(item));
            BattleHandler_PopWork(flow, damageParam);
        }
    }
}

static const BattleEventHandlerEntry sHandlersDestinyKnot[] = {
    { 0x69, HandlerDestinyKnot },
};

static const BattleEventHandlerEntry *EventAddDestinyKnot(u32 *priority) {
    *priority = NELEMS(sHandlersDestinyKnot);
    return sHandlersDestinyKnot;
}

static void HandlerDestinyKnot(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 attackerId;
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(4) == monId && BattleEventVar_GetValue(0x1d) == 7) {
        attackerId = BattleEventVar_GetValue(3);
        if (attackerId != 0x1f) {
            param = BattleHandler_PushWork(flow, 0xc, monId);
            param->targetIndex = attackerId;
            param->condition = 7;
            param->value = func_ov167_021ce1dc(monId);
            param->noMessage = 1;
            BattleHandler_StrSetup(&param->string, 2, 0x14a);
            BattleHandler_AddArg(&param->string, attackerId);
            BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
            BattleHandler_PopWork(flow, param);
        }
    }
}

static const BattleEventHandlerEntry sHandlersStickyBarb[] = {
    { 0x4b, HandlerStickyBarbDamageReaction },
    { 0x77, HandlerStickyBarbTurnCheck },
};

static const BattleEventHandlerEntry *EventAddStickyBarb(u32 *priority) {
    *priority = NELEMS(sHandlersStickyBarb);
    return sHandlersStickyBarb;
}

static void HandlerStickyBarbDamageReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 attackerId;
    BattleHandlerSwapItemParam *param;

    if (BattleEventVar_GetValue(4) == monId && BattleEventVar_GetValue(0x46) == 0 &&
        getMoveFlag(BattleEventVar_GetValue(0x12), 0)) {
        attackerId = BattleEventVar_GetValue(3);
        if (GetBattleMonHeldItem(GetBattleMon(flow, attackerId)) == ITEM_NONE) {
            param = BattleHandler_PushWork(flow, 0x24, monId);
            param->otherIndex = attackerId;
            BattleHandler_PopWork(flow, param);
        }
    }
}

static void HandlerStickyBarbTurnCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    BattleHandlerDamageParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        mon = GetBattleMon(flow, monId);
        param = BattleHandler_PushWork(flow, 7, monId);
        param->targetIndex = monId;
        param->amount = DivideMaxHPZeroCheck(mon, CommonGetItemParam(item, 2));
        param->showViewEffect = 1;
        param->effect = 0x25f;
        param->effectArg1 = func_ov167_021abb50(flow, monId);
        param->effectArg2 = 6;
        BattleHandler_StrSetup(&param->string, 2, 0x40e);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersPowerBracer[] = {
    { 0x13, HandlerPowerItemCalcSpeed },
};

static const BattleEventHandlerEntry *EventAddPowerBracer(u32 *priority) {
    *priority = NELEMS(sHandlersPowerBracer);
    return sHandlersPowerBracer;
}

static const BattleEventHandlerEntry sHandlersPowerBelt[] = {
    { 0x13, HandlerPowerItemCalcSpeed },
};

static const BattleEventHandlerEntry *EventAddPowerBelt(u32 *priority) {
    *priority = NELEMS(sHandlersPowerBelt);
    return sHandlersPowerBelt;
}

static const BattleEventHandlerEntry sHandlersPowerLens[] = {
    { 0x13, HandlerPowerItemCalcSpeed },
};

static const BattleEventHandlerEntry *EventAddPowerLens(u32 *priority) {
    *priority = NELEMS(sHandlersPowerLens);
    return sHandlersPowerLens;
}

static const BattleEventHandlerEntry sHandlersPowerBand[] = {
    { 0x13, HandlerPowerItemCalcSpeed },
};

static const BattleEventHandlerEntry *EventAddPowerBand(u32 *priority) {
    *priority = NELEMS(sHandlersPowerBand);
    return sHandlersPowerBand;
}

static const BattleEventHandlerEntry sHandlersPowerAnklet[] = {
    { 0x13, HandlerPowerItemCalcSpeed },
};

static const BattleEventHandlerEntry *EventAddPowerAnklet(u32 *priority) {
    *priority = NELEMS(sHandlersPowerAnklet);
    return sHandlersPowerAnklet;
}

static const BattleEventHandlerEntry sHandlersPowerWeight[] = {
    { 0x13, HandlerPowerItemCalcSpeed },
};

static const BattleEventHandlerEntry *EventAddPowerWeight(u32 *priority) {
    *priority = NELEMS(sHandlersPowerWeight);
    return sHandlersPowerWeight;
}

static void HandlerPowerItemCalcSpeed(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        BattleEventVar_MulValue(0x35, FX32_CONST(0.5));
    }
}

static const BattleEventHandlerEntry sHandlersFlamePlate[] = {
    { 0x38, HandlerCharcoal },
};

static const BattleEventHandlerEntry *EventAddFlamePlate(u32 *priority) {
    *priority = NELEMS(sHandlersFlamePlate);
    return sHandlersFlamePlate;
}

static const BattleEventHandlerEntry sHandlersSplashPlate[] = {
    { 0x38, HandlerMysticWater },
};

static const BattleEventHandlerEntry *EventAddSplashPlate(u32 *priority) {
    *priority = NELEMS(sHandlersSplashPlate);
    return sHandlersSplashPlate;
}

static const BattleEventHandlerEntry sHandlersZapPlate[] = {
    { 0x38, HandlerMagnet },
};

static const BattleEventHandlerEntry *EventAddZapPlate(u32 *priority) {
    *priority = NELEMS(sHandlersZapPlate);
    return sHandlersZapPlate;
}

static const BattleEventHandlerEntry sHandlersMeadowPlate[] = {
    { 0x38, HandlerMiracleSeed },
};

static const BattleEventHandlerEntry *EventAddMeadowPlate(u32 *priority) {
    *priority = NELEMS(sHandlersMeadowPlate);
    return sHandlersMeadowPlate;
}

static const BattleEventHandlerEntry sHandlersIciclePlate[] = {
    { 0x38, HandlerNeverMeltIce },
};

static const BattleEventHandlerEntry *EventAddIciclePlate(u32 *priority) {
    *priority = NELEMS(sHandlersIciclePlate);
    return sHandlersIciclePlate;
}

static const BattleEventHandlerEntry sHandlersFistPlate[] = {
    { 0x38, HandlerBlackBelt },
};

static const BattleEventHandlerEntry *EventAddFistPlate(u32 *priority) {
    *priority = NELEMS(sHandlersFistPlate);
    return sHandlersFistPlate;
}

static const BattleEventHandlerEntry sHandlersToxicPlate[] = {
    { 0x38, HandlerPoisonBarb },
};

static const BattleEventHandlerEntry *EventAddToxicPlate(u32 *priority) {
    *priority = NELEMS(sHandlersToxicPlate);
    return sHandlersToxicPlate;
}

static const BattleEventHandlerEntry sHandlersEarthPlate[] = {
    { 0x38, HandlerSoftSand },
};

static const BattleEventHandlerEntry *EventAddEarthPlate(u32 *priority) {
    *priority = NELEMS(sHandlersEarthPlate);
    return sHandlersEarthPlate;
}

static const BattleEventHandlerEntry sHandlersSkyPlate[] = {
    { 0x38, HandlerSharpBeak },
};

static const BattleEventHandlerEntry *EventAddSkyPlate(u32 *priority) {
    *priority = NELEMS(sHandlersSkyPlate);
    return sHandlersSkyPlate;
}

static const BattleEventHandlerEntry sHandlersMindPlate[] = {
    { 0x38, HandlerTwistedSpoon },
};

static const BattleEventHandlerEntry *EventAddMindPlate(u32 *priority) {
    *priority = NELEMS(sHandlersMindPlate);
    return sHandlersMindPlate;
}

static const BattleEventHandlerEntry sHandlersInsectPlate[] = {
    { 0x38, HandlerSilverPowder },
};

static const BattleEventHandlerEntry *EventAddInsectPlate(u32 *priority) {
    *priority = NELEMS(sHandlersInsectPlate);
    return sHandlersInsectPlate;
}

static const BattleEventHandlerEntry sHandlersStonePlate[] = {
    { 0x38, HandlerHardStone },
};

static const BattleEventHandlerEntry *EventAddStonePlate(u32 *priority) {
    *priority = NELEMS(sHandlersStonePlate);
    return sHandlersStonePlate;
}

static const BattleEventHandlerEntry sHandlersSpookyPlate[] = {
    { 0x38, HandlerSpellTag },
};

static const BattleEventHandlerEntry *EventAddSpookyPlate(u32 *priority) {
    *priority = NELEMS(sHandlersSpookyPlate);
    return sHandlersSpookyPlate;
}

static const BattleEventHandlerEntry sHandlersDracoPlate[] = {
    { 0x38, HandlerDragonFang },
};

static const BattleEventHandlerEntry *EventAddDracoPlate(u32 *priority) {
    *priority = NELEMS(sHandlersDracoPlate);
    return sHandlersDracoPlate;
}

static const BattleEventHandlerEntry sHandlersDreadPlate[] = {
    { 0x38, HandlerBlackGlasses },
};

static const BattleEventHandlerEntry *EventAddDreadPlate(u32 *priority) {
    *priority = NELEMS(sHandlersDreadPlate);
    return sHandlersDreadPlate;
}

static const BattleEventHandlerEntry sHandlersIronPlate[] = {
    { 0x38, HandlerMetalCoat },
};

static const BattleEventHandlerEntry *EventAddIronPlate(u32 *priority) {
    *priority = NELEMS(sHandlersIronPlate);
    return sHandlersIronPlate;
}

static const BattleEventHandlerEntry sHandlersBigRoot[] = {
    { 0x8c, HandlerPlate },
};

static const BattleEventHandlerEntry *func_ov167_021c4b8c(u32 *priority) {
    *priority = NELEMS(sHandlersBigRoot);
    return sHandlersBigRoot;
}

static void HandlerPlate(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_MulValue(0x35, ItemAttackValueToRatio(item));
    }
}

static const BattleEventHandlerEntry sHandlersSmokeBall[] = {
    { 0xb, HandlerSmokeBall },
    { 0xd, HandlerSmokeBallMessage },
};

static const BattleEventHandlerEntry *EventAddSmokeBall(u32 *priority) {
    *priority = NELEMS(sHandlersSmokeBall);
    return sHandlersSmokeBall;
}

static void HandlerSmokeBall(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonRunCalcSkip(item, flow, monId, work);
}

static void HandlerSmokeBallMessage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *param;

    if (CommonCheckRunMessage(item, flow, monId)) {
        param = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&param->string, 2, 0x3ec);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
        BattleHandler_AddSoundEffect(&param->string, 0x56a);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersAmuletCoin[] = {
    { 0x55, HandlerAmuletCoin },
    { 0x58, func_ov167_021c4c48 },
};

static const BattleEventHandlerEntry *EventAddAmuletCoin(u32 *priority) {
    *priority = NELEMS(sHandlersAmuletCoin);
    return sHandlersAmuletCoin;
}

static void HandlerAmuletCoin(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        func_ov167_021abf48(flow, monId);
    }
}

static void func_ov167_021c4c48(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    func_ov167_021abf48(flow, monId);
}

static const BattleEventHandlerEntry sHandlersGriseousOrb[] = {
    { 0x38, HandlerGriseousOrb },
};

static const BattleEventHandlerEntry *EventAddGriseousOrb(u32 *priority) {
    *priority = NELEMS(sHandlersGriseousOrb);
    return sHandlersGriseousOrb;
}

static void HandlerGriseousOrb(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 type;

    if (BattleEventVar_GetValue(3) == monId && GetBattleMonSpecies(GetBattleMon(flow, monId)) == SPECIES_GIRATINA) {
        type = BattleEventVar_GetValue(0x16);
        if (type == TYPE_DRAGON || type == TYPE_GHOST) {
            BattleEventVar_MulValue(0x31, FX32_CONST(1.2));
        }
    }
}

static const BattleEventHandlerEntry sHandlersIcyRock[] = {
    { 0x7c, HandlerIcyRock },
};

static const BattleEventHandlerEntry *EventAddIcyRock(u32 *priority) {
    *priority = NELEMS(sHandlersIcyRock);
    return sHandlersIcyRock;
}

static void HandlerIcyRock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonWeatherMoveIncreaseTurns(item, flow, monId, BTL_WEATHER_HAIL);
}

static const BattleEventHandlerEntry sHandlersSmoothRock[] = {
    { 0x7c, HandlerSmoothRock },
};

static const BattleEventHandlerEntry *EventAddSmoothRock(u32 *priority) {
    *priority = NELEMS(sHandlersSmoothRock);
    return sHandlersSmoothRock;
}

static void HandlerSmoothRock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonWeatherMoveIncreaseTurns(item, flow, monId, BTL_WEATHER_SANDSTORM);
}

static const BattleEventHandlerEntry sHandlersHeatRock[] = {
    { 0x7c, HandlerHeatRock },
};

static const BattleEventHandlerEntry *EventAddHeatRock(u32 *priority) {
    *priority = NELEMS(sHandlersHeatRock);
    return sHandlersHeatRock;
}

static void HandlerHeatRock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonWeatherMoveIncreaseTurns(item, flow, monId, BTL_WEATHER_SUN);
}

static const BattleEventHandlerEntry sHandlersDampRock[] = {
    { 0x7c, HandlerDampRock },
};

static const BattleEventHandlerEntry *EventAddDampRock(u32 *priority) {
    *priority = NELEMS(sHandlersDampRock);
    return sHandlersDampRock;
}

static void HandlerDampRock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonWeatherMoveIncreaseTurns(item, flow, monId, BTL_WEATHER_RAIN);
}

static void CommonWeatherMoveIncreaseTurns(BattleEventItem *item, BtlServerFlow *flow, u8 monId, u8 weather) {
    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x39) == weather) {
        BattleEventVar_RewriteValue(0x24, CommonGetItemParam(item, 2));
    }
}

static const BattleEventHandlerEntry sHandlersLightBall[] = {
    { 0x3b, HandlerLightBall },
    { 0x73, HandlerLightBallUseTemp },
};

static const BattleEventHandlerEntry *EventAddLightBall(u32 *priority) {
    *priority = NELEMS(sHandlersLightBall);
    return sHandlersLightBall;
}

static void HandlerLightBall(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && GetBattleMonSpecies(GetBattleMon(flow, monId)) == SPECIES_PIKACHU) {
        BattleEventVar_MulValue(0x35, FX32_CONST(2));
    }
}

static void HandlerLightBallUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 attackerId;
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        attackerId = BattleEventVar_GetValue(3);
        param = BattleHandler_PushWork(flow, 0xc, attackerId);
        param->targetIndex = monId;
        param->condition = 1;
        param->value = func_ov167_021bd52c(1);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersToxicOrb[] = {
    { 0x77, HandlerToxicOrb },
    { 0x73, HandlerToxicOrbUseTemp },
};

static const BattleEventHandlerEntry *EventAddToxicOrb(u32 *priority) {
    *priority = NELEMS(sHandlersToxicOrb);
    return sHandlersToxicOrb;
}

static void HandlerToxicOrb(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        param = BattleHandler_PushWork(flow, 0xc, monId);
        param->condition = 5;
        param->value = func_ov167_021ce298();
        param->showFail = 0;
        param->targetIndex = monId;
        BattleHandler_StrSetup(&param->string, 2, 0xf0);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
        BattleHandler_PopWork(flow, param);
    }
}

static void HandlerToxicOrbUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 attackerId;
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        attackerId = BattleEventVar_GetValue(3);
        param = BattleHandler_PushWork(flow, 0xc, attackerId);
        param->targetIndex = monId;
        param->condition = 5;
        param->value = func_ov167_021ce298();
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersFlameOrb[] = {
    { 0x77, HandlerFlameOrb },
    { 0x73, HandlerFlameOrbUseTemp },
};

static const BattleEventHandlerEntry *EventAddFlameOrb(u32 *priority) {
    *priority = NELEMS(sHandlersFlameOrb);
    return sHandlersFlameOrb;
}

static void HandlerFlameOrb(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        param = BattleHandler_PushWork(flow, 0xc, monId);
        param->condition = 4;
        param->value = func_ov167_021bd52c(4);
        param->showFail = 0;
        param->targetIndex = monId;
        param->string.args[1] = BattleEventItem_GetSubID(item);
        BattleHandler_StrSetup(&param->string, 2, 0x102);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
        BattleHandler_PopWork(flow, param);
    }
}

static void HandlerFlameOrbUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 attackerId;
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        attackerId = BattleEventVar_GetValue(3);
        param = BattleHandler_PushWork(flow, 0xc, attackerId);
        param->targetIndex = monId;
        param->condition = 4;
        param->value = func_ov167_021bd52c(4);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersSilverPowder[] = {
    { 0x38, HandlerSilverPowder },
};

static const BattleEventHandlerEntry *EventAddSilverPowder(u32 *priority) {
    *priority = NELEMS(sHandlersSilverPowder);
    return sHandlersSilverPowder;
}

static void HandlerSilverPowder(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_BUG);
}

static const BattleEventHandlerEntry sHandlersSoftSand[] = {
    { 0x38, HandlerSoftSand },
};

static const BattleEventHandlerEntry *EventAddSoftSand(u32 *priority) {
    *priority = NELEMS(sHandlersSoftSand);
    return sHandlersSoftSand;
}

static void HandlerSoftSand(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_GROUND);
}

static const BattleEventHandlerEntry sHandlersHardStone[] = {
    { 0x38, HandlerHardStone },
};

static const BattleEventHandlerEntry *EventAddHardStone(u32 *priority) {
    *priority = NELEMS(sHandlersHardStone);
    return sHandlersHardStone;
}

static void HandlerHardStone(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_ROCK);
}

static const BattleEventHandlerEntry sHandlersMiracleSeed[] = {
    { 0x38, HandlerMiracleSeed },
};

static const BattleEventHandlerEntry *EventAddMiracleSeed(u32 *priority) {
    *priority = NELEMS(sHandlersMiracleSeed);
    return sHandlersMiracleSeed;
}

static void HandlerMiracleSeed(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_GRASS);
}

static const BattleEventHandlerEntry sHandlersBlackGlasses[] = {
    { 0x38, HandlerBlackGlasses },
};

static const BattleEventHandlerEntry *EventAddBlackGlasses(u32 *priority) {
    *priority = NELEMS(sHandlersBlackGlasses);
    return sHandlersBlackGlasses;
}

static void HandlerBlackGlasses(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_DARK);
}

static const BattleEventHandlerEntry sHandlersBlackBelt[] = {
    { 0x38, HandlerBlackBelt },
};

static const BattleEventHandlerEntry *EventAddBlackBelt(u32 *priority) {
    *priority = NELEMS(sHandlersBlackBelt);
    return sHandlersBlackBelt;
}

static void HandlerBlackBelt(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_FIGHTING);
}

static const BattleEventHandlerEntry sHandlersMagnet[] = {
    { 0x38, HandlerMagnet },
};

static const BattleEventHandlerEntry *EventAddMagnet(u32 *priority) {
    *priority = NELEMS(sHandlersMagnet);
    return sHandlersMagnet;
}

static void HandlerMagnet(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_ELECTRIC);
}

static const BattleEventHandlerEntry sHandlersMetalCoat[] = {
    { 0x38, HandlerMetalCoat },
};

static const BattleEventHandlerEntry *EventAddMetalCoat(u32 *priority) {
    *priority = NELEMS(sHandlersMetalCoat);
    return sHandlersMetalCoat;
}

static void HandlerMetalCoat(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_STEEL);
}

static const BattleEventHandlerEntry sHandlersMysticWater[] = {
    { 0x38, HandlerMysticWater },
};

static const BattleEventHandlerEntry *EventAddMysticWater(u32 *priority) {
    *priority = NELEMS(sHandlersMysticWater);
    return sHandlersMysticWater;
}

static void HandlerMysticWater(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_WATER);
}

static const BattleEventHandlerEntry sHandlersSharpBeak[] = {
    { 0x38, HandlerSharpBeak },
};

static const BattleEventHandlerEntry *EventAddSharpBeak(u32 *priority) {
    *priority = NELEMS(sHandlersSharpBeak);
    return sHandlersSharpBeak;
}

static void HandlerSharpBeak(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_FLYING);
}

static const BattleEventHandlerEntry sHandlersPoisonBarb[] = {
    { 0x38, HandlerPoisonBarb },
    { 0x73, HandlerPoisonBarbUseTemp },
};

static const BattleEventHandlerEntry *EventAddPoisonBarb(u32 *priority) {
    *priority = NELEMS(sHandlersPoisonBarb);
    return sHandlersPoisonBarb;
}

static void HandlerPoisonBarb(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_POISON);
}

static void HandlerPoisonBarbUseTemp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 attackerId;
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        attackerId = BattleEventVar_GetValue(3);
        param = BattleHandler_PushWork(flow, 0xc, attackerId);
        param->targetIndex = monId;
        param->condition = 5;
        param->value = func_ov167_021bd52c(5);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersNeverMeltIce[] = {
    { 0x38, HandlerNeverMeltIce },
};

static const BattleEventHandlerEntry *EventAddNeverMeltIce(u32 *priority) {
    *priority = NELEMS(sHandlersNeverMeltIce);
    return sHandlersNeverMeltIce;
}

static void HandlerNeverMeltIce(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_ICE);
}

static const BattleEventHandlerEntry sHandlersSpellTag[] = {
    { 0x38, HandlerSpellTag },
};

static const BattleEventHandlerEntry *EventAddSpellTag(u32 *priority) {
    *priority = NELEMS(sHandlersSpellTag);
    return sHandlersSpellTag;
}

static void HandlerSpellTag(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_GHOST);
}

static const BattleEventHandlerEntry sHandlersTwistedSpoon[] = {
    { 0x38, HandlerTwistedSpoon },
};

static const BattleEventHandlerEntry *EventAddTwistedSpoon(u32 *priority) {
    *priority = NELEMS(sHandlersTwistedSpoon);
    return sHandlersTwistedSpoon;
}

static void HandlerTwistedSpoon(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_PSYCHIC);
}

static const BattleEventHandlerEntry sHandlersCharcoal[] = {
    { 0x38, HandlerCharcoal },
};

static const BattleEventHandlerEntry *EventAddCharcoal(u32 *priority) {
    *priority = NELEMS(sHandlersCharcoal);
    return sHandlersCharcoal;
}

static void HandlerCharcoal(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_FIRE);
}

static const BattleEventHandlerEntry sHandlersDragonFang[] = {
    { 0x38, HandlerDragonFang },
};

static const BattleEventHandlerEntry *EventAddDragonFang(u32 *priority) {
    *priority = NELEMS(sHandlersDragonFang);
    return sHandlersDragonFang;
}

static void HandlerDragonFang(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_DRAGON);
}

static const BattleEventHandlerEntry sHandlersSilkScarf[] = {
    { 0x38, HandlerSilkScarf },
};

static const BattleEventHandlerEntry *EventAddSilkScarf(u32 *priority) {
    *priority = NELEMS(sHandlersSilkScarf);
    return sHandlersSilkScarf;
}

static void HandlerSilkScarf(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_NORMAL);
}

static const BattleEventHandlerEntry sHandlersOddIncense[] = {
    { 0x38, HandlerOddIncense },
};

static const BattleEventHandlerEntry *EventAddOddIncense(u32 *priority) {
    *priority = NELEMS(sHandlersOddIncense);
    return sHandlersOddIncense;
}

static void HandlerOddIncense(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_PSYCHIC);
}

static const BattleEventHandlerEntry sHandlersRockIncense[] = {
    { 0x38, HandlerRockIncense },
};

static const BattleEventHandlerEntry *EventAddRockIncense(u32 *priority) {
    *priority = NELEMS(sHandlersRockIncense);
    return sHandlersRockIncense;
}

static void HandlerRockIncense(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_ROCK);
}

static const BattleEventHandlerEntry sHandlersWaveIncense[] = {
    { 0x38, HandlerWaveIncense },
};

static const BattleEventHandlerEntry *EventAddWaveIncense(u32 *priority) {
    *priority = NELEMS(sHandlersWaveIncense);
    return sHandlersWaveIncense;
}

static void HandlerWaveIncense(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_WATER);
}

static const BattleEventHandlerEntry sHandlersSeaIncense[] = {
    { 0x38, HandlerSeaIncense },
};

static const BattleEventHandlerEntry *EventAddSeaIncense(u32 *priority) {
    *priority = NELEMS(sHandlersSeaIncense);
    return sHandlersSeaIncense;
}

static void HandlerSeaIncense(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_WATER);
}

static const BattleEventHandlerEntry sHandlersRoseIncense[] = {
    { 0x38, HandlerRoseIncense },
};

static const BattleEventHandlerEntry *EventAddRoseIncense(u32 *priority) {
    *priority = NELEMS(sHandlersRoseIncense);
    return sHandlersRoseIncense;
}

static void HandlerRoseIncense(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonTypeBoostingItem(item, flow, monId, TYPE_GRASS);
}

static void CommonTypeBoostingItem(BattleEventItem *item, BtlServerFlow *flow, u8 monId, u8 type) {
    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x16) == type) {
        BattleEventVar_MulValue(0x31, ItemAttackValueToRatio(item));
    }
}

static const BattleEventHandlerEntry sHandlersLustrousOrb[] = {
    { 0x38, HandlerLustrousOrb },
};

static const BattleEventHandlerEntry *EventAddLustrousOrb(u32 *priority) {
    *priority = NELEMS(sHandlersLustrousOrb);
    return sHandlersLustrousOrb;
}

static void HandlerLustrousOrb(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 type;

    if (BattleEventVar_GetValue(3) == monId && GetBattleMonSpecies(GetBattleMon(flow, monId)) == SPECIES_PALKIA) {
        type = BattleEventVar_GetValue(0x16);
        if (type == TYPE_DRAGON || type == TYPE_WATER) {
            BattleEventVar_MulValue(0x31, ItemAttackValueToRatio(item));
        }
    }
}

static const BattleEventHandlerEntry sHandlersAdamantOrb[] = {
    { 0x38, HandlerAdamantOrb },
};

static const BattleEventHandlerEntry *EventAddAdamantOrb(u32 *priority) {
    *priority = NELEMS(sHandlersAdamantOrb);
    return sHandlersAdamantOrb;
}

static void HandlerAdamantOrb(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 type;

    if (BattleEventVar_GetValue(3) == monId && GetBattleMonSpecies(GetBattleMon(flow, monId)) == SPECIES_DIALGA) {
        type = BattleEventVar_GetValue(0x16);
        if (type == TYPE_DRAGON || type == TYPE_STEEL) {
            BattleEventVar_MulValue(0x31, ItemAttackValueToRatio(item));
        }
    }
}

static const BattleEventHandlerEntry sHandlersIronBall[] = {
    { 0x13, HandlerIronBallCalcSpeed },
    { 0x12, HandlerIronBallCheckFly },
};

static const BattleEventHandlerEntry *EventAddIronBall(u32 *priority) {
    *priority = NELEMS(sHandlersIronBall);
    return sHandlersIronBall;
}

static void HandlerIronBallCalcSpeed(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        BattleEventVar_MulValue(0x35, FX32_CONST(0.5));
    }
}

static void HandlerIronBallCheckFly(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        BattleEventVar_RewriteValue(0x41, 1);
    }
}

static const BattleEventHandlerEntry sHandlersFloatStone[] = {
    { 0x7b, HandlerFloatStone },
};

static const BattleEventHandlerEntry *EventAddFloatStone(u32 *priority) {
    *priority = NELEMS(sHandlersFloatStone);
    return sHandlersFloatStone;
}

static void HandlerFloatStone(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        BattleEventVar_MulValue(0x35, FX32_CONST(0.5));
    }
}

static const BattleEventHandlerEntry sHandlersEviolite[] = {
    { 0x3c, HandlerEviolite },
};

static const BattleEventHandlerEntry *EventAddEviolite(u32 *priority) {
    *priority = NELEMS(sHandlersEviolite);
    return sHandlersEviolite;
}

static void HandlerEviolite(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(4) == monId && CheckEvolution(flow, monId)) {
        BattleEventVar_MulValue(0x35, FX32_CONST(1.5));
    }
}

static const BattleEventHandlerEntry sHandlersRockyHelmet[] = {
    { 0x4b, HandlerRockyHelmet },
};

static const BattleEventHandlerEntry *EventAddRockyHelmet(u32 *priority) {
    *priority = NELEMS(sHandlersRockyHelmet);
    return sHandlersRockyHelmet;
}

static void HandlerRockyHelmet(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerDamageParam *param;
    BattleMon *attacker;
    u8 divisor;

    if (BattleEventVar_GetValue(4) == monId && BattleEventVar_GetValue(0x46) == 0 &&
        getMoveFlag(BattleEventVar_GetValue(0x12), 0)) {
        param = BattleHandler_PushWork(flow, 7, monId);
        param->targetIndex = BattleEventVar_GetValue(3);
        attacker = GetBattleMon(flow, param->targetIndex);
        divisor = CommonGetItemParam(item, 2);
        param->amount = DivideMaxHPZeroCheck(attacker, divisor);
        BattleHandler_StrSetup(&param->string, 2, 0x1a8);
        BattleHandler_AddArg(&param->string, param->targetIndex);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersAirBalloon[] = {
    { 0x55, HandlerAirBalloonMemberIn },
    { 0x12, HandlerAirBalloonCheckFlying },
    { 0x4b, HandlerAirBalloonDamageReaction },
};

static const BattleEventHandlerEntry *EventAddAirBalloon(u32 *priority) {
    *priority = NELEMS(sHandlersAirBalloon);
    return sHandlersAirBalloon;
}

static void HandlerAirBalloonMemberIn(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *param;

    if (BattleEventVar_GetValue(2) == monId && !IsFieldEffectActive(FIELD_CONDITION_GRAVITY)) {
        param = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&param->string, 2, 0x198);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_PopWork(flow, param);
    }
}

static void HandlerAirBalloonCheckFlying(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        BattleEventVar_RewriteValue(0x51, 1);
    }
}

static void HandlerAirBalloonDamageReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerSetItemParam *param;

    if (BattleEventVar_GetValue(4) == monId) {
        param = BattleHandler_PushWork(flow, 0x20, monId);
        param->targetIndex = monId;
        param->item = ITEM_NONE;
        BattleHandler_StrSetup(&param->string, 2, 0x19b);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersRedCard[] = {
    { 0x83, HandlerRedCard },
};

static const BattleEventHandlerEntry *EventAddRedCard(u32 *priority) {
    *priority = NELEMS(sHandlersRedCard);
    return sHandlersRedCard;
}

static void HandlerRedCard(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 attackerId;
    BattleHandlerConsumeItemParam *consumeParam;
    BattleHandlerForceSwitchParam *switchParam;

    if (func_ov167_021cde38(monId)) {
        attackerId = BattleEventVar_GetValue(3);
        if (func_ov167_021ab840(flow, attackerId) != 6 && BattleEventVar_GetValue(0x4d) == 0 &&
            func_ov167_021aba64(flow, attackerId) && !func_ov167_021abf14(flow)) {
            consumeParam = BattleHandler_PushWork(flow, 0x23, monId);
            BattleHandler_StrSetup(&consumeParam->string, 2, 0x1a1);
            BattleHandler_AddArg(&consumeParam->string, monId);
            BattleHandler_AddArg(&consumeParam->string, attackerId);
            BattleHandler_PopWork(flow, consumeParam);
            switchParam = BattleHandler_PushWork(flow, 0x2e, monId);
            switchParam->targetIndex = BattleEventVar_GetValue(3);
            switchParam->ignoreLevel = 1;
            switchParam->effect = 0x28c;
            BattleHandler_PopWork(flow, switchParam);
        }
    }
}

static const BattleEventHandlerEntry sHandlersRingTarget[] = {
    { 0x3e, HandlerRingTarget },
};

static const BattleEventHandlerEntry *EventAddRingTarget(u32 *priority) {
    *priority = NELEMS(sHandlersRingTarget);
    return sHandlersRingTarget;
}

static void HandlerRingTarget(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (!func_ov167_021aba04(flow) && BattleEventVar_GetValue(4) == monId) {
        BattleEventVar_RewriteValue(0x4b, 1);
    }
}

static const BattleEventHandlerEntry sHandlersBindingBand[] = {
    { 0x62, HandlerBindingBand },
};

static const BattleEventHandlerEntry *EventAddBindingBand(u32 *priority) {
    *priority = NELEMS(sHandlersBindingBand);
    return sHandlersBindingBand;
}

static void HandlerBindingBand(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleCondition condition;

    if (BattleEventVar_GetValue(3) == monId) {
        condition.raw = BattleEventVar_GetValue(0x1e);
        SetConditionFlag(&condition, 1);
        BattleEventVar_RewriteValue(0x1e, condition.raw);
    }
}

static const BattleEventHandlerEntry sHandlersAbsorbBulb[] = {
    { 0x4b, HandlerAbsorbBulbDamageReaction },
    { 0x72, HandlerAbsorbBulbUse },
};

static const BattleEventHandlerEntry *EventAddAbsorbBulb(u32 *priority) {
    *priority = NELEMS(sHandlersAbsorbBulb);
    return sHandlersAbsorbBulb;
}

static void HandlerAbsorbBulbDamageReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(4) == monId && BattleEventVar_GetValue(0x46) == 0 &&
        BattleEventVar_GetValue(0x16) == TYPE_WATER &&
        IsStatChangeValid(GetBattleMon(flow, monId), BATTLEMON_SP_ATTACK_STAGE, 1)) {
        ItemEvent_PushRun(item, flow, monId);
    }
}

static void HandlerAbsorbBulbUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerStatChangeParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        param = BattleHandler_PushWork(flow, 0xe, monId);
        param->count = 1;
        param->monIds[0] = monId;
        param->stat = BATTLEMON_SP_ATTACK_STAGE;
        param->change = 1;
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersCellBattery[] = {
    { 0x4b, HandlerCellBatteryDamageReaction },
    { 0x72, HandlerCellBatteryUse },
};

static const BattleEventHandlerEntry *EventAddCellBattery(u32 *priority) {
    *priority = NELEMS(sHandlersCellBattery);
    return sHandlersCellBattery;
}

static void HandlerCellBatteryDamageReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(4) == monId && BattleEventVar_GetValue(0x46) == 0 &&
        BattleEventVar_GetValue(0x16) == TYPE_ELECTRIC &&
        IsStatChangeValid(GetBattleMon(flow, monId), BATTLEMON_ATTACK_STAGE, 1)) {
        ItemEvent_PushRun(item, flow, monId);
    }
}

static void HandlerCellBatteryUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerStatChangeParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        param = BattleHandler_PushWork(flow, 0xe, monId);
        param->count = 1;
        param->monIds[0] = monId;
        param->stat = BATTLEMON_ATTACK_STAGE;
        param->change = 1;
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersEjectButton[] = {
    { 0x83, HandlerEjectButtonReaction },
    { 0x72, HandlerEjectButtonUse },
};

static const BattleEventHandlerEntry *EventAddEjectButton(u32 *priority) {
    *priority = NELEMS(sHandlersEjectButton);
    return sHandlersEjectButton;
}

static void HandlerEjectButtonReaction(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (func_ov167_021cde38(monId) && BattleEventVar_GetValue(0x4d) == 0 && !func_ov167_021abeb4(flow, monId) &&
        !func_ov167_021abf14(flow) && !IsMonSwitchingOut(flow) && func_ov167_021aba64(flow, monId) &&
        func_ov167_021aba8c(flow)) {
        ItemEvent_PushRun(item, flow, monId);
    }
}

static void HandlerEjectButtonUse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *messageParam;
    BattleHandlerSwitchParam *switchParam;

    if (BattleEventVar_GetValue(2) == monId && func_ov167_021aba64(flow, monId)) {
        messageParam = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&messageParam->string, 2, 0x19e);
        BattleHandler_AddArg(&messageParam->string, monId);
        BattleHandler_PopWork(flow, messageParam);
        switchParam = BattleHandler_PushWork(flow, 0x29, monId);
        switchParam->monIndex = monId;
        BattleHandler_PopWork(flow, switchParam);
    }
}

static const BattleEventHandlerEntry sHandlersFireGem[] = {
    { 0x81, HandlerFireGemDecide },
    { 0x38, HandlerFireGemPower },
    { 0x88, HandlerGemEnd },
};

static const BattleEventHandlerEntry *EventAddFireGem(u32 *priority) {
    *priority = NELEMS(sHandlersFireGem);
    return sHandlersFireGem;
}

static void HandlerFireGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemPower(item, flow, monId, work, TYPE_FIRE);
}

static void HandlerFireGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemDecide(item, flow, monId, work, TYPE_FIRE);
}

static const BattleEventHandlerEntry sHandlersWaterGem[] = {
    { 0x81, HandlerWaterGemDecide },
    { 0x38, HandlerWaterGemPower },
    { 0x88, HandlerGemEnd },
};

static const BattleEventHandlerEntry *EventAddWaterGem(u32 *priority) {
    *priority = NELEMS(sHandlersWaterGem);
    return sHandlersWaterGem;
}

static void HandlerWaterGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemPower(item, flow, monId, work, TYPE_WATER);
}

static void HandlerWaterGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemDecide(item, flow, monId, work, TYPE_WATER);
}

static const BattleEventHandlerEntry sHandlersElectricGem[] = {
    { 0x81, HandlerElectricGemDecide },
    { 0x38, HandlerElectricGemPower },
    { 0x88, HandlerGemEnd },
};

static const BattleEventHandlerEntry *EventAddElectricGem(u32 *priority) {
    *priority = NELEMS(sHandlersElectricGem);
    return sHandlersElectricGem;
}

static void HandlerElectricGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemPower(item, flow, monId, work, TYPE_ELECTRIC);
}

static void HandlerElectricGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemDecide(item, flow, monId, work, TYPE_ELECTRIC);
}

static const BattleEventHandlerEntry sHandlersGrassGem[] = {
    { 0x81, HandlerGrassGemDecide },
    { 0x38, HandlerGrassGemPower },
    { 0x88, HandlerGemEnd },
};

static const BattleEventHandlerEntry *EventAddGrassGem(u32 *priority) {
    *priority = NELEMS(sHandlersGrassGem);
    return sHandlersGrassGem;
}

static void HandlerGrassGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemPower(item, flow, monId, work, TYPE_GRASS);
}

static void HandlerGrassGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemDecide(item, flow, monId, work, TYPE_GRASS);
}

static const BattleEventHandlerEntry sHandlersIceGem[] = {
    { 0x81, HandlerIceGemDecide },
    { 0x38, HandlerIceGemPower },
    { 0x88, HandlerGemEnd },
};

static const BattleEventHandlerEntry *EventAddIceGem(u32 *priority) {
    *priority = NELEMS(sHandlersIceGem);
    return sHandlersIceGem;
}

static void HandlerIceGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemPower(item, flow, monId, work, TYPE_ICE);
}

static void HandlerIceGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemDecide(item, flow, monId, work, TYPE_ICE);
}

static const BattleEventHandlerEntry sHandlersFightingGem[] = {
    { 0x81, HandlerFightingGemDecide },
    { 0x38, HandlerFightingGemPower },
    { 0x88, HandlerGemEnd },
};

static const BattleEventHandlerEntry *EventAddFightingGem(u32 *priority) {
    *priority = NELEMS(sHandlersFightingGem);
    return sHandlersFightingGem;
}

static void HandlerFightingGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemPower(item, flow, monId, work, TYPE_FIGHTING);
}

static void HandlerFightingGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemDecide(item, flow, monId, work, TYPE_FIGHTING);
}

static const BattleEventHandlerEntry sHandlersPoisonGem[] = {
    { 0x81, HandlerPoisonGemDecide },
    { 0x38, HandlerPoisonGemPower },
    { 0x88, HandlerGemEnd },
};

static const BattleEventHandlerEntry *EventAddPoisonGem(u32 *priority) {
    *priority = NELEMS(sHandlersPoisonGem);
    return sHandlersPoisonGem;
}

static void HandlerPoisonGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemPower(item, flow, monId, work, TYPE_POISON);
}

static void HandlerPoisonGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemDecide(item, flow, monId, work, TYPE_POISON);
}

static const BattleEventHandlerEntry sHandlersGroundGem[] = {
    { 0x81, HandlerGroundGemDecide },
    { 0x38, HandlerGroundGemPower },
    { 0x88, HandlerGemEnd },
};

static const BattleEventHandlerEntry *EventAddGroundGem(u32 *priority) {
    *priority = NELEMS(sHandlersGroundGem);
    return sHandlersGroundGem;
}

static void HandlerGroundGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemPower(item, flow, monId, work, TYPE_GROUND);
}

static void HandlerGroundGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemDecide(item, flow, monId, work, TYPE_GROUND);
}

static const BattleEventHandlerEntry sHandlersFlyingGem[] = {
    { 0x81, HandlerFlyingGemDecide },
    { 0x38, HandlerFlyingGemPower },
    { 0x88, HandlerGemEnd },
};

static const BattleEventHandlerEntry *EventAddFlyingGem(u32 *priority) {
    *priority = NELEMS(sHandlersFlyingGem);
    return sHandlersFlyingGem;
}

static void HandlerFlyingGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemPower(item, flow, monId, work, TYPE_FLYING);
}

static void HandlerFlyingGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemDecide(item, flow, monId, work, TYPE_FLYING);
}

static const BattleEventHandlerEntry sHandlersPsychicGem[] = {
    { 0x81, HandlerPsychicGemDecide },
    { 0x38, HandlerPsychicGemPower },
    { 0x88, HandlerGemEnd },
};

static const BattleEventHandlerEntry *EventAddPsychicGem(u32 *priority) {
    *priority = NELEMS(sHandlersPsychicGem);
    return sHandlersPsychicGem;
}

static void HandlerPsychicGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemPower(item, flow, monId, work, TYPE_PSYCHIC);
}

static void HandlerPsychicGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemDecide(item, flow, monId, work, TYPE_PSYCHIC);
}

static const BattleEventHandlerEntry sHandlersBugGem[] = {
    { 0x81, HandlerBugGemDecide },
    { 0x38, HandlerBugGemPower },
    { 0x88, HandlerGemEnd },
};

static const BattleEventHandlerEntry *EventAddBugGem(u32 *priority) {
    *priority = NELEMS(sHandlersBugGem);
    return sHandlersBugGem;
}

static void HandlerBugGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemPower(item, flow, monId, work, TYPE_BUG);
}

static void HandlerBugGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemDecide(item, flow, monId, work, TYPE_BUG);
}

static const BattleEventHandlerEntry sHandlersRockGem[] = {
    { 0x81, HandlerRockGemDecide },
    { 0x38, HandlerRockGemPower },
    { 0x88, HandlerGemEnd },
};

static const BattleEventHandlerEntry *EventAddRockGem(u32 *priority) {
    *priority = NELEMS(sHandlersRockGem);
    return sHandlersRockGem;
}

static void HandlerRockGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemPower(item, flow, monId, work, TYPE_ROCK);
}

static void HandlerRockGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemDecide(item, flow, monId, work, TYPE_ROCK);
}

static const BattleEventHandlerEntry sHandlersGhostGem[] = {
    { 0x81, HandlerGhostGemDecide },
    { 0x38, HandlerGhostGemPower },
    { 0x88, HandlerGemEnd },
};

static const BattleEventHandlerEntry *EventAddGhostGem(u32 *priority) {
    *priority = NELEMS(sHandlersGhostGem);
    return sHandlersGhostGem;
}

static void HandlerGhostGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemPower(item, flow, monId, work, TYPE_GHOST);
}

static void HandlerGhostGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemDecide(item, flow, monId, work, TYPE_GHOST);
}

static const BattleEventHandlerEntry sHandlersDragonGem[] = {
    { 0x81, HandlerDragonGemDecide },
    { 0x38, HandlerDragonGemPower },
    { 0x88, HandlerGemEnd },
};

static const BattleEventHandlerEntry *EventAddDragonGem(u32 *priority) {
    *priority = NELEMS(sHandlersDragonGem);
    return sHandlersDragonGem;
}

static void HandlerDragonGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemPower(item, flow, monId, work, TYPE_DRAGON);
}

static void HandlerDragonGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemDecide(item, flow, monId, work, TYPE_DRAGON);
}

static const BattleEventHandlerEntry sHandlersDarkGem[] = {
    { 0x81, HandlerDarkGemDecide },
    { 0x38, HandlerDarkGemPower },
    { 0x88, HandlerGemEnd },
};

static const BattleEventHandlerEntry *EventAddDarkGem(u32 *priority) {
    *priority = NELEMS(sHandlersDarkGem);
    return sHandlersDarkGem;
}

static void HandlerDarkGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemPower(item, flow, monId, work, TYPE_DARK);
}

static void HandlerDarkGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemDecide(item, flow, monId, work, TYPE_DARK);
}

static const BattleEventHandlerEntry sHandlersSteelGem[] = {
    { 0x81, HandlerSteelGemDecide },
    { 0x38, HandlerSteelGemPower },
    { 0x88, HandlerGemEnd },
};

static const BattleEventHandlerEntry *EventAddSteelGem(u32 *priority) {
    *priority = NELEMS(sHandlersSteelGem);
    return sHandlersSteelGem;
}

static void HandlerSteelGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemPower(item, flow, monId, work, TYPE_STEEL);
}

static void HandlerSteelGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemDecide(item, flow, monId, work, TYPE_STEEL);
}

static const BattleEventHandlerEntry sHandlersNormalGem[] = {
    { 0x81, HandlerNormalGemPower },
    { 0x38, HandlerNormalGemDecide },
    { 0x88, HandlerGemEnd },
};

static const BattleEventHandlerEntry *EventAddNormalGem(u32 *priority) {
    *priority = NELEMS(sHandlersNormalGem);
    return sHandlersNormalGem;
}

// swan's names for Normal Gem's handlers are swapped: this one decides, the next one powers up
static void HandlerNormalGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemDecide(item, flow, monId, work, TYPE_NORMAL);
}

static void HandlerNormalGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonGemPower(item, flow, monId, work, TYPE_NORMAL);
}

static void CommonGemDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work, u8 type) {
    u16 move;
    BattleHandlerConsumeItemParam *param;

    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x16) == type) {
        move = BattleEventVar_GetValue(0x12);
        if (PML_MoveIsDamaging(move) && !func_ov169_0689ca74(move) && !BattleEventItem_IsIsolated(item) &&
            !func_ov167_021aba04(flow)) {
            BattleEventItem_ConvertToIsolated(item);
            param = BattleHandler_PushWork(flow, 0x23, monId);
            BattleHandler_StrSetup(&param->string, 1, 0xb6);
            BattleHandler_AddArg(&param->string, BattleEventItem_GetSubID(item));
            BattleHandler_AddArg(&param->string, BattleEventVar_GetValue(0x12));
            BattleHandler_PopWork(flow, param);
            *work = 1;
        }
    }
}

static void CommonGemPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work, u8 type) {
    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x16) == type) {
        if (*work == 1 || func_ov167_021aba04(flow)) {
            BattleEventVar_MulValue(0x31, FX32_CONST(1.5));
        }
    }
}

static void HandlerGemEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && *work == 1) {
        *work = 2;
    }
}
