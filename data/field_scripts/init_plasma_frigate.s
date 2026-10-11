#include "asm/field_script.inc"

    MapScript 4, 1
    MapScript 3, 2
    MapScript 2, 3
    MapScriptConditions Conditions_001A
    MapScriptsEnd

Conditions_001A:
    MapScriptCondition 0x40f0, 1, 22
    MapScriptCondition 0x40f1, 1, 23
    MapScriptCondition 0x40f2, 1, 23
    MapScriptConditionsEnd
    .byte 0x00
    .byte 0x00
