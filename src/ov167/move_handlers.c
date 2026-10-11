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
#include "battle/btl_move.h"
#include "battle/btl_ov169.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_server_flow.h"
#include "battle/handler_common.h"
#include "constants/moves.h"
#include "constants/species.h"
#include "gfl/std.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "pml/waza.h"

// One move's handlers: the move and the function that returns its table
typedef struct MoveEventAddEntry {
    u16 move;
    const BattleEventHandlerEntry *(*eventAdd)(u32 *numHandlers);
} MoveEventAddEntry;

static BOOL DoesMoveEventExist(u8 monId, u16 move, u8 *found);
static void RemoveHandlerForce(u8 monId, u16 move);
static const BattleEventHandlerEntry *EventAddConversion(u32 *priority);
static void HandlerConversion(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddCamouflage(u32 *priority);
static void HandlerCamouflage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddTrickRoom(u32 *priority);
static void HandlerTrickRoom(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddGravity(u32 *priority);
static void HandlerGravity(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddWaterSport(u32 *priority);
static void HandlerWaterSport(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMudSport(u32 *priority);
static void HandlerMudSport(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddDefog(u32 *priority);
static void HandlerDefog(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddBrickBreak(u32 *priority);
static void HandlerBrickBreakStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerBrickBreakEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerBrickBreakCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static BOOL func_ov167_021c6130(BattleEventItem *item, BtlServerFlow *flow, u32 factorType, u32 event, u16 subId,
                                u8 monId);
static const BattleEventHandlerEntry *EventJumpKickAdd(u32 *priority);
static void HandlerJumpKickMiss(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void CommonJumpKickEffect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMimic(u32 *priority);
static void HandlerMimic(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSketch(u32 *priority);
static void HandlerSketch(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFollowMe(u32 *priority);
static void HandlerFollowMeCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerFollowMeTextSet(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerFollowMeBaitTarget(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerFollowMeTurnCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRefresh(u32 *priority);
static void HandlerRefresh(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSpiderWeb(u32 *priority);
static void HandlerSpiderWeb(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddHaze(u32 *priority);
static void HandlerHaze(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSplash(u32 *priority);
static void HandlerSplash(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddCurse(u32 *priority);
static void HandlerCurseMoveParam(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerCurse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddThunderWave(u32 *priority);
static void HandlerThunderWave(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddWorrySeed(u32 *priority);
static void HandlerWorrySeedCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerWorrySeed(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddDreamEater(u32 *priority);
static void HandlerDreamEater(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerDreamEaterSubstituteCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddCaptivate(u32 *priority);
static void HandlerCaptivate(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddTriAttack(u32 *priority);
static void HandlerTriAttack(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSecretPower(u32 *priority);
static u8 CommonSecretPowerGetParams(BtlServerFlow *flow, u8 *kind, u8 *arg);
static void HandlerSecretPowerStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerSecretPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddChatter(u32 *priority);
static void HandlerChatter(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddBind(u32 *priority);
static void HandlerBind(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerBindTextSet(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddWhirlpool(u32 *priority);
static void HandlerWhirlpoolDiveCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerWhirlpoolPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSuperFang(u32 *priority);
static void HandlerSuperFang(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddDragonRage(u32 *priority);
static void HandlerDragonRage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSonicBoom(u32 *priority);
static void HandlerSonicBoom(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddEndeavor(u32 *priority);
static void HandlerEndeavorCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerEndeavor(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSeismicToss(u32 *priority);
static void HandlerSeismicToss(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPsywave(u32 *priority);
static void HandlerPsywave(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddStockpile(u32 *priority);
static void HandlerStockpileCheckCount(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerStockpileIncreaseStats(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSpitUp(u32 *priority);
static void HandlerSpitUpStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerSpitUpPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerSpitUpResetStats(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSwallow(u32 *priority);
static void HandlerSwallow(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddCounter(u32 *priority);
static void HandlerCounterStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerCounterSetTarget(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerCounterCalcDamage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMirrorCoat(u32 *priority);
static void HandlerMirrorCoatStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMirrorCoatSetTarget(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMirrorCoatCalcDamage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMetalBurst(u32 *priority);
static void HandlerMetalBurstStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMetalBurstSetTarget(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMetalBurstCalcDamage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void CommonCounterStart(BtlServerFlow *flow, u8 monId, s32 *work, u32 category);
static void CommonCounterSetTarget(BtlServerFlow *flow, u8 monId, s32 *work, u32 category);
static void CommonCounterCalcDamage(BtlServerFlow *flow, u8 monId, s32 *work, u32 category, u32 ratio);
static BOOL CommonCounterCheckDamageRecieved(BattleMon *mon, u32 category, BattleMonDamageRecord *record);
static const BattleEventHandlerEntry *EventAddLastResort(u32 *priority);
static void HandlerLastResort(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSnore(u32 *priority);
static void HandlerSnoreCheck1(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerSnoreCheck2(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddNightmare(u32 *priority);
static void HandlerNightmareNoEffect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSuckerPunch(u32 *priority);
static void HandlerSuckerPunch(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPursuit(u32 *priority);
static void HandlerPursuitStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerPursuitHitCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerPursuitPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddExplosion(u32 *priority);
static void HandlerExplosionDamageDetermine(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerExplosionStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerExplosionEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFocusEnergy(u32 *priority);
static void HandlerFocusEnergy(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddCharge(u32 *priority);
static void HandlerCharge(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerChargePower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerChargeStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerChargeEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPerishSong(u32 *priority);
static void HandlerPerishSongStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerBypassSubstitute(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddLeechSeed(u32 *priority);
static void HandlerLeechSeed(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPayDay(u32 *priority);
static void HandlerPayDay(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRage(u32 *priority);
static void HandlerRageStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerRageBuild(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerRageEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddAquaRing(u32 *priority);
static void func_ov167_021c79c0(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddThrash(u32 *priority);
static void HandlerThrash(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerThrashEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerThrashTurnCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddUproar(u32 *priority);
static void HandlerUproar(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerUproarEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerUproarUnlock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerUproarPreventSleep(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void func_ov167_021c7e14(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRollout(u32 *priority);
static void HandlerRolloutStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerRolloutMiss(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerRolloutEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerRolloutUnlock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerRolloutPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddTripleKick(u32 *priority);
static void HandlerTripleKickPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerTripleKickHitCount(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddGyroBall(u32 *priority);
static void HandlerGyroBall(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRevenge(u32 *priority);
static void HandlerRevenge(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFlail(u32 *priority);
static void HandlerFlail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFacade(u32 *priority);
static void HandlerFacade(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPayback(u32 *priority);
static void HandlerPayback(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddEruption(u32 *priority);
static void HandlerEruption(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddCrushGrip(u32 *priority);
static void HandlerCrushGrip(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddBrine(u32 *priority);
static void HandlerBrine(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddReturn(u32 *priority);
static void HandlerReturn(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFrustration(u32 *priority);
static void HandlerFrustration(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddWakeUpSlap(u32 *priority);
static void HandlerWakeUpSlapPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerWakeUpSlapSleepCure(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSmellingSalts(u32 *priority);
static void HandlerSmellingSaltsPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerSmellingSaltsParaCure(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPresent(u32 *priority);
static void HandlerPresentRandomCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerPresentHeal(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerPresentPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddTrumpCard(u32 *priority);
static void HandlerTrumpCard(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPunishment(u32 *priority);
static void HandlerPunishment(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFuryCutter(u32 *priority);
static void HandlerFuryCutter(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddAssurance(u32 *priority);
static void HandlerAssurance(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddLowKick(u32 *priority);
static void HandlerLowKick(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddWeatherBall(u32 *priority);
static void HandlerWeatherBallType(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerWeatherBallPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddTwister(u32 *priority);
static void HandlerTwisterFlyCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerTwisterPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSkyUppercut(u32 *priority);
static void HandlerSkyUppercut(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddThunder(u32 *priority);
static void HandlerThunderFlyCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerThunderRainCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerThunderSunCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddBlizzard(u32 *priority);
static void HandlerBlizzard(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddEarthquake(u32 *priority);
static void HandlerEarthquakeDigCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerEarthquakeDamage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddJudgement(u32 *priority);
static void HandlerJudgement(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddTechnoBlast(u32 *priority);
static void HandlerTechnoBlast(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddHiddenPower(u32 *priority);
static void HandlerHiddenPowerType(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerHiddenPowerPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddNaturalGift(u32 *priority);
static void HandlerNaturalGiftCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerNaturalGiftType(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerNaturalGiftPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerNaturalGiftEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddKnockOff(u32 *priority);
static void HandlerKnockOff(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMagicCoat(u32 *priority);
static void HandlerMagicCoatCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMagicCoat(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMagicCoatWait(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMagicCoatReflect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMagicCoatTurnCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSnatch(u32 *priority);
static void HandlerSnatch(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerSnatchCheckMoveEffect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerSnatchStealMove(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerSnatchTurnCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddThief(u32 *priority);
static void HandlerThiefStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerThief(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddTrick(u32 *priority);
static void HandlerTrick(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMagnitude(u32 *priority);
static void HandlerMagnitudeEffect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMagnitudePower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSurf(u32 *priority);
static void HandlerSurfDiveCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerSurfPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddStomp(u32 *priority);
static void HandlerStomp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFalseSwipe(u32 *priority);
static void HandlerFalseSwipe(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddEndure(u32 *priority);
static void HandlerEndure(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerEndureCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerEndureTurnCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddProtect(u32 *priority);
static void HandlerProtectStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerProtectCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerProtectResetCounter(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerProtect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void IncrementProtectCounter(BtlServerFlow *flow, u8 monId, BOOL checkPrevResult);
static const BattleEventHandlerEntry *EventAddBide(u32 *priority);
static void HandlerBide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerBideTextSet(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerBideCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerBideTarget(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerBideDamageRecieved(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerBideCalcDamage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerBideFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static u8 GetBideTargetID(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void BideEnd(BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRecycle(u32 *priority);
static void HandlerRecycle(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPsychoShift(u32 *priority);
static void HandlerPsychoShift(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPainSplit(u32 *priority);
static void func_ov167_021c98b8(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventBellyDrum(u32 *priority);
static void HandlerBellyDrum(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFeint(u32 *priority);
static void HandlerFeintBreakProtect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerFeintTextSet(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerFeintResetProtectCounter(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddAcupressure(u32 *priority);
static void HandlerAcupressure(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRest(u32 *priority);
static void HandlerRestCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerRest(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddAttract(u32 *priority);
static void HandlerAttractCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddConversion2(u32 *priority);
static void HandlerConversion2(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddEncore(u32 *priority);
static void HandlerEncore(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddTaunt(u32 *priority);
static void HandlerTaunt(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddTorment(u32 *priority);
static void HandlerTorment(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddDisable(u32 *priority);
static void HandlerDisable(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddImprison(u32 *priority);
static void HandlerImprison(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddAromatherapy(u32 *priority);
static void HandlerAromatherapy(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddHealBell(u32 *priority);
static void HandlerHealBell(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void CommonCureAllyMonStatus(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work, u16 message);
static u8 CheckMonStatus(BattleParty *party, u8 pos, u8 max, u8 *monIds);
static const BattleEventHandlerEntry *EventAddMemento(u32 *priority);
static void HandlerMemento(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSpite(u32 *priority);
static void HandlerSpite(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPsychUp(u32 *priority);
static void HandlerPsychUp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddHeartSwap(u32 *priority);
static void HandlerHeartSwap(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPowerSwap(u32 *priority);
static void HandlerPowerSwap(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddGuardSwap(u32 *priority);
static void HandlerGuardSwap(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPowerTrick(u32 *priority);
static void HandlerPowerTrick(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPowerSplit(u32 *priority);
static void HandlerPowerSplit(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddGuardSplit(u32 *priority);
static void HandlerGuardSplit(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddLockOn(u32 *priority);
static void HandlerLockOn(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddReflect(u32 *priority);
static void HandlerReflect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddLightScreen(u32 *priority);
static void HandlerLightScreen(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSafeguard(u32 *priority);
static void HandlerSafeguard(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMist(u32 *priority);
static void HandlerMist(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddTailwind(u32 *priority);
static void HandlerTailwind(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddLuckyChant(u32 *priority);
static void HandlerLuckyChant(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSpikes(u32 *priority);
static void HandlerSpikes(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddToxicSpikes(u32 *priority);
static void HandlerToxicSpikes(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddStealthRock(u32 *priority);
static void HandlerStealthRock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddWideGuard(u32 *priority);
static void HandlerWideGuard(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void CommonCreateSideEffect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work, u8 side, u32 effect,
                                   BattleCondition cont, u16 message);
static const BattleEventHandlerEntry *EventAddTransform(u32 *priority);
static void HandlerTransform(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddLunarDance(u32 *priority);
static void HandlerLunarDance(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddHealingWish(u32 *priority);
static void HandlerHealingWish(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddWish(u32 *priority);
static void HandlerWish(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFutureSight(u32 *priority);
static void HandlerFutureSight(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerFutureSightLand(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void CommonDelayAttack(BattleEventItem *item, BtlServerFlow *flow, u8 monId, u8 targetPos);
static const BattleEventHandlerEntry *EventAddDoomDesire(u32 *priority);
static void HandlerDoomDesire(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerDoomDesireLand(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddGastroAcid(u32 *priority);
static void HandlerGastroAcid(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRolePlay(u32 *priority);
static void HandlerRolePlay(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddUturn(u32 *priority);
static void HandlerUturn(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRapidSpin(u32 *priority);
static void HandlerRapidSpin(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddBatonPass(u32 *priority);
static void HandlerBatonPass(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddTeleport(u32 *priority);
static void HandlerTeleportCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerTeleport(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerTeleportExitText(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFling(u32 *priority);
static void HandlerFlingCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerFlingPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerFlingStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerFlingDamageAfter(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerFlingEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMagnetRise(u32 *priority);
static void HandlerMagnetRiseCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMagnetRise(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddHelpingHand(u32 *priority);
static void HandlerHelpingHandSkip(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerHelpingHandCheckInvuln(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerHelpingHandReady(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerHelpingHandPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerHelpingHandTurnCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddBeatUp(u32 *priority);
static void HandlerBeatUp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerBeatUpPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static BattleMon *CommonBeatUpGetParam(BtlServerFlow *flow, u8 monId, u8 index);
static const BattleEventHandlerEntry *EventAddFakeOut(u32 *priority);
static void HandlerFakeOut(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMorningSun(u32 *priority);
static void HandlerMorningSun(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFly(u32 *priority);
static void HandlerFly(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void func_ov167_021cba48(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddShadowForce(u32 *priority);
static void HandlerShadowForce(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void func_ov167_021cbaac(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerShadowForceEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddBounce(u32 *priority);
static void HandlerBounce(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void func_ov167_021cbb80(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddDive(u32 *priority);
static void HandlerDive(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void func_ov167_021cbbe4(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddDig(u32 *priority);
static void HandlerDig(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void func_ov167_021cbc48(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSolarBeam(u32 *priority);
static void HandlerSolarBeamSunCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerSolarBeamCharge(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerSolarBeamPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRazorWind(u32 *priority);
static void HandlerRazorWindCharge(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSkyAttack(u32 *priority);
static void HandlerSkyAttack(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSkullBash(u32 *priority);
static void HandlerSkullBash(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPluck(u32 *priority);
static void HandlerPluck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddStruggle(u32 *priority);
static void HandlerStruggleRecoil(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerStruggleStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerStruggleMoveParam(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddDestinyBond(u32 *priority);
static void HandlerDestinyBondReady(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerDestinyBondStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerDestinyBondDamage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddGrudge(u32 *priority);
static void HandlerGrudgeReady(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerGrudgeReducePP(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMinimize(u32 *priority);
static void HandlerMinimize(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddDefenseCurl(u32 *priority);
static void HandlerDefenseCurl(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRoost(u32 *priority);
static void HandlerRoost(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFocusPunch(u32 *priority);
static void HandlerFocuspunch(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMetronome(u32 *priority);
static void HandlerMetronome(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMetronomeTextSet(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddNaturePower(u32 *priority);
static void HandlerNaturePower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerNaturePowerTextSet(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddAssist(u32 *priority);
static void HandlerAssist(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSleepTalk(u32 *priority);
static void HandlerSleepTalk(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMirrorMove(u32 *priority);
static void HandlerMirrorMove(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMeFirst(u32 *priority);
static void HandlerMeFirst(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerMeFirstPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddCopycat(u32 *priority);
static void HandlerCopycat(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddAncientPower(u32 *priority);
static void HandlerAncientPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddVenoshock(u32 *priority);
static void HandlerVenoshock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddHex(u32 *priority);
static void HandlerHex(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddAcrobatics(u32 *priority);
static void HandlerAcrobatics(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddStoredPower(u32 *priority);
static void HandlerStoredPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddHeavySlam(u32 *priority);
static void HandlerHeavySlam(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddElectroBall(u32 *priority);
static void HandlerElectroBall(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddEchoedVoice(u32 *priority);
static void HandlerEchoedVoice(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRetaliate(u32 *priority);
static void HandlerRetaliate(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFoulPlay(u32 *priority);
static void HandlerFoulPlay(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSoak(u32 *priority);
static void HandlerSoak(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSimpleBeam(u32 *priority);
static void HandlerSimpleBeam(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddEntrainment(u32 *priority);
static void HandlerEntrainment(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddClearSmog(u32 *priority);
static void HandlerClearSmog(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddIncinerate(u32 *priority);
static void HandlerIncinerate(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddCircleThrow(u32 *priority);
static void HandlerCircleThrow(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSmackDown(u32 *priority);
static void HandlerSmackDown(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddShellSmash(u32 *priority);
static void HandlerShellSmash(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddReflectType(u32 *priority);
static void HandlerReflectType(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddAutotomize(u32 *priority);
static void HandlerAutotomize(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddPsyshock(u32 *priority);
static void HandlerPsyshock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddChipAway(u32 *priority);
static void HandlerChipAwayCalcDamage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerChipAwayHitCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddWonderRoom(u32 *priority);
static void HandlerWonderRoom(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMagicRoom(u32 *priority);
static void HandlerMagicRoom(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFlameBurst(u32 *priority);
static void HandlerFlameBurst(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSynchronoise(u32 *priority);
static void HandlerSynchronoise(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddBestow(u32 *priority);
static void HandlerBestow(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFinalGambit(u32 *priority);
static void HandlerFinalGambit(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddAfterYou(u32 *priority);
static void HandlerAfterYou(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddQuash(u32 *priority);
static void HandlerQuash(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRound(u32 *priority);
static void HandlerRound(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerRoundPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddQuickGuard(u32 *priority);
static void HandlerQuickGuard(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddAllySwitch(u32 *priority);
static void HandlerAllySwitch(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddTelekinesis(u32 *priority);
static void HandlerTelekinesisCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerTelekinesis(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddSkyDrop(u32 *priority);
static void HandlerSkyDropGrabFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerSkyDropGrab(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerSkyDropRelease(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerSkyDropTarget(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerSkyDropCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerSkyDropTypeCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddRelicSong(u32 *priority);
static void HandlerRelicSong(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddForesight(u32 *priority);
static void HandlerForesight(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddMiracleEye(u32 *priority);
static void HandlerMiracleEye(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddGrowth(u32 *priority);
static void HandlerGrowth(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFreezeShock(u32 *priority);
static void HandlerFreezeShock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static const BattleEventHandlerEntry *EventAddFusionFlare(u32 *priority);
static void HandlerFusionFlare(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static u32 func_ov167_021cdbc4(u16 move, u16 partnerMove);
static const BattleEventHandlerEntry *EventAddWaterPledge(u32 *priority);
static void HandlerWaterPledgeCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerWaterPledgeDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerWaterPledgeTypeMatch(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerWaterPledgePower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerWaterPledgeChangeEffect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);
static void HandlerWaterPledgeFieldEffect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);

static const MoveEventAddEntry sMoveEventAddTable[] = {
    { MOVE_CONVERSION, EventAddConversion },
    { MOVE_CAMOUFLAGE, EventAddCamouflage },
    { MOVE_HAZE, EventAddHaze },
    { MOVE_DREAM_EATER, EventAddDreamEater },
    { MOVE_TRI_ATTACK, EventAddTriAttack },
    { MOVE_SECRET_POWER, EventAddSecretPower },
    { MOVE_CHATTER, EventAddChatter },
    { MOVE_SUPER_FANG, EventAddSuperFang },
    { MOVE_DRAGON_RAGE, EventAddDragonRage },
    { MOVE_SEISMIC_TOSS, EventAddSeismicToss },
    { MOVE_NIGHT_SHADE, EventAddSeismicToss },
    { MOVE_PSYWAVE, EventAddPsywave },
    { MOVE_SNORE, EventAddSnore },
    { MOVE_LAST_RESORT, EventAddLastResort },
    { MOVE_FLAIL, EventAddFlail },
    { MOVE_REVERSAL, EventAddFlail },
    { MOVE_FALSE_SWIPE, EventAddFalseSwipe },
    { MOVE_SPIDER_WEB, EventAddSpiderWeb },
    { MOVE_MEAN_LOOK, EventAddSpiderWeb },
    { MOVE_BLOCK, EventAddSpiderWeb },
    { MOVE_ENDURE, EventAddEndure },
    { MOVE_SONIC_BOOM, EventAddSonicBoom },
    { MOVE_FAKE_OUT, EventAddFakeOut },
    { MOVE_ENDEAVOR, EventAddEndeavor },
    { MOVE_ERUPTION, EventAddEruption },
    { MOVE_WATER_SPOUT, EventAddEruption },
    { MOVE_REFRESH, EventAddRefresh },
    { MOVE_MORNING_SUN, EventAddMorningSun },
    { MOVE_MOONLIGHT, EventAddMorningSun },
    { MOVE_SYNTHESIS, EventAddMorningSun },
    { MOVE_WRING_OUT, EventAddCrushGrip },
    { MOVE_CRUSH_GRIP, EventAddCrushGrip },
    { MOVE_WEATHER_BALL, EventAddWeatherBall },
    { MOVE_PROTECT, EventAddProtect },
    { MOVE_DETECT, EventAddProtect },
    { MOVE_SPLASH, EventAddSplash },
    { MOVE_CURSE, EventAddCurse },
    { MOVE_THRASH, EventAddThrash },
    { MOVE_UPROAR, EventAddUproar },
    { MOVE_PETAL_DANCE, EventAddThrash },
    { MOVE_OUTRAGE, EventAddThrash },
    { MOVE_BRINE, EventAddBrine },
    { MOVE_WAKE_UP_SLAP, EventAddWakeUpSlap },
    { MOVE_SMELLING_SALT, EventAddSmellingSalts },
    { MOVE_ACUPRESSURE, EventAddAcupressure },
    { MOVE_TRUMP_CARD, EventAddTrumpCard },
    { MOVE_FLY, EventAddFly },
    { MOVE_BOUNCE, EventAddBounce },
    { MOVE_DIVE, EventAddDive },
    { MOVE_DIG, EventAddDig },
    { MOVE_SOLAR_BEAM, EventAddSolarBeam },
    { MOVE_RAZOR_WIND, EventAddRazorWind },
    { MOVE_SKY_ATTACK, EventAddSkyAttack },
    { MOVE_SKULL_BASH, EventAddSkullBash },
    { MOVE_ENCORE, EventAddEncore },
    { MOVE_TWISTER, EventAddTwister },
    { MOVE_GUST, EventAddTwister },
    { MOVE_EARTHQUAKE, EventAddEarthquake },
    { MOVE_SURF, EventAddSurf },
    { MOVE_SKY_UPPERCUT, EventAddSkyUppercut },
    { MOVE_MAGNITUDE, EventAddMagnitude },
    { MOVE_FEINT, EventAddFeint },
    { MOVE_SHADOW_FORCE, EventAddShadowForce },
    { MOVE_STRUGGLE, EventAddStruggle },
    { MOVE_PUNISHMENT, EventAddPunishment },
    { MOVE_TAUNT, EventAddTaunt },
    { MOVE_CAPTIVATE, EventAddCaptivate },
    { MOVE_BELLY_DRUM, EventBellyDrum },
    { MOVE_DESTINY_BOND, EventAddDestinyBond },
    { MOVE_FACADE, EventAddFacade },
    { MOVE_PAYBACK, EventAddPayback },
    { MOVE_HIDDEN_POWER, EventAddHiddenPower },
    { MOVE_MINIMIZE, EventAddMinimize },
    { MOVE_DEFENSE_CURL, EventAddDefenseCurl },
    { MOVE_STOMP, EventAddStomp },
    { MOVE_NIGHTMARE, EventAddNightmare },
    { MOVE_SUCKER_PUNCH, EventAddSuckerPunch },
    { MOVE_ROLLOUT, EventAddRollout },
    { MOVE_ICE_BALL, EventAddRollout },
    { MOVE_AROMATHERAPY, EventAddAromatherapy },
    { MOVE_HEAL_BELL, EventAddHealBell },
    { MOVE_MEMENTO, EventAddMemento },
    { MOVE_SPITE, EventAddSpite },
    { MOVE_REST, EventAddRest },
    { MOVE_LOCK_ON, EventAddLockOn },
    { MOVE_MIND_READER, EventAddLockOn },
    { MOVE_REFLECT, EventAddReflect },
    { MOVE_LIGHT_SCREEN, EventAddLightScreen },
    { MOVE_SAFEGUARD, EventAddSafeguard },
    { MOVE_MIST, EventAddMist },
    { MOVE_TAILWIND, EventAddTailwind },
    { MOVE_RETURN, EventAddReturn },
    { MOVE_FRUSTRATION, EventAddFrustration },
    { MOVE_PRESENT, EventAddPresent },
    { MOVE_TORMENT, EventAddTorment },
    { MOVE_IMPRISON, EventAddImprison },
    { MOVE_GRAVITY, EventAddGravity },
    { MOVE_GRUDGE, EventAddGrudge },
    { MOVE_HELPING_HAND, EventAddHelpingHand },
    { MOVE_GASTRO_ACID, EventAddGastroAcid },
    { MOVE_ROLE_PLAY, EventAddRolePlay },
    { MOVE_SPIKES, EventAddSpikes },
    { MOVE_TOXIC_SPIKES, EventAddToxicSpikes },
    { MOVE_STEALTH_ROCK, EventAddStealthRock },
    { MOVE_ROOST, EventAddRoost },
    { MOVE_MAGNET_RISE, EventAddMagnetRise },
    { MOVE_FURY_CUTTER, EventAddFuryCutter },
    { MOVE_PSYCHO_SHIFT, EventAddPsychoShift },
    { MOVE_ASSURANCE, EventAddAssurance },
    { MOVE_CONVERSION_2, EventAddConversion2 },
    { MOVE_COUNTER, EventAddCounter },
    { MOVE_MIRROR_COAT, EventAddMirrorCoat },
    { MOVE_METAL_BURST, EventAddMetalBurst },
    { MOVE_REVENGE, EventAddRevenge },
    { MOVE_AVALANCHE, EventAddRevenge },
    { MOVE_TRIPLE_KICK, EventAddTripleKick },
    { MOVE_GYRO_BALL, EventAddGyroBall },
    { MOVE_PAIN_SPLIT, EventAddPainSplit },
    { MOVE_FOLLOW_ME, EventAddFollowMe },
    { MOVE_WORRY_SEED, EventAddWorrySeed },
    { MOVE_THUNDER_WAVE, EventAddThunderWave },
    { MOVE_PSYCH_UP, EventAddPsychUp },
    { MOVE_HEART_SWAP, EventAddHeartSwap },
    { MOVE_POWER_SWAP, EventAddPowerSwap },
    { MOVE_GUARD_SWAP, EventAddGuardSwap },
    { MOVE_ATTRACT, EventAddAttract },
    { MOVE_JUDGMENT, EventAddJudgement },
    { MOVE_NATURAL_GIFT, EventAddNaturalGift },
    { MOVE_KNOCK_OFF, EventAddKnockOff },
    { MOVE_DISABLE, EventAddDisable },
    { MOVE_THIEF, EventAddThief },
    { MOVE_COVET, EventAddThief },
    { MOVE_TRICK, EventAddTrick },
    { MOVE_SWITCHEROO, EventAddTrick },
    { MOVE_MIMIC, EventAddMimic },
    { MOVE_SKETCH, EventAddSketch },
    { MOVE_JUMP_KICK, EventJumpKickAdd },
    { MOVE_HI_JUMP_KICK, EventJumpKickAdd },
    { MOVE_DEFOG, EventAddDefog },
    { MOVE_BRICK_BREAK, EventAddBrickBreak },
    { MOVE_TRICK_ROOM, EventAddTrickRoom },
    { MOVE_WATER_SPORT, EventAddWaterSport },
    { MOVE_MUD_SPORT, EventAddMudSport },
    { MOVE_CHARGE, EventAddCharge },
    { MOVE_PERISH_SONG, EventAddPerishSong },
    { MOVE_LEECH_SEED, EventAddLeechSeed },
    { MOVE_BEAT_UP, EventAddBeatUp },
    { MOVE_AQUA_RING, EventAddAquaRing },
    { MOVE_LUNAR_DANCE, EventAddLunarDance },
    { MOVE_HEALING_WISH, EventAddHealingWish },
    { MOVE_METRONOME, EventAddMetronome },
    { MOVE_NATURE_POWER, EventAddNaturePower },
    { MOVE_ASSIST, EventAddAssist },
    { MOVE_MIRROR_MOVE, EventAddMirrorMove },
    { MOVE_ME_FIRST, EventAddMeFirst },
    { MOVE_COPYCAT, EventAddCopycat },
    { MOVE_SLEEP_TALK, EventAddSleepTalk },
    { MOVE_LOW_KICK, EventAddLowKick },
    { MOVE_GRASS_KNOT, EventAddLowKick },
    { MOVE_FOCUS_PUNCH, EventAddFocusPunch },
    { MOVE_STOCKPILE, EventAddStockpile },
    { MOVE_SPIT_UP, EventAddSpitUp },
    { MOVE_SWALLOW, EventAddSwallow },
    { MOVE_FUTURE_SIGHT, EventAddFutureSight },
    { MOVE_DOOM_DESIRE, EventAddDoomDesire },
    { MOVE_RECYCLE, EventAddRecycle },
    { MOVE_PURSUIT, EventAddPursuit },
    { MOVE_PAY_DAY, EventAddPayDay },
    { MOVE_BIDE, EventAddBide },
    { MOVE_SNATCH, EventAddSnatch },
    { MOVE_MAGIC_COAT, EventAddMagicCoat },
    { MOVE_TELEPORT, EventAddTeleport },
    { MOVE_U_TURN, EventAddUturn },
    { MOVE_BATON_PASS, EventAddBatonPass },
    { MOVE_PLUCK, EventAddPluck },
    { MOVE_BUG_BITE, EventAddPluck },
    { MOVE_FLING, EventAddFling },
    { MOVE_WRAP, EventAddBind },
    { MOVE_BIND, EventAddBind },
    { MOVE_FIRE_SPIN, EventAddBind },
    { MOVE_CLAMP, EventAddBind },
    { MOVE_SAND_TOMB, EventAddBind },
    { MOVE_MAGMA_STORM, EventAddBind },
    { MOVE_WHIRLPOOL, EventAddWhirlpool },
    { MOVE_RAPID_SPIN, EventAddRapidSpin },
    { MOVE_WHIRLWIND, EventAddRapidSpin },
    { MOVE_POWER_TRICK, EventAddPowerTrick },
    { MOVE_TRANSFORM, EventAddTransform },
    { MOVE_EXPLOSION, EventAddExplosion },
    { MOVE_SELFDESTRUCT, EventAddExplosion },
    { MOVE_FOCUS_ENERGY, EventAddFocusEnergy },
    { MOVE_RAGE, EventAddRage },
    { MOVE_ANCIENT_POWER, EventAddAncientPower },
    { MOVE_OMINOUS_WIND, EventAddAncientPower },
    { MOVE_SILVER_WIND, EventAddAncientPower },
    { MOVE_THUNDER, EventAddThunder },
    { MOVE_BLIZZARD, EventAddBlizzard },
    { MOVE_WISH, EventAddWish },
    { MOVE_LUCKY_CHANT, EventAddLuckyChant },
    { MOVE_FORESIGHT, EventAddForesight },
    { MOVE_ODOR_SLEUTH, EventAddForesight },
    { MOVE_MIRACLE_EYE, EventAddMiracleEye },
    { MOVE_GROWTH, EventAddGrowth },
    { MOVE_VENOSHOCK, EventAddVenoshock },
    { MOVE_RAGE_POWDER, EventAddFollowMe },
    { MOVE_SOAK, EventAddSoak },
    { MOVE_SIMPLE_BEAM, EventAddSimpleBeam },
    { MOVE_ENTRAINMENT, EventAddEntrainment },
    { MOVE_CLEAR_SMOG, EventAddClearSmog },
    { MOVE_STORED_POWER, EventAddStoredPower },
    { MOVE_SHELL_SMASH, EventAddShellSmash },
    { MOVE_HEX, EventAddHex },
    { MOVE_ACROBATICS, EventAddAcrobatics },
    { MOVE_VOLT_SWITCH, EventAddUturn },
    { MOVE_WIDE_GUARD, EventAddWideGuard },
    { MOVE_REFLECT_TYPE, EventAddReflectType },
    { MOVE_POWER_SPLIT, EventAddPowerSplit },
    { MOVE_GUARD_SPLIT, EventAddGuardSplit },
    { MOVE_AUTOTOMIZE, EventAddAutotomize },
    { MOVE_HEAVY_SLAM, EventAddHeavySlam },
    { MOVE_HEAT_CRASH, EventAddHeavySlam },
    { MOVE_WONDER_ROOM, EventAddWonderRoom },
    { MOVE_MAGIC_ROOM, EventAddMagicRoom },
    { MOVE_PSYSHOCK, EventAddPsyshock },
    { MOVE_PSYSTRIKE, EventAddPsyshock },
    { MOVE_FLAME_BURST, EventAddFlameBurst },
    { MOVE_ELECTRO_BALL, EventAddElectroBall },
    { MOVE_SYNCHRONOISE, EventAddSynchronoise },
    { MOVE_CHIP_AWAY, EventAddChipAway },
    { MOVE_SACRED_SWORD, EventAddChipAway },
    { MOVE_ECHOED_VOICE, EventAddEchoedVoice },
    { MOVE_INCINERATE, EventAddIncinerate },
    { MOVE_BESTOW, EventAddBestow },
    { MOVE_CIRCLE_THROW, EventAddCircleThrow },
    { MOVE_DRAGON_TAIL, EventAddCircleThrow },
    { MOVE_RETALIATE, EventAddRetaliate },
    { MOVE_FOUL_PLAY, EventAddFoulPlay },
    { MOVE_SMACK_DOWN, EventAddSmackDown },
    { MOVE_FINAL_GAMBIT, EventAddFinalGambit },
    { MOVE_AFTER_YOU, EventAddAfterYou },
    { MOVE_QUASH, EventAddQuash },
    { MOVE_ROUND, EventAddRound },
    { MOVE_QUICK_GUARD, EventAddQuickGuard },
    { MOVE_ALLY_SWITCH, EventAddAllySwitch },
    { MOVE_TELEKINESIS, EventAddTelekinesis },
    { MOVE_SKY_DROP, EventAddSkyDrop },
    { MOVE_STEAMROLLER, EventAddStomp },
    { MOVE_HURRICANE, EventAddThunder },
    { MOVE_SECRET_SWORD, EventAddPsyshock },
    { MOVE_RELIC_SONG, EventAddRelicSong },
    { MOVE_TECHNO_BLAST, EventAddTechnoBlast },
    { MOVE_FREEZE_SHOCK, EventAddFreezeShock },
    { MOVE_ICE_BURN, EventAddFreezeShock },
    { MOVE_WATER_PLEDGE, EventAddWaterPledge },
    { MOVE_FIRE_PLEDGE, EventAddWaterPledge },
    { MOVE_GRASS_PLEDGE, EventAddWaterPledge },
    { MOVE_FUSION_FLARE, EventAddFusionFlare },
    { MOVE_FUSION_BOLT, EventAddFusionFlare },
};

// Doesn't match: the original keeps the status in a register, which MWCC does for an enum-typed local (see the
// report); a u32 local is folded into each use.

// Function names from swan.
BOOL MoveEvent_AddItem(BattleMon *mon, u16 move, u32 subPriority) {
    u32 i;
    u8 monId;
    u8 found;
    const BattleEventHandlerEntry *handlers;
    u32 numHandlers;

    for (i = 0; i < NELEMS(sMoveEventAddTable); i++) {
        if (move == sMoveEventAddTable[i].move) {
            monId = GetMonID(mon);
            found = FALSE;
            if (DoesMoveEventExist(monId, move, &found)) {
                handlers = sMoveEventAddTable[i].eventAdd(&numHandlers);
                if (handlers != NULL) {
                    BattleEvent_AddItem(0, move, 0, subPriority, monId, handlers, numHandlers);
                    return TRUE;
                }
            }
            return FALSE;
        }
    }
    return FALSE;
}

// Whether the mon can take the move's handlers: not when it has them already, or has too many
static BOOL DoesMoveEventExist(u8 monId, u16 move, u8 *found) {
    BattleEventItem *item;
    u32 count;

    count = 0;
    item = BattleEvent_SeekItem(0, monId);
    while (item != NULL) {
        if (move == BattleEventItem_GetSubID(item)) {
            *found = TRUE;
            return FALSE;
        }
        if (++count > 8) {
            return FALSE;
        }
        item = BattleEvent_GetNextItem(item);
    }
    return TRUE;
}

// Removes the move's handlers that don't persist
void func_ov167_021c5bbc(BattleMon *mon, u16 move) {
    BattleEventItem *item;
    BattleEventItem *next;

    item = BattleEvent_SeekItem(0, GetMonID(mon));
    while (item != NULL) {
        next = BattleEvent_GetNextItem(item);
        if (move == BattleEventItem_GetSubID(item) && BattleEventItem_GetWorkValue(item, 6) == 0) {
            BattleEventItem_Remove(item);
        }
        item = next;
    }
}

void RemoveForce(BattleMon *mon, u16 move) {
    RemoveHandlerForce(GetMonID(mon), move);
}

BOOL func_ov167_021c5c10(BattleEventItem *item, s32 *work) {
    if (work[6] != 0) {
        return TRUE;
    }
    return FALSE;
}

static void RemoveHandlerForce(u8 monId, u16 move) {
    BattleEventItem *item;

    item = BattleEvent_SeekItem(0, monId);
    while (item != NULL) {
        if (move == BattleEventItem_GetSubID(item)) {
            BattleEventItem_Remove(item);
            item = BattleEvent_SeekItem(0, monId);
        } else {
            item = BattleEvent_GetNextItem(item);
        }
    }
}

void RemoveForceAll(BattleMon *mon) {
    BattleEventItem *item;
    u8 monId;

    monId = GetMonID(mon);
    item = BattleEvent_SeekItem(0, monId);
    while (item != NULL) {
        BattleEventItem_Remove(item);
        item = BattleEvent_SeekItem(0, monId);
    }
}

static const BattleEventHandlerEntry sHandlersConversion[] = {
    { 0xa0, HandlerConversion },
};

static const BattleEventHandlerEntry *EventAddConversion(u32 *priority) {
    *priority = 1;
    return sHandlersConversion;
}

static void HandlerConversion(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u32 moveCount;
    u16 i;
    u16 count;
    u16 move;
    u8 type;
    u8 types[4];
    BattleHandlerChangeTypeParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        moveCount = GetBattleMonMoveCount(mon);
        for (i = 0, count = 0; i < moveCount; i++) {
            move = MoveGetID(mon, i);
            if (move != BattleEventItem_GetSubID(item)) {
                type = PML_MoveGetType(move);
                if (!DoesMonHaveType(mon, type)) {
                    types[count++] = type;
                }
            }
        }
        if (count != 0) {
            param = BattleHandler_PushWork(flow, 0x14, monId);
            param->type = func_ov167_021ce530(types[(u16)BattleRandom(count)]);
            param->monIndex = monId;
            BattleHandler_PopWork(flow, param);
        }
    }
}

static const BattleEventHandlerEntry sHandlersCamouflage[] = {
    { 0xa0, HandlerCamouflage },
};

static const BattleEventHandlerEntry *EventAddCamouflage(u32 *priority) {
    *priority = 1;
    return sHandlersCamouflage;
}

static void HandlerCamouflage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 type;
    PokeTypePair pair;
    BattleHandlerChangeTypeParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        switch (GetBattleTerrain(flow)) {
        case 1:
        case 2:
        case 3:
        case 8:
        case 9:
        case 15:
        case 17:
            type = 4;
            break;
        default:
            type = 0;
            break;
        case 10:
        case 19:
            type = 5;
            break;
        case 0:
        case 5:
            type = 0xb;
            break;
        case 6:
        case 11:
        case 12:
            type = 0xa;
            break;
        case 7:
        case 13:
            type = 0xe;
            break;
        }
        pair = func_ov167_021ce530(type);
        if (pair != GetPokeType(GetBattleMon(flow, monId))) {
            param = BattleHandler_PushWork(flow, 0x14, monId);
            param->type = pair;
            param->monIndex = monId;
            BattleHandler_PopWork(flow, param);
        }
    }
}

static const BattleEventHandlerEntry sHandlersTrickRoom[] = {
    { 0x9e, HandlerTrickRoom },
};

static const BattleEventHandlerEntry *EventAddTrickRoom(u32 *priority) {
    *priority = 1;
    return sHandlersTrickRoom;
}

static void HandlerTrickRoom(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerAddFieldEffectParam *add;
    BattleHandlerRemoveFieldEffectParam *remove;

    if (BattleEventVar_GetValue(3) == monId) {
        if (!IsFieldEffectActive(1)) {
            add = BattleHandler_PushWork(flow, 0x1b, monId);
            add->effect = 1;
            add->value = SetConditionTurns(5);
            BattleHandler_StrSetup(&add->string, 2, 0x359);
            BattleHandler_AddArg(&add->string, monId);
            BattleHandler_PopWork(flow, add);
        } else {
            remove = BattleHandler_PushWork(flow, 0x1c, monId);
            remove->effect = 1;
            BattleHandler_PopWork(flow, remove);
        }
    }
}

static const BattleEventHandlerEntry sHandlersGravity[] = {
    { 0x9e, HandlerGravity },
};

static const BattleEventHandlerEntry *EventAddGravity(u32 *priority) {
    *priority = 1;
    return sHandlersGravity;
}

static void HandlerGravity(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerAddFieldEffectParam *param;

    if (BattleEventVar_GetValue(3) == monId && !IsFieldEffectActive(2)) {
        param = BattleHandler_PushWork(flow, 0x1b, monId);
        param->effect = 2;
        param->value = SetConditionTurns(5);
        param->string.enabled = 1;
        param->string.message = 0x75;
        BattleHandler_PopWork(flow, param);
        BattleHandler_PushRun(flow, 0x35, monId);
    }
}

static const BattleEventHandlerEntry sHandlersWaterSport[] = {
    { 0x9e, HandlerWaterSport },
};

static const BattleEventHandlerEntry *EventAddWaterSport(u32 *priority) {
    *priority = 1;
    return sHandlersWaterSport;
}

static void HandlerWaterSport(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerAddFieldEffectParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        param = BattleHandler_PushWork(flow, 0x1b, monId);
        param->effect = 4;
        param->value = func_ov167_021ce1dc(monId);
        BattleHandler_StrSetup(&param->string, 1, 0x72);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersMudSport[] = {
    { 0x9e, HandlerMudSport },
};

static const BattleEventHandlerEntry *EventAddMudSport(u32 *priority) {
    *priority = 1;
    return sHandlersMudSport;
}

static void HandlerMudSport(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerAddFieldEffectParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        param = BattleHandler_PushWork(flow, 0x1b, monId);
        param->effect = 5;
        param->value = func_ov167_021ce1dc(monId);
        BattleHandler_StrSetup(&param->string, 1, 0x73);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersDefog[] = {
    { 0xa0, HandlerDefog },
    { 0x5, HandlerBypassSubstitute },
};

static const BattleEventHandlerEntry *EventAddDefog(u32 *priority) {
    *priority = 2;
    return sHandlersDefog;
}

static void HandlerDefog(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleHandlerStatChangeParam *stat;
    BattleHandlerRemoveSideEffectParam *remove;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        if (!IsSubstituteActive(GetBattleMon(flow, targetId))) {
            stat = BattleHandler_PushWork(flow, 0xe, monId);
            stat->stat = 7;
            stat->change = -1;
            stat->unk0e = 1;
            stat->count = 1;
            stat->monIds[0] = targetId;
            BattleHandler_PopWork(flow, stat);
        }
        remove = BattleHandler_PushWork(flow, 0x1a, monId);
        remove->side = GetSideFromMonID(targetId);
        BattleHandler_InitFlags(remove->effects, sizeof(remove->effects));
        BattleHandler_SetFlag(remove->effects, 0);
        BattleHandler_SetFlag(remove->effects, 1);
        BattleHandler_SetFlag(remove->effects, 3);
        BattleHandler_SetFlag(remove->effects, 2);
        BattleHandler_SetFlag(remove->effects, 6);
        BattleHandler_SetFlag(remove->effects, 7);
        BattleHandler_SetFlag(remove->effects, 8);
        BattleHandler_PopWork(flow, remove);
    }
}

static const BattleEventHandlerEntry sHandlersBrickBreak[] = {
    { 0x46, HandlerBrickBreakStart },
    { 0x48, HandlerBrickBreakEnd },
    { 0x45, HandlerBrickBreakCheck },
};

static const BattleEventHandlerEntry *EventAddBrickBreak(u32 *priority) {
    *priority = 3;
    return sHandlersBrickBreak;
}

static void HandlerBrickBreakStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventItem_AttachSkipCheckHandler(item, func_ov167_021c6130);
    }
}

static void HandlerBrickBreakEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventItem_DetachSkipCheckHandler(item);
    }
}

static void HandlerBrickBreakCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 side;
    BattleHandlerRemoveSideEffectParam *remove;
    BattleHandlerMoveEffectParam *effect;

    if (BattleEventVar_GetValue(3) == monId) {
        side = GetSideFromMonID(BattleEventVar_GetValue(4));
        if (func_ov169_06898cf4(side, 0) || func_ov169_06898cf4(side, 1)) {
            remove = BattleHandler_PushWork(flow, 0x1a, monId);
            remove->side = side;
            BattleHandler_InitFlags(remove->effects, sizeof(remove->effects));
            BattleHandler_SetFlag(remove->effects, 0);
            BattleHandler_SetFlag(remove->effects, 1);
            BattleHandler_PopWork(flow, remove);
            effect = BattleHandler_PushWork(flow, 0x3a, monId);
            effect->index = 1;
            BattleHandler_PopWork(flow, effect);
        }
    }
}

// Skips the side effects' handlers, Reflect's and Light Screen's, while Brick Break hits
static BOOL func_ov167_021c6130(BattleEventItem *item, BtlServerFlow *flow, u32 factorType, u32 event, u16 subId,
                                u8 monId) {
    if (factorType == 2 && subId <= 1) {
        return TRUE;
    }
    return FALSE;
}

static const BattleEventHandlerEntry sHandlersJumpKick[] = {
    { 0x26, HandlerJumpKickMiss },
};

static const BattleEventHandlerEntry *EventJumpKickAdd(u32 *priority) {
    *priority = 1;
    return sHandlersJumpKick;
}

static void HandlerJumpKickMiss(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        CommonJumpKickEffect(item, flow, monId, work);
    }
}

static void CommonJumpKickEffect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u16 damage;
    BattleHandlerDamageParam *param;

    damage = DivideMaxHPZeroCheck(GetBattleMon(flow, monId), 2);
    param = BattleHandler_PushWork(flow, 7, monId);
    param->targetIndex = monId;
    param->amount = damage;
    BattleHandler_StrSetup(&param->string, 2, 0x386);
    BattleHandler_AddArg(&param->string, monId);
    BattleHandler_PopWork(flow, param);
}

static const BattleEventHandlerEntry sHandlersMimic[] = {
    { 0xa0, HandlerMimic },
};

static const BattleEventHandlerEntry *EventAddMimic(u32 *priority) {
    *priority = 1;
    return sHandlersMimic;
}

static void HandlerMimic(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    BattleMon *target;
    u16 move;
    u16 used;
    u8 slot;
    BattleHandlerUpdateMoveParam *update;
    BattleHandlerMessageParam *message;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        if (!TransformCheck(mon)) {
            target = GetBattleMon(flow, BattleEventVar_GetValue(6));
            move = GetPreviousMoveID(target);
            used = GetPreviousMoveUsed(target);
            if (used != move) {
                move = used;
            }
            if (move != 0 && !func_ov169_0689cc04(move) && func_ov167_021baf78(mon, move) == 4) {
                slot = func_ov167_021baf78(mon, BattleEventItem_GetSubID(item));
                if (slot != 4) {
                    update = BattleHandler_PushWork(flow, 0x25, monId);
                    update->monIndex = monId;
                    update->slot = slot;
                    update->move = move;
                    update->maxPP = 0;
                    update->updateCurrent = 0;
                    BattleHandler_PopWork(flow, update);
                    message = BattleHandler_PushWork(flow, 4, monId);
                    BattleHandler_StrSetup(&message->string, 2, 0x2b0);
                    BattleHandler_AddArg(&message->string, monId);
                    BattleHandler_AddArg(&message->string, move);
                    BattleHandler_PopWork(flow, message);
                }
            }
        }
    }
}

static const BattleEventHandlerEntry sHandlersSketch[] = {
    { 0xa0, HandlerSketch },
};

static const BattleEventHandlerEntry *EventAddSketch(u32 *priority) {
    *priority = 1;
    return sHandlersSketch;
}

static void HandlerSketch(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    BattleMon *target;
    u16 move;
    u16 used;
    u8 slot;
    BattleHandlerUpdateMoveParam *update;
    BattleHandlerMessageParam *message;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        target = GetBattleMon(flow, BattleEventVar_GetValue(6));
        move = GetPreviousMoveID(target);
        used = GetPreviousMoveUsed(target);
        if (used != move) {
            move = used;
        }
        if (move != 0 && move != 0xa6 && move != 0xa5 && move != 0x1c0 && !MoveIsUsable(mon, move) &&
            !TransformCheck(mon)) {
            slot = func_ov167_021baf78(mon, BattleEventItem_GetSubID(item));
            if (slot != 4) {
                update = BattleHandler_PushWork(flow, 0x25, monId);
                update->monIndex = monId;
                update->slot = slot;
                update->move = move;
                update->maxPP = 0;
                update->updateCurrent = 1;
                BattleHandler_PopWork(flow, update);
                message = BattleHandler_PushWork(flow, 4, monId);
                BattleHandler_StrSetup(&message->string, 2, 0x2b0);
                BattleHandler_AddArg(&message->string, monId);
                BattleHandler_AddArg(&message->string, move);
                BattleHandler_PopWork(flow, message);
            }
        }
    }
}

static const BattleEventHandlerEntry sHandlersFollowMe[] = {
    { 0x1f, HandlerFollowMeCheckFail },
    { 0xa0, HandlerFollowMeTextSet },
    { 0x2a, HandlerFollowMeBaitTarget },
    { 0x76, HandlerFollowMeTurnCheck },
};

static const BattleEventHandlerEntry *EventAddFollowMe(u32 *priority) {
    *priority = 4;
    return sHandlersFollowMe;
}

static void HandlerFollowMeCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if ((func_ov167_021abc9c(flow) == 0 || func_ov167_021abc9c(flow) == 3) && BattleEventVar_GetValue(2) == monId &&
        BattleEventVar_GetValue(0x22) == 0) {
        BattleEventVar_RewriteValue(0x22, 0x1a);
    }
}

static void HandlerFollowMeTextSet(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *message;

    if (BattleEventVar_GetValue(3) == monId) {
        message = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&message->string, 2, 0x29e);
        BattleHandler_AddArg(&message->string, monId);
        BattleHandler_PopWork(flow, message);
        work[6] = TRUE;
    }
}

static void HandlerFollowMeBaitTarget(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 attackerId;
    BattleMon *mon;
    u16 move;

    if (func_ov167_021abc9c(flow) != 0 && func_ov167_021abc9c(flow) != 3) {
        attackerId = BattleEventVar_GetValue(3);
        mon = GetBattleMon(flow, monId);
        if (!IsAllyMonID(monId, attackerId) && BattleEventVar_GetValue(0x12) != 0x1fb && !CheckCondition(mon, 0x21) &&
            !IsMonSwitchingOut(flow)) {
            move = BattleEventVar_GetValue(0x12);
            if (!func_ov167_021abdd0(flow, attackerId, monId, move)) {
                BattleEventVar_RewriteValue(4, monId);
            }
        }
    }
}

static void HandlerFollowMeTurnCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    RemoveHandlerForce(monId, BattleEventItem_GetSubID(item));
}

static const BattleEventHandlerEntry sHandlersRefresh[] = {
    { 0xa0, HandlerRefresh },
};

static const BattleEventHandlerEntry *EventAddRefresh(u32 *priority) {
    *priority = 1;
    return sHandlersRefresh;
}

static void HandlerRefresh(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 status;
    BattleHandlerCureConditionParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        status = GetBattleMonStatus(GetBattleMon(flow, monId));
        if (status == 5 || status == 1 || status == 4) {
            param = BattleHandler_PushWork(flow, 0xb, monId);
            param->condition = status;
            param->monIds[0] = monId;
            param->count = 1;
            BattleHandler_PopWork(flow, param);
        }
    }
}

static const BattleEventHandlerEntry sHandlersSpiderWeb[] = {
    { 0xa0, HandlerSpiderWeb },
};

static const BattleEventHandlerEntry *EventAddSpiderWeb(u32 *priority) {
    *priority = 1;
    return sHandlersSpiderWeb;
}

static void HandlerSpiderWeb(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        if (!CheckCondition(GetBattleMon(flow, targetId), 0x16)) {
            param = BattleHandler_PushWork(flow, 0xc, monId);
            param->targetIndex = targetId;
            param->condition = 0x16;
            param->value = func_ov167_021bd5b0(monId);
            BattleHandler_StrSetup(&param->string, 2, 0x368);
            BattleHandler_AddArg(&param->string, targetId);
            BattleHandler_PopWork(flow, param);
        }
    }
}

static const BattleEventHandlerEntry sHandlersHaze[] = {
    { 0x9e, HandlerHaze },
};

static const BattleEventHandlerEntry *EventAddHaze(u32 *priority) {
    *priority = 1;
    return sHandlersHaze;
}

static void HandlerHaze(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerResetStatStageParam *reset;
    BattleHandlerMessageParam *message;

    if (BattleEventVar_GetValue(3) == monId) {
        reset = BattleHandler_PushWork(flow, 0x10, monId);
        reset->count = HandlerGetAlivePartyCount(flow, 0x800 | func_ov167_021ab840(flow, monId), reset->monIndices);
        BattleHandler_PopWork(flow, reset);
        message = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&message->string, 1, 0x65);
        BattleHandler_PopWork(flow, message);
    }
}

static const BattleEventHandlerEntry sHandlersSplash[] = {
    { 0xa0, HandlerSplash },
};

static const BattleEventHandlerEntry *EventAddSplash(u32 *priority) {
    *priority = 1;
    return sHandlersSplash;
}

static void HandlerSplash(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *message;

    if (BattleEventVar_GetValue(3) == monId) {
        message = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&message->string, 1, 0x66);
        BattleHandler_PopWork(flow, message);
    }
}

static const BattleEventHandlerEntry sHandlersCurse[] = {
    { 0x28, HandlerCurseMoveParam },
    { 0xa0, HandlerCurse },
};

static const BattleEventHandlerEntry *EventAddCurse(u32 *priority) {
    *priority = 2;
    return sHandlersCurse;
}

static void HandlerCurseMoveParam(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        BattleEventVar_RewriteValue(0x1b, func_ov167_021bd8d0(GetBattleMon(flow, monId)));
    }
}

static void HandlerCurse(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u8 effect;
    u8 targetId;
    BattleHandlerAddConditionParam *condition;
    BattleHandlerChangeHPParam *hp;
    BattleHandlerStatChangeParam *stat;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        effect = 0;
        if (DoesMonHaveType(mon, 7)) {
            if ((s32)GetBattleMonStat(mon, 0xd) > 0) {
                targetId = BattleEventVar_GetValue(6);
                if (targetId != 0x1f) {
                    condition = BattleHandler_PushWork(flow, 0xc, monId);
                    condition->targetIndex = targetId;
                    condition->condition = 0xa;
                    condition->value = MakeConditionPermanent();
                    BattleHandler_StrSetup(&condition->string, 2, 0x428);
                    BattleHandler_AddArg(&condition->string, monId);
                    BattleHandler_AddArg(&condition->string, targetId);
                    BattleHandler_PopWork(flow, condition);
                    hp = BattleHandler_PushWork(flow, 8, monId);
                    hp->monIds[0] = monId;
                    hp->hpChanges[0] = -DivideMaxHPZeroCheck(mon, 2);
                    hp->count = 1;
                    hp->header.checkPrevResult = TRUE;
                    BattleHandler_PopWork(flow, hp);
                }
            }
            effect = 1;
        } else {
            stat = BattleHandler_PushWork(flow, 0xe, monId);
            stat->count = 1;
            stat->monIds[0] = monId;
            stat->stat = 5;
            stat->change = -1;
            BattleHandler_PopWork(flow, stat);
            stat = BattleHandler_PushWork(flow, 0xe, monId);
            stat->count = 1;
            stat->monIds[0] = monId;
            stat->stat = 1;
            stat->change = 1;
            BattleHandler_PopWork(flow, stat);
            stat = BattleHandler_PushWork(flow, 0xe, monId);
            stat->count = 1;
            stat->monIds[0] = monId;
            stat->stat = 2;
            stat->change = 1;
            BattleHandler_PopWork(flow, stat);
        }
        SetMoveEffectIndex(flow, effect);
    }
}

static const BattleEventHandlerEntry sHandlersThunderWave[] = {
    { 0x3d, HandlerThunderWave },
};

static const BattleEventHandlerEntry *EventAddThunderWave(u32 *priority) {
    *priority = 1;
    return sHandlersThunderWave;
}

static void HandlerThunderWave(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_RewriteValue(0x51, 1);
    }
}

static const BattleEventHandlerEntry sHandlersWorrySeed[] = {
    { 0x2c, HandlerWorrySeedCheckFail },
    { 0xa0, HandlerWorrySeed },
};

static const BattleEventHandlerEntry *EventAddWorrySeed(u32 *priority) {
    *priority = 2;
    return sHandlersWorrySeed;
}

static void HandlerWorrySeedCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *target;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(4);
        target = GetBattleMon(flow, targetId);
        if (GetBattleMonStat(target, 0x11) == 0x36 || GetBattleMonStat(target, 0x11) == 0xf) {
            BattleEventVar_RewriteValue(0x40, 1);
        }
    }
}

static void HandlerWorrySeed(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleHandlerAbilityChangeParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        param = BattleHandler_PushWork(flow, 0x1f, monId);
        param->targetIndex = targetId;
        param->ability = 0xf;
        BattleHandler_StrSetup(&param->string, 2, 0x195);
        BattleHandler_AddArg(&param->string, param->targetIndex);
        BattleHandler_AddArg(&param->string, param->ability);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersDreamEater[] = {
    { 0x2c, HandlerDreamEater },
    { 0x5, HandlerDreamEaterSubstituteCheck },
};

static const BattleEventHandlerEntry *EventAddDreamEater(u32 *priority) {
    *priority = 2;
    return sHandlersDreamEater;
}

static void HandlerDreamEater(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(4);
        if (!CheckCondition(GetBattleMon(flow, targetId), 2)) {
            BattleEventVar_RewriteValue(0x40, 1);
        }
    }
}

static void HandlerDreamEaterSubstituteCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_RewriteValue(0x40, 1);
    }
}

static const BattleEventHandlerEntry sHandlersCaptivate[] = {
    { 0x2c, HandlerCaptivate },
};

static const BattleEventHandlerEntry *EventAddCaptivate(u32 *priority) {
    *priority = 1;
    return sHandlersCaptivate;
}

static void HandlerCaptivate(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *target;
    u8 sex;
    u8 targetSex;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(4);
        target = GetBattleMon(flow, targetId);
        sex = GetBattleMonStat(GetBattleMon(flow, monId), 0x12);
        targetSex = GetBattleMonStat(target, 0x12);
        if (sex == 2 || targetSex == 2 || sex == targetSex) {
            BattleEventVar_RewriteValue(0x40, 1);
        }
    }
}

static const BattleEventHandlerEntry sHandlersTriAttack[] = {
    { 0x64, HandlerTriAttack },
};

static const BattleEventHandlerEntry *EventAddTriAttack(u32 *priority) {
    *priority = 1;
    return sHandlersTriAttack;
}

// Paralysis, burn and freeze
static void HandlerTriAttack(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    static const u32 sTriAttackStatuses[3] = { 1, 4, 3 };
    u32 status;
    BattleCondition condition;

    if (BattleEventVar_GetValue(3) == monId) {
        status = sTriAttackStatuses[(u8)BattleRandom(3)];
        condition = func_ov167_021bd52c(status);
        BattleEventVar_RewriteValue(0x1d, status);
        BattleEventVar_RewriteValue(0x1e, condition.raw);
        BattleEventVar_RewriteValue(0x26, 20);
    }
}

static const BattleEventHandlerEntry sHandlersSecretPower[] = {
    { 0x83, HandlerSecretPower },
    { 0x24, HandlerSecretPowerStart },
};

static const BattleEventHandlerEntry *EventAddSecretPower(u32 *priority) {
    *priority = 2;
    return sHandlersSecretPower;
}

// The effect of Secret Power on the battle's terrain: its effect index, then whether it inflicts a status (0), lowers
// a stat (1) or flinches (2), and which status or stat
static u8 CommonSecretPowerGetParams(BtlServerFlow *flow, u8 *kind, u8 *arg) {
    u32 terrain;
    u8 effect;
    u8 effectKind;
    u8 effectArg;

    terrain = GetBattleTerrain(flow);
    switch (terrain) {
    case 1:
    case 2:
    case 3:
    case 8:
    case 15:
    case 17:
        effectKind = 1;
        effectArg = 6;
        effect = 1;
        break;
    default:
        effectKind = 0;
        effectArg = 1;
        effect = 0;
        break;
    case 10:
    case 19:
        effectKind = 2;
        effectArg = 0;
        effect = 6;
        break;
    case 0:
    case 5:
        effectKind = 0;
        effectArg = 2;
        effect = 7;
        break;
    case 6:
    case 11:
    case 12:
        effectKind = 1;
        effectArg = 1;
        effect = 5;
        break;
    case 9:
        effectKind = 1;
        effectArg = 5;
        effect = 2;
        break;
    case 7:
    case 13:
        effectKind = 0;
        effectArg = 3;
        effect = terrain == 7 ? 3 : 4;
        break;
    }
    if (kind != NULL) {
        *kind = effectKind;
    }
    if (arg != NULL) {
        *arg = effectArg;
    }
    return effect;
}

static void HandlerSecretPowerStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        SetMoveEffectIndex(flow, CommonSecretPowerGetParams(flow, NULL, NULL));
    }
}

static void HandlerSecretPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u8 targetId;
    u8 kind;
    u8 arg;
    BattleHandlerAddConditionParam *condition;
    BattleHandlerStatChangeParam *stat;
    BattleHandlerFlinchParam *flinch;

    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x47) == 0 && BattleRandom(100) < 30) {
        mon = GetBattleMon(flow, monId);
        targetId = BattleEventVar_GetValue(6);
        CommonSecretPowerGetParams(flow, &kind, &arg);
        switch (kind) {
        case 0:
            condition = BattleHandler_PushWork(flow, 0xc, monId);
            condition->targetIndex = targetId;
            condition->condition = arg;
            func_ov167_021bd5d4(arg, mon, &condition->value);
            BattleHandler_PopWork(flow, condition);
            break;
        case 1:
            stat = BattleHandler_PushWork(flow, 0xe, monId);
            stat->count = 1;
            stat->monIds[0] = targetId;
            stat->stat = arg;
            stat->change = -1;
            BattleHandler_PopWork(flow, stat);
            break;
        case 2:
            flinch = BattleHandler_PushWork(flow, 0x2b, monId);
            flinch->monIndex = targetId;
            flinch->flag = 100;
            BattleHandler_PopWork(flow, flinch);
            break;
        }
    }
}

static const BattleEventHandlerEntry sHandlersChatter[] = {
    { 0x64, HandlerChatter },
};

static const BattleEventHandlerEntry *EventAddChatter(u32 *priority) {
    *priority = 1;
    return sHandlersChatter;
}

static void HandlerChatter(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u8 volume;
    BattleConditionID status;
    u32 chance;
    BattleCondition condition;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        if (GetBattleMonSpecies(mon) == 0x1b9) {
            volume = func_ov167_021aba18(flow, monId);
            if (volume != 0) {
                status = CONDITION_CONFUSION;
                GetSrcData(mon);
                if (volume > 2) {
                    chance = 30;
                } else {
                    chance = 10;
                }
                func_ov167_021bd5d4(status, mon, &condition);
                BattleEventVar_RewriteValue(0x1d, status);
                BattleEventVar_RewriteValue(0x1e, condition.raw);
                BattleEventVar_RewriteValue(0x26, chance);
            }
        }
    }
}

static const BattleEventHandlerEntry sHandlersBind[] = {
    { 0x64, HandlerBind },
    { 0x61, HandlerBindTextSet },
};

static const BattleEventHandlerEntry *EventAddBind(u32 *priority) {
    *priority = 2;
    return sHandlersBind;
}

static void HandlerBind(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u16 move;
    BattleCondition condition;

    if (BattleEventVar_GetValue(3) == monId) {
        move = BattleEventItem_GetSubID(item);
        condition.raw = BattleEventVar_GetValue(0x1e);
        func_ov167_021ce368(&condition, move);
        BattleEventVar_RewriteValue(0x1e, condition.raw);
    }
}

static void HandlerBindTextSet(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerString *string;
    u8 targetId;
    u16 message;

    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x1d) == 8) {
        string = (BattleHandlerString *)BattleEventVar_GetValue(0x3f);
        targetId = BattleEventVar_GetValue(4);
        switch (BattleEventItem_GetSubID(item)) {
        case 0x23:
            message = 0x32a;
            break;
        case 0x14:
            message = 0x323;
            break;
        case 0x53:
            message = 0x33b;
            break;
        case 0x80:
            message = 0x331;
            break;
        case 0x148:
            message = 0x341;
            break;
        case 0x1cf:
            message = 0x33e;
            break;
        case 0xfa:
            message = 0x338;
            break;
        default:
            return;
        }
        BattleHandler_StrSetup(string, 2, message);
        BattleHandler_AddArg(string, targetId);
        BattleHandler_AddArg(string, monId);
    }
}

static const BattleEventHandlerEntry sHandlersWhirlpool[] = {
    { 0x64, HandlerBind },
    { 0x61, HandlerBindTextSet },
    { 0x99, HandlerWhirlpoolDiveCheck },
    { 0x47, HandlerWhirlpoolPower },
};

static const BattleEventHandlerEntry *EventAddWhirlpool(u32 *priority) {
    *priority = 4;
    return sHandlersWhirlpool;
}

static void HandlerWhirlpoolDiveCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x21) == 4) {
        BattleEventVar_RewriteValue(0x42, 0);
    }
}

static void HandlerWhirlpoolPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId &&
        GetAdditionalConditionFlag(GetBattleMon(flow, BattleEventVar_GetValue(4)), 4)) {
        BattleEventVar_MulValue(0x35, 0x2000);
    }
}

static const BattleEventHandlerEntry sHandlersSuperFang[] = {
    { 0x46, HandlerSuperFang },
};

static const BattleEventHandlerEntry *EventAddSuperFang(u32 *priority) {
    *priority = 1;
    return sHandlersSuperFang;
}

static void HandlerSuperFang(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    s32 hp;
    u16 damage;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(4);
        hp = GetBattleMonStat(GetBattleMon(flow, targetId), 0xd);
        damage = hp / 2;
        if (damage == 0) {
            damage = 1;
        }
        BattleEventVar_RewriteValue(0x37, damage);
    }
}

static const BattleEventHandlerEntry sHandlersDragonRage[] = {
    { 0x46, HandlerDragonRage },
};

static const BattleEventHandlerEntry *EventAddDragonRage(u32 *priority) {
    *priority = 1;
    return sHandlersDragonRage;
}

static void HandlerDragonRage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_RewriteValue(0x37, 40);
    }
}

static const BattleEventHandlerEntry sHandlersSonicBoom[] = {
    { 0x46, HandlerSonicBoom },
};

static const BattleEventHandlerEntry *EventAddSonicBoom(u32 *priority) {
    *priority = 1;
    return sHandlersSonicBoom;
}

static void HandlerSonicBoom(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_RewriteValue(0x37, 20);
    }
}

static const BattleEventHandlerEntry sHandlersEndeavor[] = {
    { 0x2c, HandlerEndeavorCheckFail },
    { 0x46, HandlerEndeavor },
};

static const BattleEventHandlerEntry *EventAddEndeavor(u32 *priority) {
    *priority = 2;
    return sHandlersEndeavor;
}

static void HandlerEndeavorCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *target;
    BattleMon *mon;
    s32 diff;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(4);
        target = GetBattleMon(flow, targetId);
        mon = GetBattleMon(flow, monId);
        diff = GetBattleMonStat(target, 0xd) - GetBattleMonStat(mon, 0xd);
        if (diff <= 0) {
            BattleEventVar_RewriteValue(0x40, 1);
        }
    }
}

static void HandlerEndeavor(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    BattleMon *target;
    s32 damage;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        target = GetBattleMon(flow, BattleEventVar_GetValue(4));
        damage = GetBattleMonStat(target, 0xd) - GetBattleMonStat(mon, 0xd);
        if (damage < 0) {
            damage = 1;
        }
        BattleEventVar_RewriteValue(0x37, damage);
    }
}

static const BattleEventHandlerEntry sHandlersSeismicToss[] = {
    { 0x46, HandlerSeismicToss },
};

static const BattleEventHandlerEntry *EventAddSeismicToss(u32 *priority) {
    *priority = 1;
    return sHandlersSeismicToss;
}

static void HandlerSeismicToss(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 level;

    if (BattleEventVar_GetValue(3) == monId) {
        level = GetBattleMonStat(GetBattleMon(flow, monId), 0xf);
        BattleEventVar_RewriteValue(0x37, level);
    }
}

static const BattleEventHandlerEntry sHandlersPsywave[] = {
    { 0x46, HandlerPsywave },
};

static const BattleEventHandlerEntry *EventAddPsywave(u32 *priority) {
    *priority = 1;
    return sHandlersPsywave;
}

static void HandlerPsywave(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 level;
    u16 percent;
    u16 damage;

    if (BattleEventVar_GetValue(3) == monId) {
        level = GetBattleMonStat(GetBattleMon(flow, monId), 0xf);
        percent = BattleRandom(101) + 50;
        damage = level * percent / 100;
        if (damage == 0) {
            damage = 1;
        }
        BattleEventVar_RewriteValue(0x37, damage);
    }
}

static const BattleEventHandlerEntry sHandlersStockpile[] = {
    { 0x1f, HandlerStockpileCheckCount },
    { 0xa0, HandlerStockpileIncreaseStats },
};

static const BattleEventHandlerEntry *EventAddStockpile(u32 *priority) {
    *priority = 2;
    return sHandlersStockpile;
}

static void HandlerStockpileCheckCount(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId && GetConditionCount(GetBattleMon(flow, monId), 0) >= 3) {
        BattleEventVar_RewriteValue(0x22, 0x1a);
    }
}

static void HandlerStockpileIncreaseStats(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u8 count;
    BattleHandlerSetCounterParam *counter;
    BattleHandlerMessageParam *message;
    BattleHandlerStatChangeParam *stat;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        count = GetConditionCount(mon, 0);
        if (count < 3) {
            counter = BattleHandler_PushWork(flow, 0x26, monId);
            counter->monIndex = monId;
            counter->counter = 0;
            counter->value = count + 1;
            BattleHandler_PopWork(flow, counter);
            message = BattleHandler_PushWork(flow, 4, monId);
            BattleHandler_StrSetup(&message->string, 2, 0x2d1);
            BattleHandler_AddArg(&message->string, monId);
            BattleHandler_AddArg(&message->string, count + 1);
            BattleHandler_PopWork(flow, message);
            stat = BattleHandler_PushWork(flow, 0xe, monId);
            stat->count = 1;
            stat->monIds[0] = monId;
            stat->stat = 2;
            stat->change = 1;
            stat->unk0e = 1;
            BattleHandler_PopWork(flow, stat);
            counter = BattleHandler_PushWork(flow, 0x26, monId);
            counter->header.checkPrevResult = TRUE;
            counter->monIndex = monId;
            counter->counter = 1;
            counter->value = GetConditionCount(mon, 1) + 1;
            BattleHandler_PopWork(flow, counter);
            stat = BattleHandler_PushWork(flow, 0xe, monId);
            stat->count = 1;
            stat->monIds[0] = monId;
            stat->stat = 4;
            stat->change = 1;
            stat->unk0e = 1;
            BattleHandler_PopWork(flow, stat);
            counter = BattleHandler_PushWork(flow, 0x26, monId);
            counter->header.checkPrevResult = TRUE;
            counter->monIndex = monId;
            counter->counter = 2;
            counter->value = GetConditionCount(mon, 2) + 1;
            BattleHandler_PopWork(flow, counter);
        }
    }
}

static const BattleEventHandlerEntry sHandlersSpitUp[] = {
    { 0x1f, HandlerSpitUpStart },
    { 0x37, HandlerSpitUpPower },
    { 0x27, HandlerSpitUpResetStats },
};

static const BattleEventHandlerEntry *EventAddSpitUp(u32 *priority) {
    *priority = 3;
    return sHandlersSpitUp;
}

static void HandlerSpitUpStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId && GetConditionCount(GetBattleMon(flow, monId), 0) == 0) {
        BattleEventVar_RewriteValue(0x22, 0x1a);
    }
}

static void HandlerSpitUpPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 power;

    if (BattleEventVar_GetValue(3) == monId) {
        power = GetConditionCount(GetBattleMon(flow, monId), 0) * 100;
        BattleEventVar_RewriteValue(0x30, power);
    }
}

static void HandlerSpitUpResetStats(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u8 defense;
    u8 spDefense;
    u8 count;
    BattleHandlerStatChangeParam *stat;
    BattleHandlerSetCounterParam *counter;
    BattleHandlerMessageParam *message;

    if (BattleEventVar_GetValue(2) == monId) {
        mon = GetBattleMon(flow, monId);
        defense = GetConditionCount(mon, 1);
        spDefense = GetConditionCount(mon, 2);
        count = GetConditionCount(mon, 0);
        if (defense != 0) {
            stat = BattleHandler_PushWork(flow, 0xe, monId);
            stat->count = 1;
            stat->monIds[0] = monId;
            stat->stat = 2;
            stat->change = -defense;
            BattleHandler_PopWork(flow, stat);
            counter = BattleHandler_PushWork(flow, 0x26, monId);
            counter->monIndex = monId;
            counter->counter = 1;
            counter->value = 0;
            BattleHandler_PopWork(flow, counter);
        }
        if (spDefense != 0) {
            stat = BattleHandler_PushWork(flow, 0xe, monId);
            stat->count = 1;
            stat->monIds[0] = monId;
            stat->stat = 4;
            stat->change = -spDefense;
            BattleHandler_PopWork(flow, stat);
            counter = BattleHandler_PushWork(flow, 0x26, monId);
            counter->monIndex = monId;
            counter->counter = 2;
            counter->value = 0;
            BattleHandler_PopWork(flow, counter);
        }
        if (count != 0) {
            counter = BattleHandler_PushWork(flow, 0x26, monId);
            counter->monIndex = monId;
            counter->counter = 0;
            counter->value = 0;
            BattleHandler_PopWork(flow, counter);
            message = BattleHandler_PushWork(flow, 4, monId);
            BattleHandler_StrSetup(&message->string, 2, 0x2d4);
            BattleHandler_AddArg(&message->string, monId);
            BattleHandler_PopWork(flow, message);
        }
    }
}

static const BattleEventHandlerEntry sHandlersSwallow[] = {
    { 0x1f, HandlerSpitUpStart },
    { 0x8f, HandlerSwallow },
    { 0x27, HandlerSpitUpResetStats },
};

static const BattleEventHandlerEntry *EventAddSwallow(u32 *priority) {
    *priority = 3;
    return sHandlersSwallow;
}

static void HandlerSwallow(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    s32 ratio;

    if (BattleEventVar_GetValue(2) == monId) {
        switch (GetConditionCount(GetBattleMon(flow, monId), 0)) {
        default:
        case 1:
            ratio = 0x400;
            break;
        case 2:
            ratio = 0x800;
            break;
        case 3:
            ratio = 0x1000;
            break;
        }
        BattleEventVar_RewriteValue(0x35, ratio);
    }
}

static const BattleEventHandlerEntry sHandlersCounter[] = {
    { 0x1f, HandlerCounterStart },
    { 0x29, HandlerCounterSetTarget },
    { 0x46, HandlerCounterCalcDamage },
};

static const BattleEventHandlerEntry *EventAddCounter(u32 *priority) {
    *priority = 3;
    return sHandlersCounter;
}

static void HandlerCounterStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonCounterStart(flow, monId, work, 1);
}

static void HandlerCounterSetTarget(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonCounterSetTarget(flow, monId, work, 1);
}

static void HandlerCounterCalcDamage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonCounterCalcDamage(flow, monId, work, 1, 0x2000);
}

static const BattleEventHandlerEntry sHandlersMirrorCoat[] = {
    { 0x1f, HandlerMirrorCoatStart },
    { 0x29, HandlerMirrorCoatSetTarget },
    { 0x46, HandlerMirrorCoatCalcDamage },
};

static const BattleEventHandlerEntry *EventAddMirrorCoat(u32 *priority) {
    *priority = 3;
    return sHandlersMirrorCoat;
}

static void HandlerMirrorCoatStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonCounterStart(flow, monId, work, 2);
}

static void HandlerMirrorCoatSetTarget(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonCounterSetTarget(flow, monId, work, 2);
}

static void HandlerMirrorCoatCalcDamage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonCounterCalcDamage(flow, monId, work, 2, 0x2000);
}

static const BattleEventHandlerEntry sHandlersMetalBurst[] = {
    { 0x1f, HandlerMetalBurstStart },
    { 0x29, HandlerMetalBurstSetTarget },
    { 0x46, HandlerMetalBurstCalcDamage },
};

static const BattleEventHandlerEntry *EventAddMetalBurst(u32 *priority) {
    *priority = 3;
    return sHandlersMetalBurst;
}

static void HandlerMetalBurstStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonCounterStart(flow, monId, work, 0);
}

static void HandlerMetalBurstSetTarget(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonCounterSetTarget(flow, monId, work, 0);
}

static void HandlerMetalBurstCalcDamage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonCounterCalcDamage(flow, monId, work, 0, 0x1800);
}

// Fails unless the mon took a hit of the category (any for 0) this turn
static void CommonCounterStart(BtlServerFlow *flow, u8 monId, s32 *work, u32 category) {
    BattleMonDamageRecord record;

    if (BattleEventVar_GetValue(2) == monId &&
        !CommonCounterCheckDamageRecieved(GetBattleMon(flow, monId), category, &record)) {
        BattleEventVar_RewriteValue(0x22, 0x1a);
    }
}

// Targets the mon that hit, or the one now in its place
static void CommonCounterSetTarget(BtlServerFlow *flow, u8 monId, s32 *work, u32 category) {
    BattleMonDamageRecord record;
    u8 targetId;
    u8 pos;

    if (BattleEventVar_GetValue(3) == monId &&
        CommonCounterCheckDamageRecieved(GetBattleMon(flow, monId), category, &record)) {
        targetId = record.attackerId;
        if (func_ov167_021ab840(flow, targetId) == 6) {
            targetId = 0x1f;
            if (record.attackerPos != 6) {
                targetId = func_ov167_021ab884(flow, record.attackerPos);
            }
            if (targetId == 0x1f) {
                pos = func_ov167_021abb70(flow, monId, 1);
                if (pos != 6) {
                    targetId = func_ov167_021ab884(flow, pos);
                }
                if (targetId == 0x1f) {
                    targetId = record.attackerId;
                }
            }
        }
        BattleEventVar_RewriteValue(4, targetId);
    }
}

static void CommonCounterCalcDamage(BtlServerFlow *flow, u8 monId, s32 *work, u32 category, u32 ratio) {
    BattleMonDamageRecord record;

    if (BattleEventVar_GetValue(3) == monId &&
        CommonCounterCheckDamageRecieved(GetBattleMon(flow, monId), category, &record)) {
        BattleEventVar_RewriteValue(0x37, fixed_round(record.damage, ratio));
    }
}

// Finds the latest hit this turn from a foe with a move of the category (any for 0)
static BOOL CommonCounterCheckDamageRecieved(BattleMon *mon, u32 category, BattleMonDamageRecord *record) {
    u8 monId;
    u8 i;

    monId = GetMonID(mon);
    i = 0;
    while (GetDamageReceived(mon, 0, i++, record)) {
        if (!IsAllyMonID(monId, record->attackerId) &&
            (category == 0 || category == PML_MoveGetCategory(record->move))) {
            return TRUE;
        }
    }
    return FALSE;
}

static const BattleEventHandlerEntry sHandlersLastResort[] = {
    { 0x1f, HandlerLastResort },
};

static const BattleEventHandlerEntry *EventAddLastResort(u32 *priority) {
    *priority = 1;
    return sHandlersLastResort;
}

static void HandlerLastResort(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u32 moveCount;
    u8 used;
    u8 i;
    BOOL hasLastResort;

    if (BattleEventVar_GetValue(2) == monId) {
        mon = GetBattleMon(flow, monId);
        moveCount = GetBattleMonMoveCount(mon);
        used = 0;
        hasLastResort = FALSE;
        for (i = 0; i < moveCount; i++) {
            if (MoveGetID(mon, i) == 0x183) {
                hasLastResort = TRUE;
            } else if (CheckIfMoveWasUsed(mon, i)) {
                used++;
            }
        }
        if (!hasLastResort || moveCount < 2 || used < (s32)(moveCount - 1)) {
            BattleEventVar_RewriteValue(0x22, 0x1a);
        }
    }
}

static const BattleEventHandlerEntry sHandlersSnore[] = {
    { 0x1d, HandlerSnoreCheck1 },
    { 0x1f, HandlerSnoreCheck2 },
};

static const BattleEventHandlerEntry *EventAddSnore(u32 *priority) {
    *priority = 2;
    return sHandlersSnore;
}

static void HandlerSnoreCheck1(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId && BattleEventVar_GetValue(0x22) == 2) {
        BattleEventVar_RewriteValue(0x41, 1);
    }
}

static void HandlerSnoreCheck2(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;

    if (BattleEventVar_GetValue(2) == monId) {
        mon = GetBattleMon(flow, monId);
        if (BattleEventVar_GetValue(0x22) == 2) {
            BattleEventVar_RewriteValue(0x22, 0);
        } else if (!CheckCondition(mon, 2)) {
            BattleEventVar_RewriteValue(0x22, 0x1a);
        }
    }
}

static const BattleEventHandlerEntry sHandlersNightmare[] = {
    { 0x2c, HandlerNightmareNoEffect },
};

static const BattleEventHandlerEntry *EventAddNightmare(u32 *priority) {
    *priority = 1;
    return sHandlersNightmare;
}

static void HandlerNightmareNoEffect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(4);
        if (!CheckCondition(GetBattleMon(flow, targetId), 2)) {
            BattleEventVar_RewriteValue(0x40, 1);
        }
    }
}

static const BattleEventHandlerEntry sHandlersSuckerPunch[] = {
    { 0x2c, HandlerSuckerPunch },
};

static const BattleEventHandlerEntry *EventAddSuckerPunch(u32 *priority) {
    *priority = 1;
    return sHandlersSuckerPunch;
}

static void HandlerSuckerPunch(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *target;
    BOOL attacking;
    u16 move;
    BattleAction action;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(4);
        target = GetBattleMon(flow, targetId);
        attacking = FALSE;
        if (!GetTurnFlag(target, 3) && func_ov167_021abb8c(flow, targetId, &action)) {
            move = func_ov167_021bdb68(&action);
            if (move != 0 && PML_MoveIsDamaging(move)) {
                attacking = TRUE;
            }
        }
        if (!attacking) {
            move = func_ov167_021bdb68(&action);
            if (move == 0 || !PML_MoveIsDamaging(move)) {
                if (BattleEventVar_RewriteValue(0x40, 1)) {
                    BattleHandler_StrSetup((BattleHandlerString *)BattleEventVar_GetValue(0x3f), 1, 0x47);
                }
            }
        }
    }
}

static const BattleEventHandlerEntry sHandlersPursuit[] = {
    { 0x53, HandlerPursuitStart },
    { 0x32, HandlerPursuitHitCheck },
    { 0x37, HandlerPursuitPower },
};

static const BattleEventHandlerEntry *EventAddPursuit(u32 *priority) {
    *priority = 3;
    return sHandlersPursuit;
}

static void HandlerPursuitStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleAction action;
    u8 targetId;

    if (func_ov167_021abb8c(flow, monId, &action)) {
        targetId = BattleEventVar_GetValue(6);
        if (!IsAllyMonID(targetId, monId)) {
            func_ov167_021abb50(flow, targetId);
            func_ov167_021abb50(flow, monId);
            if (!func_ov167_021abdd0(flow, monId, targetId, 0)) {
                AddSwitchOutInterrupt(flow, monId);
            }
        }
    }
}

static void HandlerPursuitHitCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && IsMonSwitchingOut(flow)) {
        BattleEventVar_RewriteValue(0x51, 1);
    }
}

static void HandlerPursuitPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && IsMonSwitchingOut(flow)) {
        MultiplyBasePower(2);
    }
}

static const BattleEventHandlerEntry sHandlersExplosion[] = {
    { 0x45, HandlerExplosionDamageDetermine },
    { 0x24, HandlerExplosionStart },
    { 0x27, HandlerExplosionEnd },
};

static const BattleEventHandlerEntry *EventAddExplosion(u32 *priority) {
    *priority = 3;
    return sHandlersExplosion;
}

static void HandlerExplosionDamageDetermine(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerFaintParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        param = BattleHandler_PushWork(flow, 0x13, monId);
        param->monIndex = monId;
        BattleHandler_PopWork(flow, param);
    }
}

static void HandlerExplosionStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        BattleHandler_PushRun(flow, 0x3b, monId);
    }
}

static void HandlerExplosionEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerFaintParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        param = BattleHandler_PushWork(flow, 0x13, monId);
        param->monIndex = monId;
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersFocusEnergy[] = {
    { 0xa0, HandlerFocusEnergy },
};

static const BattleEventHandlerEntry *EventAddFocusEnergy(u32 *priority) {
    *priority = 1;
    return sHandlersFocusEnergy;
}

static void HandlerFocusEnergy(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerFlagParam *flag;
    BattleHandlerMessageParam *message;

    if (BattleEventVar_GetValue(3) == monId && !GetAdditionalConditionFlag(GetBattleMon(flow, monId), 9)) {
        flag = BattleHandler_PushWork(flow, 0x17, monId);
        flag->monIndex = monId;
        flag->flag = 9;
        BattleHandler_PopWork(flow, flag);
        message = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&message->string, 2, 0x411);
        BattleHandler_AddArg(&message->string, monId);
        BattleHandler_PopWork(flow, message);
    }
}

static const BattleEventHandlerEntry sHandlersCharge[] = {
    { 0x24, HandlerCharge },
    { 0x3, HandlerChargeStart },
    { 0x38, HandlerChargePower },
    { 0x4, HandlerChargeEnd },
};

static const BattleEventHandlerEntry *EventAddCharge(u32 *priority) {
    *priority = 4;
    return sHandlersCharge;
}

static void HandlerCharge(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *message;

    if (BattleEventVar_GetValue(2) == monId && BattleEventVar_GetValue(0x12) == BattleEventItem_GetSubID(item)) {
        message = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&message->string, 2, 0x298);
        BattleHandler_AddArg(&message->string, monId);
        BattleHandler_PopWork(flow, message);
        BattleEventVar_RewriteValue(0x3e, 1);
        work[6] = TRUE;
        work[0] = 1;
    }
}

static void HandlerChargePower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (work[0] == 2 && BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x16) == 0xc) {
        BattleEventVar_MulValue(0x31, 0x2000);
    }
}

static void HandlerChargeStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x12) != BattleEventItem_GetSubID(item)) {
        work[0] = 2;
    }
}

static void HandlerChargeEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId && work[0] == 2) {
        BattleEventItem_Remove(item);
    }
}

static const BattleEventHandlerEntry sHandlersPerishSong[] = {
    { 0x25, HandlerPerishSongStart },
    { 0x5, HandlerBypassSubstitute },
};

static const BattleEventHandlerEntry *EventAddPerishSong(u32 *priority) {
    *priority = 2;
    return sHandlersPerishSong;
}

static void HandlerPerishSongStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *message;

    if (BattleEventVar_GetValue(2) == monId) {
        message = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&message->string, 1, 0x77);
        BattleHandler_PopWork(flow, message);
    }
}

static void HandlerBypassSubstitute(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x12) == BattleEventItem_GetSubID(item)) {
        BattleEventVar_RewriteValue(0x40, 0);
    }
}

static const BattleEventHandlerEntry sHandlersLeechSeed[] = {
    { 0x62, HandlerLeechSeed },
};

static const BattleEventHandlerEntry *EventAddLeechSeed(u32 *priority) {
    *priority = 1;
    return sHandlersLeechSeed;
}

static void HandlerLeechSeed(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_RewriteValue(0x1e, MakeConditionParamPermanent(func_ov167_021ab840(flow, monId)).raw);
    }
}

static const BattleEventHandlerEntry sHandlersPayDay[] = {
    { 0x45, HandlerPayDay },
};

static const BattleEventHandlerEntry *EventAddPayDay(u32 *priority) {
    *priority = 1;
    return sHandlersPayDay;
}

static void HandlerPayDay(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *message;

    if (BattleEventVar_GetValue(3) == monId &&
        func_ov167_021abf28(flow, GetBattleMonStat(GetBattleMon(flow, monId), 0xf) * 5, monId)) {
        message = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&message->string, 1, 0x7a);
        BattleHandler_PopWork(flow, message);
    }
}

static const BattleEventHandlerEntry sHandlersRage[] = {
    { 0x24, HandlerRageStart },
    { 0x4b, HandlerRageBuild },
    { 0x1, HandlerRageEnd },
};

static const BattleEventHandlerEntry *EventAddRage(u32 *priority) {
    *priority = 3;
    return sHandlersRage;
}

static void HandlerRageStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        if (BattleEventVar_GetValue(0x12) == BattleEventItem_GetSubID(item)) {
            work[6] = TRUE;
        } else {
            RemoveForce(GetBattleMon(flow, monId), BattleEventItem_GetSubID(item));
        }
    }
}

static void HandlerRageBuild(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerStatChangeParam *param;

    if (BattleEventVar_GetValue(4) == monId && BattleEventVar_GetValue(0x46) == 0 &&
        !IsFainted(GetBattleMon(flow, monId))) {
        param = BattleHandler_PushWork(flow, 0xe, monId);
        param->stat = 1;
        param->change = 1;
        param->unk0e = 0;
        param->count = 1;
        param->monIds[0] = monId;
        param->flag = 1;
        BattleHandler_StrSetup(&param->string, 2, 0x214);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_PopWork(flow, param);
    }
}

static void HandlerRageEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId && work[6] != 0) {
        RemoveForce(GetBattleMon(flow, monId), BattleEventItem_GetSubID(item));
    }
}

static const BattleEventHandlerEntry sHandlersAquaRing[] = {
    { 0xa0, func_ov167_021c79c0 },
};

static const BattleEventHandlerEntry *EventAddAquaRing(u32 *priority) {
    *priority = 1;
    return sHandlersAquaRing;
}

static void func_ov167_021c79c0(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(3) == monId && !CheckCondition(GetBattleMon(flow, monId), 0x23)) {
        param = BattleHandler_PushWork(flow, 0xc, monId);
        param->condition = 0x23;
        param->value = MakeConditionPermanent();
        param->targetIndex = monId;
        BattleHandler_StrSetup(&param->string, 2, 0x259);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersThrash[] = {
    { 0x24, HandlerThrash },
    { 0x4, HandlerThrashEnd },
    { 0x77, HandlerThrashTurnCheck },
};

static const BattleEventHandlerEntry *EventAddThrash(u32 *priority) {
    *priority = 3;
    return sHandlersThrash;
}

static void HandlerThrash(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 turns;
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(2) == monId && !CheckCondition(GetBattleMon(flow, monId), 0x19) && work[6] == 0) {
        turns = BattleRandom(2) + 2;
        param = BattleHandler_PushWork(flow, 0xc, monId);
        param->condition = 0x19;
        param->value = AddTurnCondition(turns, BattleEventItem_GetSubID(item));
        param->showFail = 0;
        param->targetIndex = monId;
        BattleHandler_PopWork(flow, param);
        work[6] = TRUE;
        work[0] = turns;
    }
}

static void HandlerThrashEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    BOOL remove;
    BattleHandlerCureConditionParam *cure;
    BattleHandlerAddConditionParam *confuse;

    if (BattleEventVar_GetValue(2) == monId) {
        mon = GetBattleMon(flow, monId);
        if (work[6] != 0) {
            remove = FALSE;
            if (work[0] != 0) {
                work[0]--;
            }
            if (BattleEventVar_GetValue(0x51) == 0) {
                cure = BattleHandler_PushWork(flow, 0xb, monId);
                cure->condition = 0x19;
                cure->count = 1;
                cure->monIds[0] = monId;
                BattleHandler_PopWork(flow, cure);
                remove = TRUE;
            }
            if (work[0] == 0) {
                confuse = BattleHandler_PushWork(flow, 0xc, monId);
                confuse->condition = 6;
                func_ov167_021bd5d4(6, mon, &confuse->value);
                confuse->noMessage = TRUE;
                confuse->targetIndex = monId;
                BattleHandler_StrSetup(&confuse->string, 2, 0x168);
                BattleHandler_AddArg(&confuse->string, monId);
                BattleHandler_PopWork(flow, confuse);
                remove = TRUE;
            }
            if (remove) {
                RemoveForce(mon, BattleEventItem_GetSubID(item));
            }
        }
    }
}

static void HandlerThrashTurnCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    BattleHandlerCureConditionParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        mon = GetBattleMon(flow, monId);
        if (CheckCondition(mon, 2)) {
            param = BattleHandler_PushWork(flow, 0xb, monId);
            param->condition = 0x19;
            param->count = 1;
            param->monIds[0] = monId;
            BattleHandler_PopWork(flow, param);
            RemoveForce(mon, BattleEventItem_GetSubID(item));
        }
    }
}

// This part

static const BattleEventHandlerEntry sHandlersUproar[] = {
    { 0x45, HandlerUproar },       { 0x04, HandlerUproarEnd },
    { 0x77, HandlerUproarUnlock }, { 0x65, HandlerUproarPreventSleep },
    { 0x0e, func_ov167_021c7e14 },
};

static const BattleEventHandlerEntry *EventAddUproar(u32 *priority) {
    *priority = NELEMS(sHandlersUproar);
    return sHandlersUproar;
}

static void HandlerUproar(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerAddConditionParam *addParam;
    BattleHandlerMessageParam *msgParam;
    BattleHandlerCureConditionParam *cureParam;
    u8 side;
    u8 *mons;
    u32 i;

    if (BattleEventVar_GetValue(3) == monId) {
        if (!CheckCondition(GetBattleMon(flow, monId), 0x19) && work[6] == 0) {
            addParam = BattleHandler_PushWork(flow, 0xc, monId);
            addParam->condition = 0x19;
            addParam->value = AddTurnCondition(3, BattleEventItem_GetSubID(item));
            addParam->showFail = 0;
            addParam->targetIndex = monId;
            BattleHandler_PopWork(flow, addParam);

            msgParam = BattleHandler_PushWork(flow, 4, monId);
            BattleHandler_StrSetup(&msgParam->string, 2, 0x2bf);
            BattleHandler_AddArg(&msgParam->string, monId);
            BattleHandler_PopWork(flow, msgParam);

            side = func_ov167_021ab840(flow, monId);
            mons = func_ov167_021abc60(flow, 7);
            mons[0] = HandlerGetAlivePartyCount(flow, side | 0x800, &mons[1]);
            for (i = 0; i < mons[0]; i++) {
                if (CheckCondition(GetBattleMon(flow, mons[1 + i]), 2)) {
                    cureParam = BattleHandler_PushWork(flow, 0xb, monId);
                    cureParam->condition = 2;
                    cureParam->count = 1;
                    cureParam->monIds[0] = mons[1 + i];
                    BattleHandler_StrSetup(&cureParam->string, 2, 0x2c2);
                    BattleHandler_AddArg(&cureParam->string, mons[1 + i]);
                    BattleHandler_PopWork(flow, cureParam);
                }
            }
            work[6] = 1;
        }
    }
}

static void HandlerUproarEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerCureConditionParam *param;

    if (BattleEventVar_GetValue(2) == monId && BattleEventVar_GetValue(0x51) == 0) {
        param = BattleHandler_PushWork(flow, 0xb, monId);
        param->condition = 0x19;
        param->count = 1;
        param->monIds[0] = monId;
        BattleHandler_PopWork(flow, param);
    }
}

static void HandlerUproarUnlock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    BattleHandlerMessageParam *param;

    if (BattleEventVar_GetValue(2) == monId && work[6] != 0) {
        mon = GetBattleMon(flow, monId);
        if (!IsFainted(mon)) {
            if (!CheckCondition(mon, 0x19)) {
                param = BattleHandler_PushWork(flow, 4, monId);
                BattleHandler_StrSetup(&param->string, 2, 0x2ce);
                BattleHandler_AddArg(&param->string, monId);
                BattleHandler_PopWork(flow, param);
                RemoveForce(mon, BattleEventItem_GetSubID(item));
            } else {
                param = BattleHandler_PushWork(flow, 4, monId);
                BattleHandler_StrSetup(&param->string, 2, 0x2cb);
                BattleHandler_AddArg(&param->string, monId);
                BattleHandler_PopWork(flow, param);
            }
        }
    }
}

static void HandlerUproarPreventSleep(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *param;
    u8 targetId;

    if (work[6] != 0 && BattleEventVar_GetValue(0x1d) == 2 && BattleEventVar_RewriteValue(0x41, 1)) {
        param = BattleHandler_PushWork(flow, 4, monId);
        targetId = BattleEventVar_GetValue(4);
        BattleHandler_StrSetup(&param->string, 2, targetId == monId ? 0x2c8 : 0x2c5);
        BattleHandler_AddArg(&param->string, targetId);
        BattleHandler_PopWork(flow, param);
    }
}

static void func_ov167_021c7e14(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (work[6] != 0) {
        BattleEventVar_RewriteValue(0x41, 1);
    }
}

static const BattleEventHandlerEntry sHandlersRollout[] = {
    { 0x24, HandlerRolloutStart }, { 0x21, HandlerRolloutMiss },  { 0x26, HandlerRolloutMiss },
    { 0x04, HandlerRolloutEnd },   { 0x37, HandlerRolloutPower },
};

static const BattleEventHandlerEntry *EventAddRollout(u32 *priority) {
    *priority = NELEMS(sHandlersRollout);
    return sHandlersRollout;
}

static void HandlerRolloutStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        mon = GetBattleMon(flow, monId);
        if (!CheckCondition(mon, 0x19) && !CheckCondition(mon, 2) && work[6] == 0) {
            param = BattleHandler_PushWork(flow, 0xc, monId);
            param->condition = 0x19;
            param->value = AddTurnCondition(5, BattleEventItem_GetSubID(item));
            param->showFail = 0;
            param->targetIndex = monId;
            BattleHandler_PopWork(flow, param);
            work[0] = 0;
            work[1] = 5;
            work[6] = 1;
        }
    }
}

static void HandlerRolloutMiss(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        HandlerRolloutUnlock(item, flow, monId, work);
    }
}

static void HandlerRolloutEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId && BattleEventVar_GetValue(0x51) == 0) {
        HandlerRolloutUnlock(item, flow, monId, work);
    }
}

static void HandlerRolloutUnlock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerCureConditionParam *param;

    param = BattleHandler_PushWork(flow, 0xb, monId);
    param->monIds[0] = monId;
    param->count = 1;
    param->condition = 0x19;
    BattleHandler_PopWork(flow, param);
    work[6] = 0;
    RemoveHandlerForce(monId, BattleEventItem_GetSubID(item));
}

static void HandlerRolloutPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u32 i;
    u32 ratio;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        ratio = 1;
        for (i = 0; i < work[0]; i++) {
            ratio *= 2;
        }
        if (GetAdditionalConditionFlag(mon, 7)) {
            ratio *= 2;
        }
        MultiplyBasePower(ratio);
        work[0]++;
        if (work[0] >= work[1]) {
            RemoveHandlerForce(monId, BattleEventItem_GetSubID(item));
        }
    }
}

static const BattleEventHandlerEntry sHandlersTripleKick[] = {
    { 0x37, HandlerTripleKickPower },
    { 0x35, HandlerTripleKickHitCount },
};

static const BattleEventHandlerEntry *EventAddTripleKick(u32 *priority) {
    *priority = NELEMS(sHandlersTripleKick);
    return sHandlersTripleKick;
}

static void HandlerTripleKickPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        work[0]++;
        BattleEventVar_RewriteValue(0x30, work[0] * 10);
    }
}

static void HandlerTripleKickHitCount(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_RewriteValue(0x42, 1);
    }
}

static const BattleEventHandlerEntry sHandlersGyroBall[] = {
    { 0x37, HandlerGyroBall },
};

static const BattleEventHandlerEntry *EventAddGyroBall(u32 *priority) {
    *priority = NELEMS(sHandlersGyroBall);
    return sHandlersGyroBall;
}

static void HandlerGyroBall(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *attacker;
    BattleMon *target;
    u16 attackerSpeed;
    u32 power;

    if (BattleEventVar_GetValue(3) == monId) {
        attacker = GetBattleMon(flow, monId);
        target = GetBattleMon(flow, BattleEventVar_GetValue(4));
        attackerSpeed = func_ov167_021abd08(flow, attacker, FALSE);
        power = 25 * func_ov167_021abd08(flow, target, FALSE) / attackerSpeed + 1;
        if (power > 150) {
            power = 150;
        }
        BattleEventVar_RewriteValue(0x30, power);
    }
}

static const BattleEventHandlerEntry sHandlersRevenge[] = {
    { 0x37, HandlerRevenge },
};

static const BattleEventHandlerEntry *EventAddRevenge(u32 *priority) {
    *priority = NELEMS(sHandlersRevenge);
    return sHandlersRevenge;
}

static void HandlerRevenge(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u8 targetId;
    u8 i;
    BattleMonDamageRecord record;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        targetId = BattleEventVar_GetValue(4);
        i = 0;
        while (GetDamageReceived(mon, 0, i++, &record)) {
            if (record.attackerId == targetId && record.damage != 0) {
                MultiplyBasePower(2);
                return;
            }
        }
    }
}

// Flail's and Reversal's power for HP left, in 48ths of the maximum (table name from swan)
typedef struct FlailPowerEntry {
    u16 ratio;
    u16 power;
} FlailPowerEntry;

static const BattleEventHandlerEntry sHandlersFlail[] = {
    { 0x37, HandlerFlail },
};

static const BattleEventHandlerEntry *EventAddFlail(u32 *priority) {
    *priority = NELEMS(sHandlersFlail);
    return sHandlersFlail;
}

static void HandlerFlail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    static const FlailPowerEntry FLAIL_POWER_TABLE[] = {
        { 1, 200 }, { 4, 150 }, { 9, 100 }, { 16, 80 }, { 32, 40 }, { 48, 20 },
    };
    BattleMon *mon;
    s32 hp;
    s32 maxHp;
    u32 ratio;
    u32 i;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        hp = GetBattleMonStat(mon, 0xd);
        maxHp = GetBattleMonStat(mon, 0xe);
        ratio = hp * 48 / maxHp;
        if (ratio == 0) {
            ratio = 1;
        }
        for (i = 0; i < NELEMS(FLAIL_POWER_TABLE) - 1; i++) {
            if (ratio <= FLAIL_POWER_TABLE[i].ratio) {
                break;
            }
        }
        BattleEventVar_RewriteValue(0x30, FLAIL_POWER_TABLE[i].power);
    }
}

static const BattleEventHandlerEntry sHandlersFacade[] = {
    { 0x38, HandlerFacade },
};

static const BattleEventHandlerEntry *EventAddFacade(u32 *priority) {
    *priority = NELEMS(sHandlersFacade);
    return sHandlersFacade;
}

static void HandlerFacade(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 status;

    if (BattleEventVar_GetValue(3) == monId) {
        status = GetBattleMonStatus(GetBattleMon(flow, monId));
        if (status == 5 || status == 1 || status == 4) {
            BattleEventVar_MulValue(0x31, FX32_CONST(2));
        }
    }
}

static const BattleEventHandlerEntry sHandlersPayback[] = {
    { 0x37, HandlerPayback },
};

static const BattleEventHandlerEntry *EventAddPayback(u32 *priority) {
    *priority = NELEMS(sHandlersPayback);
    return sHandlersPayback;
}

static void HandlerPayback(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        if (GetTurnFlag(GetBattleMon(flow, BattleEventVar_GetValue(4)), 1)) {
            MultiplyBasePower(2);
        }
    }
}

static const BattleEventHandlerEntry sHandlersEruption[] = {
    { 0x37, HandlerEruption },
};

static const BattleEventHandlerEntry *EventAddEruption(u32 *priority) {
    *priority = NELEMS(sHandlersEruption);
    return sHandlersEruption;
}

static void HandlerEruption(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u32 hp;
    u32 maxHp;
    u32 power;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        hp = GetBattleMonStat(mon, 0xd);
        maxHp = GetBattleMonStat(mon, 0xe);
        power = PML_MoveGetBasePower(BattleEventItem_GetSubID(item));
        power = power * hp / maxHp;
        if (power == 0) {
            power = 1;
        }
        BattleEventVar_RewriteValue(0x30, power);
    }
}

static const BattleEventHandlerEntry sHandlersCrushGrip[] = {
    { 0x37, HandlerCrushGrip },
};

static const BattleEventHandlerEntry *EventAddCrushGrip(u32 *priority) {
    *priority = NELEMS(sHandlersCrushGrip);
    return sHandlersCrushGrip;
}

static void HandlerCrushGrip(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 power;

    if (BattleEventVar_GetValue(3) == monId) {
        power = GetRatioOverZero(120, GetHPRatio(GetBattleMon(flow, BattleEventVar_GetValue(4)))) / 100;
        if (power == 0) {
            power = 1;
        }
        BattleEventVar_RewriteValue(0x30, power);
    }
}

static const BattleEventHandlerEntry sHandlersBrine[] = {
    { 0x38, HandlerBrine },
};

static const BattleEventHandlerEntry *EventAddBrine(u32 *priority) {
    *priority = NELEMS(sHandlersBrine);
    return sHandlersBrine;
}

static void HandlerBrine(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        if (GetHPRatio(GetBattleMon(flow, BattleEventVar_GetValue(4))) <= FX32_CONST(50)) {
            BattleEventVar_MulValue(0x31, FX32_CONST(2));
        }
    }
}

static const BattleEventHandlerEntry sHandlersReturn[] = {
    { 0x37, HandlerReturn },
};

static const BattleEventHandlerEntry *EventAddReturn(u32 *priority) {
    *priority = NELEMS(sHandlersReturn);
    return sHandlersReturn;
}

static void HandlerReturn(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 power;

    if (BattleEventVar_GetValue(3) == monId) {
        power = PokeParty_GetParam(GetSrcData(GetBattleMon(flow, monId)), 9, NULL) * 10 / 25;
        if (power == 0) {
            power = 1;
        }
        BattleEventVar_RewriteValue(0x30, power);
    }
}

static const BattleEventHandlerEntry sHandlersFrustration[] = {
    { 0x37, HandlerFrustration },
};

static const BattleEventHandlerEntry *EventAddFrustration(u32 *priority) {
    *priority = NELEMS(sHandlersFrustration);
    return sHandlersFrustration;
}

static void HandlerFrustration(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 power;

    if (BattleEventVar_GetValue(3) == monId) {
        power = (255 - PokeParty_GetParam(GetSrcData(GetBattleMon(flow, monId)), 9, NULL)) * 10 / 25;
        if (power == 0) {
            power = 1;
        }
        BattleEventVar_RewriteValue(0x30, power);
    }
}

static const BattleEventHandlerEntry sHandlersWakeUpSlap[] = {
    { 0x37, HandlerWakeUpSlapPower },
    { 0x83, HandlerWakeUpSlapSleepCure },
};

static const BattleEventHandlerEntry *EventAddWakeUpSlap(u32 *priority) {
    *priority = NELEMS(sHandlersWakeUpSlap);
    return sHandlersWakeUpSlap;
}

static void HandlerWakeUpSlapPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *target;

    if (BattleEventVar_GetValue(3) == monId) {
        target = GetBattleMon(flow, BattleEventVar_GetValue(4));
        if (!IsSubstituteActive(target) && CheckCondition(target, 2)) {
            MultiplyBasePower(2);
        }
    }
}

static void HandlerWakeUpSlapSleepCure(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleHandlerCureConditionParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        if (CheckCondition(GetBattleMon(flow, targetId), 2)) {
            param = BattleHandler_PushWork(flow, 0xb, monId);
            param->monIds[0] = targetId;
            param->count = 1;
            param->condition = 2;
            BattleHandler_PopWork(flow, param);
        }
    }
}

static const BattleEventHandlerEntry sHandlersSmellingSalts[] = {
    { 0x37, HandlerSmellingSaltsPower },
    { 0x83, HandlerSmellingSaltsParaCure },
};

static const BattleEventHandlerEntry *EventAddSmellingSalts(u32 *priority) {
    *priority = NELEMS(sHandlersSmellingSalts);
    return sHandlersSmellingSalts;
}

static void HandlerSmellingSaltsPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *target;

    if (BattleEventVar_GetValue(3) == monId) {
        target = GetBattleMon(flow, BattleEventVar_GetValue(4));
        if (!IsSubstituteActive(target) && CheckCondition(target, 1)) {
            MultiplyBasePower(2);
        }
    }
}

static void HandlerSmellingSaltsParaCure(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleHandlerCureConditionParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        if (targetId != 0x1f && CheckCondition(GetBattleMon(flow, targetId), 1)) {
            param = BattleHandler_PushWork(flow, 0xb, monId);
            param->monIds[0] = targetId;
            param->count = 1;
            param->condition = 1;
            BattleHandler_PopWork(flow, param);
        }
    }
}

static const BattleEventHandlerEntry sHandlersPresent[] = {
    { 0x30, HandlerPresentRandomCheck },
    { 0x31, HandlerPresentHeal },
    { 0x37, HandlerPresentPower },
};

static const BattleEventHandlerEntry *EventAddPresent(u32 *priority) {
    *priority = NELEMS(sHandlersPresent);
    return sHandlersPresent;
}

static void HandlerPresentRandomCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        if (RollEffectChance(20)) {
            work[0] = BattleEventVar_RewriteValue(0x51, 1);
        }
        SetMoveEffectIndex(flow, work[0] != 0 ? 1 : 0);
    }
}

static void HandlerPresentHeal(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *target;
    BattleHandlerRecoverHPParam *healParam;
    BattleHandlerMessageParam *msgParam;

    if (BattleEventVar_GetValue(3) == monId && work[0] != 0) {
        targetId = BattleEventVar_GetValue(4);
        target = GetBattleMon(flow, targetId);
        if (!IsMonFullHP(target)) {
            healParam = BattleHandler_PushWork(flow, 5, monId);
            healParam->targetIndex = targetId;
            healParam->amount = DivideMaxHPZeroCheck(target, 4);
            BattleHandler_StrSetup(&healParam->string, 2, 0x183);
            BattleHandler_AddArg(&healParam->string, targetId);
            BattleHandler_PopWork(flow, healParam);
        } else {
            msgParam = BattleHandler_PushWork(flow, 4, monId);
            BattleHandler_StrSetup(&msgParam->string, 2, 0xd2);
            BattleHandler_AddArg(&msgParam->string, targetId);
            BattleHandler_PopWork(flow, msgParam);
        }
    }
}

static void HandlerPresentPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 rand;
    u32 power;

    if (BattleEventVar_GetValue(3) == monId) {
        power = 80;
        rand = BattleRandom(80);
        if (rand < 40) {
            power = 40;
        } else if (rand >= 70) {
            power = 120;
        }
        BattleEventVar_RewriteValue(0x30, power);
    }
}

// Trump Card's power for the PP left after it is used
static const BattleEventHandlerEntry sHandlersTrumpCard[] = {
    { 0x37, HandlerTrumpCard },
};

static const BattleEventHandlerEntry *EventAddTrumpCard(u32 *priority) {
    *priority = NELEMS(sHandlersTrumpCard);
    return sHandlersTrumpCard;
}

static void HandlerTrumpCard(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    static const u16 sTrumpCardPower[] = { 200, 80, 60, 50, 40 };
    BattleMon *mon;
    u8 slot;
    u8 pp;
    u32 power;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        slot = func_ov167_021baf78(mon, BattleEventItem_GetSubID(item));
        if (slot != 4) {
            pp = GetMovePP(mon, slot);
            if (pp >= NELEMS(sTrumpCardPower)) {
                pp = 4;
            }
            power = sTrumpCardPower[pp];
        } else {
            power = 40;
        }
        BattleEventVar_RewriteValue(0x30, power);
    }
}

// The stat stages whose raises Punishment counts
static const BattleEventHandlerEntry sHandlersPunishment[] = {
    { 0x37, HandlerPunishment },
};

static const BattleEventHandlerEntry *EventAddPunishment(u32 *priority) {
    *priority = NELEMS(sHandlersPunishment);
    return sHandlersPunishment;
}

static void HandlerPunishment(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    static const u32 sPunishmentStats[] = { 1, 2, 3, 4, 5, 6, 7 };
    BattleMon *target;
    u32 i;
    u32 count;
    s32 stage;
    u32 power;

    if (BattleEventVar_GetValue(3) == monId) {
        target = GetBattleMon(flow, BattleEventVar_GetValue(4));
        count = 0;
        for (i = 0; i < NELEMS(sPunishmentStats); i++) {
            stage = GetBattleMonStat(target, sPunishmentStats[i]);
            if (stage > 6) {
                count += stage - 6;
            }
        }
        power = 60 + count * 20;
        if (power > 200) {
            power = 200;
        }
        BattleEventVar_RewriteValue(0x30, power);
    }
}

static const BattleEventHandlerEntry sHandlersFuryCutter[] = {
    { 0x37, HandlerFuryCutter },
};

static const BattleEventHandlerEntry *EventAddFuryCutter(u32 *priority) {
    *priority = NELEMS(sHandlersFuryCutter);
    return sHandlersFuryCutter;
}

static void HandlerFuryCutter(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u32 power;
    u32 count;
    u32 i;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        if (GetPreviousMoveID(mon) == BattleEventItem_GetSubID(item)) {
            power = BattleEventVar_GetValue(0x30);
            count = GetConsecutiveMoveCount(mon);
            for (i = 0; i < count; i++) {
                power *= 2;
                if (power > 160) {
                    power = 160;
                    break;
                }
            }
            BattleEventVar_RewriteValue(0x30, power);
        }
    }
}

static const BattleEventHandlerEntry sHandlersAssurance[] = {
    { 0x37, HandlerAssurance },
};

static const BattleEventHandlerEntry *EventAddAssurance(u32 *priority) {
    *priority = NELEMS(sHandlersAssurance);
    return sHandlersAssurance;
}

static void HandlerAssurance(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        if (GetTurnFlag(GetBattleMon(flow, BattleEventVar_GetValue(4)), 2)) {
            MultiplyBasePower(2);
        }
    }
}

static const BattleEventHandlerEntry sHandlersLowKick[] = {
    { 0x37, HandlerLowKick },
};

static const BattleEventHandlerEntry *EventAddLowKick(u32 *priority) {
    *priority = NELEMS(sHandlersLowKick);
    return sHandlersLowKick;
}

static void HandlerLowKick(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 weight;
    u32 power;

    if (BattleEventVar_GetValue(3) == monId) {
        weight = func_ov167_021abee0(flow, GetMonID(GetBattleMon(flow, BattleEventVar_GetValue(4))));
        if (weight >= 2000) {
            power = 120;
        } else if (weight >= 1000) {
            power = 100;
        } else if (weight >= 500) {
            power = 80;
        } else if (weight >= 250) {
            power = 60;
        } else if (weight >= 100) {
            power = 40;
        } else {
            power = 20;
        }
        BattleEventVar_RewriteValue(0x30, power);
    }
}

static const BattleEventHandlerEntry sHandlersWeatherBall[] = {
    { 0x28, HandlerWeatherBallType },
    { 0x37, HandlerWeatherBallPower },
};

static const BattleEventHandlerEntry *EventAddWeatherBall(u32 *priority) {
    *priority = NELEMS(sHandlersWeatherBall);
    return sHandlersWeatherBall;
}

static void HandlerWeatherBallType(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 weather;
    u8 type;
    u8 effect;

    if (BattleEventVar_GetValue(2) == monId) {
        weather = GetWeather(flow);
        type = BattleEventVar_GetValue(0x16);
        effect = 0;
        switch (weather) {
        case 1:
            type = 9;
            effect = 1;
            break;
        case 2:
            type = 10;
            effect = 4;
            break;
        case 4:
            type = 5;
            effect = 3;
            break;
        case 3:
            type = 14;
            effect = 2;
            break;
        }
        BattleEventVar_RewriteValue(0x16, type);
        SetMoveEffectIndex(flow, effect);
    }
}

static void HandlerWeatherBallPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        if (GetWeather(flow) != 0) {
            BattleEventVar_RewriteValue(0x30, BattleEventVar_GetValue(0x30) * 2);
        }
    }
}

static const BattleEventHandlerEntry sHandlersTwister[] = {
    { 0x99, HandlerTwisterFlyCheck },
    { 0x37, HandlerTwisterPower },
};

static const BattleEventHandlerEntry *EventAddTwister(u32 *priority) {
    *priority = NELEMS(sHandlersTwister);
    return sHandlersTwister;
}

static void HandlerTwisterFlyCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x21) == 3) {
        BattleEventVar_RewriteValue(0x42, 0);
    }
}

static void HandlerTwisterPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        if (GetAdditionalConditionFlag(GetBattleMon(flow, BattleEventVar_GetValue(4)), 3)) {
            MultiplyBasePower(2);
        }
    }
}

static const BattleEventHandlerEntry sHandlersSkyUppercut[] = {
    { 0x99, HandlerSkyUppercut },
};

static const BattleEventHandlerEntry *EventAddSkyUppercut(u32 *priority) {
    *priority = NELEMS(sHandlersSkyUppercut);
    return sHandlersSkyUppercut;
}

static void HandlerSkyUppercut(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x21) == 3) {
        BattleEventVar_RewriteValue(0x42, 0);
    }
}

static const BattleEventHandlerEntry sHandlersThunder[] = {
    { 0x99, HandlerThunderFlyCheck },
    { 0x32, HandlerThunderRainCheck },
    { 0x34, HandlerThunderSunCheck },
};

static const BattleEventHandlerEntry *EventAddThunder(u32 *priority) {
    *priority = NELEMS(sHandlersThunder);
    return sHandlersThunder;
}

static void HandlerThunderFlyCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x21) == 3) {
        BattleEventVar_RewriteValue(0x42, 0);
    }
}

static void HandlerThunderRainCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && GetWeather(flow) == 2) {
        BattleEventVar_RewriteValue(0x51, 1);
    }
}

static void HandlerThunderSunCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && GetWeather(flow) == 1) {
        BattleEventVar_RewriteValue(0x2b, 50);
    }
}

static const BattleEventHandlerEntry sHandlersBlizzard[] = {
    { 0x32, HandlerBlizzard },
};

static const BattleEventHandlerEntry *EventAddBlizzard(u32 *priority) {
    *priority = NELEMS(sHandlersBlizzard);
    return sHandlersBlizzard;
}

static void HandlerBlizzard(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && GetWeather(flow) == 3) {
        BattleEventVar_RewriteValue(0x51, 1);
    }
}

static const BattleEventHandlerEntry sHandlersEarthquake[] = {
    { 0x99, HandlerEarthquakeDigCheck },
    { 0x47, HandlerEarthquakeDamage },
};

static const BattleEventHandlerEntry *EventAddEarthquake(u32 *priority) {
    *priority = NELEMS(sHandlersEarthquake);
    return sHandlersEarthquake;
}

static void HandlerEarthquakeDigCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x21) == 5) {
        BattleEventVar_RewriteValue(0x42, 0);
    }
}

static void HandlerEarthquakeDamage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        if (GetAdditionalConditionFlag(GetBattleMon(flow, BattleEventVar_GetValue(4)), 5)) {
            BattleEventVar_MulValue(0x35, FX32_CONST(2));
        }
    }
}

static const BattleEventHandlerEntry sHandlersJudgement[] = {
    { 0x28, HandlerJudgement },
};

static const BattleEventHandlerEntry *EventAddJudgement(u32 *priority) {
    *priority = NELEMS(sHandlersJudgement);
    return sHandlersJudgement;
}

static void HandlerJudgement(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u16 heldItem;
    u8 type;

    if (BattleEventVar_GetValue(2) == monId && func_ov167_021abd8c(flow, monId)) {
        heldItem = GetBattleMonHeldItem(GetBattleMon(flow, monId));
        type = 0;
        switch (heldItem) {
        case 298:
            type = 9;
            break;
        case 299:
            type = 10;
            break;
        case 300:
            type = 12;
            break;
        case 301:
            type = 11;
            break;
        case 302:
            type = 14;
            break;
        case 303:
            type = 1;
            break;
        case 304:
            type = 3;
            break;
        case 305:
            type = 4;
            break;
        case 306:
            type = 2;
            break;
        case 307:
            type = 13;
            break;
        case 308:
            type = 6;
            break;
        case 309:
            type = 5;
            break;
        case 310:
            type = 7;
            break;
        case 311:
            type = 15;
            break;
        case 312:
            type = 16;
            break;
        case 313:
            type = 8;
            break;
        }
        BattleEventVar_RewriteValue(0x16, type);
    }
}

static const BattleEventHandlerEntry sHandlersTechnoBlast[] = {
    { 0x28, HandlerTechnoBlast },
};

static const BattleEventHandlerEntry *EventAddTechnoBlast(u32 *priority) {
    *priority = NELEMS(sHandlersTechnoBlast);
    return sHandlersTechnoBlast;
}

static void HandlerTechnoBlast(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u16 heldItem;
    u8 type;
    u8 effect;

    if (BattleEventVar_GetValue(2) == monId && func_ov167_021abd8c(flow, monId)) {
        heldItem = GetBattleMonHeldItem(GetBattleMon(flow, monId));
        type = 0;
        effect = 0;
        switch (heldItem) {
        case 116:
            type = 10;
            effect = 1;
            break;
        case 117:
            type = 12;
            effect = 2;
            break;
        case 118:
            type = 9;
            effect = 3;
            break;
        case 119:
            type = 14;
            effect = 4;
            break;
        }
        BattleEventVar_RewriteValue(0x16, type);
        SetMoveEffectIndex(flow, effect);
    }
}

static const BattleEventHandlerEntry sHandlersHiddenPower[] = {
    { 0x28, HandlerHiddenPowerType },
    { 0x37, HandlerHiddenPowerPower },
};

static const BattleEventHandlerEntry *EventAddHiddenPower(u32 *priority) {
    *priority = NELEMS(sHandlersHiddenPower);
    return sHandlersHiddenPower;
}

static void HandlerHiddenPowerType(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        BattleEventVar_RewriteValue(0x16, getHiddenPowerType(GetSrcData(GetBattleMon(flow, monId))));
    }
}

static void HandlerHiddenPowerPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_RewriteValue(0x30, getHiddenPowerBasePwr(GetSrcData(GetBattleMon(flow, monId))));
    }
}

static const BattleEventHandlerEntry sHandlersNaturalGift[] = {
    { 0x1f, HandlerNaturalGiftCheckFail },
    { 0x28, HandlerNaturalGiftType },
    { 0x37, HandlerNaturalGiftPower },
    { 0x27, HandlerNaturalGiftEnd },
};

static const BattleEventHandlerEntry *EventAddNaturalGift(u32 *priority) {
    *priority = NELEMS(sHandlersNaturalGift);
    return sHandlersNaturalGift;
}

static void HandlerNaturalGiftCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u16 heldItem;

    if (BattleEventVar_GetValue(2) == monId) {
        if (!func_ov167_021abd8c(flow, monId)) {
            BattleEventVar_RewriteValue(0x22, 0x1a);
            return;
        }
        heldItem = GetBattleMonHeldItem(GetBattleMon(flow, monId));
        // Read once for nothing, then again for the check
        ItemGetParam(heldItem, 0xb);
        if (heldItem == 0 || ItemGetParam(heldItem, 0xb) == 0) {
            BattleEventVar_RewriteValue(0x22, 0x1a);
        }
    }
}

static void HandlerNaturalGiftType(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        BattleEventVar_RewriteValue(0x16, ItemGetParam(GetBattleMonHeldItem(GetBattleMon(flow, monId)), 0xc));
    }
}

static void HandlerNaturalGiftPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_RewriteValue(0x30, ItemGetParam(GetBattleMonHeldItem(GetBattleMon(flow, monId)), 0xb));
    }
}

static void HandlerNaturalGiftEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerConsumeItemParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        param = BattleHandler_PushWork(flow, 0x23, monId);
        param->skipDisplay = 1;
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersKnockOff[] = {
    { 0x83, HandlerKnockOff },
};

static const BattleEventHandlerEntry *EventAddKnockOff(u32 *priority) {
    *priority = NELEMS(sHandlersKnockOff);
    return sHandlersKnockOff;
}

static void HandlerKnockOff(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    u16 heldItem;
    BattleHandlerSetItemParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        if (!func_ov167_021cdf28(flow, monId, targetId)) {
            heldItem = GetBattleMonHeldItem(GetBattleMon(flow, targetId));
            if (heldItem != 0) {
                param = BattleHandler_PushWork(flow, 0x20, monId);
                param->targetIndex = targetId;
                param->item = 0;
                BattleHandler_StrSetup(&param->string, 2, 0x41a);
                BattleHandler_AddArg(&param->string, monId);
                BattleHandler_AddArg(&param->string, targetId);
                BattleHandler_AddArg(&param->string, heldItem);
                BattleHandler_PopWork(flow, param);
            }
        }
    }
}

static const BattleEventHandlerEntry sHandlersMagicCoat[] = {
    { 0x1f, HandlerMagicCoatCheckFail }, { 0xa0, HandlerMagicCoat },          { 0x2d, HandlerMagicCoatWait },
    { 0x09, HandlerMagicCoatReflect },   { 0x76, HandlerMagicCoatTurnCheck },
};

static const BattleEventHandlerEntry *EventAddMagicCoat(u32 *priority) {
    *priority = NELEMS(sHandlersMagicCoat);
    return sHandlersMagicCoat;
}

static void HandlerMagicCoatCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 attackerId;

    attackerId = BattleEventVar_GetValue(2);
    if (attackerId == monId) {
        if (IsMonLastInTurnOrder(flow, monId)) {
            BattleEventVar_RewriteValue(0x22, 0x1a);
        }
    } else if (!IsAllyMonID(monId, attackerId)) {
        CommonMagicCoatCheckMoveEffect(item, flow, monId, work);
    }
}

static void HandlerMagicCoat(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *param;

    if (BattleEventVar_GetValue(3) == monId && work[6] == 0) {
        param = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&param->string, 2, 0x2f9);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_PopWork(flow, param);
        work[6] = 1;
    }
}

static void HandlerMagicCoatWait(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonMagicCoatWait(item, flow, monId, work);
}

static void HandlerMagicCoatReflect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId && BattleEventVar_RewriteValue(0x51, 1)) {
        func_ov167_021ce044(item, flow, monId, work);
    }
}

static void HandlerMagicCoatTurnCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        BattleEventItem_Remove(item);
    }
}

static const BattleEventHandlerEntry sHandlersSnatch[] = {
    { 0xa0, HandlerSnatch },
    { 0x1a, HandlerSnatchCheckMoveEffect },
    { 0x08, HandlerSnatchStealMove },
    { 0x76, HandlerSnatchTurnCheck },
};

static const BattleEventHandlerEntry *EventAddSnatch(u32 *priority) {
    *priority = NELEMS(sHandlersSnatch);
    return sHandlersSnatch;
}

static void HandlerSnatch(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *param;

    if (BattleEventVar_GetValue(3) == monId && !IsMonLastInTurnOrder(flow, monId)) {
        param = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&param->string, 2, 0x2ef);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_PopWork(flow, param);
        work[6] = 1;
    }
}

static void HandlerSnatchCheckMoveEffect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 attackerId;

    attackerId = BattleEventVar_GetValue(3);
    if (attackerId != monId && getMoveFlag(BattleEventVar_GetValue(0x12), 5) &&
        !CheckCondition(GetBattleMon(flow, monId), 0x21) && BattleEventVar_RewriteValue(2, monId)) {
        BattleEventVar_RewriteValue(4, monId);
        work[0] = 1;
    }
}

static void HandlerSnatchStealMove(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *param;

    if (BattleEventVar_GetValue(2) == monId && work[0] != 0) {
        param = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&param->string, 2, 0x2f2);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, BattleEventVar_GetValue(3));
        BattleHandler_PopWork(flow, param);
        BattleEventItem_Remove(item);
    }
}

static void HandlerSnatchTurnCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        BattleEventItem_Remove(item);
    }
}

static const BattleEventHandlerEntry sHandlersThief[] = {
    { 0x81, HandlerThiefStart },
    { 0x83, HandlerThief },
};

static const BattleEventHandlerEntry *EventAddThief(u32 *priority) {
    *priority = NELEMS(sHandlersThief);
    return sHandlersThief;
}

static void HandlerThiefStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        work[0] = GetBattleMonHeldItem(GetBattleMon(flow, monId)) != 0 ? TRUE : FALSE;
    }
}

static void HandlerThief(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *target;
    BattleHandlerSwapItemParam *swapParam;
    BattleHandlerMoveEffectParam *effectParam;

    if (BattleEventVar_GetValue(3) == monId && GetBattleMonHeldItem(GetBattleMon(flow, monId)) == 0 && work[0] == 0) {
        targetId = BattleEventVar_GetValue(6);
        if (targetId != 0x1f) {
            target = GetBattleMon(flow, targetId);
            if (GetBattleMonHeldItem(target) != 0 && !func_ov167_021cdf28(flow, monId, targetId)) {
                swapParam = BattleHandler_PushWork(flow, 0x24, monId);
                swapParam->otherIndex = targetId;
                BattleHandler_StrSetup(&swapParam->firstString, 2, 0x421);
                BattleHandler_AddArg(&swapParam->firstString, monId);
                BattleHandler_AddArg(&swapParam->firstString, targetId);
                BattleHandler_AddArg(&swapParam->firstString, GetBattleMonHeldItem(target));
                BattleHandler_PopWork(flow, swapParam);

                effectParam = BattleHandler_PushWork(flow, 0x3a, monId);
                effectParam->header.checkPrevResult = 1;
                effectParam->index = 1;
                BattleHandler_PopWork(flow, effectParam);
            }
        }
    }
}

static const BattleEventHandlerEntry sHandlersTrick[] = {
    { 0xa0, HandlerTrick },
};

static const BattleEventHandlerEntry *EventAddTrick(u32 *priority) {
    *priority = NELEMS(sHandlersTrick);
    return sHandlersTrick;
}

static void HandlerTrick(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *user;
    BattleMon *target;
    u16 userItem;
    u16 targetItem;
    u16 userSpecies;
    u16 targetSpecies;
    BattleHandlerSwapItemParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        if (!func_ov167_021cdf08(flow, monId)) {
            user = GetBattleMon(flow, monId);
            target = GetBattleMon(flow, targetId);
            userItem = GetBattleMonHeldItem(user);
            targetItem = GetBattleMonHeldItem(target);
            if (userItem != 0 || targetItem != 0) {
                userSpecies = GetBattleMonSpecies(user);
                targetSpecies = GetBattleMonSpecies(target);
                if (!PML_ItemIsMail(userItem) && !PML_ItemIsMail(targetItem) &&
                    !GiratinaArceusGenesectItemCheck(userSpecies, targetItem) &&
                    !GiratinaArceusGenesectItemCheck(targetSpecies, userItem) &&
                    !GiratinaArceusGenesectItemCheck(userSpecies, userItem) &&
                    !GiratinaArceusGenesectItemCheck(targetSpecies, targetItem)) {
                    param = BattleHandler_PushWork(flow, 0x24, monId);
                    param->otherIndex = targetId;
                    BattleHandler_StrSetup(&param->firstString, 2, 0x2aa);
                    BattleHandler_AddArg(&param->firstString, monId);
                    if (targetItem != 0) {
                        BattleHandler_StrSetup(&param->thirdString, 2, 0x2ad);
                        BattleHandler_AddArg(&param->thirdString, monId);
                        BattleHandler_AddArg(&param->thirdString, targetItem);
                    }
                    if (userItem != 0) {
                        BattleHandler_StrSetup(&param->secondString, 2, 0x2ad);
                        BattleHandler_AddArg(&param->secondString, targetId);
                        BattleHandler_AddArg(&param->secondString, userItem);
                    }
                    BattleHandler_PopWork(flow, param);
                }
            }
        }
    }
}

// Each magnitude from 4 to 10: the roll out of 100 it is below, and its power (table name from swan)
typedef struct MagnitudeEntry {
    u8 chance;
    u8 power;
} MagnitudeEntry;

static const BattleEventHandlerEntry sHandlersMagnitude[] = {
    { 0x99, HandlerEarthquakeDigCheck },
    { 0x47, HandlerEarthquakeDamage },
    { 0x24, HandlerMagnitudeEffect },
    { 0x37, HandlerMagnitudePower },
};

static const BattleEventHandlerEntry *EventAddMagnitude(u32 *priority) {
    *priority = NELEMS(sHandlersMagnitude);
    return sHandlersMagnitude;
}

static void HandlerMagnitudeEffect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    static const MagnitudeEntry MAGNITUDE_POWER_TABLE[] = {
        { 5, 10 }, { 15, 30 }, { 35, 50 }, { 65, 70 }, { 85, 90 }, { 95, 110 }, { 100, 150 },
    };
    u8 rand;
    u8 i;
    BattleHandlerMessageParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        rand = BattleRandom(100);
        for (i = 0; i < NELEMS(MAGNITUDE_POWER_TABLE); i++) {
            if (rand < MAGNITUDE_POWER_TABLE[i].chance) {
                break;
            }
        }
        work[0] = MAGNITUDE_POWER_TABLE[i].power;
        param = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&param->string, 1, 0x68 + i);
        BattleHandler_PopWork(flow, param);
    }
}

static void HandlerMagnitudePower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        if (work[0] == 0) {
            work[0] = 10;
        }
        BattleEventVar_RewriteValue(0x30, work[0]);
    }
}

static const BattleEventHandlerEntry sHandlersSurf[] = {
    { 0x99, HandlerSurfDiveCheck },
    { 0x47, HandlerSurfPower },
};

static const BattleEventHandlerEntry *EventAddSurf(u32 *priority) {
    *priority = NELEMS(sHandlersSurf);
    return sHandlersSurf;
}

static void HandlerSurfDiveCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x21) == 4) {
        BattleEventVar_RewriteValue(0x42, 0);
    }
}

static void HandlerSurfPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        if (GetAdditionalConditionFlag(GetBattleMon(flow, BattleEventVar_GetValue(4)), 4)) {
            BattleEventVar_MulValue(0x35, FX32_CONST(2));
        }
    }
}

static const BattleEventHandlerEntry sHandlersStomp[] = {
    { 0x47, HandlerStomp },
};

static const BattleEventHandlerEntry *EventAddStomp(u32 *priority) {
    *priority = NELEMS(sHandlersStomp);
    return sHandlersStomp;
}

static void HandlerStomp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        if (GetAdditionalConditionFlag(GetBattleMon(flow, BattleEventVar_GetValue(4)), 8)) {
            BattleEventVar_MulValue(0x35, FX32_CONST(2));
        }
    }
}

static const BattleEventHandlerEntry sHandlersFalseSwipe[] = {
    { 0x74, HandlerFalseSwipe },
};

static const BattleEventHandlerEntry *EventAddFalseSwipe(u32 *priority) {
    *priority = NELEMS(sHandlersFalseSwipe);
    return sHandlersFalseSwipe;
}

static void HandlerFalseSwipe(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x51) != 0) {
        BattleEventVar_RewriteValue(0x3a, 2);
    }
}

static const BattleEventHandlerEntry sHandlersEndure[] = {
    { 0x03, HandlerProtectStart }, { 0x1f, HandlerProtectCheckFail }, { 0x21, HandlerProtectResetCounter },
    { 0xa0, HandlerEndure },       { 0x74, HandlerEndureCheck },      { 0x76, HandlerEndureTurnCheck },
};

static const BattleEventHandlerEntry *EventAddEndure(u32 *priority) {
    *priority = NELEMS(sHandlersEndure);
    return sHandlersEndure;
}

static void HandlerEndure(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        param = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&param->string, 2, 0x1ff);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_PopWork(flow, param);
        work[6] = 1;
        IncrementProtectCounter(flow, monId, FALSE);
    }
}

static void HandlerEndureCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(4) == monId && work[6] != 0 && BattleEventVar_GetValue(0x51) != 0) {
        BattleEventVar_RewriteValue(0x3a, 1);
    }
}

static void HandlerEndureTurnCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        BattleEventItem_Remove(item);
    }
}

// Protect, Detect, Endure, Wide Guard and Quick Guard
// Protecting again succeeds one time in n, n by how many protections in a row came before
static const BattleEventHandlerEntry sHandlersProtect[] = {
    { 0x03, HandlerProtectStart },
    { 0x1f, HandlerProtectCheckFail },
    { 0x21, HandlerProtectResetCounter },
    { 0xa0, HandlerProtect },
};

static const BattleEventHandlerEntry *EventAddProtect(u32 *priority) {
    *priority = NELEMS(sHandlersProtect);
    return sHandlersProtect;
}

static void HandlerProtectStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    static const u16 sProtectMoves[] = { 182, 197, 203, 469, 501 };
    u16 move;
    u32 i;
    BattleHandlerSetCounterParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        move = GetPreviousMoveID(GetBattleMon(flow, monId));
        for (i = 0; i < NELEMS(sProtectMoves); i++) {
            if (move == sProtectMoves[i]) {
                return;
            }
        }
        param = BattleHandler_PushWork(flow, 0x26, monId);
        param->monIndex = monId;
        param->counter = 3;
        param->value = 0;
        BattleHandler_PopWork(flow, param);
    }
}

static void HandlerProtectCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    static const u8 sProtectFailChance[] = { 1, 2, 4, 8, 16, 32, 64, 128, 0, 0 };
    u8 count;

    if (BattleEventVar_GetValue(2) == monId) {
        count = GetConditionCount(GetBattleMon(flow, monId), 3);
        if (count != 0) {
            if (count >= NELEMS(sProtectFailChance)) {
                count = NELEMS(sProtectFailChance) - 1;
            }
            if (BattleRandom(sProtectFailChance[count]) != 0) {
                BattleEventVar_RewriteValue(0x22, 0x1a);
                return;
            }
        }
        if (IsMonLastInTurnOrder(flow, monId)) {
            BattleEventVar_RewriteValue(0x22, 0x1a);
        }
    }
}

static void HandlerProtectResetCounter(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerSetCounterParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        param = BattleHandler_PushWork(flow, 0x26, monId);
        param->monIndex = monId;
        param->counter = 3;
        param->value = 0;
        BattleHandler_PopWork(flow, param);
        BattleEventItem_Remove(item);
    }
}

static void HandlerProtect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerFlagParam *flagParam;
    BattleHandlerMessageParam *msgParam;

    if (BattleEventVar_GetValue(3) == monId) {
        GetBattleMon(flow, monId);
        flagParam = BattleHandler_PushWork(flow, 0x15, monId);
        flagParam->monIndex = monId;
        flagParam->flag = 7;
        BattleHandler_PopWork(flow, flagParam);
        IncrementProtectCounter(flow, monId, FALSE);
        msgParam = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&msgParam->string, 2, 0x205);
        BattleHandler_AddArg(&msgParam->string, monId);
        BattleHandler_PopWork(flow, msgParam);
    }
}

static void IncrementProtectCounter(BtlServerFlow *flow, u8 monId, BOOL checkPrevResult) {
    BattleMon *mon;
    BattleHandlerSetCounterParam *param;

    mon = GetBattleMon(flow, monId);
    param = BattleHandler_PushWork(flow, 0x26, monId);
    param->value = GetConditionCount(mon, 3) + 1;
    param->monIndex = monId;
    param->counter = 3;
    param->header.checkPrevResult = checkPrevResult;
    BattleHandler_PopWork(flow, param);
}

static const BattleEventHandlerEntry sHandlersBide[] = {
    { 0x24, HandlerBide },
    { 0x19, HandlerBideTextSet },
    { 0x1f, HandlerBideCheckFail },
    { 0x29, HandlerBideTarget },
    { 0x46, HandlerBideCalcDamage },
    { 0x21, HandlerBideFail },
    { 0x4b, HandlerBideDamageRecieved },
};

static const BattleEventHandlerEntry *EventAddBide(u32 *priority) {
    *priority = NELEMS(sHandlersBide);
    return sHandlersBide;
}

static void HandlerBide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        switch (work[0]) {
        case 0:
            param = BattleHandler_PushWork(flow, 0xc, monId);
            param->condition = 0x19;
            param->value = MakeConditionParamPermanent(BattleEventItem_GetSubID(item));
            param->targetIndex = monId;
            BattleHandler_PopWork(flow, param);
            BattleEventVar_RewriteValue(0x3e, 2);
            work[6] = 1;
            work[0] = 1;
            work[1] = 0;
            break;
        case 1:
            BattleEventVar_RewriteValue(0x3e, 2);
            work[0] = 2;
            break;
        case 2:
        default:
            SetMoveEffectIndex(flow, 1);
            BideEnd(flow, monId, work);
            break;
        }
    }
}

static void HandlerBideTextSet(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerString *string;

    if (BattleEventVar_GetValue(2) == monId) {
        string = (BattleHandlerString *)BattleEventVar_GetValue(0x3f);
        switch (work[0]) {
        case 1:
            BattleHandler_StrSetup(string, 2, 0x2e9);
            BattleHandler_AddArg(string, monId);
            BattleEventVar_RewriteValue(0x51, 1);
            break;
        case 2:
            BattleHandler_StrSetup(string, 2, 0x2ec);
            BattleHandler_AddArg(string, monId);
            BattleEventVar_RewriteValue(0x51, 1);
            break;
        }
    }
}

static void HandlerBideCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId && work[0] >= 2) {
        if (work[1] == 0) {
            BattleEventVar_RewriteValue(0x22, 0x1a);
        } else if (work[2] == 0x1f) {
            BattleEventVar_RewriteValue(0x22, 0x1a);
        }
        work[0] = 3;
    }
}

static void HandlerBideTarget(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && work[0] >= 2) {
        work[2] = GetBideTargetID(item, flow, monId, work);
        BattleEventVar_RewriteValue(4, work[2]);
    }
}

static void HandlerBideDamageRecieved(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(4) == monId && BattleEventVar_GetValue(0x46) == 0 && work[6] != 0) {
        work[1] += BattleEventVar_GetValue(0x32);
    }
}

static void HandlerBideCalcDamage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    s32 damage;

    if (BattleEventVar_GetValue(3) == monId && work[0] >= 2) {
        GetBattleMon(flow, monId);
        damage = work[1];
        if (damage != 0) {
            damage *= 2;
        }
        BattleEventVar_RewriteValue(0x37, damage);
        work[0] = 3;
    }
}

static void HandlerBideFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        BideEnd(flow, monId, work);
    }
}

static u8 GetBideTargetID(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u32 i;
    BattleMonDamageRecord record;
    u8 mons[6];
    u8 targetId;
    u8 count;

    mon = GetBattleMon(flow, monId);
    for (i = 0; i < 3; i++) {
        if (func_ov167_021bc120(mon, i) != 0) {
            GetDamageReceived(mon, i, 0, &record);
            targetId = record.attackerId;
            if (func_ov167_021ab840(flow, targetId) == 6) {
                count = HandlerGetAlivePartyCount(flow, func_ov167_021abb50(flow, monId) | 0x100, mons);
                if (count != 0) {
                    targetId = mons[(u8)BattleRandom(count)];
                } else {
                    targetId = 0x1f;
                }
            }
            return targetId;
        }
    }
    return 0x1f;
}

static void BideEnd(BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerCureConditionParam *param;

    if (work[6] != 0) {
        param = BattleHandler_PushWork(flow, 0xb, monId);
        param->condition = 0x19;
        param->count = 1;
        param->monIds[0] = monId;
        param->useString = 1;
        BattleHandler_PopWork(flow, param);
        work[6] = 0;
    }
}

static const BattleEventHandlerEntry sHandlersRecycle[] = {
    { 0xa0, HandlerRecycle },
};

static const BattleEventHandlerEntry *EventAddRecycle(u32 *priority) {
    *priority = NELEMS(sHandlersRecycle);
    return sHandlersRecycle;
}

static void HandlerRecycle(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u16 consumedItem;
    BattleHandlerSetItemParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        if (GetBattleMonHeldItem(mon) == 0) {
            consumedItem = GetConsumedItem(mon);
            if (consumedItem != 0) {
                param = BattleHandler_PushWork(flow, 0x20, monId);
                param->item = consumedItem;
                param->targetIndex = monId;
                param->clearConsumed = 1;
                BattleHandler_StrSetup(&param->string, 2, 0x2dd);
                BattleHandler_AddArg(&param->string, monId);
                BattleHandler_AddArg(&param->string, consumedItem);
                BattleHandler_PopWork(flow, param);
            }
        }
    }
}

static const BattleEventHandlerEntry sHandlersPsychoShift[] = {
    { 0xa0, HandlerPsychoShift },
};

static const BattleEventHandlerEntry *EventAddPsychoShift(u32 *priority) {
    *priority = NELEMS(sHandlersPsychoShift);
    return sHandlersPsychoShift;
}

static void HandlerPsychoShift(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u32 status;
    u8 targetId;
    BattleConditionCont cont;
    BattleCondition value;
    BattleHandlerAddConditionParam *addParam;
    BattleHandlerCureConditionParam *cureParam;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        status = GetBattleMonStatus(mon);
        if (status != 0) {
            targetId = BattleEventVar_GetValue(6);
            if (GetBattleMonStatus(GetBattleMon(flow, targetId)) == 0) {
                cont = GetConditionContinuationParam(mon, status);
                addParam = BattleHandler_PushWork(flow, 0xc, monId);
                addParam->condition = status;
                if (status == 5 && Condition_IsBadlyPoisoned(cont)) {
                    value = func_ov167_021ce298();
                } else {
                    value = func_ov167_021bd52c(status);
                }
                addParam->value = value;
                addParam->targetIndex = targetId;
                addParam->showFail = 1;
                BattleHandler_PopWork(flow, addParam);

                cureParam = BattleHandler_PushWork(flow, 0xb, monId);
                cureParam->condition = status;
                cureParam->count = 1;
                cureParam->monIds[0] = monId;
                cureParam->checkPrevResult = 1;
                BattleHandler_PopWork(flow, cureParam);
                BattleEventVar_RewriteValue(0x51, 0);
            }
        }
    }
}

static const BattleEventHandlerEntry sHandlersPainSplit[] = {
    { 0xa0, func_ov167_021c98b8 },
};

static const BattleEventHandlerEntry *EventAddPainSplit(u32 *priority) {
    *priority = NELEMS(sHandlersPainSplit);
    return sHandlersPainSplit;
}

static void func_ov167_021c98b8(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *user;
    BattleMon *target;
    u32 userHp;
    u32 targetHp;
    u32 average;
    BattleHandlerChangeHPParam *hpParam;
    BattleHandlerMessageParam *msgParam;
    BattleHandlerCheckHeldItemParam *itemParam;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        user = GetBattleMon(flow, monId);
        target = GetBattleMon(flow, targetId);
        userHp = GetBattleMonStat(user, 0xd);
        targetHp = GetBattleMonStat(target, 0xd);
        average = (userHp + targetHp) / 2;

        hpParam = BattleHandler_PushWork(flow, 8, monId);
        hpParam->count = 2;
        hpParam->monIds[0] = monId;
        hpParam->hpChanges[0] = average - userHp;
        hpParam->monIds[1] = targetId;
        hpParam->hpChanges[1] = average - targetHp;
        hpParam->skipReaction = 1;
        BattleHandler_PopWork(flow, hpParam);

        msgParam = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&msgParam->string, 1, 0x71);
        BattleHandler_PopWork(flow, msgParam);

        itemParam = BattleHandler_PushWork(flow, 0x21, monId);
        itemParam->monIndex = monId;
        itemParam->reaction = 1;
        BattleHandler_PopWork(flow, itemParam);

        itemParam = BattleHandler_PushWork(flow, 0x21, monId);
        itemParam->monIndex = targetId;
        itemParam->reaction = 1;
        BattleHandler_PopWork(flow, itemParam);
    }
}

static const BattleEventHandlerEntry sHandlersBellyDrum[] = {
    { 0xa0, HandlerBellyDrum },
};

static const BattleEventHandlerEntry *EventBellyDrum(u32 *priority) {
    *priority = NELEMS(sHandlersBellyDrum);
    return sHandlersBellyDrum;
}

static void HandlerBellyDrum(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u32 hpCost;
    s32 stages;
    BattleHandlerChangeHPParam *hpParam;
    BattleHandlerStatChangeParam *statParam;
    BattleHandlerMessageParam *msgParam;
    BattleHandlerCheckHeldItemParam *itemParam;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        hpCost = DivideMaxHPZeroCheck(mon, 2);
        stages = func_ov167_021bb550(mon, 1);
        if (GetBattleMonStat(mon, 0xd) > hpCost && stages != 0) {
            hpParam = BattleHandler_PushWork(flow, 8, monId);
            hpParam->count = 1;
            hpParam->skipReaction = 1;
            hpParam->monIds[0] = monId;
            hpParam->hpChanges[0] = -hpCost;
            BattleHandler_PopWork(flow, hpParam);

            statParam = BattleHandler_PushWork(flow, 0xe, monId);
            statParam->stat = 1;
            statParam->change = stages;
            statParam->count = 1;
            statParam->monIds[0] = monId;
            statParam->flag = 1;
            statParam->unk0e = 1;
            BattleHandler_PopWork(flow, statParam);

            msgParam = BattleHandler_PushWork(flow, 4, monId);
            BattleHandler_StrSetup(&msgParam->string, 2, 0x265);
            BattleHandler_AddArg(&msgParam->string, monId);
            BattleHandler_PopWork(flow, msgParam);

            itemParam = BattleHandler_PushWork(flow, 0x21, monId);
            itemParam->monIndex = monId;
            itemParam->reaction = 1;
            BattleHandler_PopWork(flow, itemParam);
        }
    }
}

static const BattleEventHandlerEntry sHandlersFeint[] = {
    { 0x2e, HandlerFeintBreakProtect },
    { 0x45, HandlerFeintTextSet },
    { 0x23, HandlerFeintResetProtectCounter },
};

static const BattleEventHandlerEntry *EventAddFeint(u32 *priority) {
    *priority = NELEMS(sHandlersFeint);
    return sHandlersFeint;
}

static void HandlerFeintBreakProtect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_RewriteValue(0x51, 1);
    }
}

static void HandlerFeintTextSet(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleHandlerFlagParam *flagParam;
    BattleHandlerMessageParam *msgParam;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(4);
        if (GetTurnFlag(GetBattleMon(flow, targetId), 7) || work[0] != 0) {
            flagParam = BattleHandler_PushWork(flow, 0x16, monId);
            flagParam->monIndex = targetId;
            flagParam->flag = 7;
            BattleHandler_PopWork(flow, flagParam);

            msgParam = BattleHandler_PushWork(flow, 4, monId);
            BattleHandler_StrSetup(&msgParam->string, 2, 0x20e);
            BattleHandler_AddArg(&msgParam->string, targetId);
            BattleHandler_PopWork(flow, msgParam);
        }
    }
}

static void HandlerFeintResetProtectCounter(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 side;
    BattleHandlerRemoveSideEffectParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        side = func_ov167_0219d338(GetSideFromMonID(monId));
        if (func_ov167_021abe04(flow, side, 9) || func_ov167_021abe04(flow, side, 10)) {
            param = BattleHandler_PushWork(flow, 0x1a, monId);
            param->side = side;
            BattleHandler_InitFlags(param->effects, 3);
            BattleHandler_SetFlag(param->effects, 9);
            BattleHandler_SetFlag(param->effects, 10);
            BattleHandler_PopWork(flow, param);
            work[0] = 1;
        }
    }
}

// The stats Acupressure picks one of to raise
static const BattleEventHandlerEntry sHandlersAcupressure[] = {
    { 0xa0, HandlerAcupressure },
};

static const BattleEventHandlerEntry *EventAddAcupressure(u32 *priority) {
    *priority = NELEMS(sHandlersAcupressure);
    return sHandlersAcupressure;
}

static void HandlerAcupressure(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    static const u32 sAcupressureStats[] = { 1, 2, 5, 3, 4, 6, 7 };
    u8 targetId;
    BattleMon *target;
    u8 count;
    u8 i;
    u8 rand;
    BattleHandlerStatChangeParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        target = GetBattleMon(flow, targetId);
        for (i = 0, count = 0; i < NELEMS(sAcupressureStats); i++) {
            if (IsStatChangeValid(target, sAcupressureStats[i], 2)) {
                count++;
            }
        }
        if (count != 0) {
            rand = BattleRandom(count);
            for (i = 0; i < NELEMS(sAcupressureStats); i++) {
                if (IsStatChangeValid(target, sAcupressureStats[i], 2)) {
                    if (rand == 0) {
                        param = BattleHandler_PushWork(flow, 0xe, monId);
                        param->monIds[0] = targetId;
                        param->count = 1;
                        param->stat = sAcupressureStats[i];
                        param->change = 2;
                        param->unk0e = 0;
                        BattleHandler_PopWork(flow, param);
                        return;
                    }
                    rand--;
                }
            }
        }
    }
}

// Other parts of this file

// This part

static const BattleEventHandlerEntry sHandlersRest[] = {
    { 0x1f, HandlerRestCheckFail },
    { 0xa0, HandlerRest },
};

static const BattleEventHandlerEntry *EventAddRest(u32 *priority) {
    *priority = 2;
    return sHandlersRest;
}

static void HandlerRestCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u16 ability;

    if (BattleEventVar_GetValue(2) == monId && BattleEventVar_GetValue(0x22) == 0) {
        mon = GetBattleMon(flow, monId);
        if (CheckCondition(mon, 2)) {
            BattleEventVar_RewriteValue(0x22, 0x1a);
            return;
        }
        if (CheckCondition(mon, 0xf)) {
            BattleEventVar_RewriteValue(0x22, 0xd);
            return;
        }
        if (IsMonFullHP(mon)) {
            BattleEventVar_RewriteValue(0x22, 0xe);
            return;
        }
        ability = GetBattleMonStat(mon, 0x11);
        if (ability == 0xf || ability == 0x48) {
            BattleEventVar_RewriteValue(0x22, 0xf);
        }
    }
}

static void HandlerRest(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    BattleHandlerAddConditionParam *condition;
    BattleHandlerRecoverHPParam *recover;
    BattleHandlerCheckHeldItemParam *check;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        if (!IsMonFullHP(mon)) {
            condition = BattleHandler_PushWork(flow, 0xc, monId);
            condition->targetIndex = monId;
            condition->condition = 2;
            condition->value = func_ov167_021bd58c(3);
            condition->showFail = 1;
            condition->overwrite = 1;
            condition->skipItemReaction = 1;
            BattleHandler_StrSetup(&condition->string, 2, 0x27e);
            BattleHandler_AddArg(&condition->string, monId);
            BattleHandler_PopWork(flow, condition);

            recover = BattleHandler_PushWork(flow, 5, monId);
            recover->targetIndex = monId;
            recover->amount = GetBattleMonStat(mon, 0xe) - GetBattleMonStat(mon, 0xd);
            recover->checkPrevResult = 1;
            BattleHandler_PopWork(flow, recover);

            check = BattleHandler_PushWork(flow, 0x21, monId);
            check->monIndex = monId;
            check->reaction = 3;
            check->header.checkPrevResult = 1;
            BattleHandler_PopWork(flow, check);

            BattleEventVar_RewriteValue(0x51, 0);
        }
    }
}

static const BattleEventHandlerEntry sHandlersAttract[] = {
    { 0x2c, HandlerAttractCheckFail },
};

static const BattleEventHandlerEntry *EventAddAttract(u32 *priority) {
    *priority = 1;
    return sHandlersAttract;
}

static void HandlerAttractCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *attacker;
    BattleMon *defender;
    u8 attackerSex;
    u8 defenderSex;

    if (BattleEventVar_GetValue(3) == monId) {
        attacker = GetBattleMon(flow, monId);
        defender = GetBattleMon(flow, BattleEventVar_GetValue(4));
        attackerSex = GetBattleMonStat(attacker, 0x12);
        defenderSex = GetBattleMonStat(defender, 0x12);
        if (attackerSex == 2 || defenderSex == 2 || attackerSex == defenderSex) {
            BattleEventVar_RewriteValue(0x40, 1);
        }
    }
}

static const BattleEventHandlerEntry sHandlersConversion2[] = {
    { 0xa0, HandlerConversion2 },
};

static const BattleEventHandlerEntry *EventAddConversion2(u32 *priority) {
    *priority = 1;
    return sHandlersConversion2;
}

static void HandlerConversion2(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 type;
    u8 *types;
    u32 count;
    BattleMon *mon;
    u32 i;
    int length;
    BattleHandlerChangeTypeParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        type = func_ov167_021bbfb0(GetBattleMon(flow, BattleEventVar_GetValue(6)));
        if (type != 0x11) {
            types = func_ov167_021abc60(flow, 0x11);
            count = GetTypeWeaknesses(type, types);
            if (count != 0) {
                mon = GetBattleMon(flow, monId);
                i = 0;
                while (i < count) {
                    if (DoesMonHaveType(mon, types[i])) {
                        length = count - 1 - i;
                        if (length > 0) {
                            sys_memcpy(&types[i + 1], &types[i], length);
                        }
                        count--;
                    } else {
                        i++;
                    }
                }
                if (count != 0) {
                    type = types[BattleRandom(count)];
                    param = BattleHandler_PushWork(flow, 0x14, monId);
                    param->type = func_ov167_021ce530(type);
                    param->monIndex = monId;
                    BattleHandler_PopWork(flow, param);
                }
            }
        }
    }
}

