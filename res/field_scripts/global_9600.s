#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    VMCall L_0024
    VMCall L_0324
    FieldSetTeleportZone 18
    MoneyAdd 3000
    VMHalt

L_0024:
    FlagSet EVENT_FLAG_0x03dd
    FlagSet EVENT_FLAG_0x025f
    FlagSet EVENT_FLAG_0x0265
    FlagSet EVENT_FLAG_0x025c
    FlagSet EVENT_FLAG_0x025d
    FlagSet EVENT_FLAG_0x0267
    FlagSet EVENT_FLAG_0x02ff
    FlagSet EVENT_FLAG_0x0300
    FlagSet EVENT_FLAG_0x0301
    FlagSet EVENT_FLAG_0x026d
    FlagSet EVENT_FLAG_0x0266
    FlagSet EVENT_FLAG_0x0294
    FlagSet EVENT_FLAG_0x029b
    FlagSet EVENT_FLAG_0x0285
    WorkSetConst EVENT_WORK_0x4165, 1
    FlagSet EVENT_FLAG_0x02e4
    FlagSet EVENT_FLAG_0x02e3
    FlagSet EVENT_FLAG_0x03c5
    FlagSet EVENT_FLAG_0x02be
    FlagSet EVENT_FLAG_0x0302
    FlagSet EVENT_FLAG_0x0303
    FlagSet EVENT_FLAG_0x03f6
    FlagSet EVENT_FLAG_0x02f9
    FlagSet EVENT_FLAG_0x02fa
    FlagSet EVENT_FLAG_0x02dd
    FlagSet EVENT_FLAG_0x02de
    FlagSet EVENT_FLAG_0x02df
    FlagSet EVENT_FLAG_0x02e0
    FlagSet EVENT_FLAG_0x02e1
    FlagSet EVENT_FLAG_0x02d7
    FlagSet EVENT_FLAG_0x02e7
    FlagSet EVENT_FLAG_0x02e5
    FlagSet EVENT_FLAG_0x097e
    FlagSet EVENT_FLAG_0x02da
    FlagSet EVENT_FLAG_0x03e4
    FlagSet EVENT_FLAG_0x02d4
    FlagSet EVENT_FLAG_0x02d3
    FlagSet EVENT_FLAG_0x02d2
    FlagSet EVENT_FLAG_0x02d1
    FlagSet EVENT_FLAG_0x02ea
    FlagSet EVENT_FLAG_0x02ef
    FlagSet EVENT_FLAG_0x0406
    FlagSet EVENT_FLAG_0x02f1
    FlagSet EVENT_FLAG_0x02f2
    FlagSet EVENT_FLAG_0x02f4
    FlagSet EVENT_FLAG_0x02f6
    FlagSet EVENT_FLAG_0x02cd
    FlagSet EVENT_FLAG_0x02cc
    FlagSet EVENT_FLAG_0x02b9
    FlagSet EVENT_FLAG_0x02c8
    FlagSet EVENT_FLAG_0x02c6
    FlagSet EVENT_FLAG_0x037e
    FlagSet EVENT_FLAG_0x037b
    FlagSet EVENT_FLAG_0x037a
    FlagSet EVENT_FLAG_0x0304
    FlagSet EVENT_FLAG_0x0305
    FlagSet EVENT_FLAG_0x0306
    FlagSet EVENT_FLAG_0x0307
    FlagSet EVENT_FLAG_0x030e
    FlagSet EVENT_FLAG_0x0310
    FlagSet EVENT_FLAG_0x0311
    FlagSet EVENT_FLAG_0x031a
    FlagSet EVENT_FLAG_0x031b
    FlagSet EVENT_FLAG_0x031c
    FlagSet EVENT_FLAG_0x031d
    FlagSet EVENT_FLAG_0x031e
    FlagSet EVENT_FLAG_0x0320
    FlagSet EVENT_FLAG_0x040a
    FlagSet EVENT_FLAG_0x03f2
    FlagSet EVENT_FLAG_0x0318
    FlagSet EVENT_FLAG_0x0325
    FlagSet EVENT_FLAG_0x0328
    FlagSet EVENT_FLAG_0x03d9
    FlagSet EVENT_FLAG_0x03f0
    FlagSet EVENT_FLAG_0x032b
    FlagSet EVENT_FLAG_0x032a
    FlagSet EVENT_FLAG_0x032e
    FlagSet EVENT_FLAG_0x0335
    FlagSet EVENT_FLAG_0x0336
    FlagSet EVENT_FLAG_0x0337
    FlagSet EVENT_FLAG_0x0338
    FlagSet EVENT_FLAG_0x0339
    FlagSet EVENT_FLAG_0x033a
    FlagSet EVENT_FLAG_0x033c
    FlagSet EVENT_FLAG_0x0355
    FlagSet EVENT_FLAG_0x0340
    FlagSet EVENT_FLAG_0x0341
    FlagSet EVENT_FLAG_0x0342
    FlagSet EVENT_FLAG_0x0347
    FlagSet EVENT_FLAG_0x0350
    FlagSet EVENT_FLAG_0x0356
    FlagSet EVENT_FLAG_0x034a
    FlagSet EVENT_FLAG_0x034b
    FlagSet EVENT_FLAG_0x03d1
    FlagSet EVENT_FLAG_0x0405
    FlagSet EVENT_FLAG_0x0a00
    FlagSet EVENT_FLAG_0x0357
    FlagSet EVENT_FLAG_0x0359
    FlagSet EVENT_FLAG_0x0358
    FlagSet EVENT_FLAG_0x035b
    FlagSet EVENT_FLAG_0x035c
    FlagSet EVENT_FLAG_0x035d
    FlagSet EVENT_FLAG_0x035e
    FlagSet EVENT_FLAG_0x035f
    FlagSet EVENT_FLAG_0x0360
    FlagSet EVENT_FLAG_0x0361
    FlagSet EVENT_FLAG_0x0362
    FlagSet EVENT_FLAG_0x037f
    FlagSet EVENT_FLAG_0x0380
    FlagSet EVENT_FLAG_0x036e
    FlagSet EVENT_FLAG_0x0370
    FlagSet EVENT_FLAG_0x0376
    FlagSet EVENT_FLAG_0x0378
    FlagSet EVENT_FLAG_0x0379
    FlagSet EVENT_FLAG_0x03c8
    FlagSet EVENT_FLAG_0x03c9
    FlagSet EVENT_FLAG_0x0407
    FlagSet EVENT_FLAG_0x0372
    FlagSet EVENT_FLAG_0x03ec
    FlagSet EVENT_FLAG_0x03eb
    FlagSet EVENT_FLAG_0x03f3
    FlagSet EVENT_FLAG_0x0384
    FlagSet EVENT_FLAG_0x0382
    FlagSet EVENT_FLAG_0x0381
    FlagSet EVENT_FLAG_0x0395
    FlagSet EVENT_FLAG_0x0394
    FlagSet EVENT_FLAG_0x0396
    FlagSet EVENT_FLAG_0x0397
    FlagSet EVENT_FLAG_0x0398
    FlagSet EVENT_FLAG_0x0390
    FlagSet EVENT_FLAG_0x03ad
    FlagSet EVENT_FLAG_0x0391
    FlagSet EVENT_FLAG_0x040b
    FlagSet EVENT_FLAG_0x039d
    FlagSet EVENT_FLAG_0x039e
    FlagSet EVENT_FLAG_0x039f
    FlagSet EVENT_FLAG_0x02b7
    FlagSet EVENT_FLAG_0x03b0
    FlagSet EVENT_FLAG_0x03ae
    FlagSet EVENT_FLAG_0x03b1
    FlagSet EVENT_FLAG_0x03c0
    FlagSet EVENT_FLAG_0x03b6
    FlagSet EVENT_FLAG_0x03b7
    FlagSet EVENT_FLAG_0x03b9
    FlagSet EVENT_FLAG_0x03bb
    FlagSet EVENT_FLAG_0x03bd
    FlagSet EVENT_FLAG_0x02ab
    FlagSet EVENT_FLAG_0x03ca
    FlagSet EVENT_FLAG_0x03cb
    FlagSet EVENT_FLAG_0x03cc
    FlagSet EVENT_FLAG_0x03cd
    FlagSet EVENT_FLAG_0x03ce
    FlagSet EVENT_FLAG_0x03d2
    FlagSet EVENT_FLAG_0x03d3
    FlagSet EVENT_FLAG_0x03da
    FlagSet EVENT_FLAG_0x0273
    FlagSet EVENT_FLAG_0x0274
    FlagSet EVENT_FLAG_0x0275
    FlagSet EVENT_FLAG_0x0276
    FlagSet EVENT_FLAG_0x0277
    FlagSet EVENT_FLAG_0x03df
    FlagSet EVENT_FLAG_0x03e0
    FlagSet EVENT_FLAG_0x03e2
    FlagSet EVENT_FLAG_0x03e3
    FlagSet EVENT_FLAG_0x02b3
    FlagSet EVENT_FLAG_0x02b4
    FlagSet EVENT_FLAG_0x02b5
    FlagSet EVENT_FLAG_0x02b6
    FlagSet EVENT_FLAG_0x03ef
    FlagSet EVENT_FLAG_0x03f8
    FlagSet EVENT_FLAG_0x03f9
    FlagSet EVENT_FLAG_0x03fa
    FlagSet EVENT_FLAG_0x03fb
    FlagSet EVENT_FLAG_0x03fc
    FlagSet EVENT_FLAG_0x03fd
    FlagSet EVENT_FLAG_0x03fe
    FlagSet EVENT_FLAG_0x03ff
    FlagSet EVENT_FLAG_0x0404
    FlagSet EVENT_FLAG_0x0408
    FlagSet EVENT_FLAG_0x0409
    FlagSet EVENT_FLAG_0x040d
    FlagSet EVENT_FLAG_0x0411
    FlagSet EVENT_FLAG_0x0412
    FlagSet EVENT_FLAG_0x0413
    FlagSet EVENT_FLAG_0x0414
    FlagSet EVENT_FLAG_0x0415
    FlagSet EVENT_FLAG_0x0416
    FlagSet EVENT_FLAG_0x0417
    FlagSet EVENT_FLAG_0x0418
    FlagSet EVENT_FLAG_0x0419
    FlagSet EVENT_FLAG_0x041a
    VMReturn

