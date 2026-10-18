#include "asm/field_script.inc"
#include "text/script/nacrene_city_3.h"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
    ScriptEntry Script_10
    ScriptEntry Script_11
    ScriptEntry Script_12
    ScriptEntry Script_13
    ScriptEntry Script_14
    ScriptEntriesEnd

Script_14:
    FlagSet EVENT_FLAG_0x01ef
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, hello!\nSorry, I don't have any Fresh Water.[f000]븁\u0000\nThis isn't a Pokémon Gym anymore.\nIt's a library now!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity3_Text_OhHelloSorryDont, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "This is the bookshelf\nfor “General Studies.\"[f000]븁\u0000"
    InfoMsg NacreneCity3_Text_BookshelfGeneralStudies, 2
    // "“Changing Unova\"\nDo you want to read this book?"
    InfoMsg NacreneCity3_Text_ChangingUnovaWantRead, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0090
    // "This book is about how much the\nUnova region has changed in two years.[f000]븁\u0000\nEach year, many people move to Unova,\nso the environment and cities change[f000]븀\u0000\nat a dizzying pace."
    InfoMsg NacreneCity3_Text_BookAboutHowMuch, 2
    LastKeyWait
    VMJump L_0090

L_0090:
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "This is the bookshelf\nfor “General Studies.\"[f000]븁\u0000"
    InfoMsg NacreneCity3_Text_BookshelfGeneralStudies, 2
    // "“The Joy of Rides\"\nDo you want to read this book?"
    InfoMsg NacreneCity3_Text_JoyRidesWantRead, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00CC
    // "This book is about the things you\ncan ride in the Unova region,[f000]븀\u0000\nsuch as Castelia City's cruise ship[f000]븀\u0000\nand Mistralton City's planes.[f000]븁\u0000\nBefore there were planes, locomotives\ncarried people all over Unova.[f000]븁\u0000\nThe railway in Nacrene City is\na legacy of those lines."
    InfoMsg NacreneCity3_Text_BookAboutThingsCan, 2
    LastKeyWait
    VMJump L_00CC

L_00CC:
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "This is the bookshelf\nfor “General Studies.\"[f000]븁\u0000"
    InfoMsg NacreneCity3_Text_BookshelfGeneralStudies, 2
    // "“Five Bridges\"\nDo you want to read this book?"
    InfoMsg NacreneCity3_Text_FiveBridgesWantRead, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0108
    // "This is a book a Backpacker wrote\nabout visiting the Unova region's[f000]븀\u0000\nfive famous bridges.[f000]븁\u0000\nThere are around 200 pages dedicated\nto the charms of drinking Lemonade[f000]븀\u0000\nand watching the cruise ship[f000]븀\u0000\nfrom the Skyarrow Bridge at sunset."
    InfoMsg NacreneCity3_Text_BookBackpackerWroteAbout, 2
    LastKeyWait
    VMJump L_0108

L_0108:
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "This is the bookshelf\nfor “General Studies.\"[f000]븁\u0000"
    InfoMsg NacreneCity3_Text_BookshelfGeneralStudies, 2
    // "“Unova Gourmet\"\nDo you want to read this book?"
    InfoMsg NacreneCity3_Text_UnovaGourmetWantRead, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0144
    // "This is a magazine about the\ngourmet foods in the Unova region.[f000]븁\u0000\nPeople in the know start out with\nMoomoo Milk and a Village Sandwich.[f000]븁\u0000\nThen, to finish off the meal, a\nclassic favorite, a Casteliacone!"
    InfoMsg NacreneCity3_Text_MagazineAboutGourmetFoods, 2
    LastKeyWait
    VMJump L_0144

L_0144:
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "This is the bookshelf\nfor “Pokémon.\"[f000]븁\u0000"
    InfoMsg NacreneCity3_Text_BookshelfPokemon, 2
    // "“Bones, Fossils, and Us\"\nDo you want to read this book?"
    InfoMsg NacreneCity3_Text_BonesFossilsUsWant, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0180
    // "This book collects research on\nPokémon bones and fossils.[f000]븁\u0000\nNot only can we learn about ancient\nPokémon's bodies from bones and fossils,[f000]븀\u0000\nwe can learn about how people of ancient[f000]븀\u0000\ntimes interacted with Pokémon.[f000]븁\u0000\nTo ancient people, Pokémon were a sacred\npresence. They were revered to a much[f000]븀\u0000\ngreater extent than happens today."
    InfoMsg NacreneCity3_Text_BookCollectsResearchPokemon, 2
    LastKeyWait
    VMJump L_0180

L_0180:
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "This is the bookshelf\nfor “Pokémon.\"[f000]븁\u0000"
    InfoMsg NacreneCity3_Text_BookshelfPokemon, 2
    // "“Pokémon and Work\"\nDo you want to read this book?"
    InfoMsg NacreneCity3_Text_PokemonWorkWantRead, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01BC
    // "This book introduces the various Pokémon\nthat help out in the world of work.[f000]븁\u0000\nFor a potter to fire plates and pots in a\nkiln, the temperature must reach 600° F.[f000]븁\u0000\nThis is where Pansear can help![f000]븁\u0000\nIt increases the temperature in its\nprized tuft and fires the pottery!"
    InfoMsg NacreneCity3_Text_BookIntroducesVariousPokemon, 2
    LastKeyWait
    VMJump L_01BC