static const BattleEventHandlerEntry sHandlersEncore[] = {
    { 0xa0, HandlerEncore },
};

static const BattleEventHandlerEntry *EventAddEncore(u32 *priority) {
    *priority = 1;
    return sHandlersEncore;
}

static void HandlerEncore(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 turns = 3;
    u8 targetId;
    BattleMon *target;
    u16 move;
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        target = GetBattleMon(flow, targetId);
        move = GetPreviousMoveUsed(target);
        if (!CheckCondition(target, 0x17) && !func_ov169_0689ca34(move) && func_ov167_021bada0(target, move)) {
            if (GetTurnFlag(target, 1)) {
                turns++;
            }
            param = BattleHandler_PushWork(flow, 0xc, monId);
            param->condition = 0x17;
            param->value = AddTurnCondition(turns, move);
            param->targetIndex = targetId;
            param->showFail = 0;
            BattleHandler_StrSetup(&param->string, 2, 0x22f);
            BattleHandler_AddArg(&param->string, targetId);
            BattleHandler_PopWork(flow, param);
        }
    }
}

static const BattleEventHandlerEntry sHandlersTaunt[] = {
    { 0xa0, HandlerTaunt },
};

static const BattleEventHandlerEntry *EventAddTaunt(u32 *priority) {
    *priority = 1;
    return sHandlersTaunt;
}

