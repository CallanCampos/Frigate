#include "asm/field_script.inc"

    MapScript 2, 1
    MapScript 4, 2
    MapScript 3, 3
    MapScriptConditions Conditions_001A
    MapScriptsEnd

Conditions_001A:
    MapScriptCondition 0x4103, 1, 16
    MapScriptConditionsEnd
    .byte 0x00
    .byte 0x00
