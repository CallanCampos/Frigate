#include "asm/field_script.inc"
#include "text/script/nimbasa_city_14.h"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "The Battle Test...[f000]븁\u0000\nThis woman of mystery will show you\nthe ropes.[f000]븁\u0000\nBattle effectively, and keep your\nPokémon from fainting!"
    ParentActorMsg MSGFILE_SCRIPT, NimbasaCity14_Text_BattleTestWomanMystery, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