static void HandlerTaunt(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 turns = 3;
    u8 targetId;
    BattleMon *target;
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        target = GetBattleMon(flow, targetId);
        if (!CheckCondition(target, 0xb)) {
            if (GetTurnFlag(target, 1)) {
                turns++;
            }
            param = BattleHandler_PushWork(flow, 0xc, monId);
            param->condition = 0xb;
            param->value = func_ov167_021bd58c(turns);
            param->targetIndex = targetId;
            BattleHandler_PopWork(flow, param);
        }
    }
}

static const BattleEventHandlerEntry sHandlersTorment[] = {
    { 0xa0, HandlerTorment },
    { 0x5, HandlerBypassSubstitute },
};

static const BattleEventHandlerEntry *EventAddTorment(u32 *priority) {
    *priority = 2;
    return sHandlersTorment;
}

static void HandlerTorment(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        if (!CheckCondition(GetBattleMon(flow, targetId), 0xc)) {
            param = BattleHandler_PushWork(flow, 0xc, monId);
            param->condition = 0xc;
            param->value = MakeConditionPermanent();
            param->targetIndex = targetId;
            BattleHandler_PopWork(flow, param);
        }
    }
}

static const BattleEventHandlerEntry sHandlersDisable[] = {
    { 0xa0, HandlerDisable },
};

