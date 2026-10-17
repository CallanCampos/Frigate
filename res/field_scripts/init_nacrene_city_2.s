#include "asm/field_script.inc"

    MapScript 4, 1
    MapScript 2, 2
    MapScriptConditions Conditions_001A
    MapScript 3, 21
    MapScriptsEnd

Conditions_001A:
    MapScriptCondition 0x4140, 2, 20
    MapScriptConditionsEnd
    .byte 0x00
    .byte 0x00