L_0324:
    GameGetVersion 0x8010
    WorkCmpConst 0x8010, 23
    VMJumpIf CMP_EQ, L_033B
    VMJump L_0347

L_033B:
    Cmd_0209 0, 1
    VMJump L_0366

L_0347:
    WorkCmpConst 0x8010, 22
    VMJumpIf CMP_EQ, L_035A
    VMJump L_0366

L_035A:
    Cmd_0209 0, 2
    VMJump L_0366

L_0366:
    TrainerCardGetSex 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_037D
    VMJump L_0389

L_037D:
    Cmd_0209 1, 1
    VMJump L_03A8

L_0389:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_039C
    VMJump L_03A8

L_039C:
    Cmd_0209 1, 2
    VMJump L_03A8

L_03A8:
    Cmd_0209 29, 1
    VMReturn

Script_3:
    VMCall L_0024
    FieldSetTeleportZone 3
    FlagSet EVENT_FLAG_0x006a
    FlagSet EVENT_FLAG_0x0961
    FlagSet EVENT_FLAG_0x0962
    GiveRunningShoes
    VMHalt

Script_2:
    VMStackPushFlag EVENT_FLAG_0x0960
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03E3
    VMCall L_06DD

L_03E3:
    PokePartyRecoverAll
    FieldSetTeleportZone 18
    FieldSetNextZone 428, 2, 13, 0, 5
    FlagSet EVENT_FLAG_CONTINUE_SCRIPT
    VMStackPushFlag EVENT_FLAG_0x02a7
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0101
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0420
    FlagReset EVENT_FLAG_0x02a7