static const BattleEventHandlerEntry *EventAddDisable(u32 *priority) {
    *priority = 1;
    return sHandlersDisable;
}

static void HandlerDisable(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *target;
    u16 move;
    u8 turns;
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        target = GetBattleMon(flow, targetId);
        if (!CheckCondition(target, 0xd)) {
            move = GetPreviousMoveUsed(target);
            if (move != 0 && move != 0xa5 && MoveIsUsable(target, move)) {
                turns = 4;
                if (GetTurnFlag(target, 1)) {
                    turns++;
                }
                param = BattleHandler_PushWork(flow, 0xc, monId);
                param->condition = 0xd;
                param->value = AddTurnCondition(turns, move);
                param->targetIndex = targetId;
                BattleHandler_PopWork(flow, param);
            }
        }
    }
}

static const BattleEventHandlerEntry sHandlersImprison[] = {
    { 0xa0, HandlerImprison },
};

static const BattleEventHandlerEntry *EventAddImprison(u32 *priority) {
    *priority = 1;
    return sHandlersImprison;
}

static void HandlerImprison(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerAddFieldEffectParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        param = BattleHandler_PushWork(flow, 0x1b, monId);
        param->effect = 3;
        param->value = func_ov167_021ce1dc(monId);
        param->duration = 1;
        param->string.enabled = 2;
        param->string.message = 0x24a;
        param->string.count = 1;
        param->string.args[0] = monId;
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersAromatherapy[] = {
    { 0x1c, HandlerHelpingHandSkip },
    { 0x99, HandlerHelpingHandCheckInvuln },
    { 0xa1, HandlerAromatherapy },
};

static const BattleEventHandlerEntry *EventAddAromatherapy(u32 *priority) {
    *priority = 3;
    return sHandlersAromatherapy;
}

static void HandlerAromatherapy(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonCureAllyMonStatus(item, flow, monId, work, 0x70);
}

static const BattleEventHandlerEntry sHandlersHealBell[] = {
    { 0x1c, HandlerHelpingHandSkip },
    { 0x99, HandlerHelpingHandCheckInvuln },
    { 0xa1, HandlerHealBell },
};

static const BattleEventHandlerEntry *EventAddHealBell(u32 *priority) {
    *priority = 3;
    return sHandlersHealBell;
}

static void HandlerHealBell(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    CommonCureAllyMonStatus(item, flow, monId, work, 0x6f);
}

static void CommonCureAllyMonStatus(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work, u16 message) {
    BattleParty *party;
    BattleParty *allyParty;
    u8 *monIds;
    BattleHandlerMessageParam *msg;
    u8 count;
    BattleHandlerCureConditionParam *param;
    u8 i;

    if (BattleEventVar_GetValue(3) == monId) {
        party = func_ov167_021abb0c(flow, monId);
        allyParty = func_ov167_021abb20(flow, monId);
        monIds = func_ov167_021abc60(flow, 0xc);
        msg = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&msg->string, 1, message);
        BattleHandler_PopWork(flow, msg);
        count = CheckMonStatus(party, 0, 0xc, monIds);
        if (allyParty != NULL) {
            count += CheckMonStatus(allyParty, count, 0xc, monIds);
        }
        if (count != 0) {
            if (count > 0xc) {
                count = 0xc;
            }
            param = BattleHandler_PushWork(flow, 0xb, monId);
            param->count = count;
            for (i = 0; i < count; i++) {
                param->monIds[i] = monIds[i];
            }
            param->condition = 0x24;
            BattleHandler_PopWork(flow, param);
        }
    }
}

