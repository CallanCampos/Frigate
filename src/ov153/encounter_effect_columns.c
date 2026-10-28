#include "types.h"
#include "field/encounter_effect.h"
#include "field/field.h"

GameEvent *EncEffColumns_Create(GameSystem *gsys, Field *field) {
    VecFx32 pos = { 0, 0, FX32_CONST(264) };

    EncEff_LoadCaptureGridSubOvls(Field_GetEncEff(field));
    return EncEffCapture_CreateFlashEvent(gsys, &pos, EncEffColumns_CreateGridEvent, EncEffGrid_Draw);
}

GameEvent *EncEffColumns_CreateGridEvent(GameSystem *gsys) {
    EncEffGridParam param;

    param.cellWidth = 2;
    param.cellHeight = 2;
    param.init = NULL;
    param.update = EncEffColumns_Update;
    param.cellUpdate = EncEffColumns_UpdateCell;
    return EncEffGrid_CreateEvent(gsys, &param, 1);
}

// Shows one column at a time
void EncEffColumns_Update(EncEffGrid *grid) {
    s32 *work = grid->work;
    int row;

    if (work[0] < grid->columns) {
        if (work[1] == 0) {
            for (row = 0; row < grid->rows; row++) {
                grid->cells[work[0] + row * grid->columns].visible = TRUE;
            }
            work[0]++;
            work[1] = 1;
        } else {
            work[1]--;
        }
    }
}

BOOL EncEffColumns_UpdateCell(EncEffCell *cell) {
    if (cell->frame < 64) {
        cell->offset += FX32_CONST(0.125);
        cell->frame++;
    } else {
        cell->done = TRUE;
    }
    return cell->done;
}
