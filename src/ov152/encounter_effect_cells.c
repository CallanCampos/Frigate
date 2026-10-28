#include "types.h"
#include "field/encounter_effect.h"
#include "field/field.h"

GameEvent *EncEffCells_Create(GameSystem *gsys, Field *field) {
    VecFx32 pos = { 0, 0, FX32_CONST(264) };

    EncEff_LoadCaptureGridSubOvls(Field_GetEncEff(field));
    return EncEffCapture_CreateFlashEvent(gsys, &pos, EncEffCells_CreateGridEvent, EncEffGrid_Draw);
}

GameEvent *EncEffCells_CreateGridEvent(GameSystem *gsys) {
    EncEffGridParam param;

    param.cellWidth = 4;
    param.cellHeight = 4;
    param.init = NULL;
    param.update = EncEffCells_Update;
    param.cellUpdate = EncEffCells_UpdateCell;
    return EncEffGrid_CreateEvent(gsys, &param, 0);
}

// Shows one cell at a time
void EncEffCells_Update(EncEffGrid *grid) {
    s32 *work = grid->work;

    if (work[0] < grid->columns * grid->rows) {
        if (work[1] == 0) {
            grid->cells[work[0]].visible = TRUE;
            work[0]++;
            work[1] = 0;
        } else {
            work[1]--;
        }
    }
}

BOOL EncEffCells_UpdateCell(EncEffCell *cell) {
    if (cell->frame < 1) {
        cell->offset += FX32_ONE;
        if (cell->offset >= FX32_CONST(16)) {
            cell->offset -= FX32_CONST(16);
            cell->frame++;
        }
    } else if (cell->frame == 1) {
        cell->offset += FX32_ONE;
        if (cell->offset >= FX32_CONST(8)) {
            cell->offset = FX32_CONST(8);
            cell->frame++;
        }
    } else {
        cell->done = TRUE;
    }
    return cell->done;
}