// Adds the IDs of the party's mons with a status condition to monIds from pos up to max, and returns how many it added
static u8 CheckMonStatus(BattleParty *party, u8 pos, u8 max, u8 *monIds) {
    u8 numMons;
    u8 count;
    u8 i;
    BattleMon *mon;

    numMons = GetNumMonsInParty(party);
    count = 0;
    for (i = 0; i < numMons; i++) {
        if (pos >= max) {
            break;
        }
        mon = GetBattleMonFromParty(party, i);
        if (GetBattleMonStatus(mon) != 0) {
            monIds[pos] = GetMonID(mon);
            pos++;
            count++;
        }
    }
    return count;
}

static const BattleEventHandlerEntry sHandlersMemento[] = {
    { 0xa0, HandlerMemento },
};

static const BattleEventHandlerEntry *EventAddMemento(u32 *priority) {
    *priority = 1;
    return sHandlersMemento;
}

static void HandlerMemento(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    u32 value;
    BattleHandlerStatChangeParam *param;
    BattleHandlerFaintParam *faint;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        value = func_ov167_021abf0c(flow);

        param = BattleHandler_PushWork(flow, 0xe, monId);
        param->count = 1;
        param->monIds[0] = targetId;
        param->stat = 1;
        param->change = -2;
        param->value = value;
        param->unk0e = 1;
        BattleHandler_PopWork(flow, param);

        param = BattleHandler_PushWork(flow, 0xe, monId);
        param->count = 1;
        param->monIds[0] = targetId;
        param->stat = 3;
        param->change = -2;
        param->value = value;
        param->unk0e = 1;
        BattleHandler_PopWork(flow, param);

        faint = BattleHandler_PushWork(flow, 0x13, monId);
        faint->monIndex = monId;
        BattleHandler_PopWork(flow, faint);
    }
}

static const BattleEventHandlerEntry sHandlersSpite[] = {
    { 0xa0, HandlerSpite },
};

static const BattleEventHandlerEntry *EventAddSpite(u32 *priority) {
    *priority = 1;
    return sHandlersSpite;
}

static void HandlerSpite(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *target;
    u16 move;
    u8 slot;
    u8 pp;
    BattleHandlerPPParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        target = GetBattleMon(flow, targetId);
        move = GetPreviousMoveUsed(target);
        slot = func_ov167_021baf78(target, move);
        if (slot != 4) {
            pp = GetMovePP(target, slot);
            if (pp > 4) {
                pp = 4;
            }
            if (pp != 0) {
                param = BattleHandler_PushWork(flow, 0xa, monId);
                param->monIndex = targetId;
                param->amount = pp;
                param->moveIndex = slot;
                BattleHandler_StrSetup(&param->string, 2, 0x281);
                BattleHandler_AddArg(&param->string, targetId);
                BattleHandler_AddArg(&param->string, move);
                BattleHandler_AddArg(&param->string, pp);
                BattleHandler_PopWork(flow, param);
            }
        }
    }
}

static const BattleEventHandlerEntry sHandlersPsychUp[] = {
    { 0xa0, HandlerPsychUp },
};

static const BattleEventHandlerEntry *EventAddPsychUp(u32 *priority) {
    *priority = 1;
    return sHandlersPsychUp;
}

static void HandlerPsychUp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *target;
    BattleHandlerSetStatStageParam *param;
    BattleHandlerMessageParam *msg;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        target = GetBattleMon(flow, targetId);
        param = BattleHandler_PushWork(flow, 0xf, monId);
        param->monIndex = monId;
        param->attack = GetBattleMonStat(target, 1);
        param->defense = GetBattleMonStat(target, 2);
        param->spAttack = GetBattleMonStat(target, 3);
        param->spDefense = GetBattleMonStat(target, 4);
        param->speed = GetBattleMonStat(target, 5);
        param->accuracy = GetBattleMonStat(target, 6);
        param->evasion = GetBattleMonStat(target, 7);
        BattleHandler_PopWork(flow, param);

        msg = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&msg->string, 2, 0x417);
        BattleHandler_AddArg(&msg->string, monId);
        BattleHandler_AddArg(&msg->string, targetId);
        BattleHandler_PopWork(flow, msg);
    }
}

static const BattleEventHandlerEntry sHandlersHeartSwap[] = {
    { 0xa0, HandlerHeartSwap },
};

static const BattleEventHandlerEntry *EventAddHeartSwap(u32 *priority) {
    *priority = 1;
    return sHandlersHeartSwap;
}

static void HandlerHeartSwap(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *target;
    BattleMon *mon;
    u32 *stages;
    int i;
    BattleHandlerSetStatStageParam *param;
    BattleHandlerMessageParam *msg;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        target = GetBattleMon(flow, targetId);
        mon = GetBattleMon(flow, monId);
        stages = (u32 *)func_ov167_021abc60(flow, 0x1c);
        for (i = 1; i <= 7; i++) {
            stages[i - 1] = GetBattleMonStat(mon, i);
        }

        param = BattleHandler_PushWork(flow, 0xf, monId);
        param->monIndex = monId;
        param->attack = GetBattleMonStat(target, 1);
        param->defense = GetBattleMonStat(target, 2);
        param->spAttack = GetBattleMonStat(target, 3);
        param->spDefense = GetBattleMonStat(target, 4);
        param->speed = GetBattleMonStat(target, 5);
        param->accuracy = GetBattleMonStat(target, 6);
        param->evasion = GetBattleMonStat(target, 7);
        BattleHandler_PopWork(flow, param);

        param = BattleHandler_PushWork(flow, 0xf, monId);
        param->monIndex = targetId;
        param->attack = stages[0];
        param->defense = stages[1];
        param->spAttack = stages[2];
        param->spDefense = stages[3];
        param->speed = stages[4];
        param->accuracy = stages[5];
        param->evasion = stages[6];
        BattleHandler_PopWork(flow, param);

        msg = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&msg->string, 2, 0x2a1);
        BattleHandler_AddArg(&msg->string, monId);
        BattleHandler_PopWork(flow, msg);
    }
}

