// NitroSystem NODEMIX's actual SDK implementation and geometry HAL, with one cached joint: the normal matrix is
// nine words, but its geometry command sends twelve. ASan must reject the old stack overread; the bounded host copy
// must keep the command packet, weighted normal matrix and final position matrix unchanged (lib/nnsys/src/g3d/sbc.c).
#include "gfx/ge/ge.h"
#include "hal/hw/hw.h"
#include "nnsys/g3d.h"
#include <stdio.h>

static int sFailures;

#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "line %d: %s\n", __LINE__, #condition);                                                    \
            sFailures++;                                                                                               \
        }                                                                                                              \
    } while (0)

int main(void) {
    NNSG3dResMdl model = { 0 };
    NNSG3dRenderObj object = { 0 };
    NNSG3dRS state = { 0 };
    // One cached joint, weighted 128/256; store the result in matrix slot 3.
    u8 command[] = { NNS_G3D_SBC_NODEMIX, 3, 1, 0, 0, 128 };
    uint32_t clip[16];
    uint32_t normal[9];
    const PortG3Write *writes;
    size_t count;
    PortGe *ge = port_ge_create();

    CHECK(ge != NULL);
    if (ge == NULL) {
        return 1;
    }
    port_hw_init();
    // NODEMIX preserves the caller's projection; set it to identity before its store/restore pair.
    port_ge_write(ge, 0x04000454, 0);
    object.resMdl = &model;
    state.pRenderObj = &object;
    state.c = command;
    state.isEvpCached[0] = 1;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            NNS_G3dRSOnGlb.evpCache[0].clip.m[row][col] = (row * 4 + col + 1) * 2 * FX32_ONE;
        }
    }
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            NNS_G3dRSOnGlb.evpCache[0].normal.m[row][col] = (row * 3 + col + 21) * 2 * FX32_ONE;
        }
    }
    NNSi_G3dFuncSbc_NODEMIX(&state, 0);
    CHECK(state.c == command + sizeof(command));
    writes = port_hw_g3_take(&count);
    CHECK(count == 35);
    if (count == 35) {
        CHECK(writes[4].port == 0x04000400 && writes[4].value == G3OP_MTX_LOAD_4x3);
        CHECK(writes[18].port == 0x04000400 && writes[18].value == G3OP_MTX_LOAD_4x3);
        for (int i = 0; i < 12; i++) {
            CHECK(writes[5 + i].port == 0x04000400);
            CHECK(writes[5 + i].value == (uint32_t)(i < 9 ? (i + 21) * FX32_ONE : 0));
            CHECK(writes[19 + i].port == 0x04000400);
            CHECK(writes[19 + i].value == (uint32_t)((i / 3 * 4 + i % 3 + 1) * FX32_ONE));
        }
    }
    port_ge_run(ge, writes, count);
    port_ge_clip_matrix(ge, clip);
    port_ge_vector_matrix(ge, normal);
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            uint32_t expected = col == 3 ? (row == 3 ? FX32_ONE : 0) : (row * 4 + col + 1) * FX32_ONE;
            CHECK(clip[row * 4 + col] == expected);
        }
    }
    for (int i = 0; i < 9; i++) {
        CHECK(normal[i] == (uint32_t)((i + 21) * FX32_ONE));
    }
    port_ge_destroy(ge);
    return sFailures != 0;
}