L_0420:
    VMStackPushFlag EVENT_FLAG_0x0321
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x012d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0447
    FlagReset EVENT_FLAG_0x0321

L_0447:
    VMStackPushFlag EVENT_FLAG_0x0322
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x012f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_046E
    FlagReset EVENT_FLAG_0x0322

L_046E:
    VMStackPushFlag EVENT_FLAG_0x032a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x014a
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0495
    FlagReset EVENT_FLAG_0x032a

L_0495:
    VMStackPushFlag EVENT_FLAG_0x0176
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04B2
    WorkSetConst EVENT_WORK_0x410c, 0
    FlagSet EVENT_FLAG_0x040d

L_04B2:
    VMStackPushFlag EVENT_FLAG_0x017c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4074
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_04DB
    WorkSetConst EVENT_WORK_0x4074, 1

L_04DB:
    VMStackPushFlag EVENT_FLAG_0x017b
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4073
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0504
    WorkSetConst EVENT_WORK_0x4073, 1

L_0504:
    VMStackPushFlag EVENT_FLAG_0x018c
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4116
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_052D
    WorkSetConst EVENT_WORK_0x4116, 1

L_052D:
    VMStackPushFlag EVENT_FLAG_0x018d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4117
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0556
    WorkSetConst EVENT_WORK_0x4117, 1