static const BattleEventHandlerEntry sHandlersPowerSwap[] = {
    { 0xa0, HandlerPowerSwap },
};

static const BattleEventHandlerEntry *EventAddPowerSwap(u32 *priority) {
    *priority = 1;
    return sHandlersPowerSwap;
}

static void HandlerPowerSwap(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *target;
    BattleMon *mon;
    u32 attack;
    u32 spAttack;
    BattleHandlerSetStatStageParam *param;
    BattleHandlerMessageParam *msg;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        target = GetBattleMon(flow, targetId);
        mon = GetBattleMon(flow, monId);
        attack = GetBattleMonStat(mon, 1);
        spAttack = GetBattleMonStat(mon, 3);

        param = BattleHandler_PushWork(flow, 0xf, monId);
        param->monIndex = monId;
        param->attack = GetBattleMonStat(target, 1);
        param->spAttack = GetBattleMonStat(target, 3);
        param->defense = GetBattleMonStat(mon, 2);
        param->spDefense = GetBattleMonStat(mon, 4);
        param->speed = GetBattleMonStat(mon, 5);
        param->accuracy = GetBattleMonStat(mon, 6);
        param->evasion = GetBattleMonStat(mon, 7);
        BattleHandler_PopWork(flow, param);

        param = BattleHandler_PushWork(flow, 0xf, monId);
        param->monIndex = targetId;
        param->attack = attack;
        param->spAttack = spAttack;
        param->defense = GetBattleMonStat(target, 2);
        param->spDefense = GetBattleMonStat(target, 4);
        param->speed = GetBattleMonStat(target, 5);
        param->accuracy = GetBattleMonStat(target, 6);
        param->evasion = GetBattleMonStat(target, 7);
        BattleHandler_PopWork(flow, param);

        msg = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&msg->string, 2, 0x2a4);
        BattleHandler_AddArg(&msg->string, monId);
        BattleHandler_PopWork(flow, msg);
    }
}

static const BattleEventHandlerEntry sHandlersGuardSwap[] = {
    { 0xa0, HandlerGuardSwap },
};

static const BattleEventHandlerEntry *EventAddGuardSwap(u32 *priority) {
    *priority = 1;
    return sHandlersGuardSwap;
}

static void HandlerGuardSwap(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *target;
    BattleMon *mon;
    u32 defense;
    u32 spDefense;
    BattleHandlerSetStatStageParam *param;
    BattleHandlerMessageParam *msg;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        target = GetBattleMon(flow, targetId);
        mon = GetBattleMon(flow, monId);
        defense = GetBattleMonStat(mon, 2);
        spDefense = GetBattleMonStat(mon, 4);

        param = BattleHandler_PushWork(flow, 0xf, monId);
        param->monIndex = monId;
        param->attack = GetBattleMonStat(mon, 1);
        param->spAttack = GetBattleMonStat(mon, 3);
        param->defense = GetBattleMonStat(target, 2);
        param->spDefense = GetBattleMonStat(target, 4);
        param->speed = GetBattleMonStat(mon, 5);
        param->accuracy = GetBattleMonStat(mon, 6);
        param->evasion = GetBattleMonStat(mon, 7);
        BattleHandler_PopWork(flow, param);

        param = BattleHandler_PushWork(flow, 0xf, monId);
        param->monIndex = targetId;
        param->attack = GetBattleMonStat(target, 1);
        param->spAttack = GetBattleMonStat(target, 3);
        param->defense = defense;
        param->spDefense = spDefense;
        param->speed = GetBattleMonStat(target, 5);
        param->accuracy = GetBattleMonStat(target, 6);
        param->evasion = GetBattleMonStat(target, 7);
        BattleHandler_PopWork(flow, param);

        msg = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&msg->string, 2, 0x2a7);
        BattleHandler_AddArg(&msg->string, monId);
        BattleHandler_PopWork(flow, msg);
    }
}

static const BattleEventHandlerEntry sHandlersPowerTrick[] = {
    { 0xa0, HandlerPowerTrick },
};

static const BattleEventHandlerEntry *EventAddPowerTrick(u32 *priority) {
    *priority = 1;
    return sHandlersPowerTrick;
}

static void HandlerPowerTrick(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    BattleHandlerSetStatusParam *param;
    BattleHandlerFlagParam *flag;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        param = BattleHandler_PushWork(flow, 0x11, monId);
        param->monIndex = monId;
        param->attack = RawBattleMonStat(mon, 9);
        param->defense = RawBattleMonStat(mon, 8);
        param->setAttack = 1;
        param->setDefense = 1;
        BattleHandler_StrSetup(&param->string, 2, 0x305);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_PopWork(flow, param);

        flag = BattleHandler_PushWork(flow, 0x17, monId);
        flag->monIndex = monId;
        flag->flag = 0xa;
        BattleHandler_PopWork(flow, flag);
    }
}

static const BattleEventHandlerEntry sHandlersPowerSplit[] = {
    { 0xa0, HandlerPowerSplit },
};

static const BattleEventHandlerEntry *EventAddPowerSplit(u32 *priority) {
    *priority = 1;
    return sHandlersPowerSplit;
}

static void HandlerPowerSplit(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *mon;
    BattleMon *target;
    int attack;
    int spAttack;
    BattleHandlerSetStatusParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        mon = GetBattleMon(flow, monId);
        target = GetBattleMon(flow, targetId);
        attack = (RawBattleMonStat(mon, 8) + RawBattleMonStat(target, 8)) / 2;
        spAttack = (RawBattleMonStat(mon, 0xa) + RawBattleMonStat(target, 0xa)) / 2;

        param = BattleHandler_PushWork(flow, 0x11, monId);
        param->monIndex = targetId;
        param->attack = attack;
        param->spAttack = spAttack;
        param->setAttack = 1;
        param->setSpAttack = 1;
        BattleHandler_PopWork(flow, param);

        param = BattleHandler_PushWork(flow, 0x11, monId);
        param->monIndex = monId;
        param->attack = attack;
        param->spAttack = spAttack;
        param->setAttack = 1;
        param->setSpAttack = 1;
        BattleHandler_StrSetup(&param->string, 2, 0x448);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersGuardSplit[] = {
    { 0xa0, HandlerGuardSplit },
};

static const BattleEventHandlerEntry *EventAddGuardSplit(u32 *priority) {
    *priority = 1;
    return sHandlersGuardSplit;
}

static void HandlerGuardSplit(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *mon;
    BattleMon *target;
    int defense;
    int spDefense;
    BattleHandlerSetStatusParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        mon = GetBattleMon(flow, monId);
        target = GetBattleMon(flow, targetId);
        defense = (RawBattleMonStat(mon, 9) + RawBattleMonStat(target, 9)) / 2;
        spDefense = (RawBattleMonStat(mon, 0xb) + RawBattleMonStat(target, 0xb)) / 2;

        param = BattleHandler_PushWork(flow, 0x11, monId);
        param->monIndex = targetId;
        param->defense = defense;
        param->spDefense = spDefense;
        param->setDefense = 1;
        param->setSpDefense = 1;
        BattleHandler_PopWork(flow, param);

        param = BattleHandler_PushWork(flow, 0x11, monId);
        param->monIndex = monId;
        param->defense = defense;
        param->spDefense = spDefense;
        param->setDefense = 1;
        param->setSpDefense = 1;
        BattleHandler_StrSetup(&param->string, 2, 0x44b);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersLockOn[] = {
    { 0xa0, HandlerLockOn },
};

static const BattleEventHandlerEntry *EventAddLockOn(u32 *priority) {
    *priority = 1;
    return sHandlersLockOn;
}

static void HandlerLockOn(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        param = BattleHandler_PushWork(flow, 0xc, monId);
        param->targetIndex = monId;
        param->condition = 0x1d;
        param->value = func_ov167_021ce268(targetId, 2);
        BattleHandler_StrSetup(&param->string, 2, 0x28b);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, targetId);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersReflect[] = {
    { 0xa1, HandlerReflect },
};

static const BattleEventHandlerEntry *EventAddReflect(u32 *priority) {
    *priority = 1;
    return sHandlersReflect;
}

static void HandlerReflect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleCondition cont = SetConditionTurns(5);

    CommonCreateSideEffect(item, flow, monId, work, GetSideFromMonID(monId), 0, cont, 0x7c);
}

static const BattleEventHandlerEntry sHandlersLightScreen[] = {
    { 0xa1, HandlerLightScreen },
};

static const BattleEventHandlerEntry *EventAddLightScreen(u32 *priority) {
    *priority = 1;
    return sHandlersLightScreen;
}

static void HandlerLightScreen(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleCondition cont = SetConditionTurns(5);

    CommonCreateSideEffect(item, flow, monId, work, GetSideFromMonID(monId), 1, cont, 0x80);
}

static const BattleEventHandlerEntry sHandlersSafeguard[] = {
    { 0xa1, HandlerSafeguard },
};

static const BattleEventHandlerEntry *EventAddSafeguard(u32 *priority) {
    *priority = 1;
    return sHandlersSafeguard;
}

static void HandlerSafeguard(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 side = GetSideFromMonID(monId);
    BattleCondition cont = SetConditionTurns(5);

    CommonCreateSideEffect(item, flow, monId, work, side, 2, cont, 0x84);
}

static const BattleEventHandlerEntry sHandlersMist[] = {
    { 0xa1, HandlerMist },
};

static const BattleEventHandlerEntry *EventAddMist(u32 *priority) {
    *priority = 1;
    return sHandlersMist;
}

static void HandlerMist(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleCondition cont = SetConditionTurns(5);

    CommonCreateSideEffect(item, flow, monId, work, GetSideFromMonID(monId), 3, cont, 0x88);
}

static const BattleEventHandlerEntry sHandlersTailwind[] = {
    { 0xa1, HandlerTailwind },
};

static const BattleEventHandlerEntry *EventAddTailwind(u32 *priority) {
    *priority = 1;
    return sHandlersTailwind;
}

static void HandlerTailwind(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleCondition cont = SetConditionTurns(4);

    CommonCreateSideEffect(item, flow, monId, work, GetSideFromMonID(monId), 4, cont, 0x8c);
}

static const BattleEventHandlerEntry sHandlersLuckyChant[] = {
    { 0xa1, HandlerLuckyChant },
};

static const BattleEventHandlerEntry *EventAddLuckyChant(u32 *priority) {
    *priority = 1;
    return sHandlersLuckyChant;
}

static void HandlerLuckyChant(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleCondition cont = SetConditionTurns(5);

    CommonCreateSideEffect(item, flow, monId, work, GetSideFromMonID(monId), 5, cont, 0x90);
}

static const BattleEventHandlerEntry sHandlersSpikes[] = {
    { 0xa1, HandlerSpikes },
};

static const BattleEventHandlerEntry *EventAddSpikes(u32 *priority) {
    *priority = 1;
    return sHandlersSpikes;
}

static void HandlerSpikes(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleCondition cont = MakeConditionPermanent();

    CommonCreateSideEffect(item, flow, monId, work, GetSideFromOpposingMonID(monId), 6, cont, 0x94);
}

static const BattleEventHandlerEntry sHandlersToxicSpikes[] = {
    { 0xa1, HandlerToxicSpikes },
};

static const BattleEventHandlerEntry *EventAddToxicSpikes(u32 *priority) {
    *priority = 1;
    return sHandlersToxicSpikes;
}

static void HandlerToxicSpikes(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleCondition cont = MakeConditionPermanent();

    CommonCreateSideEffect(item, flow, monId, work, GetSideFromOpposingMonID(monId), 7, cont, 0x98);
}

static const BattleEventHandlerEntry sHandlersStealthRock[] = {
    { 0xa1, HandlerStealthRock },
};

static const BattleEventHandlerEntry *EventAddStealthRock(u32 *priority) {
    *priority = 1;
    return sHandlersStealthRock;
}

static void HandlerStealthRock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleCondition cont = MakeConditionPermanent();

    CommonCreateSideEffect(item, flow, monId, work, GetSideFromOpposingMonID(monId), 8, cont, 0x9c);
}

static const BattleEventHandlerEntry sHandlersWideGuard[] = {
    { 0x3, HandlerProtectStart },
    { 0x1f, HandlerProtectCheckFail },
    { 0x21, HandlerProtectResetCounter },
    { 0xa1, HandlerWideGuard },
};

static const BattleEventHandlerEntry *EventAddWideGuard(u32 *priority) {
    *priority = 4;
    return sHandlersWideGuard;
}

static void HandlerWideGuard(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleCondition cont = SetConditionTurns(1);

    CommonCreateSideEffect(item, flow, monId, work, GetSideFromMonID(monId), 9, cont, 0xa0);
    IncrementProtectCounter(flow, monId, TRUE);
}

static void CommonCreateSideEffect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work, u8 side, u32 effect,
                                   BattleCondition cont, u16 message) {
    BattleHandlerAddSideEffectParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        param = BattleHandler_PushWork(flow, 0x19, monId);
        param->effect = effect;
        param->side = side;
        param->cont = cont;
        BattleHandler_StrSetup(&param->string, 1, message);
        BattleHandler_AddArg(&param->string, side);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersTransform[] = {
    { 0xa0, HandlerTransform },
};

static const BattleEventHandlerEntry *EventAddTransform(u32 *priority) {
    *priority = 1;
    return sHandlersTransform;
}

static void HandlerTransform(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerTransformParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        param = BattleHandler_PushWork(flow, 0x33, monId);
        param->targetIndex = BattleEventVar_GetValue(6);
        BattleHandler_StrSetup(&param->string, 2, 0x284);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, param->targetIndex);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersLunarDance[] = {
    { 0xa0, HandlerLunarDance },
};

static const BattleEventHandlerEntry *EventAddLunarDance(u32 *priority) {
    *priority = 1;
    return sHandlersLunarDance;
}

static void HandlerLunarDance(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerPosEffectParam *param;
    BattleHandlerFaintParam *faint;

    if (BattleEventVar_GetValue(3) == monId && func_ov167_021aba64(flow, monId)) {
        param = BattleHandler_PushWork(flow, 0x1e, monId);
        param->effect = 1;
        param->pos = func_ov167_021abb50(flow, monId);
        BattleHandler_PopWork(flow, param);

        faint = BattleHandler_PushWork(flow, 0x13, monId);
        faint->monIndex = monId;
        BattleHandler_PopWork(flow, faint);
    }
}

static const BattleEventHandlerEntry sHandlersHealingWish[] = {
    { 0xa0, HandlerHealingWish },
};

static const BattleEventHandlerEntry *EventAddHealingWish(u32 *priority) {
    *priority = 1;
    return sHandlersHealingWish;
}

static void HandlerHealingWish(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerPosEffectParam *param;
    BattleHandlerFaintParam *faint;

    if (BattleEventVar_GetValue(3) == monId && func_ov167_021aba64(flow, monId)) {
        param = BattleHandler_PushWork(flow, 0x1e, monId);
        param->effect = 2;
        param->pos = func_ov167_021abb50(flow, monId);
        BattleHandler_PopWork(flow, param);

        faint = BattleHandler_PushWork(flow, 0x13, monId);
        faint->monIndex = monId;
        BattleHandler_PopWork(flow, faint);
    }
}

static const BattleEventHandlerEntry sHandlersWish[] = {
    { 0xa0, HandlerWish },
};

static const BattleEventHandlerEntry *EventAddWish(u32 *priority) {
    *priority = 1;
    return sHandlersWish;
}

static void HandlerWish(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerPosEffectParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        param = BattleHandler_PushWork(flow, 0x1e, monId);
        param->effect = 0;
        param->pos = func_ov167_021abb50(flow, monId);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersFutureSight[] = {
    { 0x6, HandlerFutureSight },
    { 0x7, HandlerFutureSightLand },
};

static const BattleEventHandlerEntry *EventAddFutureSight(u32 *priority) {
    *priority = 2;
    return sHandlersFutureSight;
}

static void HandlerFutureSight(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetPos;

    if (BattleEventVar_GetValue(3) == monId) {
        targetPos = BattleEventVar_GetValue(0xd);
        CommonDelayAttack(item, flow, monId, targetPos);
    }
}

static void HandlerFutureSightLand(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *msg;

    if (BattleEventVar_GetValue(3) == monId) {
        msg = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&msg->string, 2, 0x432);
        BattleHandler_AddArg(&msg->string, monId);
        BattleHandler_PopWork(flow, msg);
    }
}

static void CommonDelayAttack(BattleEventItem *item, BtlServerFlow *flow, u8 monId, u8 targetPos) {
    u8 pos;
    BattleHandlerPosEffectParam *param;

    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_RewriteValue(0x51, 1)) {
        if (targetPos == 6) {
            pos = func_ov167_021abb50(flow, monId);
            targetPos = func_ov167_0219c48c(func_ov167_021abc9c(flow), pos, 0);
        }
        param = BattleHandler_PushWork(flow, 0x1e, monId);
        param->effect = 3;
        param->pos = targetPos;
        param->args[0] = GetTurnCounter(flow) + 2;
        param->args[1] = BattleEventItem_GetSubID(item);
        param->argCount = 2;
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersDoomDesire[] = {
    { 0x6, HandlerDoomDesire },
    { 0x7, HandlerDoomDesireLand },
};

static const BattleEventHandlerEntry *EventAddDoomDesire(u32 *priority) {
    *priority = 2;
    return sHandlersDoomDesire;
}

static void HandlerDoomDesire(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetPos;

    if (BattleEventVar_GetValue(3) == monId) {
        targetPos = BattleEventVar_GetValue(0xd);
        CommonDelayAttack(item, flow, monId, targetPos);
    }
}

static void HandlerDoomDesireLand(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *msg;

    if (BattleEventVar_GetValue(3) == monId) {
        msg = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&msg->string, 2, 0x435);
        BattleHandler_AddArg(&msg->string, monId);
        BattleHandler_PopWork(flow, msg);
    }
}

static const BattleEventHandlerEntry sHandlersGastroAcid[] = {
    { 0xa0, HandlerGastroAcid },
};

static const BattleEventHandlerEntry *EventAddGastroAcid(u32 *priority) {
    *priority = 1;
    return sHandlersGastroAcid;
}

static void HandlerGastroAcid(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        param = BattleHandler_PushWork(flow, 0xc, monId);
        param->targetIndex = targetId;
        param->condition = 0x10;
        param->value = MakeConditionPermanent();
        BattleHandler_StrSetup(&param->string, 2, 0x235);
        BattleHandler_AddArg(&param->string, targetId);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersRolePlay[] = {
    { 0xa0, HandlerRolePlay },
};

static const BattleEventHandlerEntry *EventAddRolePlay(u32 *priority) {
    *priority = 1;
    return sHandlersRolePlay;
}

static void HandlerRolePlay(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    u32 ability;
    BattleHandlerAbilityChangeParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        ability = GetBattleMonStat(GetBattleMon(flow, targetId), 0x10);
        if (!func_ov169_0689caa4(ability)) {
            param = BattleHandler_PushWork(flow, 0x1f, monId);
            param->targetIndex = monId;
            param->ability = ability;
            BattleHandler_StrSetup(&param->string, 2, 0x26b);
            BattleHandler_AddArg(&param->string, monId);
            BattleHandler_AddArg(&param->string, targetId);
            BattleHandler_AddArg(&param->string, param->ability);
            BattleHandler_PopWork(flow, param);
        }
    }
}

static const BattleEventHandlerEntry sHandlersUturn[] = {
    { 0x86, HandlerUturn },
};

static const BattleEventHandlerEntry *EventAddUturn(u32 *priority) {
    *priority = 1;
    return sHandlersUturn;
}

static void HandlerUturn(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 trainerId;
    BattleHandlerSwitchParam *param;
    BattleHandlerEffectAtPosParam *effect;

    if (BattleEventVar_GetValue(3) == monId && func_ov167_021aba64(flow, monId) && func_ov167_021aba8c(flow)) {
        trainerId = func_ov167_0219c648(monId);
        param = BattleHandler_PushWork(flow, 0x29, monId);
        param->monIndex = monId;
        BattleHandler_StrSetup(&param->firstString, 2, 0x302);
        BattleHandler_AddArg(&param->firstString, monId);
        BattleHandler_AddArg(&param->firstString, trainerId);
        BattleHandler_PopWork(flow, param);

        effect = BattleHandler_PushWork(flow, 0x37, monId);
        effect->effect = BattleEventItem_GetSubID(item) == 0x171 ? 0x283 : 0x284;
        effect->pos1 = func_ov167_021abb50(flow, monId);
        effect->pos2 = 6;
        effect->header.checkPrevResult = 1;
        BattleHandler_PopWork(flow, effect);
    }
}

static const BattleEventHandlerEntry sHandlersRapidSpin[] = {
    { 0x4d, HandlerRapidSpin },
};

static const BattleEventHandlerEntry *EventAddRapidSpin(u32 *priority) {
    *priority = 1;
    return sHandlersRapidSpin;
}

static void HandlerRapidSpin(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    BattleHandlerCureConditionParam *cure;
    u8 side;
    u8 spikes;
    u8 toxicSpikes;
    u8 stealthRock;
    BattleHandlerRemoveSideEffectParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        if (CheckCondition(mon, 0x12)) {
            cure = BattleHandler_PushWork(flow, 0xb, monId);
            cure->count = 1;
            cure->monIds[0] = monId;
            cure->condition = 0x12;
            BattleHandler_PopWork(flow, cure);
        }
        if (CheckCondition(mon, 8)) {
            cure = BattleHandler_PushWork(flow, 0xb, monId);
            cure->count = 1;
            cure->monIds[0] = monId;
            cure->condition = 8;
            BattleHandler_PopWork(flow, cure);
        }
        side = GetSideFromMonID(monId);
        spikes = func_ov169_06898cf4(side, 6);
        toxicSpikes = func_ov169_06898cf4(side, 7);
        stealthRock = func_ov169_06898cf4(side, 8);
        if (spikes || toxicSpikes || stealthRock) {
            param = BattleHandler_PushWork(flow, 0x1a, monId);
            param->side = side;
            BattleHandler_InitFlags(param->effects, 3);
            if (spikes) {
                BattleHandler_SetFlag(param->effects, 6);
            }
            if (toxicSpikes) {
                BattleHandler_SetFlag(param->effects, 7);
            }
            if (stealthRock) {
                BattleHandler_SetFlag(param->effects, 8);
            }
            BattleHandler_PopWork(flow, param);
        }
    }
}

static const BattleEventHandlerEntry sHandlersBatonPass[] = {
    { 0xa0, HandlerBatonPass },
};

static const BattleEventHandlerEntry *EventAddBatonPass(u32 *priority) {
    *priority = 1;
    return sHandlersBatonPass;
}

static void HandlerBatonPass(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerPosEffectParam *param;
    BattleHandlerFlagParam *flag;
    BattleHandlerSwitchParam *sw;

    if (BattleEventVar_GetValue(3) == monId && func_ov167_021aba64(flow, monId) && func_ov167_021aba8c(flow)) {
        param = BattleHandler_PushWork(flow, 0x1e, monId);
        param->effect = 4;
        param->pos = func_ov167_021abb50(flow, monId);
        param->args[0] = monId;
        param->argCount = 1;
        BattleHandler_PopWork(flow, param);

        flag = BattleHandler_PushWork(flow, 0x17, monId);
        flag->monIndex = monId;
        flag->flag = 0xe;
        flag->header.checkPrevResult = 1;
        BattleHandler_PopWork(flow, flag);

        sw = BattleHandler_PushWork(flow, 0x29, monId);
        sw->monIndex = monId;
        sw->flag = 1;
        sw->header.checkPrevResult = 1;
        BattleHandler_PopWork(flow, sw);
    }
}

static const BattleEventHandlerEntry sHandlersTeleport[] = {
    { 0xa0, HandlerTeleport },
    { 0x1f, HandlerTeleportCheckFail },
    { 0xd, HandlerTeleportExitText },
};

static const BattleEventHandlerEntry *EventAddTeleport(u32 *priority) {
    *priority = 3;
    return sHandlersTeleport;
}

static void HandlerTeleportCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;

    if (func_ov167_021abca8(flow) != 0 || func_ov167_021abc9c(flow) != 0) {
        BattleEventVar_RewriteValue(0x22, 0x1a);
    }
    mon = GetBattleMon(flow, monId);
    if (CheckCondition(mon, 8) && CheckCondition(mon, 0x16)) {
        BattleEventVar_RewriteValue(0x22, 0x1a);
    }
}

static void HandlerTeleport(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventItem_SetRecallEnable(item);
        BattleHandler_PushRun(flow, 0x28, monId);
    }
}

static void HandlerTeleportExitText(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *msg;

    if (BattleEventVar_RewriteValue(0x51, 1)) {
        msg = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&msg->string, 2, 0x2ff);
        BattleHandler_AddArg(&msg->string, monId);
        BattleHandler_PopWork(flow, msg);
    }
}

static const BattleEventHandlerEntry sHandlersFling[] = {
    { 0x1f, HandlerFlingCheckFail },   { 0x37, HandlerFlingPower }, { 0x81, HandlerFlingStart },
    { 0x4b, HandlerFlingDamageAfter }, { 0x27, HandlerFlingEnd },
};

static const BattleEventHandlerEntry *EventAddFling(u32 *priority) {
    *priority = 5;
    return sHandlersFling;
}

static void HandlerFlingCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u16 heldItem;

    if (BattleEventVar_GetValue(2) == monId) {
        heldItem = GetBattleMonHeldItem(GetBattleMon(flow, monId));
        if (heldItem == 0 || ItemGetParam(heldItem, 0xa) == 0 || !func_ov167_021abd8c(flow, monId) ||
            func_ov167_021cdedc(flow, monId)) {
            BattleEventVar_RewriteValue(0x22, 0x1a);
        }
    }
}

static void HandlerFlingPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_RewriteValue(0x30, ItemGetParam(GetBattleMonHeldItem(GetBattleMon(flow, monId)), 0xa));
    }
}

static void HandlerFlingStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u16 heldItem;
    BattleHandlerMessageParam *msg;

    if (BattleEventVar_GetValue(3) == monId) {
        heldItem = GetBattleMonHeldItem(GetBattleMon(flow, monId));
        if (heldItem != 0) {
            msg = BattleHandler_PushWork(flow, 4, monId);
            BattleHandler_StrSetup(&msg->string, 2, 0x30b);
            BattleHandler_AddArg(&msg->string, monId);
            BattleHandler_AddArg(&msg->string, heldItem);
            BattleHandler_PopWork(flow, msg);
        }
    }
}

static void HandlerFlingDamageAfter(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u16 heldItem;
    BattleHandlerConsumeItemParam *consume;
    BattleHandlerForceUseItemParam *use;

    if (BattleEventVar_GetValue(3) == monId) {
        heldItem = GetBattleMonHeldItem(GetBattleMon(flow, monId));
        if (heldItem != 0) {
            consume = BattleHandler_PushWork(flow, 0x23, monId);
            consume->skipDisplay = 1;
            BattleHandler_PopWork(flow, consume);
            work[0] = 1;
            if (BattleEventVar_GetValue(0x46) == 0 && BattleEventVar_GetValue(0x47) == 0 &&
                ItemGetParam(heldItem, 9) != 0) {
                use = BattleHandler_PushWork(flow, 0x22, monId);
                use->monIndex = BattleEventVar_GetValue(4);
                use->item = heldItem;
                BattleHandler_PopWork(flow, use);
            }
        }
    }
}

static void HandlerFlingEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u16 heldItem;
    BattleHandlerConsumeItemParam *consume;

    if (BattleEventVar_GetValue(2) == monId && work[0] == 0) {
        heldItem = GetBattleMonHeldItem(GetBattleMon(flow, monId));
        if (heldItem != 0) {
            consume = BattleHandler_PushWork(flow, 0x23, monId);
            consume->skipDisplay = 1;
            BattleHandler_PopWork(flow, consume);
        }
    }
}

static const BattleEventHandlerEntry sHandlersMagnetRise[] = {
    { 0x1f, HandlerMagnetRiseCheckFail },
    { 0xa0, HandlerMagnetRise },
};

static const BattleEventHandlerEntry *EventAddMagnetRise(u32 *priority) {
    *priority = 2;
    return sHandlersMagnetRise;
}

static void HandlerMagnetRiseCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;

    if (BattleEventVar_GetValue(2) == monId) {
        mon = GetBattleMon(flow, monId);
        if (CheckCondition(mon, 0x15) || CheckCondition(mon, 0x1f)) {
            BattleEventVar_RewriteValue(0x22, 0x1a);
        }
    }
}

static void HandlerMagnetRise(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        param = BattleHandler_PushWork(flow, 0xc, monId);
        param->targetIndex = monId;
        param->condition = 0x1e;
        param->value = SetConditionTurns(5);
        BattleHandler_StrSetup(&param->string, 2, 0x292);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sHandlersHelpingHand[] = {
    { 0x1c, HandlerHelpingHandSkip },  { 0x99, HandlerHelpingHandCheckInvuln }, { 0xa0, HandlerHelpingHandReady },
    { 0x38, HandlerHelpingHandPower }, { 0x76, HandlerHelpingHandTurnCheck },
};

static const BattleEventHandlerEntry *EventAddHelpingHand(u32 *priority) {
    *priority = 5;
    return sHandlersHelpingHand;
}

static void HandlerHelpingHandSkip(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_RewriteValue(0x51, 1);
    }
}

static void HandlerHelpingHandCheckInvuln(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_RewriteValue(0x42, 0);
    }
}

static void HandlerHelpingHandReady(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *target;
    BattleHandlerMessageParam *msg;

    if (func_ov167_021abcc0(flow) > 1 && BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        target = GetBattleMon(flow, targetId);
        if (!IsFainted(target) && !GetTurnFlag(target, 1)) {
            msg = BattleHandler_PushWork(flow, 4, monId);
            BattleHandler_StrSetup(&msg->string, 2, 0x414);
            BattleHandler_AddArg(&msg->string, monId);
            BattleHandler_AddArg(&msg->string, targetId);
            BattleHandler_PopWork(flow, msg);
            work[0] = targetId;
            BattleEventItem_ConvertToIsolated(item);
        }
    }
}

static void HandlerHelpingHandPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == work[0]) {
        BattleEventVar_MulValue(0x31, FX32_CONST(1.5));
    }
}

static void HandlerHelpingHandTurnCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        BattleEventItem_Remove(item);
    }
}

static const BattleEventHandlerEntry sHandlersBeatUp[] = {
    { 0x35, HandlerBeatUp },
    { 0x37, HandlerBeatUpPower },
};

static const BattleEventHandlerEntry *EventAddBeatUp(u32 *priority) {
    *priority = 2;
    return sHandlersBeatUp;
}

static void HandlerBeatUp(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 hits;
    u32 i;

    if (BattleEventVar_GetValue(3) == monId) {
        hits = 0;
        for (i = 0; i < 6; i++) {
            if (CommonBeatUpGetParam(flow, monId, i) != NULL) {
                hits++;
            }
        }
        if (hits == 0) {
            hits = 1;
        }
        BattleEventVar_RewriteValue(0x2a, hits);
        work[0] = 0;
    }
}

static void HandlerBeatUpPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = CommonBeatUpGetParam(flow, monId, work[0]);
        if (mon == NULL) {
            mon = GetBattleMon(flow, monId);
        }
        work[0]++;
        if (work[0] >= 6) {
            work[0] = 0;
        }
        BattleEventVar_RewriteValue(0x30, GetBattleMonStat(mon, 0x15) / 10 + 5);
    }
}

// Returns the index-th mon of the party that can join Beat Up, or NULL
static BattleMon *CommonBeatUpGetParam(BtlServerFlow *flow, u8 monId, u8 index) {
    BattleParty *party;
    u32 count;
    u32 i;
    BattleMon *mon;
    BOOL valid;

    party = func_ov167_021abb0c(flow, monId);
    count = GetNumMonsInParty(party);
    if (index >= count) {
        return NULL;
    }
    for (i = 0; i < count; i++) {
        valid = FALSE;
        mon = GetBattleMonFromParty(party, i);
        if (GetMonID(mon) == monId) {
            valid = TRUE;
        } else if (CanPokemonBattle(mon) && GetBattleMonStatus(mon) == 0) {
            valid = TRUE;
        }
        if (valid) {
            if (index == 0) {
                return mon;
            }
            index--;
        }
    }
    return NULL;
}

static const BattleEventHandlerEntry sHandlersFakeOut[] = {
    { 0x1f, HandlerFakeOut },
};

static const BattleEventHandlerEntry *EventAddFakeOut(u32 *priority) {
    *priority = 1;
    return sHandlersFakeOut;
}

static void HandlerFakeOut(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId && GetAdditionalConditionFlag(GetBattleMon(flow, monId), 0)) {
        BattleEventVar_RewriteValue(0x22, 0x1a);
    }
}

static const BattleEventHandlerEntry sHandlersMorningSun[] = {
    { 0x8f, HandlerMorningSun },
};

static const BattleEventHandlerEntry *EventAddMorningSun(u32 *priority) {
    *priority = 1;
    return sHandlersMorningSun;
}

static void HandlerMorningSun(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    fx32 ratio;

    if (BattleEventVar_GetValue(2) == monId) {
        switch (GetWeather(flow)) {
        case 1:
            ratio = 0xaac;
            break;
        case 2:
        case 3:
        case 4:
            ratio = FX32_CONST(0.25);
            break;
        default:
            ratio = FX32_CONST(0.5);
            break;
        }
        BattleEventVar_RewriteValue(0x35, ratio);
    }
}

static const BattleEventHandlerEntry sHandlersFly[] = {
    { 0x95, HandlerFly },
    { 0x98, func_ov167_021cba48 },
};

