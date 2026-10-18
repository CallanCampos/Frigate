#include "asm/field_script.inc"

    MapScript 4, 3
    MapScript 3, 4
    MapScript 2, 2
    MapScriptConditions Conditions_001A
    MapScriptsEnd

Conditions_001A:
    MapScriptCondition 0x40f3, 1, 5
    MapScriptConditionsEnd
    .byte 0x00
    .byte 0x00
