// NitroSDK libgx's g3imm.c: the geometry commands that load or multiply a matrix, which are written to the FIFO port
// as a command word followed by the matrix. The name is NitroSDK's.
#include "nitro/gx.h"

void G3_LoadMtx44(const MtxFx44 *mtx) {
    reg_G3X_GXFIFO = G3OP_MTX_LOAD_4x4;
    GX_SendFifo64B(mtx, &reg_G3X_GXFIFO);
}

void G3_LoadMtx43(const MtxFx43 *mtx) {
    reg_G3X_GXFIFO = G3OP_MTX_LOAD_4x3;
    GX_SendFifo48B(mtx, &reg_G3X_GXFIFO);
}

void G3_MultMtx44(const MtxFx44 *mtx) {
    reg_G3X_GXFIFO = G3OP_MTX_MULT_4x4;
    GX_SendFifo64B(mtx, &reg_G3X_GXFIFO);
}

void G3_MultMtx43(const MtxFx43 *mtx) {
    reg_G3X_GXFIFO = G3OP_MTX_MULT_4x3;
    GX_SendFifo48B(mtx, &reg_G3X_GXFIFO);
}

void G3_MultMtx33(const MtxFx33 *mtx) {
    reg_G3X_GXFIFO = G3OP_MTX_MULT_3x3;
    MI_Copy36B(mtx, (void *)&reg_G3X_GXFIFO);
}

void G3_MultTransMtx33(const MtxFx33 *mtx, const VecFx32 *trans) {
    reg_G3X_GXFIFO = G3OP_MTX_MULT_4x3;
    MI_Copy36B(mtx, (void *)&reg_G3X_GXFIFO);
    reg_G3X_GXFIFO = trans->x;
    reg_G3X_GXFIFO = trans->y;
    reg_G3X_GXFIFO = trans->z;
}