static const BattleEventHandlerEntry *EventAddFly(u32 *priority) {
    *priority = 2;
    return sHandlersFly;
}

static void HandlerFly(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerFlagParam *flag;
    BattleHandlerMessageParam *msg;

    if (BattleEventVar_GetValue(3) == monId) {
        flag = BattleHandler_PushWork(flow, 0x17, monId);
        flag->monIndex = monId;
        flag->flag = 3;
        BattleHandler_PopWork(flow, flag);

        msg = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&msg->string, 2, 0x211);
        BattleHandler_AddArg(&msg->string, monId);
        BattleHandler_PopWork(flow, msg);
    }
}

static void func_ov167_021cba48(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
}

static const BattleEventHandlerEntry sHandlersShadowForce[] = {
    { 0x95, HandlerShadowForce },       { 0x98, func_ov167_021cbaac },
    { 0x2e, HandlerFeintBreakProtect }, { 0x81, HandlerFeintResetProtectCounter },
    { 0x84, HandlerShadowForceEnd },
};

static const BattleEventHandlerEntry *EventAddShadowForce(u32 *priority) {
    *priority = 5;
    return sHandlersShadowForce;
}

static void HandlerShadowForce(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerFlagParam *flag;
    BattleHandlerMessageParam *msg;

    if (BattleEventVar_GetValue(3) == monId) {
        flag = BattleHandler_PushWork(flow, 0x17, monId);
        flag->monIndex = monId;
        flag->flag = 6;
        BattleHandler_PopWork(flow, flag);

        msg = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&msg->string, 2, 0x21d);
        BattleHandler_AddArg(&msg->string, monId);
        BattleHandler_PopWork(flow, msg);
    }
}

static void func_ov167_021cbaac(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
}

static void HandlerShadowForceEnd(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleHandlerFlagParam *flag;
    BattleHandlerMessageParam *msg;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        if (GetTurnFlag(GetBattleMon(flow, targetId), 7)) {
            flag = BattleHandler_PushWork(flow, 0x16, monId);
            flag->monIndex = targetId;
            flag->flag = 7;
            BattleHandler_PopWork(flow, flag);

            msg = BattleHandler_PushWork(flow, 4, monId);
            BattleHandler_StrSetup(&msg->string, 2, 0x208);
            BattleHandler_AddArg(&msg->string, targetId);
            BattleHandler_PopWork(flow, msg);
        }
    }
}

static const BattleEventHandlerEntry sHandlersBounce[] = {
    { 0x95, HandlerBounce },
    { 0x98, func_ov167_021cbb80 },
};

static const BattleEventHandlerEntry *EventAddBounce(u32 *priority) {
    *priority = 2;
    return sHandlersBounce;
}

static void HandlerBounce(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerFlagParam *flag;
    BattleHandlerMessageParam *msg;

    if (BattleEventVar_GetValue(3) == monId) {
        flag = BattleHandler_PushWork(flow, 0x17, monId);
        flag->monIndex = monId;
        flag->flag = 3;
        BattleHandler_PopWork(flow, flag);

        msg = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&msg->string, 2, 0x220);
        BattleHandler_AddArg(&msg->string, monId);
        BattleHandler_PopWork(flow, msg);
    }
}

static void func_ov167_021cbb80(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
}

static const BattleEventHandlerEntry sHandlersDive[] = {
    { 0x95, HandlerDive },
    { 0x98, func_ov167_021cbbe4 },
};

static const BattleEventHandlerEntry *EventAddDive(u32 *priority) {
    *priority = 2;
    return sHandlersDive;
}

static void HandlerDive(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerFlagParam *flag;
    BattleHandlerMessageParam *msg;

    if (BattleEventVar_GetValue(3) == monId) {
        flag = BattleHandler_PushWork(flow, 0x17, monId);
        flag->monIndex = monId;
        flag->flag = 4;
        BattleHandler_PopWork(flow, flag);

        msg = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&msg->string, 2, 0x217);
        BattleHandler_AddArg(&msg->string, monId);
        BattleHandler_PopWork(flow, msg);
    }
}

static void func_ov167_021cbbe4(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
}

static const BattleEventHandlerEntry sHandlersDig[] = {
    { 0x95, HandlerDig },
    { 0x98, func_ov167_021cbc48 },
};

static const BattleEventHandlerEntry *EventAddDig(u32 *priority) {
    *priority = 2;
    return sHandlersDig;
}

static void HandlerDig(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerFlagParam *flag;
    BattleHandlerMessageParam *msg;

    if (BattleEventVar_GetValue(3) == monId) {
        flag = BattleHandler_PushWork(flow, 0x17, monId);
        flag->monIndex = monId;
        flag->flag = 5;
        BattleHandler_PopWork(flow, flag);

        msg = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&msg->string, 2, 0x21a);
        BattleHandler_AddArg(&msg->string, monId);
        BattleHandler_PopWork(flow, msg);
    }
}

static void func_ov167_021cbc48(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
}

static const BattleEventHandlerEntry sHandlersSolarBeam[] = {
    { 0x94, HandlerSolarBeamSunCheck },
    { 0x95, HandlerSolarBeamCharge },
    { 0x38, HandlerSolarBeamPower },
};

static const BattleEventHandlerEntry *EventAddSolarBeam(u32 *priority) {
    *priority = 3;
    return sHandlersSolarBeam;
}

static void HandlerSolarBeamSunCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && GetWeather(flow) == 1) {
        BattleEventVar_RewriteValue(0x51, 1);
    }
}

static void HandlerSolarBeamCharge(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *msg;

    if (BattleEventVar_GetValue(3) == monId) {
        msg = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&msg->string, 2, 0x229);
        BattleHandler_AddArg(&msg->string, monId);
        BattleHandler_PopWork(flow, msg);
    }
}

static void HandlerSolarBeamPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 weather;

    if (BattleEventVar_GetValue(3) == monId) {
        weather = GetWeather(flow);
        if (weather == 2 || weather == 3 || weather == 4) {
            BattleEventVar_MulValue(0x31, FX32_CONST(0.5));
        }
    }
}

static const BattleEventHandlerEntry sHandlersRazorWind[] = {
    { 0x95, HandlerRazorWindCharge },
};

static const BattleEventHandlerEntry *EventAddRazorWind(u32 *priority) {
    *priority = 1;
    return sHandlersRazorWind;
}

static void HandlerRazorWindCharge(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *msg;

    if (BattleEventVar_GetValue(3) == monId) {
        msg = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&msg->string, 2, 0x223);
        BattleHandler_AddArg(&msg->string, monId);
        BattleHandler_PopWork(flow, msg);
    }
}

static const BattleEventHandlerEntry sHandlersSkyAttack[] = {
    { 0x95, HandlerSkyAttack },
};

static const BattleEventHandlerEntry *EventAddSkyAttack(u32 *priority) {
    *priority = 1;
    return sHandlersSkyAttack;
}

static void HandlerSkyAttack(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *msg;

    if (BattleEventVar_GetValue(3) == monId) {
        msg = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&msg->string, 2, 0x226);
        BattleHandler_AddArg(&msg->string, monId);
        BattleHandler_PopWork(flow, msg);
    }
}

static const BattleEventHandlerEntry sSkullBashHandlers[] = {
    { 0x96, HandlerSkullBash },
};

static const BattleEventHandlerEntry *EventAddSkullBash(u32 *priority) {
    *priority = NELEMS(sSkullBashHandlers);
    return sSkullBashHandlers;
}

static void HandlerSkullBash(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *message;
    BattleHandlerStatChangeParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        message = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&message->string, 2, 0x22c);
        BattleHandler_AddArg(&message->string, monId);
        BattleHandler_PopWork(flow, message);

        param = BattleHandler_PushWork(flow, 0xe, monId);
        param->count = 1;
        param->monIds[0] = monId;
        param->stat = 2;
        param->change = 1;
        param->unk0e = 1;
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sPluckHandlers[] = {
    { 0x83, HandlerPluck },
};

static const BattleEventHandlerEntry *EventAddPluck(u32 *priority) {
    *priority = NELEMS(sPluckHandlers);
    return sPluckHandlers;
}

static void HandlerPluck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    u16 berry;
    BattleHandlerSetItemParam *param;
    BattleHandlerForceUseItemParam *useParam;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        berry = GetBattleMonHeldItem(GetBattleMon(flow, targetId));
        if (PML_ItemIsBerry(berry)) {
            param = BattleHandler_PushWork(flow, 0x20, monId);
            param->targetIndex = targetId;
            param->item = 0;
            BattleHandler_StrSetup(&param->string, 2, 0x308);
            BattleHandler_AddArg(&param->string, monId);
            BattleHandler_AddArg(&param->string, berry);
            BattleHandler_PopWork(flow, param);

            useParam = BattleHandler_PushWork(flow, 0x22, monId);
            useParam->checkPrevResult = TRUE;
            useParam->item = berry;
            useParam->monIndex = monId;
            BattleHandler_PopWork(flow, useParam);
        }
    }
}

static const BattleEventHandlerEntry sStruggleHandlers[] = {
    { 0x22, HandlerStruggleStart },
    { 0x28, HandlerStruggleMoveParam },
    { 0x50, HandlerStruggleRecoil },
};

static const BattleEventHandlerEntry *EventAddStruggle(u32 *priority) {
    *priority = NELEMS(sStruggleHandlers);
    return sStruggleHandlers;
}

static void HandlerStruggleRecoil(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_RewriteValue(0x51, 1);
    }
}

static void HandlerStruggleStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        param = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&param->string, 2, 0x344);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_PopWork(flow, param);
    }
}

static void HandlerStruggleMoveParam(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        BattleEventVar_RewriteValue(0x4b, 1);
    }
}

static const BattleEventHandlerEntry sDestinyBondHandlers[] = {
    { 0xa0, HandlerDestinyBondReady },
    { 0x1, HandlerDestinyBondStart },
    { 0x4c, HandlerDestinyBondDamage },
};

static const BattleEventHandlerEntry *EventAddDestinyBond(u32 *priority) {
    *priority = NELEMS(sDestinyBondHandlers);
    return sDestinyBondHandlers;
}

static void HandlerDestinyBondReady(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        param = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&param->string, 2, 0x272);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_PopWork(flow, param);
        work[6] = 1;
    }
}

static void HandlerDestinyBondStart(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId) {
        BattleEventItem_Remove(item);
    }
}

static void HandlerDestinyBondDamage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 attackerId;
    BattleMon *attacker;
    BattleHandlerFaintParam *param;

    if (BattleEventVar_GetValue(4) == monId && IsFainted(GetBattleMon(flow, monId))) {
        if (!func_ov169_0689ca54(BattleEventVar_GetValue(0x12))) {
            attackerId = BattleEventVar_GetValue(3);
            attacker = GetBattleMon(flow, attackerId);
            if (!IsAllyMonID(monId, attackerId) && !IsFainted(attacker)) {
                BattleEventItem_ConvertToIsolated(item);

                param = BattleHandler_PushWork(flow, 0x13, monId);
                param->monIndex = monId;
                param->force = 1;
                BattleHandler_PopWork(flow, param);

                param = BattleHandler_PushWork(flow, 0x13, monId);
                param->monIndex = BattleEventVar_GetValue(3);
                BattleHandler_StrSetup(&param->string, 2, 0x275);
                BattleHandler_AddArg(&param->string, monId);
                BattleHandler_PopWork(flow, param);
            }
        }
        BattleEventItem_Remove(item);
    }
}

static const BattleEventHandlerEntry sGrudgeHandlers[] = {
    { 0xa0, HandlerGrudgeReady },
    { 0x4b, HandlerGrudgeReducePP },
    { 0x1, HandlerDestinyBondStart },
};

static const BattleEventHandlerEntry *EventAddGrudge(u32 *priority) {
    *priority = NELEMS(sGrudgeHandlers);
    return sGrudgeHandlers;
}

static void HandlerGrudgeReady(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        param = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&param->string, 2, 0x278);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_PopWork(flow, param);
        work[6] = 1;
    }
}

static void HandlerGrudgeReducePP(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 attackerId;
    BattleMon *attacker;
    u16 move;
    u32 slot;
    BattleHandlerPPParam *param;

    if (BattleEventVar_GetValue(4) == monId && !func_ov169_0689ca54(BattleEventVar_GetValue(0x12)) &&
        IsFainted(GetBattleMon(flow, monId))) {
        attackerId = BattleEventVar_GetValue(3);
        attacker = GetBattleMon(flow, attackerId);
        move = BattleEventVar_GetValue(0x14);
        slot = func_ov167_021baf78(attacker, move);
        if (slot != 4) {
            param = BattleHandler_PushWork(flow, 0xa, monId);
            param->monIndex = attackerId;
            param->moveIndex = slot;
            param->amount = GetMovePP(attacker, slot) * -1;
            param->allowFainted = TRUE;
            BattleHandler_StrSetup(&param->string, 2, 0x27b);
            BattleHandler_AddArg(&param->string, attackerId);
            BattleHandler_AddArg(&param->string, move);
            BattleHandler_PopWork(flow, param);
        }
        BattleEventItem_Remove(item);
    }
}

static const BattleEventHandlerEntry sMinimizeHandlers[] = {
    { 0x24, HandlerMinimize },
};

static const BattleEventHandlerEntry *EventAddMinimize(u32 *priority) {
    *priority = NELEMS(sMinimizeHandlers);
    return sMinimizeHandlers;
}

static void HandlerMinimize(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerFlagParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        param = BattleHandler_PushWork(flow, 0x17, monId);
        param->monIndex = monId;
        param->flag = 8;
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sDefenseCurlHandlers[] = {
    { 0x24, HandlerDefenseCurl },
};

static const BattleEventHandlerEntry *EventAddDefenseCurl(u32 *priority) {
    *priority = NELEMS(sDefenseCurlHandlers);
    return sDefenseCurlHandlers;
}

static void HandlerDefenseCurl(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerFlagParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        param = BattleHandler_PushWork(flow, 0x17, monId);
        param->monIndex = monId;
        param->flag = 7;
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sRoostHandlers[] = {
    { 0x25, HandlerRoost },
};

static const BattleEventHandlerEntry *EventAddRoost(u32 *priority) {
    *priority = NELEMS(sRoostHandlers);
    return sRoostHandlers;
}

static void HandlerRoost(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerAddConditionParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        param = BattleHandler_PushWork(flow, 0xc, monId);
        param->targetIndex = monId;
        param->noMessage = 1;
        param->condition = 0x18;
        param->value = AddTurnCondition(1, 2);
        SetConditionFlag(&param->value, 1);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sFocusPunchHandlers[] = {
    { 0x15, HandlerFocuspunch },
};

static const BattleEventHandlerEntry *EventAddFocusPunch(u32 *priority) {
    *priority = NELEMS(sFocusPunchHandlers);
    return sFocusPunchHandlers;
}

static void HandlerFocuspunch(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerFlagParam *flag;
    BattleHandlerEffectAtPosParam *effect;
    BattleHandlerMessageParam *message;

    if (BattleEventVar_GetValue(2) == monId) {
        flag = BattleHandler_PushWork(flow, 0x15, monId);
        flag->monIndex = monId;
        flag->flag = 5;
        BattleHandler_PopWork(flow, flag);

        if (!IsSemiInvulnMove(GetBattleMon(flow, monId))) {
            effect = BattleHandler_PushWork(flow, 0x37, monId);
            effect->effect = 0x272;
            effect->pos1 = func_ov167_021abb50(flow, monId);
            effect->pos2 = 6;
            effect->hideMessageWindow = 1;
            BattleHandler_PopWork(flow, effect);
        }

        message = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&message->string, 2, 0x268);
        BattleHandler_AddArg(&message->string, monId);
        BattleHandler_PopWork(flow, message);
    }
}

static const BattleEventHandlerEntry sMetronomeHandlers[] = {
    { 0x18, HandlerMetronome },
    { 0x19, HandlerMetronomeTextSet },
};

static const BattleEventHandlerEntry *EventAddMetronome(u32 *priority) {
    *priority = NELEMS(sMetronomeHandlers);
    return sMetronomeHandlers;
}

static void HandlerMetronome(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 count;
    const u16 *excluded;
    u16 move;
    u8 targetPos;

    if (BattleEventVar_GetValue(2) == monId) {
        excluded = func_ov169_0689cc14(&count);
        move = func_ov167_021bd658(excluded, count);
        targetPos = func_ov167_021abb70(flow, monId, move);
        BattleEventVar_RewriteValue(0x12, move);
        BattleEventVar_RewriteValue(0xd, targetPos);
    }
}

static void HandlerMetronomeTextSet(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerString *string;
    u16 move;

    if (BattleEventVar_GetValue(2) == monId) {
        string = (BattleHandlerString *)BattleEventVar_GetValue(0x3f);
        move = BattleEventVar_GetValue(0x12);
        BattleHandler_StrSetup(string, 1, 0x78);
        BattleHandler_AddArg(string, move);
        BattleEventVar_RewriteValue(0x51, 1);
    }
}

static const BattleEventHandlerEntry sNaturePowerHandlers[] = {
    { 0x18, HandlerNaturePower },
    { 0x19, HandlerNaturePowerTextSet },
};

static const BattleEventHandlerEntry *EventAddNaturePower(u32 *priority) {
    *priority = NELEMS(sNaturePowerHandlers);
    return sNaturePowerHandlers;
}

static void HandlerNaturePower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u16 move;
    u8 targetPos;

    if (BattleEventVar_GetValue(2) == monId) {
        switch (GetBattleTerrain(flow)) {
        case 1:
        case 2:
        case 3:
        case 8:
        case 15:
        case 17:
            move = MOVE_EARTHQUAKE;
            break;
        case 4:
        case 14:
        case 16:
        case 18:
        default:
            move = MOVE_TRI_ATTACK;
            break;
        case 10:
        case 19:
            move = MOVE_ROCK_SLIDE;
            break;
        case 0:
        case 5:
            move = MOVE_SEED_BOMB;
            break;
        case 6:
        case 11:
        case 12:
            move = MOVE_HYDRO_PUMP;
            break;
        case 9:
            move = MOVE_MUD_BOMB;
            break;
        case 7:
            move = MOVE_BLIZZARD;
            break;
        case 13:
            move = MOVE_ICE_BEAM;
            break;
        }
        targetPos = func_ov167_021abb70(flow, monId, move);
        BattleEventVar_RewriteValue(0x12, move);
        BattleEventVar_RewriteValue(0xd, targetPos);
    }
}

static void HandlerNaturePowerTextSet(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerString *string;
    u16 move;

    if (BattleEventVar_GetValue(2) == monId) {
        string = (BattleHandlerString *)BattleEventVar_GetValue(0x3f);
        move = BattleEventVar_GetValue(0x12);
        BattleHandler_StrSetup(string, 1, 0x79);
        BattleHandler_AddArg(string, move);
        BattleEventVar_RewriteValue(0x51, 1);
    }
}

static const BattleEventHandlerEntry sAssistHandlers[] = {
    { 0x18, HandlerAssist },
};

static const BattleEventHandlerEntry *EventAddAssist(u32 *priority) {
    *priority = NELEMS(sAssistHandlers);
    return sAssistHandlers;
}

static void HandlerAssist(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleParty *party;
    u16 *moves;
    u8 count;
    u8 numMons;
    u8 i;
    BattleMon *mon;
    u8 numMoves;
    u8 j;
    u16 move;
    u8 index;
    u8 targetPos;

    if (BattleEventVar_GetValue(2) == monId) {
        party = func_ov167_021abb0c(flow, monId);
        moves = (u16 *)func_ov167_021abc60(flow, 0x30);
        count = 0;
        numMons = GetNumMonsInParty(party);
        for (i = 0; i < numMons; i++) {
            mon = GetBattleMonFromParty(party, i);
            if (GetMonID(mon) != monId) {
                numMoves = GetBattleMonMoveCount(mon);
                for (j = 0; j < numMoves; j++) {
                    move = MoveGetID(mon, j);
                    if (!func_ov169_0689cbc4(move)) {
                        moves[count++] = move;
                    }
                }
            }
        }
        if (count != 0) {
            index = BattleRandom(count);
            targetPos = func_ov167_021abb70(flow, monId, moves[index]);
            BattleEventVar_RewriteValue(0x12, moves[index]);
            BattleEventVar_RewriteValue(0xd, targetPos);
        }
    }
}

static const BattleEventHandlerEntry sSleepTalkHandlers[] = {
    { 0x1d, HandlerSnoreCheck1 },
    { 0x1f, HandlerSnoreCheck2 },
    { 0x18, HandlerSleepTalk },
};

static const BattleEventHandlerEntry *EventAddSleepTalk(u32 *priority) {
    *priority = NELEMS(sSleepTalkHandlers);
    return sSleepTalkHandlers;
}

static void HandlerSleepTalk(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u8 numMoves;
    u8 i;
    u8 count;
    u16 move;
    u8 targetPos;

    if (BattleEventVar_GetValue(2) == monId) {
        mon = GetBattleMon(flow, monId);
        if (CheckCondition(mon, 2)) {
            numMoves = GetBattleMonMoveCount(mon);
            for (i = 0, count = 0; i < numMoves; i++) {
                move = MoveGetID(mon, i);
                if (!func_ov169_0689cba4(move) && !getMoveFlag(move, 1)) {
                    work[count++] = move;
                }
            }
            if (count != 0) {
                move = work[(u8)BattleRandom(count)];
                targetPos = func_ov167_021abb70(flow, monId, move);
                BattleEventVar_RewriteValue(0x12, move);
                BattleEventVar_RewriteValue(0xd, targetPos);
            } else {
                BattleEventVar_RewriteValue(0x41, 1);
            }
        }
    }
}

static const BattleEventHandlerEntry sMirrorMoveHandlers[] = {
    { 0x18, HandlerMirrorMove },
};

static const BattleEventHandlerEntry *EventAddMirrorMove(u32 *priority) {
    *priority = NELEMS(sMirrorMoveHandlers);
    return sMirrorMoveHandlers;
}

static void HandlerMirrorMove(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 pos;
    u8 side;
    u16 move;

    if (BattleEventVar_GetValue(2) == monId) {
        pos = BattleEventVar_GetValue(0xe);
        if (pos == 6) {
            side = func_ov167_021ab840(flow, monId);
            pos = func_ov167_0219c48c(func_ov167_021abc9c(flow), side, 0);
        }
        if (HandlerGetAlivePartyCount(flow, pos, (u8 *)work)) {
            move = GetPreviousMoveID(GetBattleMon(flow, ((u8 *)work)[0]));
            if (move == 0 || !getMoveFlag(move, 6)) {
                BattleEventVar_RewriteValue(0x41, 1);
            } else {
                BattleEventVar_RewriteValue(0x12, move);
                BattleEventVar_RewriteValue(0xd, pos);
            }
        }
    }
}

static const BattleEventHandlerEntry sMeFirstHandlers[] = {
    { 0x18, HandlerMeFirst },
    { 0x38, HandlerMeFirstPower },
};

static const BattleEventHandlerEntry *EventAddMeFirst(u32 *priority) {
    *priority = NELEMS(sMeFirstHandlers);
    return sMeFirstHandlers;
}

static void HandlerMeFirst(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 pos;
    u8 side;
    BattleMon *target;
    u8 targetId;
    BOOL success;
    BattleAction action;
    u16 move;

    if (BattleEventVar_GetValue(2) == monId) {
        pos = BattleEventVar_GetValue(0xe);
        if (pos == 6) {
            side = func_ov167_021ab840(flow, monId);
            pos = func_ov167_0219c48c(func_ov167_021abc9c(flow), side, 0);
        }
        if (HandlerGetAlivePartyCount(flow, pos, (u8 *)work)) {
            target = GetBattleMon(flow, ((u8 *)work)[0]);
            targetId = ((u8 *)work)[0];
            success = FALSE;
            if (!GetTurnFlag(target, 3) && func_ov167_021abb8c(flow, targetId, &action) &&
                func_ov167_021abc8c(flow, targetId) && !func_ov167_021abeb4(flow, targetId)) {
                move = func_ov167_021bdb68(&action);
                if (move != 0 && PML_MoveIsDamaging(move) && !func_ov169_0689ca44(move)) {
                    success = TRUE;
                }
            }
            if (success) {
                BattleEventVar_RewriteValue(0x12, move);
                BattleEventVar_RewriteValue(0xd, pos);
                return;
            }
        }
        BattleEventVar_RewriteValue(0x41, 1);
    }
}

static void HandlerMeFirstPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_MulValue(0x31, 0x1800);
    }
}

static const BattleEventHandlerEntry sCopycatHandlers[] = {
    { 0x18, HandlerCopycat },
};

static const BattleEventHandlerEntry *EventAddCopycat(u32 *priority) {
    *priority = NELEMS(sCopycatHandlers);
    return sCopycatHandlers;
}

static void HandlerCopycat(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u16 move;
    u8 targetPos;

    if (BattleEventVar_GetValue(2) == monId) {
        move = func_ov167_021abc54(flow);
        if (move != 0 && !func_ov169_0689cbe4(move)) {
            targetPos = func_ov167_021abb70(flow, monId, move);
            BattleEventVar_RewriteValue(0x12, move);
            BattleEventVar_RewriteValue(0xd, targetPos);
        }
    }
}

static const BattleEventHandlerEntry sAncientPowerHandlers[] = {
    { 0x59, HandlerAncientPower },
};

static const BattleEventHandlerEntry *EventAddAncientPower(u32 *priority) {
    *priority = NELEMS(sAncientPowerHandlers);
    return sAncientPowerHandlers;
}

static void HandlerAncientPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(4) == monId && BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_RewriteValue(0x1f, 10);
    }
}

static const BattleEventHandlerEntry sVenoshockHandlers[] = {
    { 0x38, HandlerVenoshock },
};

static const BattleEventHandlerEntry *EventAddVenoshock(u32 *priority) {
    *priority = NELEMS(sVenoshockHandlers);
    return sVenoshockHandlers;
}

static void HandlerVenoshock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && CheckCondition(GetBattleMon(flow, BattleEventVar_GetValue(4)), 5)) {
        BattleEventVar_MulValue(0x31, 0x2000);
    }
}

static const BattleEventHandlerEntry sHexHandlers[] = {
    { 0x37, HandlerHex },
};

static const BattleEventHandlerEntry *EventAddHex(u32 *priority) {
    *priority = NELEMS(sHexHandlers);
    return sHexHandlers;
}

static void HandlerHex(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && GetBattleMonStatus(GetBattleMon(flow, BattleEventVar_GetValue(4)))) {
        MultiplyBasePower(2);
    }
}

static const BattleEventHandlerEntry sAcrobaticsHandlers[] = {
    { 0x37, HandlerAcrobatics },
};

static const BattleEventHandlerEntry *EventAddAcrobatics(u32 *priority) {
    *priority = NELEMS(sAcrobaticsHandlers);
    return sAcrobaticsHandlers;
}

static void HandlerAcrobatics(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && GetBattleMonHeldItem(GetBattleMon(flow, monId)) == 0) {
        MultiplyBasePower(2);
    }
}

static const BattleEventHandlerEntry sStoredPowerHandlers[] = {
    { 0x37, HandlerStoredPower },
};

static const BattleEventHandlerEntry *EventAddStoredPower(u32 *priority) {
    *priority = NELEMS(sStoredPowerHandlers);
    return sStoredPowerHandlers;
}

// The stats whose raised stages add to Stored Power's power
static void HandlerStoredPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    static const u8 sStoredPowerStats[] = { 1, 2, 5, 3, 4, 6, 7 };
    BattleMon *mon;
    int total;
    u32 i;
    int stage;
    u32 power;

    if (BattleEventVar_GetValue(3) == monId) {
        mon = GetBattleMon(flow, monId);
        total = 0;
        for (i = 0; i < NELEMS(sStoredPowerStats); i++) {
            stage = GetBattleMonStat(mon, sStoredPowerStats[i]) - 6;
            if (stage > 0) {
                total += stage;
            }
        }
        if (total != 0) {
            power = BattleEventVar_GetValue(0x30);
            power += total * 20;
            BattleEventVar_RewriteValue(0x30, power);
        }
    }
}

static const BattleEventHandlerEntry sHeavySlamHandlers[] = {
    { 0x37, HandlerHeavySlam },
};

static const BattleEventHandlerEntry *EventAddHeavySlam(u32 *priority) {
    *priority = NELEMS(sHeavySlamHandlers);
    return sHeavySlamHandlers;
}

static void HandlerHeavySlam(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *attacker;
    BattleMon *target;
    int ratio;
    u16 power;

    if (BattleEventVar_GetValue(3) == monId) {
        attacker = GetBattleMon(flow, monId);
        target = GetBattleMon(flow, BattleEventVar_GetValue(4));
        ratio = func_ov167_021abee0(flow, monId) / func_ov167_021abee0(flow, GetMonID(target));
        if (ratio >= 5) {
            power = 120;
        } else if (ratio == 4) {
            power = 100;
        } else if (ratio == 3) {
            power = 80;
        } else if (ratio == 2) {
            power = 60;
        } else {
            power = 40;
        }
        BattleEventVar_RewriteValue(0x30, power);
    }
}

static const BattleEventHandlerEntry sElectroBallHandlers[] = {
    { 0x37, HandlerElectroBall },
};

static const BattleEventHandlerEntry *EventAddElectroBall(u32 *priority) {
    *priority = NELEMS(sElectroBallHandlers);
    return sElectroBallHandlers;
}

static void HandlerElectroBall(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *attacker;
    BattleMon *target;
    int ratio;
    u16 power;

    if (BattleEventVar_GetValue(3) == monId) {
        attacker = GetBattleMon(flow, monId);
        target = GetBattleMon(flow, BattleEventVar_GetValue(4));
        ratio = func_ov167_021abd08(flow, attacker, FALSE) / func_ov167_021abd08(flow, target, FALSE);
        if (ratio >= 4) {
            power = 150;
        } else if (ratio >= 3) {
            power = 120;
        } else if (ratio >= 2) {
            power = 80;
        } else if (ratio >= 1) {
            power = 60;
        } else {
            power = 40;
        }
        BattleEventVar_RewriteValue(0x30, power);
    }
}

static const BattleEventHandlerEntry sEchoedVoiceHandlers[] = {
    { 0x37, HandlerEchoedVoice },
};

static const BattleEventHandlerEntry *EventAddEchoedVoice(u32 *priority) {
    *priority = NELEMS(sEchoedVoiceHandlers);
    return sEchoedVoiceHandlers;
}

static void HandlerEchoedVoice(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    void *data;
    u16 move;
    int turn;
    u32 count;
    u16 power;

    if (BattleEventVar_GetValue(3) == monId) {
        data = func_ov167_021abc6c(flow);
        move = BattleEventItem_GetSubID(item);
        turn = GetTurnCounter(flow) - 1;
        count = 0;
        while (turn >= 0) {
            if (!func_ov169_0689d1ec(data, move, turn--)) {
                break;
            }
            count++;
        }
        switch (count) {
        case 0:
            power = 40;
            break;
        case 1:
            power = 80;
            break;
        case 2:
            power = 120;
            break;
        case 3:
            power = 160;
            break;
        default:
            power = 200;
            break;
        }
        BattleEventVar_RewriteValue(0x30, power);
    }
}

static const BattleEventHandlerEntry sRetaliateHandlers[] = {
    { 0x38, HandlerRetaliate },
};

static const BattleEventHandlerEntry *EventAddRetaliate(u32 *priority) {
    *priority = NELEMS(sRetaliateHandlers);
    return sRetaliateHandlers;
}

static void HandlerRetaliate(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    void *data;
    u32 count;
    u8 i;

    if (BattleEventVar_GetValue(3) == monId) {
        data = func_ov167_021abc70(flow);
        count = func_ov169_0689d2fc(data, 1);
        for (i = 0; i < count; i++) {
            if (IsAllyMonID(monId, func_ov169_0689d30c(data, 1, i))) {
                BattleEventVar_MulValue(0x31, 0x2000);
                return;
            }
        }
    }
}

static const BattleEventHandlerEntry sFoulPlayHandlers[] = {
    { 0x39, HandlerFoulPlay },
};

static const BattleEventHandlerEntry *EventAddFoulPlay(u32 *priority) {
    *priority = NELEMS(sFoulPlayHandlers);
    return sFoulPlayHandlers;
}

static void HandlerFoulPlay(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(4);
        BattleEventVar_RewriteValue(0x3b, targetId);
    }
}

static const BattleEventHandlerEntry sSoakHandlers[] = {
    { 0xa0, HandlerSoak },
};

static const BattleEventHandlerEntry *EventAddSoak(u32 *priority) {
    *priority = NELEMS(sSoakHandlers);
    return sSoakHandlers;
}

static void HandlerSoak(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 count;
    u32 i;
    BattleHandlerChangeTypeParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        count = BattleEventVar_GetValue(5);
        for (i = 0; i < count; i++) {
            param = BattleHandler_PushWork(flow, 0x14, monId);
            param->type = func_ov167_021ce530(10);
            param->monIndex = BattleEventVar_GetValue(6 + i);
            BattleHandler_PopWork(flow, param);
        }
    }
}

static const BattleEventHandlerEntry sSimpleBeamHandlers[] = {
    { 0xa0, HandlerSimpleBeam },
};

static const BattleEventHandlerEntry *EventAddSimpleBeam(u32 *priority) {
    *priority = NELEMS(sSimpleBeamHandlers);
    return sSimpleBeamHandlers;
}

static void HandlerSimpleBeam(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 count;
    u32 i;
    BattleHandlerAbilityChangeParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        count = BattleEventVar_GetValue(5);
        for (i = 0; i < count; i++) {
            if (GetBattleMonStat(GetBattleMon(flow, BattleEventVar_GetValue(6 + i)), 0x10) != 0x36) {
                param = BattleHandler_PushWork(flow, 0x1f, monId);
                param->targetIndex = BattleEventVar_GetValue(6 + i);
                param->ability = 0x56;
                BattleHandler_StrSetup(&param->string, 2, 0x195);
                BattleHandler_AddArg(&param->string, param->targetIndex);
                BattleHandler_AddArg(&param->string, param->ability);
                BattleHandler_PopWork(flow, param);
            }
        }
    }
}

static const BattleEventHandlerEntry sEntrainmentHandlers[] = {
    { 0xa0, HandlerEntrainment },
};

static const BattleEventHandlerEntry *EventAddEntrainment(u32 *priority) {
    *priority = NELEMS(sEntrainmentHandlers);
    return sEntrainmentHandlers;
}

