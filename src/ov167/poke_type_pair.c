#include "types.h"
#include "battle/btl_pokeparam.h"

// Function name from swan.
PokeTypePair PokeTypePair_Make(u32 type1, u32 type2) {
    return ((type1 & 0xff) << 8) | (type2 & 0xff);
}

PokeTypePair func_ov167_021ce530(u32 type) {
    return PokeTypePair_Make(type, type);
}

u8 PokeTypePair_GetType1(PokeTypePair pair) {
    return (u8)(pair >> 8);
}

u32 PokeTypePair_GetType2(PokeTypePair pair) {
    return (u8)pair;
}

void func_ov167_021ce54c(PokeTypePair pair, u8 *type1, u8 *type2) {
    *type1 = PokeTypePair_GetType1(pair);
    *type2 = PokeTypePair_GetType2(pair);
}

BOOL func_ov167_021ce564(PokeTypePair pair, u32 type) {
    if (PokeTypePair_GetType1(pair) == type) {
        return TRUE;
    }
    if (PokeTypePair_GetType2(pair) == type) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov167_021ce588(PokeTypePair first, PokeTypePair second) {
    if (PokeTypePair_GetType1(first) == PokeTypePair_GetType1(second)) {
        return TRUE;
    }
    if (PokeTypePair_GetType1(first) == PokeTypePair_GetType2(second)) {
        return TRUE;
    }
    if (PokeTypePair_GetType2(first) == PokeTypePair_GetType1(second)) {
        return TRUE;
    }
    if (PokeTypePair_GetType2(first) == PokeTypePair_GetType2(second)) {
        return TRUE;
    }
    return FALSE;
}

BOOL PokeTypePair_IsMonotype(PokeTypePair pair) {
    return PokeTypePair_GetType1(pair) == PokeTypePair_GetType2(pair);
}
