#include "asm/field_script.inc"

    MapScript 2, 7
    MapScript 4, 6
    MapScript 3, 8
    MapScriptConditions Conditions_001A
    MapScriptsEnd

Conditions_001A:
    MapScriptCondition 0x40ae, 1, 2
    MapScriptConditionsEnd
    .byte 0x00
    .byte 0x00