static void HandlerEntrainment(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u16 ability;
    u32 count;
    u32 i;
    u8 targetId;
    BattleHandlerAbilityChangeParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        ability = GetBattleMonStat(GetBattleMon(flow, monId), 0x10);
        if (!func_ov169_0689cac4(ability)) {
            count = BattleEventVar_GetValue(5);
            for (i = 0; i < count; i++) {
                targetId = BattleEventVar_GetValue(6 + i);
                if (GetBattleMonStat(GetBattleMon(flow, targetId), 0x10) != 0x36) {
                    param = BattleHandler_PushWork(flow, 0x1f, monId);
                    param->targetIndex = BattleEventVar_GetValue(6 + i);
                    param->ability = ability;
                    BattleHandler_StrSetup(&param->string, 2, 0x195);
                    BattleHandler_AddArg(&param->string, param->targetIndex);
                    BattleHandler_AddArg(&param->string, param->ability);
                    BattleHandler_PopWork(flow, param);
                }
            }
        }
    }
}

static const BattleEventHandlerEntry sClearSmogHandlers[] = {
    { 0x4b, HandlerClearSmog },
};

static const BattleEventHandlerEntry *EventAddClearSmog(u32 *priority) {
    *priority = NELEMS(sClearSmogHandlers);
    return sClearSmogHandlers;
}

static void HandlerClearSmog(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleHandlerResetStatStageParam *param;
    BattleHandlerMessageParam *message;

    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x46) == 0) {
        targetId = BattleEventVar_GetValue(4);

        param = BattleHandler_PushWork(flow, 0x10, monId);
        param->count = 1;
        param->monIndices[0] = targetId;
        BattleHandler_PopWork(flow, param);

        message = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&message->string, 2, 0xc3);
        BattleHandler_AddArg(&message->string, targetId);
        BattleHandler_PopWork(flow, message);
    }
}

static const BattleEventHandlerEntry sIncinerateHandlers[] = {
    { 0x4b, HandlerIncinerate },
};

static const BattleEventHandlerEntry *EventAddIncinerate(u32 *priority) {
    *priority = NELEMS(sIncinerateHandlers);
    return sIncinerateHandlers;
}

static void HandlerIncinerate(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    u16 berry;
    BattleHandlerSetItemParam *param;

    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x46) == 0) {
        targetId = BattleEventVar_GetValue(4);
        berry = GetBattleMonHeldItem(GetBattleMon(flow, targetId));
        if (PML_ItemIsBerry(berry)) {
            param = BattleHandler_PushWork(flow, 0x20, monId);
            param->targetIndex = targetId;
            param->item = 0;
            BattleHandler_StrSetup(&param->string, 2, 0x454);
            BattleHandler_AddArg(&param->string, targetId);
            BattleHandler_AddArg(&param->string, berry);
            BattleHandler_PopWork(flow, param);
        }
    }
}

static const BattleEventHandlerEntry sCircleThrowHandlers[] = {
    { 0x83, HandlerCircleThrow },
};

static const BattleEventHandlerEntry *EventAddCircleThrow(u32 *priority) {
    *priority = NELEMS(sCircleThrowHandlers);
    return sCircleThrowHandlers;
}

static void HandlerCircleThrow(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerForceSwitchParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        param = BattleHandler_PushWork(flow, 0x2e, monId);
        param->targetIndex = BattleEventVar_GetValue(6);
        param->effect = BattleEventItem_GetSubID(item) == MOVE_CIRCLE_THROW ? 0x28b : 0x28c;
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sSmackDownHandlers[] = {
    { 0x83, HandlerSmackDown },
    { 0x99, HandlerThunderFlyCheck },
};

static const BattleEventHandlerEntry *EventAddSmackDown(u32 *priority) {
    *priority = NELEMS(sSmackDownHandlers);
    return sSmackDownHandlers;
}

static void HandlerSmackDown(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *target;
    BOOL grounded;
    BattleHandlerAddConditionParam *param;
    BattleHandlerCureConditionParam *cure;
    BattleHandlerHideTurnParam *cancel;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        target = GetBattleMon(flow, targetId);
        grounded = FALSE;
        if (!func_ov167_021abeb4(flow, targetId)) {
            if (func_ov167_021abd74(flow, targetId) && !CheckCondition(target, 0x21)) {
                param = BattleHandler_PushWork(flow, 0xc, monId);
                param->targetIndex = targetId;
                param->condition = 0x1f;
                param->value = MakeConditionPermanent();
                BattleHandler_StrSetup(&param->string, 2, 0x468);
                BattleHandler_AddArg(&param->string, targetId);
                BattleHandler_PopWork(flow, param);
                grounded = TRUE;

                if (CheckCondition(target, 0x1e)) {
                    cure = BattleHandler_PushWork(flow, 0xb, monId);
                    cure->count = 1;
                    cure->monIds[0] = targetId;
                    cure->useString = 1;
                    cure->condition = 0x1e;
                    BattleHandler_PopWork(flow, cure);
                }
                if (CheckCondition(target, 0x20)) {
                    cure = BattleHandler_PushWork(flow, 0xb, monId);
                    cure->count = 1;
                    cure->monIds[0] = targetId;
                    cure->useString = 1;
                    cure->condition = 0x20;
                    BattleHandler_PopWork(flow, cure);
                }
            }
            if (GetAdditionalConditionFlag(target, 3)) {
                cancel = BattleHandler_PushWork(flow, 0x36, monId);
                cancel->monIndex = targetId;
                cancel->flag = 3;
                if (!grounded) {
                    BattleHandler_StrSetup(&cancel->string, 2, 0x468);
                    BattleHandler_AddArg(&cancel->string, targetId);
                }
                BattleHandler_PopWork(flow, cancel);
            }
        }
    }
}

static const BattleEventHandlerEntry sShellSmashHandlers[] = {
    { 0xa0, HandlerShellSmash },
};

static const BattleEventHandlerEntry *EventAddShellSmash(u32 *priority) {
    *priority = NELEMS(sShellSmashHandlers);
    return sShellSmashHandlers;
}

static void HandlerShellSmash(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerStatChangeParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        param = BattleHandler_PushWork(flow, 0xe, monId);
        param->stat = 2;
        param->change = -1;
        param->count = 1;
        param->monIds[0] = monId;
        param->unk0e = 1;
        BattleHandler_PopWork(flow, param);

        param = BattleHandler_PushWork(flow, 0xe, monId);
        param->stat = 4;
        param->change = -1;
        param->count = 1;
        param->monIds[0] = monId;
        param->unk0e = 1;
        BattleHandler_PopWork(flow, param);

        param = BattleHandler_PushWork(flow, 0xe, monId);
        param->stat = 1;
        param->change = 2;
        param->count = 1;
        param->monIds[0] = monId;
        param->unk0e = 1;
        BattleHandler_PopWork(flow, param);

        param = BattleHandler_PushWork(flow, 0xe, monId);
        param->stat = 3;
        param->change = 2;
        param->count = 1;
        param->monIds[0] = monId;
        param->unk0e = 1;
        BattleHandler_PopWork(flow, param);

        param = BattleHandler_PushWork(flow, 0xe, monId);
        param->stat = 5;
        param->change = 2;
        param->count = 1;
        param->monIds[0] = monId;
        param->unk0e = 1;
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sReflectTypeHandlers[] = {
    { 0xa0, HandlerReflectType },
};

static const BattleEventHandlerEntry *EventAddReflectType(u32 *priority) {
    *priority = NELEMS(sReflectTypeHandlers);
    return sReflectTypeHandlers;
}

static void HandlerReflectType(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *target;
    BattleHandlerChangeTypeParam *param;
    BattleHandlerMessageParam *message;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        target = GetBattleMon(flow, targetId);

        param = BattleHandler_PushWork(flow, 0x14, monId);
        param->monIndex = monId;
        param->type = GetPokeType(target);
        param->suppressMessage = 1;
        BattleHandler_PopWork(flow, param);

        message = BattleHandler_PushWork(flow, 4, monId);
        message->checkPrevResult = TRUE;
        BattleHandler_StrSetup(&message->string, 2, 0x441);
        BattleHandler_AddArg(&message->string, monId);
        BattleHandler_AddArg(&message->string, targetId);
        BattleHandler_PopWork(flow, message);
    }
}

static const BattleEventHandlerEntry sAutotomizeHandlers[] = {
    { 0x5e, HandlerAutotomize },
};

static const BattleEventHandlerEntry *EventAddAutotomize(u32 *priority) {
    *priority = NELEMS(sAutotomizeHandlers);
    return sAutotomizeHandlers;
}

static void HandlerAutotomize(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 weight;
    BattleHandlerSetWeightParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        weight = GetBattleMonWeight(GetBattleMon(flow, monId));
        if (weight > 1) {
            param = BattleHandler_PushWork(flow, 0x2d, monId);
            if (weight > 1001) {
                weight -= 1000;
            } else {
                weight = 1;
            }
            param->monIndex = monId;
            param->weight = weight;
            BattleHandler_StrSetup(&param->string, 2, 0x44e);
            BattleHandler_AddArg(&param->string, monId);
            BattleHandler_PopWork(flow, param);
        }
    }
}

static const BattleEventHandlerEntry sPsyshockHandlers[] = {
    { 0x3a, HandlerPsyshock },
};

static const BattleEventHandlerEntry *EventAddPsyshock(u32 *priority) {
    *priority = NELEMS(sPsyshockHandlers);
    return sPsyshockHandlers;
}

static void HandlerPsyshock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && BattleEventVar_GetValue(0x3c) == 0xb) {
        BattleEventVar_RewriteValue(0x3d, BattleEventVar_GetValue(0x3d) + 1);
    }
}

static const BattleEventHandlerEntry sChipAwayHandlers[] = {
    { 0x3a, HandlerChipAwayCalcDamage },
    { 0x33, HandlerChipAwayHitCheck },
};

static const BattleEventHandlerEntry *EventAddChipAway(u32 *priority) {
    *priority = NELEMS(sChipAwayHandlers);
    return sChipAwayHandlers;
}

static void HandlerChipAwayCalcDamage(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_RewriteValue(0x51, 1);
    }
}

static void HandlerChipAwayHitCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_RewriteValue(0x4b, 1);
    }
}

static const BattleEventHandlerEntry sWonderRoomHandlers[] = {
    { 0x9e, HandlerWonderRoom },
};

static const BattleEventHandlerEntry *EventAddWonderRoom(u32 *priority) {
    *priority = NELEMS(sWonderRoomHandlers);
    return sWonderRoomHandlers;
}

static void HandlerWonderRoom(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerAddFieldEffectParam *param;
    BattleHandlerRemoveFieldEffectParam *removeParam;

    if (BattleEventVar_GetValue(3) == monId) {
        if (!IsFieldEffectActive(6)) {
            param = BattleHandler_PushWork(flow, 0x1b, monId);
            param->effect = 6;
            param->value = SetConditionTurns(5);
            BattleHandler_StrSetup(&param->string, 1, 0xb2);
            BattleHandler_PopWork(flow, param);
        } else {
            removeParam = BattleHandler_PushWork(flow, 0x1c, monId);
            removeParam->effect = 6;
            BattleHandler_PopWork(flow, removeParam);
        }
    }
}

static const BattleEventHandlerEntry sMagicRoomHandlers[] = {
    { 0x9e, HandlerMagicRoom },
};

static const BattleEventHandlerEntry *EventAddMagicRoom(u32 *priority) {
    *priority = NELEMS(sMagicRoomHandlers);
    return sMagicRoomHandlers;
}

static void HandlerMagicRoom(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerAddFieldEffectParam *param;
    BattleHandlerRemoveFieldEffectParam *removeParam;

    if (BattleEventVar_GetValue(3) == monId) {
        if (!IsFieldEffectActive(7)) {
            param = BattleHandler_PushWork(flow, 0x1b, monId);
            param->effect = 7;
            param->value = SetConditionTurns(5);
            BattleHandler_StrSetup(&param->string, 1, 0xb4);
            BattleHandler_PopWork(flow, param);
        } else {
            removeParam = BattleHandler_PushWork(flow, 0x1c, monId);
            removeParam->effect = 7;
            BattleHandler_PopWork(flow, removeParam);
        }
    }
}

static const BattleEventHandlerEntry sFlameBurstHandlers[] = {
    { 0x4b, HandlerFlameBurst },
};

static const BattleEventHandlerEntry *EventAddFlameBurst(u32 *priority) {
    *priority = NELEMS(sFlameBurstHandlers);
    return sFlameBurstHandlers;
}

static void HandlerFlameBurst(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    u8 pos;
    u8 count;
    u8 i;
    BattleMon *mon;
    BattleHandlerDamageParam *param;
    u8 targets[3];

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(4);
        pos = func_ov167_021ab874(flow, targetId);
        if (pos != 6) {
            count = HandlerGetAlivePartyCount(flow, 0x900 | pos, targets);
            for (i = 0; i < count; i++) {
                if (targets[i] != targetId) {
                    mon = GetBattleMon(flow, targets[i]);
                    param = BattleHandler_PushWork(flow, 7, monId);
                    param->targetIndex = targets[i];
                    param->amount = DivideMaxHPZeroCheck(mon, 16);
                    param->checkSemi = 1;
                    BattleHandler_StrSetup(&param->string, 2, 0x451);
                    BattleHandler_AddArg(&param->string, param->targetIndex);
                    BattleHandler_PopWork(flow, param);
                }
            }
        }
    }
}

static const BattleEventHandlerEntry sSynchronoiseHandlers[] = {
    { 0x2c, HandlerSynchronoise },
};

static const BattleEventHandlerEntry *EventAddSynchronoise(u32 *priority) {
    *priority = NELEMS(sSynchronoiseHandlers);
    return sSynchronoiseHandlers;
}

static void HandlerSynchronoise(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *target;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(4);
        target = GetBattleMon(flow, targetId);
        if (!func_ov167_021ce588(GetPokeType(GetBattleMon(flow, monId)), GetPokeType(target))) {
            BattleEventVar_RewriteValue(0x40, 1);
        }
    }
}

static const BattleEventHandlerEntry sBestowHandlers[] = {
    { 0xa0, HandlerBestow },
};

static const BattleEventHandlerEntry *EventAddBestow(u32 *priority) {
    *priority = NELEMS(sBestowHandlers);
    return sBestowHandlers;
}

static void HandlerBestow(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *target;
    u16 heldItem;
    BattleHandlerSetItemParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        target = GetBattleMon(flow, targetId);
        heldItem = GetBattleMonHeldItem(GetBattleMon(flow, monId));
        if (heldItem != 0 && !PML_ItemIsMail(heldItem) && GetBattleMonHeldItem(target) == 0 &&
            !func_ov167_021cdedc(flow, monId) && !func_ov167_021cdedc(flow, targetId) &&
            !GiratinaArceusGenesectItemCheck(GetBattleMonSpecies(target), heldItem)) {
            param = BattleHandler_PushWork(flow, 0x20, monId);
            param->targetIndex = monId;
            param->item = 0;
            BattleHandler_PopWork(flow, param);

            param = BattleHandler_PushWork(flow, 0x20, monId);
            param->targetIndex = targetId;
            param->item = heldItem;
            BattleHandler_StrSetup(&param->string, 2, 0x457);
            BattleHandler_AddArg(&param->string, targetId);
            BattleHandler_AddArg(&param->string, monId);
            BattleHandler_AddArg(&param->string, heldItem);
            BattleHandler_PopWork(flow, param);
        }
    }
}

static const BattleEventHandlerEntry sFinalGambitHandlers[] = {
    { 0x46, HandlerFinalGambit },
    { 0x45, HandlerExplosionDamageDetermine },
};

static const BattleEventHandlerEntry *EventAddFinalGambit(u32 *priority) {
    *priority = NELEMS(sFinalGambitHandlers);
    return sFinalGambitHandlers;
}

static void HandlerFinalGambit(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId) {
        BattleEventVar_RewriteValue(0x37, GetBattleMonStat(GetBattleMon(flow, monId), 0xd));
    }
}

static const BattleEventHandlerEntry sAfterYouHandlers[] = {
    { 0xa0, HandlerAfterYou },
};

static const BattleEventHandlerEntry *EventAddAfterYou(u32 *priority) {
    *priority = NELEMS(sAfterYouHandlers);
    return sAfterYouHandlers;
}

static void HandlerAfterYou(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerInterruptParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        param = BattleHandler_PushWork(flow, 0x2f, monId);
        param->monId = BattleEventVar_GetValue(6);
        BattleHandler_StrSetup(&param->string, 2, 0x46e);
        BattleHandler_AddArg(&param->string, param->monId);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sQuashHandlers[] = {
    { 0xa0, HandlerQuash },
};

static const BattleEventHandlerEntry *EventAddQuash(u32 *priority) {
    *priority = NELEMS(sQuashHandlers);
    return sQuashHandlers;
}

static void HandlerQuash(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerInterruptParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        param = BattleHandler_PushWork(flow, 0x31, monId);
        param->monId = BattleEventVar_GetValue(6);
        BattleHandler_StrSetup(&param->string, 2, 0x46b);
        BattleHandler_AddArg(&param->string, param->monId);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sRoundHandlers[] = {
    { 0x24, HandlerRound },
    { 0x37, HandlerRoundPower },
};

static const BattleEventHandlerEntry *EventAddRound(u32 *priority) {
    *priority = NELEMS(sRoundHandlers);
    return sRoundHandlers;
}

static void HandlerRound(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    void *data;
    u16 move;
    BattleHandlerInterruptParam *param;

    if (BattleEventVar_GetValue(2) == monId) {
        data = func_ov167_021abc6c(flow);
        move = BattleEventItem_GetSubID(item);
        if (GetUsedMoveCount(data, move, GetTurnCounter(flow)) == 1) {
            param = BattleHandler_PushWork(flow, 0x30, monId);
            param->moveId = move;
            BattleHandler_PopWork(flow, param);
        }
    }
}

static void HandlerRoundPower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    void *data;
    u16 move;

    if (BattleEventVar_GetValue(3) == monId) {
        data = func_ov167_021abc6c(flow);
        move = BattleEventItem_GetSubID(item);
        if (GetUsedMoveCount(data, move, GetTurnCounter(flow)) > 1) {
            MultiplyBasePower(2);
        }
    }
}

static const BattleEventHandlerEntry sQuickGuardHandlers[] = {
    { 0x3, HandlerProtectStart },
    { 0x1f, HandlerProtectCheckFail },
    { 0x21, HandlerProtectResetCounter },
    { 0xa1, HandlerQuickGuard },
};

static const BattleEventHandlerEntry *EventAddQuickGuard(u32 *priority) {
    *priority = NELEMS(sQuickGuardHandlers);
    return sQuickGuardHandlers;
}

static void HandlerQuickGuard(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleCondition cont;

    cont = SetConditionTurns(1);
    CommonCreateSideEffect(item, flow, monId, work, GetSideFromMonID(monId), 10, cont, 0xa2);
    IncrementProtectCounter(flow, monId, TRUE);
}

static const BattleEventHandlerEntry sAllySwitchHandlers[] = {
    { 0xa0, HandlerAllySwitch },
};

static const BattleEventHandlerEntry *EventAddAllySwitch(u32 *priority) {
    *priority = NELEMS(sAllySwitchHandlers);
    return sAllySwitchHandlers;
}

static void HandlerAllySwitch(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 pos;
    u8 allyId;
    u8 allyPos;
    BattleHandlerSwapPokeParam *param;

    pos = func_ov167_021abb50(flow, monId);
    allyId = monId;
    switch (func_ov167_021abc9c(flow)) {
    case 1:
        if (func_ov167_021abe40(flow, func_ov167_0219c648(monId)) == 4) {
            allyPos = GetPosOnSameSide(pos, 0);
            if (allyPos == pos) {
                allyPos = GetPosOnSameSide(pos, 1);
            }
            allyId = func_ov167_021abb60(flow, allyPos);
        }
        break;
    case 2:
        if (!func_ov167_0219d2cc(pos)) {
            allyPos = GetPosOnSameSide(pos, 0);
            if (allyPos == pos) {
                allyPos = GetPosOnSameSide(pos, 2);
            }
            allyId = func_ov167_021abb60(flow, allyPos);
        }
        break;
    }
    if (allyId != monId) {
        param = BattleHandler_PushWork(flow, 0x32, monId);
        param->firstMonIndex = monId;
        param->secondMonIndex = allyId;
        BattleHandler_StrSetup(&param->string, 2, 0x471);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_AddArg(&param->string, allyId);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sTelekinesisHandlers[] = {
    { 0x2d, HandlerTelekinesisCheckFail },
    { 0x60, HandlerTelekinesis },
};

static const BattleEventHandlerEntry *EventAddTelekinesis(u32 *priority) {
    *priority = NELEMS(sTelekinesisHandlers);
    return sTelekinesisHandlers;
}

static void HandlerTelekinesisCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BattleMon *target;
    u16 species;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(4);
        target = GetBattleMon(flow, targetId);
        species = GetBattleMonSpecies(target);
        if (species == SPECIES_DIGLETT || species == SPECIES_DUGTRIO || CheckCondition(target, 0x1f) ||
            CheckCondition(target, 0x15)) {
            BattleEventVar_RewriteValue(0x40, 1);
        }
    }
}

static void HandlerTelekinesis(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerString *string;

    if (BattleEventVar_GetValue(3) == monId) {
        string = (BattleHandlerString *)BattleEventVar_GetValue(0x3f);
        BattleEventVar_RewriteValue(0x1d, 0x20);
        BattleHandler_StrSetup(string, 2, 0x474);
        BattleHandler_AddArg(string, BattleEventVar_GetValue(4));
    }
}

static const BattleEventHandlerEntry sSkyDropHandlers[] = {
    { 0x93, HandlerSkyDropGrabFail }, { 0x95, HandlerSkyDropGrab },      { 0x98, HandlerSkyDropRelease },
    { 0x2a, HandlerSkyDropTarget },   { 0x1f, HandlerSkyDropCheckFail }, { 0x2c, HandlerSkyDropTypeCheck },
};

static const BattleEventHandlerEntry *EventAddSkyDrop(u32 *priority) {
    *priority = NELEMS(sSkyDropHandlers);
    return sSkyDropHandlers;
}

static void HandlerSkyDropGrabFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(4);
        if (targetId != 0x1f && IsAllyMonID(monId, targetId)) {
            BattleEventVar_RewriteValue(0x41, 1);
        }
    }
}

static void HandlerSkyDropGrab(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;
    BOOL failed;
    BattleHandlerMessageParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BattleEventVar_GetValue(6);
        failed = FALSE;
        if (targetId == 0x1f || !func_ov167_021abfac(flow, monId, targetId, &failed)) {
            BattleEventVar_RewriteValue(0x41, 1);
            if (failed) {
                BattleEventVar_RewriteValue(0x4f, 1);
            }
        } else if (BattleEventVar_RewriteValue(0x51, 1)) {
            param = BattleHandler_PushWork(flow, 4, monId);
            BattleHandler_StrSetup(&param->string, 2, 0x45e);
            BattleHandler_AddArg(&param->string, monId);
            BattleHandler_AddArg(&param->string, targetId);
            BattleHandler_PopWork(flow, param);
        }
    }
}

static void HandlerSkyDropRelease(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;

    if (BattleEventVar_GetValue(3) == monId) {
        targetId = BtlFlow_GetSkyDropTarget(GetBattleMon(flow, monId));
        if (targetId == 0x1f || func_ov167_021ab840(flow, targetId) == 6) {
            BattleEventVar_RewriteValue(0x41, 1);
        }
        func_ov167_021abfd4(flow, monId);
    }
}

static void HandlerSkyDropTarget(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;

    if (func_ov167_021abc9c(flow) != 0 && func_ov167_021abc9c(flow) != 3) {
        targetId = BtlFlow_GetSkyDropTarget(GetBattleMon(flow, monId));
        if (targetId != 0x1f) {
            BattleEventVar_RewriteValue(4, targetId);
        }
    }
}

static void HandlerSkyDropCheckFail(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u8 targetId;

    if (BattleEventVar_GetValue(2) == monId) {
        targetId = BtlFlow_GetSkyDropTarget(GetBattleMon(flow, monId));
        if (targetId != 0x1f && !func_ov167_021abc8c(flow, targetId)) {
            BattleEventVar_RewriteValue(0x22, 0x1a);
        }
    }
}

static void HandlerSkyDropTypeCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && DoesMonHaveType(GetBattleMon(flow, BattleEventVar_GetValue(4)), 2)) {
        BattleEventVar_RewriteValue(0x40, 1);
    }
}

static const BattleEventHandlerEntry sRelicSongHandlers[] = {
    { 0x85, HandlerRelicSong },
};

static const BattleEventHandlerEntry *EventAddRelicSong(u32 *priority) {
    *priority = NELEMS(sRelicSongHandlers);
    return sRelicSongHandlers;
}

static void HandlerRelicSong(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleMon *mon;
    u8 form;
    BattleHandlerChangeFormParam *param;

    mon = GetBattleMon(flow, monId);
    if (GetBattleMonSpecies(mon) == SPECIES_MELOETTA && BattleEventVar_GetValue(3) == monId) {
        form = GetBattleMonStat(mon, 0x13) == 0 ? 1 : 0;
        param = BattleHandler_PushWork(flow, 0x39, monId);
        param->monIndex = monId;
        param->form = form;
        BattleHandler_StrSetup(&param->string, 2, 0xde);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sForesightHandlers[] = {
    { 0x62, HandlerForesight },
    { 0x5, HandlerBypassSubstitute },
};

static const BattleEventHandlerEntry *EventAddForesight(u32 *priority) {
    *priority = NELEMS(sForesightHandlers);
    return sForesightHandlers;
}

static void HandlerForesight(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleCondition cont;

    if (BattleEventVar_GetValue(3) == monId) {
        cont.raw = BattleEventVar_GetValue(0x1e);
        func_ov167_021ce368(&cont, 7);
        BattleEventVar_RewriteValue(0x1e, cont.raw);
    }
}

static const BattleEventHandlerEntry sMiracleEyeHandlers[] = {
    { 0x62, HandlerMiracleEye },
    { 0x5, HandlerBypassSubstitute },
};

static const BattleEventHandlerEntry *EventAddMiracleEye(u32 *priority) {
    *priority = NELEMS(sMiracleEyeHandlers);
    return sMiracleEyeHandlers;
}

static void HandlerMiracleEye(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleCondition cont;

    if (BattleEventVar_GetValue(3) == monId) {
        cont.raw = BattleEventVar_GetValue(0x1e);
        func_ov167_021ce368(&cont, 16);
        BattleEventVar_RewriteValue(0x1e, cont.raw);
    }
}

static const BattleEventHandlerEntry sGrowthHandlers[] = {
    { 0x59, HandlerGrowth },
};

static const BattleEventHandlerEntry *EventAddGrowth(u32 *priority) {
    *priority = NELEMS(sGrowthHandlers);
    return sGrowthHandlers;
}

static void HandlerGrowth(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    int volume;

    if (BattleEventVar_GetValue(3) == monId && GetWeather(flow) == 1) {
        volume = BattleEventVar_GetValue(0x20);
        if (volume == 1) {
            volume++;
        }
        BattleEventVar_RewriteValue(0x20, volume);
    }
}

static const BattleEventHandlerEntry sFreezeShockHandlers[] = {
    { 0x95, HandlerFreezeShock },
};

static const BattleEventHandlerEntry *EventAddFreezeShock(u32 *priority) {
    *priority = NELEMS(sFreezeShockHandlers);
    return sFreezeShockHandlers;
}

static void HandlerFreezeShock(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 message;
    BattleHandlerMessageParam *param;

    if (BattleEventVar_GetValue(3) == monId) {
        message = BattleEventItem_GetSubID(item) == MOVE_FREEZE_SHOCK ? 0x35f : 0x362;
        param = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&param->string, 2, message);
        BattleHandler_AddArg(&param->string, monId);
        BattleHandler_PopWork(flow, param);
    }
}

static const BattleEventHandlerEntry sFusionFlareHandlers[] = {
    { 0x38, HandlerFusionFlare },
};

static const BattleEventHandlerEntry *EventAddFusionFlare(u32 *priority) {
    *priority = NELEMS(sFusionFlareHandlers);
    return sFusionFlareHandlers;
}

static void HandlerFusionFlare(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    void *data;
    u16 partner;

    if (BattleEventVar_GetValue(3) == monId) {
        data = func_ov167_021abc6c(flow);
        switch (BattleEventItem_GetSubID(item)) {
        case MOVE_FUSION_FLARE:
            partner = MOVE_FUSION_BOLT;
            break;
        case MOVE_FUSION_BOLT:
            partner = MOVE_FUSION_FLARE;
            break;
        default:
            partner = MOVE_NONE;
            break;
        }
        if (partner != MOVE_NONE && partner == func_ov169_0689d250(data, GetTurnCounter(flow))) {
            BattleEventVar_MulValue(0x31, 0x2000);
            SetMoveEffectIndex(flow, 1);
        }
    }
}

// Two pledge moves used together, and the effect of the combination
typedef struct PledgeCombo {
    u16 move1;
    u16 move2;
    u32 effect;
} PledgeCombo;

static u32 func_ov167_021cdbc4(u16 move, u16 partnerMove) {
    static const PledgeCombo sPledgeCombos[] = {
        { MOVE_WATER_PLEDGE, MOVE_FIRE_PLEDGE, 1 },
        { MOVE_GRASS_PLEDGE, MOVE_FIRE_PLEDGE, 2 },
        { MOVE_WATER_PLEDGE, MOVE_GRASS_PLEDGE, 3 },
    };
    u32 i;

    for (i = 0; i < NELEMS(sPledgeCombos); i++) {
        if ((move == sPledgeCombos[i].move1 && partnerMove == sPledgeCombos[i].move2) ||
            (partnerMove == sPledgeCombos[i].move1 && move == sPledgeCombos[i].move2)) {
            return sPledgeCombos[i].effect;
        }
    }
    return 0;
}

static const BattleEventHandlerEntry sWaterPledgeHandlers[] = {
    { 0xa2, HandlerWaterPledgeCheck },        { 0x23, HandlerWaterPledgeDecide },
    { 0x40, HandlerWaterPledgeTypeMatch },    { 0x37, HandlerWaterPledgePower },
    { 0x24, HandlerWaterPledgeChangeEffect }, { 0x85, HandlerWaterPledgeFieldEffect },
};

static const BattleEventHandlerEntry *EventAddWaterPledge(u32 *priority) {
    *priority = NELEMS(sWaterPledgeHandlers);
    return sWaterPledgeHandlers;
}

static void HandlerWaterPledgeCheck(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u16 partnerMove;
    u8 partnerId;
    u32 combo;
    u8 type;

    if (BattleEventVar_GetValue(3) == monId &&
        func_ov167_021bc650(GetBattleMon(flow, monId), &partnerId, &partnerMove)) {
        combo = func_ov167_021cdbc4(BattleEventItem_GetSubID(item), partnerMove);
        type = BattleEventVar_GetValue(0x16);
        if (combo != 0) {
            work[1] = combo;
            work[0] = 1;
            work[2] = partnerId;
            switch (combo) {
            case 1:
                type = 10;
                break;
            case 2:
                type = 9;
                break;
            case 3:
                type = 11;
                break;
            }
            BattleEventVar_RewriteValue(0x16, type);
        }
    }
}

static void HandlerWaterPledgeDecide(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    BattleHandlerMessageParam *param;

    if (BattleEventVar_GetValue(3) == monId && work[0]) {
        param = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&param->string, 1, 0xbb);
        BattleHandler_PopWork(flow, param);
    }
}

static void HandlerWaterPledgeTypeMatch(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(2) == monId && work[0]) {
        BattleEventVar_RewriteValue(0x51, 1);
    }
}

static void HandlerWaterPledgePower(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    if (BattleEventVar_GetValue(3) == monId && work[0]) {
        BattleEventVar_RewriteValue(0x30, 150);
    }
}

static void HandlerWaterPledgeChangeEffect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u16 move;

    if (BattleEventVar_GetValue(2) == monId && work[0]) {
        move = MOVE_NONE;
        switch (work[1]) {
        case 1:
            move = MOVE_WATER_PLEDGE;
            break;
        case 2:
            move = MOVE_FIRE_PLEDGE;
            break;
        case 3:
            move = MOVE_GRASS_PLEDGE;
            break;
        }
        BattleEventVar_RewriteValue(0x13, move);
    }
}

static void HandlerWaterPledgeFieldEffect(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work) {
    u32 effect;
    u8 side;
    u8 effectSide;
    u16 posEffect;
    u16 message;
    u8 targetSide;
    u8 posMonId;
    u8 pos;
    BattleHandlerAddSideEffectParam *param;
    BattleHandlerEffectAtPosParam *posParam;

    if (BattleEventVar_GetValue(3) == monId && work[0]) {
        effect = 0xe;
        side = GetSideFromMonID(monId);
        targetSide = GetSideFromMonID(BattleEventVar_GetValue(6));
        posEffect = 0;
        message = 0;
        posMonId = 0x1f;
        switch (work[1]) {
        case 1:
            message = 0xa4;
            effectSide = side;
            effect = 0xb;
            posMonId = monId;
            posEffect = 0x28d;
            break;
        case 2:
            message = 0xa8;
            effectSide = targetSide;
            effect = 0xc;
            posEffect = 0x28e;
            posMonId = BattleEventVar_GetValue(6);
            break;
        case 3:
            message = 0xac;
            effectSide = targetSide;
            effect = 0xd;
            posEffect = 0x28f;
            posMonId = BattleEventVar_GetValue(6);
            break;
        }
        if (effect != 0xe) {
            param = BattleHandler_PushWork(flow, 0x19, monId);
            param->effect = effect;
            param->side = effectSide;
            param->cont = SetConditionTurns(4);
            BattleHandler_StrSetup(&param->string, 1, message);
            BattleHandler_AddArg(&param->string, effectSide);
            BattleHandler_PopWork(flow, param);

            if (posMonId != 0x1f) {
                pos = func_ov167_021ab874(flow, posMonId);
                if (pos != 6) {
                    posParam = BattleHandler_PushWork(flow, 0x37, monId);
                    posParam->pos1 = pos;
                    posParam->pos2 = 6;
                    posParam->effect = posEffect;
                    BattleHandler_PopWork(flow, posParam);
                }
            }
        }
    }
}
