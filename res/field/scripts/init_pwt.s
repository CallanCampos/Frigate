#include "asm/field_script.inc"

    MapScript 2, 5
    MapScript 4, 6
    MapScriptConditions Conditions_001A
    MapScript 3, 17
    MapScriptsEnd

Conditions_001A:
    MapScriptCondition 0x40c6, 2, 2
    MapScriptConditionsEnd
    .byte 0x00
    .byte 0x00