L_01BC:
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "This is the bookshelf\nfor picture books.[f000]븁\u0000"
    InfoMsg NacreneCity3_Text_BookshelfPictureBooks, 2
    // "“Lily-Livered Lillipup's Quest, Vol. 1\"\nDo you want to read this book?"
    InfoMsg NacreneCity3_Text_LilyLiveredLillipupsQuest, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F8
    // "This is a story about a kind but\ncowardly Lillipup.[f000]븁\u0000\nIn order to save its sick grandmother,\nit goes in search of medicine[f000]븀\u0000\nin a cave where a scary ghost lives.[f000]븁\u0000\nThis volume tells the story of how the\ncrybaby Lillipup summons the courage[f000]븀\u0000\nto set off on its journey."
    InfoMsg NacreneCity3_Text_StoryAboutKindBut, 2
    LastKeyWait
    VMJump L_01F8

L_01F8:
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "This is the bookshelf\nfor picture books.[f000]븁\u0000"
    InfoMsg NacreneCity3_Text_BookshelfPictureBooks, 2
    // "“Lily-Livered Lillipup's Quest, Vol. 2\"\nDo you want to read this book?"
    InfoMsg NacreneCity3_Text_LilyLiveredLillipupsQuest_2, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0234
    // "This is the story of a cowardly Lillipup\nand a short-tempered Zorua.[f000]븁\u0000\nA violent Zorua blocks the path\nof Lillipup, who is headed to[f000]븀\u0000\na cave to get medicine.[f000]븁\u0000\nAfter this, that, and the other thing,\nthe two of them become friends[f000]븀\u0000\nand decide to travel together."
    InfoMsg NacreneCity3_Text_StoryCowardlyLillipupShort, 2
    LastKeyWait
    VMJump L_0234

L_0234:
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "This is the bookshelf\nfor picture books.[f000]븁\u0000"
    InfoMsg NacreneCity3_Text_BookshelfPictureBooks, 2
    // "“Lily-Livered Lillipup's Quest, Vol. 3\"\nDo you want to read this book?"
    InfoMsg NacreneCity3_Text_LilyLiveredLillipupsQuest_3, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0270
    // "This is the story of a cowardly Lillipup,\na short-tempered Zorua, and[f000]븀\u0000\na lonely Yamask.[f000]븁\u0000\nWhen Lillipup and Zorua enter the cave,\nthey hear a creepy cry![f000]븁\u0000\nSticking close to each other, deep into\nthe cave they go.[f000]븁\u0000\nThey find a Yamask loudly crying.[f000]븁\u0000\nIt wanted very much to make friends,\nbut everyone was scared of it,[f000]븀\u0000\nand nobody would be its friend.[f000]븁\u0000\nAfter this, that, and the other, they\nfigure this out, and the three new[f000]븀\u0000\nfriends continue into the cave."
    InfoMsg NacreneCity3_Text_StoryCowardlyLillipupShort_2, 2
    LastKeyWait
    VMJump L_0270

L_0270:
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "This is the bookshelf\nfor picture books.[f000]븁\u0000"
    InfoMsg NacreneCity3_Text_BookshelfPictureBooks, 2
    // "“Lily-Livered Lillipup's Quest, Vol. 4\"\nDo you want to read this book?"
    InfoMsg NacreneCity3_Text_LilyLiveredLillipupsQuest_4, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02AC
    // "This is a story about three pals,\nLillipup, Zorua, and Yamask.[f000]븁\u0000\nAfter fleeing from a colony of Woobat\nand working together to defeat a[f000]븀\u0000\nKrokorok that was being a bully, they[f000]븀\u0000\nfind the medicinal herb Lillipup needs.[f000]븁\u0000\nThe three of them make a pinky promise\nto play together again, and each returns[f000]븀\u0000\nhome, although they want to play more.[f000]븁\u0000\nLillipup gives the medicinal herb to its\ngrandmother, and she feels better! Yay![f000]븁\u0000\nFrom then on, the three Pokémon became\nthe best of friends forever, and[f000]븀\u0000\neveryone lived happily ever after!"
    InfoMsg NacreneCity3_Text_StoryAboutThreePals, 2
    LastKeyWait
    VMJump L_02AC

L_02AC:
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'll read to you from my favorite book--\n“The Mythology of the Sinnoh Region.\"[f000]븁\u0000\nLong ago, when Sinnoh had just been\nformed, Pokémon and humans led[f000]븀\u0000\nseparate lives.[f000]븁\u0000\nThat is not to say they did not help\none another. Indeed, they did.[f000]븁\u0000\nThey supplied one another with necessary\nitems, and they supported one another.[f000]븁\u0000\nOne Pokémon said to the others that they\nshould always be ready to help humans.[f000]븁\u0000\nIt proposed that Pokémon be ready to\nappear before humans whenever needed.[f000]븁\u0000\nThus, to this day, Pokémon appear to\nhumans if they venture into tall grass."
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity3_Text_IllReadFromFavorite, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "This library used to be\na Pokémon Gym!"
    ParentActorMsg MSGFILE_SCRIPT, NacreneCity3_Text_LibraryUsedPokemonGym, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
