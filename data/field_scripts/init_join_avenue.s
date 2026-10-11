#include "asm/field_script.inc"

    MapScript 2, 1
    MapScript 4, 2
    MapScript 3, 7
    MapScriptConditions Conditions_001A
    MapScriptsEnd

Conditions_001A:
    MapScriptCondition 0x413a, 1, 11
    MapScriptConditionsEnd
    .byte 0x00
    .byte 0x00