L_0556:
    VMStackPushFlag EVENT_FLAG_0x018e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x4118
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_057F
    WorkSetConst EVENT_WORK_0x4118, 1

L_057F:
    VMStackPushFlag EVENT_FLAG_0x03a0
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0193
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_05A6
    FlagReset EVENT_FLAG_0x03a0

L_05A6:
    VMStackPushFlag EVENT_FLAG_0x0399
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x018f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_05CD
    FlagReset EVENT_FLAG_0x0399

L_05CD:
    VMStackPushFlag EVENT_FLAG_0x039a
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0190
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_05F4
    FlagReset EVENT_FLAG_0x039a

L_05F4:
    VMStackPushFlag EVENT_FLAG_0x039b
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0191
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_061B
    FlagReset EVENT_FLAG_0x039b

L_061B:
    VMStackPushFlag EVENT_FLAG_0x039c
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x0192
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0642
    FlagReset EVENT_FLAG_0x039c

L_0642:
    VMStackPushFlag EVENT_FLAG_0x03b2
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01a4
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0669
    FlagReset EVENT_FLAG_0x03b2

L_0669:
    VMStackPushFlag EVENT_FLAG_0x029b
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x00ea
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush EVENT_WORK_0x411b
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_06A0
    FlagReset EVENT_FLAG_0x029b

L_06A0:
    VMStackPushFlag EVENT_FLAG_0x03ec
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x00f9
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPushFlag EVENT_FLAG_0x01e0
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_06DB
    FlagReset EVENT_FLAG_0x03ec
    FlagSet EVENT_FLAG_0x03eb

L_06DB:
    VMHalt

L_06DD:
    FlagSet EVENT_FLAG_0x0960
    HollowRivalCmd_0262 0, 0
    HollowRivalCmd_0262 1, 40
    HollowRivalCmd_0262 2, 14
    HollowRivalCmd_0262 3, 0
    HollowRivalCmd_0262 4, 0
    FlagSet EVENT_FLAG_0x02ce
    WorkSetConst EVENT_WORK_0x40c4, 1
    FlagReset EVENT_FLAG_0x01bc
    FlagReset EVENT_FLAG_0x09f2
    FlagReset EVENT_FLAG_0x03e1
    FlagReset EVENT_FLAG_0x0320
    FlagReset EVENT_FLAG_0x03f1
    FlagSet EVENT_FLAG_0x031a
    FlagSet EVENT_FLAG_0x03f2
    MapReplaceSetEvent 0, 0, 0
    FlagReset EVENT_FLAG_0x0285
    FlagReset EVENT_FLAG_0x02ed
    FlagSet EVENT_FLAG_0x0329
    WorkSetConst EVENT_WORK_0x40e1, 1
    FlagSet EVENT_FLAG_0x03d0
    FlagReset EVENT_FLAG_0x03d1
    FlagSet EVENT_FLAG_0x03cf
    MapReplaceSetEvent 1, 1, 1
    WorkSetConst EVENT_WORK_0x414a, 1
    WorkSetConst EVENT_WORK_0x414b, 1
    WorkSetConst EVENT_WORK_0x414c, 1
    WorkSetConst EVENT_WORK_0x414d, 1
    WorkSetConst EVENT_WORK_0x414e, 1
    WorkSetConst EVENT_WORK_0x4149, 2
    FlagSet EVENT_FLAG_0x0373
    Cmd_00E4 1
    FlagSet EVENT_FLAG_0x0388
    FlagReset EVENT_FLAG_0x0395
    WorkSetConst EVENT_WORK_0x411c, 1
    FlagReset EVENT_FLAG_0x03b1
    WorkSetConst EVENT_WORK_0x40a0, 2
    WorkSetConst EVENT_WORK_0x4098, 1
    FlagSet EVENT_FLAG_0x02ae
    WorkSetConst EVENT_WORK_0x4123, 1
    FlagReset EVENT_FLAG_0x03df
    FlagSet EVENT_FLAG_0x0284
    FlagReset EVENT_FLAG_0x03f0
    FlagSet EVENT_FLAG_0x0409
    VMReturn
    VMReturn
    .balign 4, 0
