#include "types.h"
#include "app/demo3d/demo3d_data.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "constants/zones.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"

// The 3D demos' scenes, named after the ROM's "demo3d_data.c". A demo has a 3D set-up, a list of commands for its
// frames and one for its end, and its models ("units"). A model has one or more variants, picked by the player's sex,
// the demo's parameter or the season, and a variant is the files of ARCID_DEMO3D_RESOURCE that make it up, the model
// first and then its animations, either one set or one for each period of the day.
//
// The declaration order is a fit, not the original's: MWCC heapsorts a file's data by size, so the ROM fixes only
// which object each declaration slot must hold for a given sequence of sizes. This order was found by annealing that
// sequence until the forced assignment declared every table before its user (one exception kept as a forward
// declaration below) and reproduces the ROM's layout exactly; the original's own order is unknown.

// The most files a model has
#define RESOURCE_MAX 6

typedef struct {
    const Demo3DSetup *setup;
    const Demo3DCmd *cmds;
    const Demo3DCmd *endCmds;
    const Demo3DUnit *units;
    u32 unitCount;
} Demo3DScene;

static u32 Demo3DData_CheckId(u32 demoId);
static const Demo3DVariant *Demo3DData_GetVariant(const Demo3DUnit *unit, const Demo3DSceneParam *param);

static const u16 sDemo3D26_FileIds0_0_0[] = { 443, 444, 442 };
static const u16 sDemo3D17_FileIds2_0_0[] = { 24, 25, 26 };
static const Demo3DFiles sDemo3D17_Files2_0[] = { { sDemo3D17_FileIds2_0_0, NELEMS(sDemo3D17_FileIds2_0_0) } };
static const u16 sDemo3D01_FileIds0_1_0[] = { 62, 63, 61 };
// Forward declaration: the ROM lays this table out after its user; MWCC places data by definition, so only the
// definition's position matters. A tentative `static const` definition like this is MWCC-accepted; a strict C
// compiler may reject it, so the PC port can move the definition up instead.
static const u16 sDemo3D08_FileIds1_0_0[4];
static const Demo3DFiles sDemo3D08_Files1_0[] = { { sDemo3D08_FileIds1_0_0, NELEMS(sDemo3D08_FileIds1_0_0) } };
static const u16 sDemo3D26_FileIds1_0_3[] = { 407, 408, 406 };
static const u16 sDemo3D07_FileIds2_0_0[] = { 205, 206, 203, 204 };
static const u16 sDemo3D08_FileIds0_0_0[] = { 307, 308 };
static const u16 sDemo3D16_FileIds1_0_0[] = { 300, 299, 301 };
static const u16 sDemo3D13_FileIds0_0_0[] = { 379, 380, 377, 378, 381 };
static const u16 sDemo3D15_FileIds1_0_3[] = { 429, 430, 428, 431 };
static const u16 sDemo3D01_FileIds6_0_0[] = { 77, 79, 78, 76, 80 };
static const u16 sDemo3D13_FileIds1_0_0[] = { 416, 417, 414, 415, 418 };
static const u16 sDemo3D02_FileIds1_0_0[] = { 42, 43, 41 };
static const u16 sDemo3D11_FileIds2_0_0[] = { 451, 452, 450 };
static const u16 sDemo3D13_FileIds1_0_1[] = { 416, 417, 414, 415, 418 };
static const u16 sDemo3D13_FileIds1_0_2[] = { 425, 426, 423, 424, 427 };
static const Demo3DCmd sDemo3D21_Cmds[] = {
    { DEMO3D_CMD_BGM_PLAY, DEMO3D_CMD_FRAME_START, { SEQ_BGM_SW_DEMO_UNION_02 } },
    { DEMO3D_CMD_SE_PLAY, 47, { SEQ_SE_SW_KYUREON_02_10, 120, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 92, { SEQ_SE_SW_KYUREON_02_10, 120, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 237, { SEQ_SE_SW_KYUREON_02_01, 120, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 251, { SEQ_SE_SW_KYUREON_02_02, 120, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 282, { SEQ_SE_SW_KYUREON_02_03, 120, -1, -1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 287, { SEQ_SE_SW_KYUREON_02_01, 120, 0, 37 } },
    { DEMO3D_CMD_SE_STOP, 325, { SEQ_SE_SW_KYUREON_02_01 } },
    { DEMO3D_CMD_SE_PLAY, 326, { SEQ_SE_SW_KYUREON_02_04, 120, -1, -1 } },
    { DEMO3D_CMD_FLASH, 439, { 1, 0, 16, 0, 3, 3 } },
    { DEMO3D_CMD_SE_PLAY, 462, { SEQ_SE_SW_KYUREON_02_08, 120, -1, -1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 472, { SEQ_SE_SW_KYUREON_02_03, 120, 0, 44, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 498, { SEQ_SE_SW_KYUREON_02_02, 120, 0, 44, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 511, { SEQ_SE_SW_KYUREON_02_08, 120, 0, 37 } },
    { DEMO3D_CMD_SE_STOP, 543, { SEQ_SE_SW_KYUREON_02_03, 2 } },
    { DEMO3D_CMD_SE_STOP, 543, { SEQ_SE_SW_KYUREON_02_02, 1 } },
    { DEMO3D_CMD_SE_STOP, 549, { SEQ_SE_SW_KYUREON_02_08 } },
    { DEMO3D_CMD_SE_PLAY, 553, { SEQ_SE_SW_KYUREON_02_05, 120, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 553, { SEQ_SE_SW_KYUREON_02_09, 120, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 602, { SEQ_SE_SW_KYUREON_02_06w, 95, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 687, { SEQ_SE_SW_KYUREON_02_07, 90, -1, -1, 1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 790, { 1, 60, 0, 10 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 810, { SEQ_SE_SW_KYUREON_02_05, 120, 0, 50 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 810, { SEQ_SE_SW_KYUREON_02_06w, 95, 0, 50, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 810, { SEQ_SE_SW_KYUREON_02_07, 90, 0, 50, 1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 850, { 1, 20, 10, 16 } },
    { DEMO3D_CMD_SE_STOP, 870, { SEQ_SE_SW_KYUREON_02_05 } },
    { DEMO3D_CMD_SE_STOP, 870, { SEQ_SE_SW_KYUREON_02_06w, 2 } },
    { DEMO3D_CMD_SE_STOP, 870, { SEQ_SE_SW_KYUREON_02_07, 1 } },
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D21_FileIds4_0_0[] = { 178, 179, 177 };
static const u16 sDemo3D16_FileIds2_0_0[] = { 2, 1, 3 };
static const u16 sDemo3D21_FileIds1_0_0[] = { 22 };
static const Demo3DCmd sDemo3D02_Cmds[] = {
    { DEMO3D_CMD_SE_PLAY, 1, { SEQ_SE_VDEMO_01, 117 } },
    { DEMO3D_CMD_SE_PLAY, 125, { SEQ_SE_VDEMO_03, 117, 0, -200, 2 } },
    { DEMO3D_CMD_SE_PLAY, 201, { SEQ_SE_FLD_78, 127, 0, 0, 1 } },
    { DEMO3D_CMD_SE_PLAY, 269, { SEQ_SE_DUMMY5 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 270, { 1, 30, 0, -16 } },
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D12_FileIds0_0_0[] = { 455, 454 };
static const Demo3DFiles sDemo3D12_Files0_0[] = { { sDemo3D12_FileIds0_0_0, NELEMS(sDemo3D12_FileIds0_0_0) } };
static const u16 sDemo3D24_FileIds0_3_0[] = { 358, 360, 359, 357 };
static const Demo3DFiles sDemo3D08_Files0_0[] = { { sDemo3D08_FileIds0_0_0, NELEMS(sDemo3D08_FileIds0_0_0) } };
static const Demo3DCmd sDemo3D02_EndCmds[] = {
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D02_FileIds0_0_0[] = { 39, 40 };
static const u16 sDemo3D13_FileIds1_0_3[] = { 434, 435, 432, 433, 436 };
static const u16 sDemo3D12_FileIds1_0_0[] = { 457, 456 };
static const u16 sDemo3D20_FileIds1_0_0[] = { 385, 386, 383, 384, 387 };
static const u16 sDemo3D23_FileIds1_0_2[] = { 394, 395, 392, 393 };
static const u16 sDemo3D24_FileIds2_0_2[] = { 335, 342, 340 };
static const u16 sDemo3D20_FileIds3_0_0[] = { 20, 21 };
static const u16 sDemo3D22_FileIds1_0_0[] = { 477, 478, 475, 476 };
static const u16 sDemo3D09_FileIds0_0_0[] = { 319, 320 };
static const u16 sDemo3D09_FileIds1_0_0[] = { 323, 324, 321, 322 };
static const u16 sDemo3D01_FileIds5_0_0[] = { 74, 75 };
static const Demo3DFiles sDemo3D02_Files1_0[] = { { sDemo3D02_FileIds1_0_0, NELEMS(sDemo3D02_FileIds1_0_0) } };
static const Demo3DFiles sDemo3D01_Files0_1[] = { { sDemo3D01_FileIds0_1_0, NELEMS(sDemo3D01_FileIds0_1_0) } };
static const u16 sDemo3D01_FileIds3_0_0[] = { 71, 72, 70 };
static const u16 sDemo3D05_FileIds4_1_0[] = { 286, 283 };
static const u16 sDemo3D07_FileIds1_0_0[] = { 199, 201, 200, 197, 198 };
static const Demo3DFiles sDemo3D21_Files4_0[] = { { sDemo3D21_FileIds4_0_0, NELEMS(sDemo3D21_FileIds4_0_0) } };
static const u16 sDemo3D10_FileIds2_0_1[] = { 469, 470 };
static const u16 sDemo3D03_FileIds5_0_3[] = { 219, 222 };
static const u16 sDemo3D03_FileIds1_4_0[] = { 230, 248, 437 };
static const u16 sDemo3D04_FileIds3_0_4[] = { 265, 267 };
static const u16 sDemo3D04_FileIds3_0_3[] = { 265, 267 };
static const u16 sDemo3D14_FileIds1_0_4[] = { 429, 430, 428, 431 };
static const u16 sDemo3D14_FileIds0_2_0[] = { 91, 92, 89, 90, 93 };
static const u16 sDemo3D22_FileIds2_0_0[] = { 56, 57, 54, 55 };
static const Demo3DCmd sDemo3D06_Cmds[] = {
    { DEMO3D_CMD_BRIGHTNESS_FADE, 0, { 1, 30, -16 } },
    { DEMO3D_CMD_BGM_PLAY, 0, { SEQ_BGM_E_EPIROGUE } },
    { DEMO3D_CMD_SE_PLAY, 1, { SEQ_SE_W245_BUCKL, 10 } },
    { DEMO3D_CMD_SE_PLAY, 205, { SEQ_SE_FLD_167, 127, 0, 0, 2 } },
    { DEMO3D_CMD_SE_PLAY, 210, { SEQ_SE_FLD_169, 127, 0, 0, 1 } },
    { DEMO3D_CMD_SE_PLAY, 215, { SEQ_SE_FLD_167, 127, 0, 0, 2 } },
    { DEMO3D_CMD_SE_PLAY, 250, { SEQ_SE_END_03, 127 } },
    { DEMO3D_CMD_BGM_FADE, 330, { 1, 30 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 330, { 1, 30, 0, -16 } },
    { DEMO3D_CMD_END },
};
static const Demo3DFiles sDemo3D12_Files1_0[] = { { sDemo3D12_FileIds1_0_0, NELEMS(sDemo3D12_FileIds1_0_0) } };
static const u16 sDemo3D20_FileIds4_0_0[] = { 147, 148, 145, 146, 149 };
static const u16 sDemo3D17_FileIds0_0_0[] = { 5, 4, 6 };
static const u16 sDemo3D02_FileIds2_0_0[] = { 45, 46, 44 };
static const Demo3DVariant sDemo3D02_Variants1[] = { { sDemo3D02_Files1_0, NELEMS(sDemo3D02_Files1_0) } };
static const Demo3DFiles sDemo3D02_Files0_0[] = { { sDemo3D02_FileIds0_0_0, NELEMS(sDemo3D02_FileIds0_0_0) } };
static const u16 sDemo3D14_FileIds0_3_0[] = { 102, 103, 100, 101, 104 };
static const u16 sDemo3D15_FileIds1_0_0[] = { 411, 412, 410, 413 };
static const u16 sDemo3D15_FileIds0_0_0[] = { 108, 109, 106, 107, 110 };
static const u16 sDemo3D01_FileIds1_0_0[] = { 65, 64, 66 };
static const u16 sDemo3D03_FileIds5_0_0[] = { 219, 220 };
static const u16 sDemo3D15_FileIds0_1_0[] = { 118, 119, 116, 117, 120 };
static const Demo3DCmd sDemo3D26_EndCmds[] = {
    { DEMO3D_CMD_BGM_STOP, 0, { 0 } },
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D23_FileIds1_0_4[] = { 398, 399, 396, 397 };
static const u16 sDemo3D16_FileIds0_0_0[] = { 23 };
static const Demo3DFiles sDemo3D20_Files4_0[] = { { sDemo3D20_FileIds4_0_0, NELEMS(sDemo3D20_FileIds4_0_0) } };
static const u16 sDemo3D03_FileIds5_0_1[] = { 219, 220 };
static const u16 sDemo3D03_FileIds5_0_2[] = { 219, 221 };
static const u16 sDemo3D03_FileIds1_3_0[] = { 255, 245, 437 };
static const u16 sDemo3D05_FileIds2_0_0[] = { 290, 291, 289 };
static const Demo3DFiles sDemo3D14_Files0_2[] = { { sDemo3D14_FileIds0_2_0, NELEMS(sDemo3D14_FileIds0_2_0) } };
static const u16 sDemo3D15_FileIds0_2_0[] = { 113, 114, 111, 112, 115 };
static const u16 sDemo3D12_FileIds2_0_0[] = { 459, 460, 458 };
static const Demo3DVariant sDemo3D08_Variants0[] = { { sDemo3D08_Files0_0, NELEMS(sDemo3D08_Files0_0) } };
static const Demo3DCmd sDemo3D07_Cmds[] = {
    { DEMO3D_CMD_BRIGHTNESS_FADE, 0, { 1, 30, 16 } },
    { DEMO3D_CMD_BGM_PLAY, 0, { SEQ_BGM_E_EPIROGUE_W } },
    { DEMO3D_CMD_SE_PLAY, 5, { SEQ_SE_LCDEMO_02, 30, 0, -100, 1 } },
    { DEMO3D_CMD_SE_PLAY, 31, { SEQ_SE_LCDEMO_01, 30, 0, -100, 1 } },
    { DEMO3D_CMD_SE_PLAY, 65, { SEQ_SE_LCDEMO_02, 30, 0, -100, 1 } },
    { DEMO3D_CMD_SE_PLAY, 103, { SEQ_SE_LCDEMO_01, 30, 0, -100, 1 } },
    { DEMO3D_CMD_SE_PLAY, 139, { SEQ_SE_LCDEMO_02, 30, 0, -100, 1 } },
    { DEMO3D_CMD_SE_PLAY, 185, { SEQ_SE_END_04, 127 } },
    { DEMO3D_CMD_SE_PLAY, 220, { SEQ_SE_END_06, 127, 0, 0, 1 } },
    { DEMO3D_CMD_SE_PLAY, 223, { SEQ_SE_END_05, 100, 0, -200 } },
    { DEMO3D_CMD_SE_PLAY, 295, { SEQ_SE_END_07, 30, 0, 200, 1 } },
    { DEMO3D_CMD_BGM_FADE, 390, { 1, 25 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 390, { 1, 30, 0, 16 } },
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D03_FileIds2_2_0[] = { 208, 209, 207 };
static const Demo3DFiles sDemo3D03_Files2_2[] = { { sDemo3D03_FileIds2_2_0, NELEMS(sDemo3D03_FileIds2_2_0) } };
static const u16 sDemo3D05_FileIds1_0_0[] = { 295, 296 };
static const Demo3DFiles sDemo3D20_Files1_0[] = { { sDemo3D20_FileIds1_0_0, NELEMS(sDemo3D20_FileIds1_0_0) } };
static const u16 sDemo3D03_FileIds2_0_0[] = { 211, 212, 210 };
static const u16 sDemo3D03_FileIds1_8_0[] = { 236, 248, 437 };
static const u16 sDemo3D03_FileIds5_0_4[] = { 219, 222 };
static const Demo3DFiles sDemo3D03_Files5_0[] = {
    { sDemo3D03_FileIds5_0_0, NELEMS(sDemo3D03_FileIds5_0_0) },
    { sDemo3D03_FileIds5_0_1, NELEMS(sDemo3D03_FileIds5_0_1) },
    { sDemo3D03_FileIds5_0_2, NELEMS(sDemo3D03_FileIds5_0_2) },
    { sDemo3D03_FileIds5_0_3, NELEMS(sDemo3D03_FileIds5_0_3) },
    { sDemo3D03_FileIds5_0_4, NELEMS(sDemo3D03_FileIds5_0_4) },
};
static const u16 sDemo3D03_FileIds3_0_0[] = { 224, 223 };
static const Demo3DCmd sDemo3D15_EndCmds[] = {
    { DEMO3D_CMD_BGM_STOP, 0, { 0 } },
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D15_FileIds1_0_1[] = { 411, 412, 410, 413 };
static const u16 sDemo3D15_FileIds0_3_0[] = { 123, 124, 121, 122, 125 };
static const u16 sDemo3D24_FileIds1_0_0[] = { 332, 333, 331 };
static const u16 sDemo3D03_FileIds1_2_0[] = { 231, 245, 437 };
static const u16 sDemo3D21_FileIds0_0_0[] = { 32 };
static const Demo3DCmd sDemo3D05_EndCmds[] = {
    { DEMO3D_CMD_BGM_STOP, 0, { 0 } },
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D05_FileIds4_2_0[] = { 284, 283 };
static const u16 sDemo3D01_FileIds0_0_0[] = { 59, 60, 58 };
static const u16 sDemo3D03_FileIds1_12_0[] = { 243, 249, 437 };
static const Demo3DFiles sDemo3D21_Files1_0[] = { { sDemo3D21_FileIds1_0_0, NELEMS(sDemo3D21_FileIds1_0_0) } };
static const u16 sDemo3D21_FileIds3_0_0[] = { 152, 153, 150, 151, 154 };
static const u16 sDemo3D05_FileIds0_0_0[] = { 292, 294, 293 };
static const u16 sDemo3D04_FileIds1_0_0[] = { 259, 260, 258 };
static const u16 sDemo3D04_FileIds3_0_1[] = { 265, 266 };
static const Demo3DSetup sDemo3D25_Setup = {
    376, 0xffff, ZONE_NUVEMA_TOWN,
    1, 0, 1, 1, 0, 0, 0, 1, 0, 16,
    FX32_CONST(40), FX32_CONST(40), 0x1555, 0x19a, FX32_CONST(2048), FX32_CONST(1),
    0, 0, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    FALSE, 30, FALSE, 0,
};
static const u16 sDemo3D25_FileIds1_0_1[] = { 363, 367, 365 };
static const u16 sDemo3D17_FileIds1_0_0[] = { 304, 305, 302, 303, 306 };
static const u16 sDemo3D17_FileIds4_0_0[] = { 132, 133, 130, 131, 134 };
static const u16 sDemo3D03_FileIds1_1_0[] = { 232, 245, 437 };
static const u16 sDemo3D03_FileIds0_1_0[] = { 240, 247, 268 };
static const u16 sDemo3D05_FileIds4_3_0[] = { 287, 283 };
static const u16 sDemo3D05_FileIds1_0_4[] = { 295, 297 };
static const u16 sDemo3D26_FileIds1_0_2[] = { 404, 405, 403 };
static const Demo3DCmd sDemo3D25_Cmds[] = {
    { DEMO3D_CMD_SE_PLAY, 30, { SEQ_SE_SW_AIRPLANE_02_R, 127, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 82, { SEQ_SE_SW_AIRPLANE_01, 102, -1, -1, 1 } },
    { DEMO3D_CMD_BGM_FADE, 100, { 1, 160 } },
    { DEMO3D_CMD_SE_PLAY, 281, { SEQ_SE_SW_AIRPLANE_03, 127, -1, -1, 2 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 330, { 1, 5, 0, -16 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 350, { 1, 5, -16 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 470, { 1, 30, 0, -16 } },
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D05_FileIds1_0_2[] = { 295, 298 };
static const u16 sDemo3D01_FileIds2_0_0[] = { 68, 67, 69 };
static const Demo3DCmd sDemo3D17_EndCmds[] = {
    { DEMO3D_CMD_BGM_STOP, 0, { 0 } },
    { DEMO3D_CMD_END },
};
static const Demo3DSetup sDemo3D14_Setup = {
    94, 0xffff, ZONE_NUVEMA_TOWN,
    1, 0, 1, 1, 0, 0, 1, 1, 0, 16,
    FX32_CONST(28), FX32_CONST(28), 0x1555, FX32_CONST(1), FX32_CONST(2048), 0x19a,
    0, 0, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    FALSE, 30, TRUE, 0,
};
static const u16 sDemo3D05_FileIds3_2_0[] = { 279, 283 };
static const Demo3DFiles sDemo3D05_Files2_0[] = { { sDemo3D05_FileIds2_0_0, NELEMS(sDemo3D05_FileIds2_0_0) } };
static const u16 sDemo3D19_FileIds2_0_0[] = { 53, 51, 52 };
static const Demo3DCmd sDemo3D18_Cmds[] = {
    { DEMO3D_CMD_BGM_PLAY, DEMO3D_CMD_FRAME_START, { SEQ_BGM_SW_DEMO_UNION_02 } },
    { DEMO3D_CMD_SE_PLAY, 47, { SEQ_SE_SW_KYUREON_02_10, 120, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 92, { SEQ_SE_SW_KYUREON_02_10, 120, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 237, { SEQ_SE_SW_KYUREON_02_01, 120, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 251, { SEQ_SE_SW_KYUREON_02_02, 120, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 282, { SEQ_SE_SW_KYUREON_02_03, 120, -1, -1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 287, { SEQ_SE_SW_KYUREON_02_01, 120, 0, 37 } },
    { DEMO3D_CMD_SE_STOP, 325, { SEQ_SE_SW_KYUREON_02_01 } },
    { DEMO3D_CMD_SE_PLAY, 326, { SEQ_SE_SW_KYUREON_02_04, 120, -1, -1 } },
    { DEMO3D_CMD_FLASH, 439, { 1, 0, 16, 0, 3, 3 } },
    { DEMO3D_CMD_SE_PLAY, 462, { SEQ_SE_SW_KYUREON_02_08, 120, -1, -1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 472, { SEQ_SE_SW_KYUREON_02_03, 120, 0, 44, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 498, { SEQ_SE_SW_KYUREON_02_02, 120, 0, 44, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 511, { SEQ_SE_SW_KYUREON_02_08, 120, 0, 37 } },
    { DEMO3D_CMD_SE_STOP, 543, { SEQ_SE_SW_KYUREON_02_03, 2 } },
    { DEMO3D_CMD_SE_STOP, 543, { SEQ_SE_SW_KYUREON_02_02, 1 } },
    { DEMO3D_CMD_SE_STOP, 549, { SEQ_SE_SW_KYUREON_02_08 } },
    { DEMO3D_CMD_SE_PLAY, 553, { SEQ_SE_SW_KYUREON_02_05, 110, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 553, { SEQ_SE_SW_KYUREON_02_09, 120, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 602, { SEQ_SE_SW_KYUREON_02_06, 60, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 687, { SEQ_SE_SW_KYUREON_02_07, 80, -1, -1, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 810, { SEQ_SE_SW_KYUREON_02_05, 110, 0, 50 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 810, { SEQ_SE_SW_KYUREON_02_06, 60, 0, 50, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 810, { SEQ_SE_SW_KYUREON_02_07, 80, 0, 50, 1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 815, { 1, 48, 0, 16 } },
    { DEMO3D_CMD_SE_STOP, 870, { SEQ_SE_SW_KYUREON_02_05 } },
    { DEMO3D_CMD_SE_STOP, 870, { SEQ_SE_SW_KYUREON_02_06, 2 } },
    { DEMO3D_CMD_SE_STOP, 870, { SEQ_SE_SW_KYUREON_02_07, 1 } },
    { DEMO3D_CMD_END },
};
static const Demo3DFiles sDemo3D24_Files1_0[] = { { sDemo3D24_FileIds1_0_0, NELEMS(sDemo3D24_FileIds1_0_0) } };
static const u16 sDemo3D21_FileIds2_0_0[] = { 13, 12 };
static const Demo3DFiles sDemo3D21_Files0_0[] = { { sDemo3D21_FileIds0_0_0, NELEMS(sDemo3D21_FileIds0_0_0) } };
static const Demo3DSetup sDemo3D03_Setup = {
    227, 0xffff, ZONE_NUVEMA_TOWN,
    1, 0, 1, 1, 0, 0, 1, 1, 0, 16,
    FX32_CONST(40), FX32_CONST(40), 0x1555, 0x19a, FX32_CONST(2048), FX32_CONST(1),
    0, 0, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    FALSE, 30, FALSE, 0,
};
static const u16 sDemo3D03_FileIds4_0_0[] = { 226, 225 };
static const u16 sDemo3D06_FileIds1_0_0[] = { 186, 188, 187, 184, 185 };
static const u16 sDemo3D01_FileIds4_0_0[] = { 73 };
static const Demo3DFiles sDemo3D01_Files5_0[] = { { sDemo3D01_FileIds5_0_0, NELEMS(sDemo3D01_FileIds5_0_0) } };
static const Demo3DCmd sDemo3D22_EndCmds[] = {
    { DEMO3D_CMD_BGM_STOP, 0, { 0 } },
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D23_FileIds0_0_0[] = { 439, 440, 438 };
static const u16 sDemo3D03_FileIds1_14_0[] = { 241, 248, 437 };
static const Demo3DFiles sDemo3D05_Files4_2[] = { { sDemo3D05_FileIds4_2_0, NELEMS(sDemo3D05_FileIds4_2_0) } };
static const Demo3DFiles sDemo3D16_Files0_0[] = { { sDemo3D16_FileIds0_0_0, NELEMS(sDemo3D16_FileIds0_0_0) } };
static const u16 sDemo3D03_FileIds1_5_0[] = { 237, 248, 437 };
static const u16 sDemo3D16_FileIds3_0_0[] = { 128, 126, 127, 129 };
static const Demo3DFiles sDemo3D16_Files2_0[] = { { sDemo3D16_FileIds2_0_0, NELEMS(sDemo3D16_FileIds2_0_0) } };
static const u16 sDemo3D26_FileIds1_0_4[] = { 407, 408, 406 };
static const Demo3DFiles sDemo3D15_Files0_0[] = { { sDemo3D15_FileIds0_0_0, NELEMS(sDemo3D15_FileIds0_0_0) } };
static const u16 sDemo3D23_FileIds1_0_3[] = { 398, 399, 396, 397 };
static const u16 sDemo3D24_FileIds0_0_0[] = { 350, 352, 351, 349 };
static const Demo3DFiles sDemo3D20_Files3_0[] = { { sDemo3D20_FileIds3_0_0, NELEMS(sDemo3D20_FileIds3_0_0) } };
static const u16 sDemo3D20_FileIds0_0_0[] = { 10, 9, 11 };
static const Demo3DCmd sDemo3D08_EndCmds[] = {
    { DEMO3D_CMD_BGM_STOP, 0, { 0 } },
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D25_FileIds1_0_4[] = { 363, 372 };
static const Demo3DFiles sDemo3D21_Files2_0[] = { { sDemo3D21_FileIds2_0_0, NELEMS(sDemo3D21_FileIds2_0_0) } };
static const Demo3DFiles sDemo3D03_Files1_3[] = { { sDemo3D03_FileIds1_3_0, NELEMS(sDemo3D03_FileIds1_3_0) } };
static const u16 sDemo3D04_FileIds2_0_0[] = { 261 };
static const Demo3DVariant sDemo3D16_Variants2[] = { { sDemo3D16_Files2_0, NELEMS(sDemo3D16_Files2_0) } };
static const Demo3DSetup sDemo3D15_Setup = {
    94, 0xffff, ZONE_NUVEMA_TOWN,
    1, 0, 1, 1, 0, 0, 1, 1, 0, 16,
    FX32_CONST(28), FX32_CONST(28), 0x1555, FX32_CONST(1), FX32_CONST(2048), 0x19a,
    0, 0, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    FALSE, 30, TRUE, 0,
};
static const u16 sDemo3D04_FileIds3_0_0[] = { 265, 266 };
static const u16 sDemo3D05_FileIds6_0_0[] = { 270, 269 };
static const Demo3DFiles sDemo3D21_Files3_0[] = { { sDemo3D21_FileIds3_0_0, NELEMS(sDemo3D21_FileIds3_0_0) } };
static const Demo3DCmd sDemo3D03_Cmds[] = {
    { DEMO3D_CMD_MESSAGE, 180, { TEXT_BANK_0412, 0, 3, 180, 2, 19, 28, 4 } },
    { DEMO3D_CMD_MESSAGE, 460, { TEXT_BANK_0412, 1, 3, 180, 2, 19, 28, 4 } },
    { DEMO3D_CMD_MESSAGE, 760, { TEXT_BANK_0412, 2, 3, 180, 2, 19, 28, 4 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 990, { 1, 60, 0, -16 } },
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D03_FileIds1_6_0[] = { 235, 245, 437 };
static const Demo3DSetup sDemo3D24_Setup = {
    361, 0xffff, ZONE_NUVEMA_TOWN,
    1, 0, 1, 1, 0, 0, 0, 1, 0, 16,
    FX32_CONST(40), FX32_CONST(40), 0x1555, 0x19a, FX32_CONST(2048), FX32_CONST(1),
    0, 0, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    FALSE, 30, FALSE, 0,
};
static const u16 sDemo3D05_FileIds1_0_3[] = { 295, 297 };
static const u16 sDemo3D19_FileIds0_0_0[] = { 28 };
static const Demo3DFiles sDemo3D01_Files6_0[] = { { sDemo3D01_FileIds6_0_0, NELEMS(sDemo3D01_FileIds6_0_0) } };
static const u16 sDemo3D04_FileIds3_0_2[] = { 265, 267 };
static const Demo3DFiles sDemo3D04_Files3_0[] = {
    { sDemo3D04_FileIds3_0_0, NELEMS(sDemo3D04_FileIds3_0_0) },
    { sDemo3D04_FileIds3_0_1, NELEMS(sDemo3D04_FileIds3_0_1) },
    { sDemo3D04_FileIds3_0_2, NELEMS(sDemo3D04_FileIds3_0_2) },
    { sDemo3D04_FileIds3_0_3, NELEMS(sDemo3D04_FileIds3_0_3) },
    { sDemo3D04_FileIds3_0_4, NELEMS(sDemo3D04_FileIds3_0_4) },
};
static const Demo3DSetup sDemo3D21_Setup = {
    473, 0xffff, ZONE_NUVEMA_TOWN,
    1, 1, 1, 1, 0, 0, 0, 1, 0, 16,
    FX32_CONST(12), FX32_CONST(12), 0x1555, 0x19a, FX32_CONST(2048), FX32_CONST(1),
    64, -65, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    FALSE, 60, TRUE, 0,
};
static const Demo3DCmd sDemo3D18_EndCmds[] = {
    { DEMO3D_CMD_BGM_STOP, 0, { 0 } },
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D15_FileIds1_0_2[] = { 420, 421, 419, 422 };
static const Demo3DFiles sDemo3D17_Files1_0[] = { { sDemo3D17_FileIds1_0_0, NELEMS(sDemo3D17_FileIds1_0_0) } };
static const Demo3DCmd sDemo3D09_EndCmds[] = {
    { DEMO3D_CMD_BGM_STOP, 0, { 0 } },
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D26_FileIds1_0_0[] = { 401, 402, 400 };
static const Demo3DCmd sDemo3D22_Cmds[] = {
    { DEMO3D_CMD_BGM_PLAY, DEMO3D_CMD_FRAME_START, { SEQ_BGM_SW_DEMO_UNION_03W } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 1, { 1, 19, 16, 16 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 20, { 1, 30, 16 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 115, { 1, 5, 0, 12 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 120, { 1, 24, 12 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 205, { 1, 5, 0, 12 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 210, { 1, 24, 12 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 295, { 1, 5, 0, 12 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 300, { 1, 24, 12 } },
    { DEMO3D_CMD_SE_PLAY, 386, { SEQ_SE_MEET_04, 80, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 504, { SEQ_SE_MEET_04, 80, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 601, { SEQ_SE_SW_KYUREON_03_03, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 639, { SEQ_SE_SW_KYUREON_03_04, 127, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 660, { SEQ_SE_SW_KYUREON_03_05, 127, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 682, { SEQ_SE_SW_KYUREON_03_06w, 90, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 682, { SEQ_SE_SW_KYUREON_03_07w, 90, -1, -1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 861, { SEQ_SE_SW_KYUREON_03_06w, 90, 90, 1, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 861, { SEQ_SE_SW_KYUREON_03_07w, 90, 90, 1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 950, { SEQ_SE_SW_KYUREON_03_06w, 90, 0, 60, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 950, { SEQ_SE_SW_KYUREON_03_07w, 90, 0, 60, 2 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 980, { 1, 30, 0, -16 } },
    { DEMO3D_CMD_END },
};
static const Demo3DVariant sDemo3D21_Variants2[] = { { sDemo3D21_Files2_0, NELEMS(sDemo3D21_Files2_0) } };
static const Demo3DFiles sDemo3D12_Files2_0[] = { { sDemo3D12_FileIds2_0_0, NELEMS(sDemo3D12_FileIds2_0_0) } };
static const Demo3DVariant sDemo3D21_Variants3[] = { { sDemo3D21_Files3_0, NELEMS(sDemo3D21_Files3_0) } };
static const Demo3DVariant sDemo3D01_Variants6[] = { { sDemo3D01_Files6_0, NELEMS(sDemo3D01_Files6_0) } };
static const Demo3DCmd sDemo3D04_Cmds[] = {
    { DEMO3D_CMD_SE_PLAY, 0, { SEQ_SE_VDEMO_01, 127 } },
    { DEMO3D_CMD_SE_PLAY, 75, { SEQ_SE_VDEMO_02, 100, 0, 0, 1 } },
    { DEMO3D_CMD_SE_STOP, 130, { SEQ_SE_VDEMO_01 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 130, { 1, 20, 0, -16 } },
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D03_FileIds1_7_0[] = { 228, 248, 437 };
static const Demo3DFiles sDemo3D06_Files1_0[] = { { sDemo3D06_FileIds1_0_0, NELEMS(sDemo3D06_FileIds1_0_0) } };
static const u16 sDemo3D06_FileIds2_0_0[] = { 192, 193, 190, 191 };
static const Demo3DFiles sDemo3D06_Files2_0[] = { { sDemo3D06_FileIds2_0_0, NELEMS(sDemo3D06_FileIds2_0_0) } };
static const u16 sDemo3D03_FileIds2_3_0[] = { 217, 218, 216 };
static const u16 sDemo3D08_FileIds2_0_0[] = { 315, 317, 316, 313, 314 };
static const Demo3DSetup sDemo3D16_Setup = {
    0, 0xffff, ZONE_NUVEMA_TOWN,
    1, 1, 1, 1, 0, 0, 0, 1, 0, 16,
    FX32_CONST(28), FX32_CONST(28), 0x1555, 0x19a, FX32_CONST(2048), FX32_CONST(1),
    149, -142, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    FALSE, 60, FALSE, 0,
};
static const Demo3DVariant sDemo3D21_Variants4[] = { { sDemo3D21_Files4_0, NELEMS(sDemo3D21_Files4_0) } };
static const u16 sDemo3D20_FileIds2_0_0[] = { 29, 30, 31 };
static const Demo3DVariant sDemo3D17_Variants2[] = { { sDemo3D17_Files2_0, NELEMS(sDemo3D17_Files2_0) } };
static const u16 sDemo3D18_FileIds3_0_0[] = { 137, 138, 135, 136, 139 };
static const u16 sDemo3D19_FileIds4_0_0[] = { 169, 170, 168, 171 };
static const Demo3DCmd sDemo3D13_Cmds[] = {
    { DEMO3D_CMD_BGM_PLAY, DEMO3D_CMD_FRAME_START, { SEQ_BGM_SW_DEMO_FRIGATE } },
    { DEMO3D_CMD_SE_PLAY, 1, { SEQ_SE_SW_FRIGATE_01, 40, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 1, { SEQ_SE_SW_FRIGATE_02, 40, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 1, { SEQ_SE_SW_FRIGATE_03, 40, -1, -1, 2 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 255, { 1, 5, 0, 16 } },
    { DEMO3D_CMD_SE_PLAY, 255, { SEQ_SE_SW_FRIGATE_04, 127, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 255, { SEQ_SE_SW_FRIGATE_05, 127, -1, -1, 1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 265, { 1, 5, 16 } },
    { DEMO3D_CMD_SE_PLAY, 268, { SEQ_SE_SW_FRIGATE_06, 127, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 268, { SEQ_SE_SW_FRIGATE_08, 127, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 285, { SEQ_SE_SW_FRIGATE_07, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 501, { SEQ_SE_SW_FRIGATE_09, 127, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 501, { SEQ_SE_SW_FRIGATE_10, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 551, { SEQ_SE_SW_FRIGATE_11, 127, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 551, { SEQ_SE_SW_FRIGATE_12, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 650, { SEQ_SE_SW_FRIGATE_13, 127, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 650, { SEQ_SE_SW_FRIGATE_14, 127, -1, -1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 710, { 1, 1, 0, 2 } },
    { DEMO3D_CMD_SE_PLAY, 711, { SEQ_SE_SW_FRIGATE_15, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 711, { SEQ_SE_SW_FRIGATE_16, 127, -1, -1, 2 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 740, { 1, 1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 800, { SEQ_SE_SW_FRIGATE_16, 127, 0, 115, 2 } },
    { DEMO3D_CMD_SE_PLAY, 801, { SEQ_SE_SW_FRIGATE_17, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_STOP, 918, { SEQ_SE_SW_FRIGATE_16, 2 } },
    { DEMO3D_CMD_SE_PLAY, 930, { SEQ_SE_SW_FRIGATE_18, 127, -1, -1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 930, { 1, 5, 0, 13 } },
    { DEMO3D_CMD_SE_PLAY, 942, { SEQ_SE_SW_SOURYUCITY_02, 100, -1, -1, 1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 945, { 1, 5, 13 } },
    { DEMO3D_CMD_SE_PLAY, 948, { SEQ_SE_SW_FRIGATE_16, 80, -1, -1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 950, { SEQ_SE_SW_FRIGATE_16, 80, 50, 70, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 1040, { SEQ_SE_SW_SOURYUCITY_02, 100, 80, 1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 1041, { SEQ_SE_SW_FRIGATE_16, 50, -1, -1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 1041, { SEQ_SE_SW_FRIGATE_16, 50, 127, 170, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 1041, { SEQ_SE_SW_SOURYUCITY_02, 80, 127, 170, 1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 1200, { 1, 60, 0, -16 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 1230, { SEQ_SE_SW_FRIGATE_16, 127, 0, 30, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 1230, { SEQ_SE_SW_SOURYUCITY_02, 127, 0, 30, 1 } },
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D24_FileIds2_0_4[] = { 335, 344 };
static const Demo3DFiles sDemo3D07_Files1_0[] = { { sDemo3D07_FileIds1_0_0, NELEMS(sDemo3D07_FileIds1_0_0) } };
static const u16 sDemo3D07_FileIds0_0_0[] = { 196, 194, 195 };
static const Demo3DFiles sDemo3D03_Files1_5[] = { { sDemo3D03_FileIds1_5_0, NELEMS(sDemo3D03_FileIds1_5_0) } };
static const u16 sDemo3D10_FileIds0_0_0[] = { 462, 463 };
static const Demo3DVariant sDemo3D07_Variants1[] = { { sDemo3D07_Files1_0, NELEMS(sDemo3D07_Files1_0) } };
static const u16 sDemo3D23_FileIds1_0_0[] = { 390, 391, 388, 389 };
static const u16 sDemo3D25_FileIds0_0_0[] = { 374, 375, 373 };
static const Demo3DFiles sDemo3D07_Files2_0[] = { { sDemo3D07_FileIds2_0_0, NELEMS(sDemo3D07_FileIds2_0_0) } };
static const u16 sDemo3D09_FileIds2_0_0[] = { 327, 329, 328, 325, 326 };
static const Demo3DFiles sDemo3D02_Files2_0[] = { { sDemo3D02_FileIds2_0_0, NELEMS(sDemo3D02_FileIds2_0_0) } };
static const Demo3DVariant sDemo3D07_Variants2[] = { { sDemo3D07_Files2_0, NELEMS(sDemo3D07_Files2_0) } };
static const u16 sDemo3D11_FileIds0_0_0[] = { 447, 446 };
static const Demo3DFiles sDemo3D11_Files2_0[] = { { sDemo3D11_FileIds2_0_0, NELEMS(sDemo3D11_FileIds2_0_0) } };
static const u16 sDemo3D18_FileIds2_0_0[] = { 8, 7 };
static const Demo3DSetup sDemo3D06_Setup = {
    189, 0xffff, ZONE_NUVEMA_TOWN,
    1, 1, 1, 1, 0, 0, 0, 1, 0, 16,
    FX32_CONST(13), FX32_CONST(13), 0x1555, 0x19a, FX32_CONST(2048), 0x19a,
    58, -57, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    FALSE, 30, FALSE, 0,
};
static const Demo3DFiles sDemo3D08_Files2_0[] = { { sDemo3D08_FileIds2_0_0, NELEMS(sDemo3D08_FileIds2_0_0) } };
static const u16 sDemo3D10_FileIds1_0_0[] = { 465, 466, 464 };
static const u16 sDemo3D03_FileIds1_10_0[] = { 238, 250, 437 };
static const u16 sDemo3D03_FileIds1_13_0[] = { 242, 248, 437 };
static const Demo3DCmd sDemo3D11_Cmds[] = {
    { DEMO3D_CMD_BRIGHTNESS_FADE, 701, { 1, 20, 16 } },
    { DEMO3D_CMD_END },
};
static const Demo3DVariant sDemo3D12_Variants0[] = { { sDemo3D12_Files0_0, NELEMS(sDemo3D12_Files0_0) } };
static const Demo3DFiles sDemo3D18_Files2_0[] = { { sDemo3D18_FileIds2_0_0, NELEMS(sDemo3D18_FileIds2_0_0) } };
static const u16 sDemo3D18_FileIds4_0_0[] = { 166, 167, 165 };
static const Demo3DFiles sDemo3D19_Files0_0[] = { { sDemo3D19_FileIds0_0_0, NELEMS(sDemo3D19_FileIds0_0_0) } };
static const Demo3DCmd sDemo3D10_EndCmds[] = {
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D17_FileIds3_0_0[] = { 17, 18 };
static const Demo3DSetup sDemo3D18_Setup = {
    15, 0xffff, ZONE_NUVEMA_TOWN,
    1, 1, 1, 1, 0, 0, 0, 1, 0, 16,
    FX32_CONST(12), FX32_CONST(12), 0x1555, 0x19a, FX32_CONST(2048), FX32_CONST(1),
    64, -65, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    FALSE, 60, TRUE, 0,
};
static const u16 sDemo3D24_FileIds2_0_3[] = { 335, 344 };
static const Demo3DFiles sDemo3D09_Files2_0[] = { { sDemo3D09_FileIds2_0_0, NELEMS(sDemo3D09_FileIds2_0_0) } };
static const u16 sDemo3D22_FileIds3_0_0[] = { 157, 158, 155, 156, 159 };
static const Demo3DFiles sDemo3D09_Files1_0[] = { { sDemo3D09_FileIds1_0_0, NELEMS(sDemo3D09_FileIds1_0_0) } };
static const Demo3DVariant sDemo3D09_Variants2[] = { { sDemo3D09_Files2_0, NELEMS(sDemo3D09_Files2_0) } };
static const Demo3DSetup sDemo3D08_Setup = {
    318, 0xffff, ZONE_NUVEMA_TOWN,
    1, 1, 1, 0, 0, 0, 0, 1, 0, 16,
    FX32_CONST(28), FX32_CONST(28), 0x1555, 0x19a, FX32_CONST(2048), FX32_CONST(1),
    149, -142, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    FALSE, 30, FALSE, 0,
};
static const Demo3DFiles sDemo3D22_Files2_0[] = { { sDemo3D22_FileIds2_0_0, NELEMS(sDemo3D22_FileIds2_0_0) } };
static const u16 sDemo3D24_FileIds2_0_1[] = { 335, 339, 337 };
static const u16 sDemo3D13_FileIds1_0_4[] = { 434, 435, 432, 433, 436 };
static const Demo3DCmd sDemo3D23_EndCmds[] = {
    { DEMO3D_CMD_BGM_STOP, 0, { 0 } },
    { DEMO3D_CMD_END },
};
static const Demo3DFiles sDemo3D10_Files0_0[] = { { sDemo3D10_FileIds0_0_0, NELEMS(sDemo3D10_FileIds0_0_0) } };
static const Demo3DCmd sDemo3D01_EndCmds[] = {
    { DEMO3D_CMD_SE_STOP, 0, { SEQ_SE_FLD_79, -1 } },
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D25_FileIds1_0_3[] = { 363, 372 };
static const u16 sDemo3D05_FileIds5_1_0[] = { 276, 275 };
static const u16 sDemo3D19_FileIds1_0_0[] = { 36, 38, 37, 34, 35 };
static const Demo3DFiles sDemo3D22_Files3_0[] = { { sDemo3D22_FileIds3_0_0, NELEMS(sDemo3D22_FileIds3_0_0) } };
static const Demo3DFiles sDemo3D10_Files1_0[] = { { sDemo3D10_FileIds1_0_0, NELEMS(sDemo3D10_FileIds1_0_0) } };
static const u16 sDemo3D11_FileIds1_0_0[] = { 449, 448 };
static const Demo3DCmd sDemo3D11_EndCmds[] = {
    { DEMO3D_CMD_END },
};
static const Demo3DCmd sDemo3D17_Cmds[] = {
    { DEMO3D_CMD_BGM_PLAY, DEMO3D_CMD_FRAME_START, { SEQ_BGM_SW_DEMO_UNION_01 } },
    { DEMO3D_CMD_SE_PLAY, 40, { SEQ_SE_SW_KYUREON_01_01, 120, -1, -1 } },
    { DEMO3D_CMD_CAMERA, 89, { 57344, 57344, 75, -65 } },
    { DEMO3D_CMD_SE_PLAY, 93, { SEQ_SE_SW_KYUREON_01_01b, 120, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 149, { SEQ_SE_SW_KYUREON_01_02a, 120, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 149, { SEQ_SE_SW_KYUREON_01_02b, 120, -1, -1, 2 } },
    { DEMO3D_CMD_CAMERA, 180, { 110592, 110592, 149, -142 } },
    { DEMO3D_CMD_SE_PLAY, 181, { SEQ_SE_SW_KYUREON_01_03, 120, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 185, { SEQ_SE_SW_KYUREON_01_04, 120, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 215, { SEQ_SE_SW_KYUREON_01_05, 120, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 215, { SEQ_SE_SW_KYUREON_01_14b, 120, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 268, { SEQ_SE_SW_KYUREON_01_15, 120, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 270, { SEQ_SE_SW_KYUREON_01_02a, 120, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 274, { SEQ_SE_SW_KYUREON_01_15, 120, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 276, { SEQ_SE_SW_KYUREON_01_02a, 120, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 339, { SEQ_SE_SW_KYUREON_01_06, 120, -1, -1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 360, { SEQ_SE_SW_KYUREON_01_06, 120, 0, 49 } },
    { DEMO3D_CMD_SE_PLAY, 375, { SEQ_SE_SW_KYUREON_01_05, 120, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 409, { SEQ_SE_SW_KYUREON_01_07, 120, -1, -1, 1 } },
    { DEMO3D_CMD_SE_STOP, 410, { SEQ_SE_SW_KYUREON_01_06 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 435, { SEQ_SE_SW_KYUREON_01_07, 120, 0, 45, 1 } },
    { DEMO3D_CMD_SE_STOP, 481, { SEQ_SE_SW_KYUREON_01_07, 1 } },
    { DEMO3D_CMD_SE_PLAY, 505, { SEQ_SE_SW_KYUREON_01_16, 120, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 515, { SEQ_SE_SW_KYUREON_01_02a, 120, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 515, { SEQ_SE_SW_KYUREON_01_02b, 120, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 565, { SEQ_SE_SW_KYUREON_01_07b, 120, -1, -1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 577, { SEQ_SE_SW_KYUREON_01_07b, 120, 0, 44, 2 } },
    { DEMO3D_CMD_SE_PLAY, 621, { SEQ_SE_SW_KYUREON_01_07b, 120, -1, -1 } },
    { DEMO3D_CMD_SE_STOP, 622, { SEQ_SE_SW_KYUREON_01_07b, 2 } },
    { DEMO3D_CMD_SE_PLAY, 659, { SEQ_SE_SW_KYUREON_01_08, 120, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 659, { SEQ_SE_SW_KYUREON_01_09, 1, -1, -1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 659, { SEQ_SE_SW_KYUREON_01_09, 1, 120, 30, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 660, { SEQ_SE_SW_KYUREON_01_07b, 120, 0, 50 } },
    { DEMO3D_CMD_FLASH, 675, { 1, 0, 12, 0, 25, 10 } },
    { DEMO3D_CMD_SE_PLAY, 707, { SEQ_SE_SW_KYUREON_01_10, 120, -1, -1, 2 } },
    { DEMO3D_CMD_SE_STOP, 711, { SEQ_SE_SW_KYUREON_01_07b } },
    { DEMO3D_CMD_FLASH, 760, { 1, 0, 16, 0, 20, 30 } },
    { DEMO3D_CMD_SE_PLAY, 792, { SEQ_SE_SW_KYUREON_01_16, 120, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 819, { SEQ_SE_SW_KYUREON_01_11, 120, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 884, { SEQ_SE_SW_KYUREON_01_12, 90, -1, -1, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 885, { SEQ_SE_SW_KYUREON_01_11, 120, 0, 110 } },
    { DEMO3D_CMD_SE_PLAY, 942, { SEQ_SE_SW_KYUREON_01_13, 90, -1, -1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 945, { SEQ_SE_SW_KYUREON_01_12, 90, 0, 100, 1 } },
    { DEMO3D_CMD_SE_STOP, 996, { SEQ_SE_SW_KYUREON_01_11 } },
    { DEMO3D_CMD_SE_STOP, 1046, { SEQ_SE_SW_KYUREON_01_12, 1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 1050, { 1, 50, 0, -16 } },
    { DEMO3D_CMD_END },
};
static const Demo3DCmd sDemo3D10_Cmds[] = {
    { DEMO3D_CMD_SE_PLAY, 1, { SEQ_SE_VDEMO_01, 100 } },
    { DEMO3D_CMD_SE_PLAY, 160, { SEQ_SE_VDEMO_02, 100, 0, -300, 1 } },
    { DEMO3D_CMD_SE_STOP, 179, { SEQ_SE_VDEMO_01 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 210, { 1, 30, 0, -16 } },
    { DEMO3D_CMD_END },
};
static const Demo3DVariant sDemo3D10_Variants0[] = { { sDemo3D10_Files0_0, NELEMS(sDemo3D10_Files0_0) } };
static const Demo3DVariant sDemo3D10_Variants1[] = { { sDemo3D10_Files1_0, NELEMS(sDemo3D10_Files1_0) } };
static const Demo3DSetup sDemo3D19_Setup = {
    16, 0xffff, ZONE_NUVEMA_TOWN,
    1, 1, 1, 1, 0, 0, 0, 1, 0, 16,
    FX32_CONST(12), FX32_CONST(12), 0x1555, 0x19a, FX32_CONST(2048), FX32_CONST(1),
    64, -65, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    TRUE, 5, FALSE, 0,
};
static const Demo3DCmd sDemo3D09_Cmds[] = {
    { DEMO3D_CMD_BGM_PLAY, 0, { SEQ_BGM_E_LEGEND_BORN_W } },
    { DEMO3D_CMD_SE_PLAY, 42, { SEQ_SE_LBDEMO_02, 40 } },
    { DEMO3D_CMD_SE_PLAY, 135, { SEQ_SE_LBDEMO_01, 80, 0, 0, 1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 220, { 1, 50, 0, 12 } },
    { DEMO3D_CMD_SE_PLAY, 263, { SEQ_SE_FLD_127, 127, 0, 0, 2 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 271, { 1, 59, 12 } },
    { DEMO3D_CMD_SE_PLAY, 382, { SEQ_SE_LBDEMO_03, 127 } },
    { DEMO3D_CMD_SE_PLAY, 430, { SEQ_SE_LBDEMO_03, 127 } },
    { DEMO3D_CMD_SE_PLAY, 495, { SEQ_SE_LBDEMO_03, 127 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 510, { 1, 15, 0, 8 } },
    { DEMO3D_CMD_SE_PLAY, 525, { SEQ_SE_LBDEMO_05, 127, 0, 0, 1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 545, { 1, 25, 8 } },
    { DEMO3D_CMD_SE_PLAY, 616, { SEQ_SE_LBDEMO_04, 127, -128, 0, 2 } },
    { DEMO3D_CMD_SE_PLAY, 616, { SEQ_SE_LBDEMO_04, 127, 127 } },
    { DEMO3D_CMD_SE_PLAY, 746, { SEQ_SE_LBDEMO_09, 127, 0, 0, 1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 826, { 1, 1, 0, 12 } },
    { DEMO3D_CMD_SE_PLAY, 827, { SEQ_SE_W307_EXP, 127 } },
    { DEMO3D_CMD_SE_PLAY, 827, { SEQ_SE_FLD_166, 115, 0, 0, 2 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 827, { 1, 2, 12 } },
    { DEMO3D_CMD_SE_PLAY, 840, { SEQ_SE_W348_02, 127, 0, 0, 1 } },
    { DEMO3D_CMD_SE_PLAY, 1080, { SEQ_SE_SHINKA_W060C, 100, 0, 0, 2 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 1159, { 1, 15, 0, -16 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 1179, { 1, 15, -16 } },
    { DEMO3D_CMD_BGM_FADE, 1310, { 1, 50 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 1310, { 1, 50, 0, -16 } },
    { DEMO3D_CMD_END },
};
static const Demo3DVariant sDemo3D22_Variants2[] = { { sDemo3D22_Files2_0, NELEMS(sDemo3D22_Files2_0) } };
static const Demo3DVariant sDemo3D22_Variants3[] = { { sDemo3D22_Files3_0, NELEMS(sDemo3D22_Files3_0) } };
static const Demo3DFiles sDemo3D11_Files0_0[] = { { sDemo3D11_FileIds0_0_0, NELEMS(sDemo3D11_FileIds0_0_0) } };
static const Demo3DFiles sDemo3D11_Files1_0[] = { { sDemo3D11_FileIds1_0_0, NELEMS(sDemo3D11_FileIds1_0_0) } };
static const Demo3DFiles sDemo3D19_Files1_0[] = { { sDemo3D19_FileIds1_0_0, NELEMS(sDemo3D19_FileIds1_0_0) } };
static const u16 sDemo3D19_FileIds3_0_0[] = { 142, 143, 140, 141, 144 };
static const Demo3DSetup sDemo3D23_Setup = {
    441, 0xffff, ZONE_NUVEMA_TOWN,
    1, 0, 1, 1, 0, 0, 1, 1, 0, 16,
    FX32_CONST(40), FX32_CONST(40), 0x1555, 0x19a, FX32_CONST(2048), FX32_CONST(1),
    0, 0, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    FALSE, 30, FALSE, 0,
};
static const Demo3DFiles sDemo3D03_Files1_2[] = { { sDemo3D03_FileIds1_2_0, NELEMS(sDemo3D03_FileIds1_2_0) } };
static const u16 sDemo3D10_FileIds2_0_4[] = { 469, 471 };
static const Demo3DVariant sDemo3D11_Variants1[] = { { sDemo3D11_Files1_0, NELEMS(sDemo3D11_Files1_0) } };
static const Demo3DFiles sDemo3D19_Files2_0[] = { { sDemo3D19_FileIds2_0_0, NELEMS(sDemo3D19_FileIds2_0_0) } };
static const Demo3DVariant sDemo3D02_Variants2[] = { { sDemo3D02_Files2_0, NELEMS(sDemo3D02_Files2_0) } };
static const Demo3DFiles sDemo3D19_Files3_0[] = { { sDemo3D19_FileIds3_0_0, NELEMS(sDemo3D19_FileIds3_0_0) } };
static const Demo3DVariant sDemo3D24_Variants1[] = { { sDemo3D24_Files1_0, NELEMS(sDemo3D24_Files1_0) } };
static const u16 sDemo3D05_FileIds5_0_0[] = { 274, 273 };
static const u16 sDemo3D05_FileIds1_0_1[] = { 295, 296 };
static const Demo3DVariant sDemo3D06_Variants2[] = { { sDemo3D06_Files2_0, NELEMS(sDemo3D06_Files2_0) } };
static const Demo3DVariant sDemo3D06_Variants1[] = { { sDemo3D06_Files1_0, NELEMS(sDemo3D06_Files1_0) } };
static const Demo3DFiles sDemo3D19_Files4_0[] = { { sDemo3D19_FileIds4_0_0, NELEMS(sDemo3D19_FileIds4_0_0) } };
static const Demo3DVariant sDemo3D19_Variants0[] = { { sDemo3D19_Files0_0, NELEMS(sDemo3D19_Files0_0) } };
static const Demo3DCmd sDemo3D19_Cmds[] = {
    { DEMO3D_CMD_BGM_PLAY, DEMO3D_CMD_FRAME_START, { SEQ_BGM_SW_DEMO_UNION_03B } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 1, { 1, 19, 16, 16 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 20, { 1, 30, 16 } },
    { DEMO3D_CMD_SE_PLAY, 80, { SEQ_SE_SW_KYUREON_03_01, 127, -1, -1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 115, { 1, 5, 0, 12 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 120, { 1, 24, 12 } },
    { DEMO3D_CMD_SE_PLAY, 192, { SEQ_SE_SW_KYUREON_03_02, 127, -1, -1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 205, { 1, 5, 0, 12 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 210, { 1, 24, 12 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 295, { 1, 5, 0, 12 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 300, { 1, 24, 12 } },
    { DEMO3D_CMD_SE_PLAY, 386, { SEQ_SE_MEET_02, 80, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 504, { SEQ_SE_MEET_02, 80, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 601, { SEQ_SE_SW_KYUREON_03_03, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 639, { SEQ_SE_SW_KYUREON_03_04, 127, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 660, { SEQ_SE_SW_KYUREON_03_05, 127, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 682, { SEQ_SE_SW_KYUREON_03_06b, 80, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 682, { SEQ_SE_SW_KYUREON_03_07b, 80, -1, -1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 861, { SEQ_SE_SW_KYUREON_03_06b, 80, 80, 1, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 861, { SEQ_SE_SW_KYUREON_03_07b, 80, 80, 1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 950, { SEQ_SE_SW_KYUREON_03_06b, 80, 0, 60, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 950, { SEQ_SE_SW_KYUREON_03_07b, 80, 0, 60, 2 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 980, { 1, 30, 0, -16 } },
    { DEMO3D_CMD_END },
};
static const Demo3DVariant sDemo3D12_Variants2[] = { { sDemo3D12_Files2_0, NELEMS(sDemo3D12_Files2_0) } };
static const Demo3DCmd sDemo3D12_EndCmds[] = {
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D05_FileIds3_1_0[] = { 281, 283 };
static const Demo3DVariant sDemo3D19_Variants3[] = { { sDemo3D19_Files3_0, NELEMS(sDemo3D19_Files3_0) } };
static const u16 sDemo3D10_FileIds2_0_3[] = { 469, 471 };
static const Demo3DFiles sDemo3D04_Files2_0[] = { { sDemo3D04_FileIds2_0_0, NELEMS(sDemo3D04_FileIds2_0_0) } };
static const Demo3DFiles sDemo3D03_Files0_1[] = { { sDemo3D03_FileIds0_1_0, NELEMS(sDemo3D03_FileIds0_1_0) } };
static const Demo3DFiles sDemo3D05_Files6_0[] = { { sDemo3D05_FileIds6_0_0, NELEMS(sDemo3D05_FileIds6_0_0) } };
static const u16 sDemo3D18_FileIds1_0_0[] = { 19 };
static const Demo3DFiles sDemo3D13_Files0_0[] = { { sDemo3D13_FileIds0_0_0, NELEMS(sDemo3D13_FileIds0_0_0) } };
static const Demo3DFiles sDemo3D26_Files0_0[] = { { sDemo3D26_FileIds0_0_0, NELEMS(sDemo3D26_FileIds0_0_0) } };
static const u16 sDemo3D23_FileIds1_0_1[] = { 390, 391, 388, 389 };
static const Demo3DVariant sDemo3D09_Variants1[] = { { sDemo3D09_Files1_0, NELEMS(sDemo3D09_Files1_0) } };
static const Demo3DVariant sDemo3D08_Variants1[] = { { sDemo3D08_Files1_0, NELEMS(sDemo3D08_Files1_0) } };
static const Demo3DVariant sDemo3D18_Variants2[] = { { sDemo3D18_Files2_0, NELEMS(sDemo3D18_Files2_0) } };
static const Demo3DSetup sDemo3D13_Setup = {
    382, 0xffff, ZONE_NUVEMA_TOWN,
    1, 1, 1, 1, 0, 0, 0, 1, 0, 16,
    FX32_CONST(28), FX32_CONST(28), 0x1555, 0x19a, FX32_CONST(2048), 0x19a,
    149, -142, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    TRUE, 30, FALSE, 0,
};
static const Demo3DFiles sDemo3D20_Files0_0[] = { { sDemo3D20_FileIds0_0_0, NELEMS(sDemo3D20_FileIds0_0_0) } };
static const Demo3DVariant sDemo3D20_Variants4[] = { { sDemo3D20_Files4_0, NELEMS(sDemo3D20_Files4_0) } };
static const u16 sDemo3D24_FileIds2_0_0[] = { 335, 339, 337 };
static const u16 sDemo3D26_FileIds1_0_1[] = { 401, 402, 400 };
static const u16 sDemo3D14_FileIds0_0_0[] = { 86, 87, 84, 85, 88 };
static const u16 sDemo3D14_FileIds0_1_0[] = { 97, 98, 95, 96, 99 };
static const u16 sDemo3D14_FileIds1_0_0[] = { 411, 412, 410, 413 };
static const u16 sDemo3D14_FileIds1_0_3[] = { 429, 430, 428, 431 };
static const u16 sDemo3D18_FileIds0_0_0[] = { 27 };
static const Demo3DFiles sDemo3D18_Files3_0[] = { { sDemo3D18_FileIds3_0_0, NELEMS(sDemo3D18_FileIds3_0_0) } };
static const Demo3DVariant sDemo3D21_Variants0[] = { { sDemo3D21_Files0_0, NELEMS(sDemo3D21_Files0_0) } };
static const Demo3DCmd sDemo3D14_EndCmds[] = {
    { DEMO3D_CMD_BGM_STOP, 0, { 0 } },
    { DEMO3D_CMD_END },
};
static const Demo3DFiles sDemo3D14_Files0_1[] = { { sDemo3D14_FileIds0_1_0, NELEMS(sDemo3D14_FileIds0_1_0) } };
static const u16 sDemo3D20_FileIds5_0_0[] = { 174, 175, 172, 173, 176 };
static const G3DSceneAnimationSetup sDemo3DAnimations[] = {
    { 1, 0 },
    { 2, 0 },
    { 3, 0 },
    { 4, 0 },
    { 5, 0 },
};
static const Demo3DFiles sDemo3D03_Files1_8[] = { { sDemo3D03_FileIds1_8_0, NELEMS(sDemo3D03_FileIds1_8_0) } };
static const Demo3DFiles sDemo3D14_Files0_0[] = { { sDemo3D14_FileIds0_0_0, NELEMS(sDemo3D14_FileIds0_0_0) } };
static const u16 sDemo3D05_FileIds5_3_0[] = { 278, 277 };
static const Demo3DFiles sDemo3D03_Files1_12[] = { { sDemo3D03_FileIds1_12_0, NELEMS(sDemo3D03_FileIds1_12_0) } };
static const Demo3DVariant sDemo3D02_Variants0[] = { { sDemo3D02_Files0_0, NELEMS(sDemo3D02_Files0_0) } };
static const u16 sDemo3D05_FileIds3_3_0[] = { 282, 283 };
static const Demo3DFiles sDemo3D05_Files4_1[] = { { sDemo3D05_FileIds4_1_0, NELEMS(sDemo3D05_FileIds4_1_0) } };
static const Demo3DSetup sDemo3D20_Setup = {
    472, 0xffff, ZONE_NUVEMA_TOWN,
    1, 1, 1, 1, 0, 0, 0, 1, 0, 16,
    FX32_CONST(27), FX32_CONST(27), 0x1555, 0x19a, FX32_CONST(2048), FX32_CONST(1),
    149, -142, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    FALSE, 15, FALSE, 0,
};
static const Demo3DFiles sDemo3D20_Files5_0[] = { { sDemo3D20_FileIds5_0_0, NELEMS(sDemo3D20_FileIds5_0_0) } };
static const Demo3DFiles sDemo3D01_Files4_0[] = { { sDemo3D01_FileIds4_0_0, NELEMS(sDemo3D01_FileIds4_0_0) } };
static const Demo3DFiles sDemo3D23_Files1_0[] = {
    { sDemo3D23_FileIds1_0_0, NELEMS(sDemo3D23_FileIds1_0_0) },
    { sDemo3D23_FileIds1_0_1, NELEMS(sDemo3D23_FileIds1_0_1) },
    { sDemo3D23_FileIds1_0_2, NELEMS(sDemo3D23_FileIds1_0_2) },
    { sDemo3D23_FileIds1_0_3, NELEMS(sDemo3D23_FileIds1_0_3) },
    { sDemo3D23_FileIds1_0_4, NELEMS(sDemo3D23_FileIds1_0_4) },
};
static const Demo3DFiles sDemo3D16_Files3_0[] = { { sDemo3D16_FileIds3_0_0, NELEMS(sDemo3D16_FileIds3_0_0) } };
static const Demo3DCmd sDemo3D21_EndCmds[] = {
    { DEMO3D_CMD_BGM_STOP, 0, { 0 } },
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D03_FileIds1_9_0[] = { 244, 252, 437 };
static const Demo3DCmd sDemo3D16_EndCmds[] = {
    { DEMO3D_CMD_BGM_STOP, 0, { 0 } },
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D25_FileIds1_0_0[] = { 363, 367, 365 };
static const Demo3DFiles sDemo3D25_Files0_0[] = { { sDemo3D25_FileIds0_0_0, NELEMS(sDemo3D25_FileIds0_0_0) } };
static const Demo3DFiles sDemo3D05_Files3_2[] = { { sDemo3D05_FileIds3_2_0, NELEMS(sDemo3D05_FileIds3_2_0) } };
static const Demo3DFiles sDemo3D03_Files4_0[] = { { sDemo3D03_FileIds4_0_0, NELEMS(sDemo3D03_FileIds4_0_0) } };
static const Demo3DVariant sDemo3D01_Variants4[] = { { sDemo3D01_Files4_0, NELEMS(sDemo3D01_Files4_0) } };
static const Demo3DVariant sDemo3D20_Variants5[] = { { sDemo3D20_Files5_0, NELEMS(sDemo3D20_Files5_0) } };
static const u16 sDemo3D25_FileIds1_0_2[] = { 363, 370, 368 };
static const u16 sDemo3D05_FileIds4_0_0[] = { 285, 283 };
static const Demo3DFiles sDemo3D17_Files0_0[] = { { sDemo3D17_FileIds0_0_0, NELEMS(sDemo3D17_FileIds0_0_0) } };
static const Demo3DVariant sDemo3D18_Variants3[] = { { sDemo3D18_Files3_0, NELEMS(sDemo3D18_Files3_0) } };
static const Demo3DVariant sDemo3D01_Variants5[] = { { sDemo3D01_Files5_0, NELEMS(sDemo3D01_Files5_0) } };
static const Demo3DVariant sDemo3D04_Variants2[] = { { sDemo3D04_Files2_0, NELEMS(sDemo3D04_Files2_0) } };
static const Demo3DVariant sDemo3D04_Variants3[] = { { sDemo3D04_Files3_0, NELEMS(sDemo3D04_Files3_0) } };
static const u16 sDemo3D03_FileIds2_1_0[] = { 214, 215, 213 };
static const u16 sDemo3D03_FileIds0_0_0[] = { 239, 247, 268 };
static const Demo3DFiles sDemo3D03_Files3_0[] = { { sDemo3D03_FileIds3_0_0, NELEMS(sDemo3D03_FileIds3_0_0) } };
static const Demo3DCmd sDemo3D06_EndCmds[] = {
    { DEMO3D_CMD_BGM_STOP, 0, { 0 } },
    { DEMO3D_CMD_END },
};
static const Demo3DFiles sDemo3D05_Files5_1[] = { { sDemo3D05_FileIds5_1_0, NELEMS(sDemo3D05_FileIds5_1_0) } };
static const Demo3DFiles sDemo3D05_Files5_0[] = { { sDemo3D05_FileIds5_0_0, NELEMS(sDemo3D05_FileIds5_0_0) } };
static const Demo3DFiles sDemo3D26_Files1_0[] = {
    { sDemo3D26_FileIds1_0_0, NELEMS(sDemo3D26_FileIds1_0_0) },
    { sDemo3D26_FileIds1_0_1, NELEMS(sDemo3D26_FileIds1_0_1) },
    { sDemo3D26_FileIds1_0_2, NELEMS(sDemo3D26_FileIds1_0_2) },
    { sDemo3D26_FileIds1_0_3, NELEMS(sDemo3D26_FileIds1_0_3) },
    { sDemo3D26_FileIds1_0_4, NELEMS(sDemo3D26_FileIds1_0_4) },
};
static const u16 sDemo3D10_FileIds2_0_2[] = { 469, 471 };
static const Demo3DFiles sDemo3D05_Files3_1[] = { { sDemo3D05_FileIds3_1_0, NELEMS(sDemo3D05_FileIds3_1_0) } };
static const Demo3DVariant sDemo3D16_Variants0[] = { { sDemo3D16_Files0_0, NELEMS(sDemo3D16_Files0_0) } };
static const u16 sDemo3D17_FileIds5_0_0[] = { 162, 163, 160, 161, 164 };
static const Demo3DCmd sDemo3D04_EndCmds[] = {
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D03_FileIds1_0_0[] = { 229, 254, 437 };
static const Demo3DFiles sDemo3D24_Files0_0[] = { { sDemo3D24_FileIds0_0_0, NELEMS(sDemo3D24_FileIds0_0_0) } };
static const Demo3DCmd sDemo3D24_EndCmds[] = {
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D24_FileIds0_1_0[] = { 354, 356, 355, 353 };
static const u16 sDemo3D10_FileIds2_0_0[] = { 469, 470 };
static const Demo3DFiles sDemo3D14_Files0_3[] = { { sDemo3D14_FileIds0_3_0, NELEMS(sDemo3D14_FileIds0_3_0) } };
static const Demo3DFiles sDemo3D15_Files0_3[] = { { sDemo3D15_FileIds0_3_0, NELEMS(sDemo3D15_FileIds0_3_0) } };
static const Demo3DFiles sDemo3D15_Files0_2[] = { { sDemo3D15_FileIds0_2_0, NELEMS(sDemo3D15_FileIds0_2_0) } };
static const Demo3DUnit sDemo3D02_Units[] = {
    { sDemo3D02_Variants0, NELEMS(sDemo3D02_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D02_Variants1, NELEMS(sDemo3D02_Variants1), DEMO3D_UNIT_SINGLE },
    { sDemo3D02_Variants2, NELEMS(sDemo3D02_Variants2), DEMO3D_UNIT_SINGLE },
};
static const Demo3DVariant sDemo3D20_Variants3[] = { { sDemo3D20_Files3_0, NELEMS(sDemo3D20_Files3_0) } };
static const Demo3DVariant sDemo3D20_Variants1[] = { { sDemo3D20_Files1_0, NELEMS(sDemo3D20_Files1_0) } };
static const Demo3DCmd sDemo3D20_Cmds[] = {
    { DEMO3D_CMD_BGM_PLAY, DEMO3D_CMD_FRAME_START, { SEQ_BGM_SW_DEMO_UNION_01 } },
    { DEMO3D_CMD_SE_PLAY, 40, { SEQ_SE_SW_KYUREON_01_01, 120, -1, -1 } },
    { DEMO3D_CMD_CAMERA, 89, { 57344, 57344, 75, -65 } },
    { DEMO3D_CMD_SE_PLAY, 93, { SEQ_SE_SW_KYUREON_01_01b, 120, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 149, { SEQ_SE_SW_KYUREON_01_02a, 120, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 149, { SEQ_SE_SW_KYUREON_01_02b, 120, -1, -1, 2 } },
    { DEMO3D_CMD_CAMERA, 180, { 110592, 110592, 149, -142 } },
    { DEMO3D_CMD_SE_PLAY, 181, { SEQ_SE_SW_KYUREON_01_03, 120, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 185, { SEQ_SE_SW_KYUREON_01_04, 120, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 215, { SEQ_SE_SW_KYUREON_01_05, 120, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 215, { SEQ_SE_SW_KYUREON_01_14w, 120, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 268, { SEQ_SE_SW_KYUREON_01_15, 120, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 270, { SEQ_SE_SW_KYUREON_01_02a, 120, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 274, { SEQ_SE_SW_KYUREON_01_15, 120, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 276, { SEQ_SE_SW_KYUREON_01_02a, 120, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 339, { SEQ_SE_SW_KYUREON_01_06, 120, -1, -1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 360, { SEQ_SE_SW_KYUREON_01_06, 120, 0, 49 } },
    { DEMO3D_CMD_SE_PLAY, 375, { SEQ_SE_SW_KYUREON_01_05, 120, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 409, { SEQ_SE_SW_KYUREON_01_07, 120, -1, -1, 1 } },
    { DEMO3D_CMD_SE_STOP, 410, { SEQ_SE_SW_KYUREON_01_06 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 435, { SEQ_SE_SW_KYUREON_01_07, 120, 0, 45, 1 } },
    { DEMO3D_CMD_SE_STOP, 481, { SEQ_SE_SW_KYUREON_01_07, 1 } },
    { DEMO3D_CMD_SE_PLAY, 505, { SEQ_SE_SW_KYUREON_01_16, 120, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 515, { SEQ_SE_SW_KYUREON_01_02a, 120, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 515, { SEQ_SE_SW_KYUREON_01_02b, 120, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 565, { SEQ_SE_SW_KYUREON_01_07b, 120, -1, -1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 577, { SEQ_SE_SW_KYUREON_01_07b, 120, 0, 44, 2 } },
    { DEMO3D_CMD_SE_PLAY, 621, { SEQ_SE_SW_KYUREON_01_07b, 120, -1, -1 } },
    { DEMO3D_CMD_SE_STOP, 622, { SEQ_SE_SW_KYUREON_01_07b, 2 } },
    { DEMO3D_CMD_SE_PLAY, 659, { SEQ_SE_SW_KYUREON_01_08, 120, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 659, { SEQ_SE_SW_KYUREON_01_09, 1, -1, -1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 659, { SEQ_SE_SW_KYUREON_01_09, 1, 120, 30, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 660, { SEQ_SE_SW_KYUREON_01_07b, 120, 0, 50 } },
    { DEMO3D_CMD_FLASH, 675, { 1, 0, 12, 0, 25, 10 } },
    { DEMO3D_CMD_SE_PLAY, 707, { SEQ_SE_SW_KYUREON_01_10, 120, -1, -1, 2 } },
    { DEMO3D_CMD_SE_STOP, 711, { SEQ_SE_SW_KYUREON_01_07b } },
    { DEMO3D_CMD_FLASH, 760, { 1, 0, 16, 0, 20, 30 } },
    { DEMO3D_CMD_SE_PLAY, 792, { SEQ_SE_SW_KYUREON_01_16, 120, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 819, { SEQ_SE_SW_KYUREON_01_11, 120, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 884, { SEQ_SE_SW_KYUREON_01_12, 90, -1, -1, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 885, { SEQ_SE_SW_KYUREON_01_11, 120, 0, 110 } },
    { DEMO3D_CMD_SE_PLAY, 942, { SEQ_SE_SW_KYUREON_01_13, 90, -1, -1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 945, { SEQ_SE_SW_KYUREON_01_12, 90, 0, 100, 1 } },
    { DEMO3D_CMD_SE_STOP, 996, { SEQ_SE_SW_KYUREON_01_11 } },
    { DEMO3D_CMD_SE_STOP, 1046, { SEQ_SE_SW_KYUREON_01_12, 1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 1050, { 1, 50, 0, -16 } },
    { DEMO3D_CMD_END },
};
static const Demo3DVariant sDemo3D21_Variants1[] = { { sDemo3D21_Files1_0, NELEMS(sDemo3D21_Files1_0) } };
static const u16 sDemo3D24_FileIds0_2_0[] = { 346, 348, 347, 345 };
static const Demo3DCmd sDemo3D08_Cmds[] = {
    { DEMO3D_CMD_BGM_PLAY, 0, { SEQ_BGM_E_LEGEND_BORN_B } },
    { DEMO3D_CMD_SE_PLAY, 42, { SEQ_SE_LBDEMO_02, 40 } },
    { DEMO3D_CMD_SE_PLAY, 135, { SEQ_SE_LBDEMO_01, 80, 0, 0, 1 } },
    { DEMO3D_CMD_SE_PLAY, 270, { SEQ_SE_FLD_127, 127, 0, 0, 2 } },
    { DEMO3D_CMD_SE_PLAY, 382, { SEQ_SE_LBDEMO_03, 127, 0, 0, 2 } },
    { DEMO3D_CMD_SE_PLAY, 390, { SEQ_SE_DUMMY5 } },
    { DEMO3D_CMD_SE_PLAY, 430, { SEQ_SE_LBDEMO_03, 127, 0, 0, 2 } },
    { DEMO3D_CMD_SE_PLAY, 495, { SEQ_SE_LBDEMO_03, 127, 0, 0, 2 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 510, { 1, 15, 0, 8 } },
    { DEMO3D_CMD_SE_PLAY, 525, { SEQ_SE_LBDEMO_05, 127, 0, 0, 1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 545, { 1, 25, 8 } },
    { DEMO3D_CMD_SE_PLAY, 589, { SEQ_SE_W307_02, 127 } },
    { DEMO3D_CMD_SE_PLAY, 601, { SEQ_SE_W307_02, 127, 0, -500, 2 } },
    { DEMO3D_CMD_SE_PLAY, 616, { SEQ_SE_LBDEMO_04, 127, -128 } },
    { DEMO3D_CMD_SE_PLAY, 616, { SEQ_SE_LBDEMO_04, 127, 127, 0, 1 } },
    { DEMO3D_CMD_SE_PLAY, 746, { SEQ_SE_LBDEMO_06, 117 } },
    { DEMO3D_CMD_SE_PLAY, 827, { SEQ_SE_FLD_127, 100, 0, 0, 1 } },
    { DEMO3D_CMD_SE_PLAY, 832, { SEQ_SE_W307_EXP, 127, -128, -1000, 2 } },
    { DEMO3D_CMD_SE_PLAY, 832, { SEQ_SE_W307_EXP, 127, 127, -1000 } },
    { DEMO3D_CMD_SE_PLAY, 837, { SEQ_SE_FLD_127, 127, 0, 0, 1 } },
    { DEMO3D_CMD_SE_PLAY, 853, { SEQ_SE_W307_EXP, 127, 0, -1000, 2 } },
    { DEMO3D_CMD_SE_PLAY, 876, { SEQ_SE_LBDEMO_07, 127, 0, 0, 1 } },
    { DEMO3D_CMD_SE_PLAY, 1080, { SEQ_SE_FLD_131, 127 } },
    { DEMO3D_CMD_SE_PLAY, 1114, { SEQ_SE_DUMMY5, 0, 0, 0, 1 } },
    { DEMO3D_CMD_SE_PLAY, 1114, { SEQ_SE_DUMMY5, 0, 0, 0, 2 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 1159, { 1, 15, 0, -16 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 1179, { 1, 15, -16 } },
    { DEMO3D_CMD_BGM_FADE, 1310, { 1, 50 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 1310, { 1, 50, 0, -16 } },
    { DEMO3D_CMD_END },
};
static const Demo3DFiles sDemo3D17_Files3_0[] = { { sDemo3D17_FileIds3_0_0, NELEMS(sDemo3D17_FileIds3_0_0) } };
static const u16 sDemo3D06_FileIds0_0_0[] = { 182, 183, 180, 181 };
static const Demo3DFiles sDemo3D06_Files0_0[] = { { sDemo3D06_FileIds0_0_0, NELEMS(sDemo3D06_FileIds0_0_0) } };
static const Demo3DVariant sDemo3D06_Variants0[] = { { sDemo3D06_Files0_0, NELEMS(sDemo3D06_Files0_0) } };
static const Demo3DVariant sDemo3D12_Variants1[] = { { sDemo3D12_Files1_0, NELEMS(sDemo3D12_Files1_0) } };
static const Demo3DFiles sDemo3D05_Files1_0[] = {
    { sDemo3D05_FileIds1_0_0, NELEMS(sDemo3D05_FileIds1_0_0) },
    { sDemo3D05_FileIds1_0_1, NELEMS(sDemo3D05_FileIds1_0_1) },
    { sDemo3D05_FileIds1_0_2, NELEMS(sDemo3D05_FileIds1_0_2) },
    { sDemo3D05_FileIds1_0_3, NELEMS(sDemo3D05_FileIds1_0_3) },
    { sDemo3D05_FileIds1_0_4, NELEMS(sDemo3D05_FileIds1_0_4) },
};
static const Demo3DVariant sDemo3D17_Variants1[] = { { sDemo3D17_Files1_0, NELEMS(sDemo3D17_Files1_0) } };
static const Demo3DFiles sDemo3D01_Files0_0[] = { { sDemo3D01_FileIds0_0_0, NELEMS(sDemo3D01_FileIds0_0_0) } };
static const Demo3DVariant sDemo3D17_Variants3[] = { { sDemo3D17_Files3_0, NELEMS(sDemo3D17_Files3_0) } };
static const Demo3DVariant sDemo3D19_Variants4[] = { { sDemo3D19_Files4_0, NELEMS(sDemo3D19_Files4_0) } };
static const Demo3DUnit sDemo3D21_Units[] = {
    { sDemo3D21_Variants0, NELEMS(sDemo3D21_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D21_Variants1, NELEMS(sDemo3D21_Variants1), DEMO3D_UNIT_SINGLE },
    { sDemo3D21_Variants2, NELEMS(sDemo3D21_Variants2), DEMO3D_UNIT_SINGLE },
    { sDemo3D21_Variants3, NELEMS(sDemo3D21_Variants3), DEMO3D_UNIT_SINGLE },
    { sDemo3D21_Variants4, NELEMS(sDemo3D21_Variants4), DEMO3D_UNIT_SINGLE },
};
static const Demo3DFiles sDemo3D01_Files2_0[] = { { sDemo3D01_FileIds2_0_0, NELEMS(sDemo3D01_FileIds2_0_0) } };
static const Demo3DFiles sDemo3D03_Files1_7[] = { { sDemo3D03_FileIds1_7_0, NELEMS(sDemo3D03_FileIds1_7_0) } };
static const Demo3DFiles sDemo3D01_Files3_0[] = { { sDemo3D01_FileIds3_0_0, NELEMS(sDemo3D01_FileIds3_0_0) } };
static const u16 sDemo3D08_FileIds1_0_0[] = { 311, 312, 309, 310 };
static const Demo3DFiles sDemo3D18_Files0_0[] = { { sDemo3D18_FileIds0_0_0, NELEMS(sDemo3D18_FileIds0_0_0) } };
static const Demo3DFiles sDemo3D25_Files1_0[] = {
    { sDemo3D25_FileIds1_0_0, NELEMS(sDemo3D25_FileIds1_0_0) },
    { sDemo3D25_FileIds1_0_1, NELEMS(sDemo3D25_FileIds1_0_1) },
    { sDemo3D25_FileIds1_0_2, NELEMS(sDemo3D25_FileIds1_0_2) },
    { sDemo3D25_FileIds1_0_3, NELEMS(sDemo3D25_FileIds1_0_3) },
    { sDemo3D25_FileIds1_0_4, NELEMS(sDemo3D25_FileIds1_0_4) },
};
static const Demo3DVariant sDemo3D08_Variants2[] = { { sDemo3D08_Files2_0, NELEMS(sDemo3D08_Files2_0) } };
static const Demo3DFiles sDemo3D09_Files0_0[] = { { sDemo3D09_FileIds0_0_0, NELEMS(sDemo3D09_FileIds0_0_0) } };
static const Demo3DFiles sDemo3D22_Files1_0[] = { { sDemo3D22_FileIds1_0_0, NELEMS(sDemo3D22_FileIds1_0_0) } };
static const Demo3DFiles sDemo3D24_Files2_0[] = {
    { sDemo3D24_FileIds2_0_0, NELEMS(sDemo3D24_FileIds2_0_0) },
    { sDemo3D24_FileIds2_0_1, NELEMS(sDemo3D24_FileIds2_0_1) },
    { sDemo3D24_FileIds2_0_2, NELEMS(sDemo3D24_FileIds2_0_2) },
    { sDemo3D24_FileIds2_0_3, NELEMS(sDemo3D24_FileIds2_0_3) },
    { sDemo3D24_FileIds2_0_4, NELEMS(sDemo3D24_FileIds2_0_4) },
};
static const Demo3DVariant sDemo3D09_Variants0[] = { { sDemo3D09_Files0_0, NELEMS(sDemo3D09_Files0_0) } };
static const Demo3DSetup sDemo3D09_Setup = {
    330, 0xffff, ZONE_NUVEMA_TOWN,
    1, 1, 1, 0, 0, 0, 0, 1, 0, 16,
    FX32_CONST(28), FX32_CONST(28), 0x1555, 0x19a, FX32_CONST(2048), FX32_CONST(1),
    149, -142, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    FALSE, 30, FALSE, 0,
};
static const Demo3DFiles sDemo3D18_Files4_0[] = { { sDemo3D18_FileIds4_0_0, NELEMS(sDemo3D18_FileIds4_0_0) } };
static const Demo3DFiles sDemo3D03_Files2_1[] = { { sDemo3D03_FileIds2_1_0, NELEMS(sDemo3D03_FileIds2_1_0) } };
static const Demo3DVariant sDemo3D25_Variants1[] = { { sDemo3D25_Files1_0, NELEMS(sDemo3D25_Files1_0) } };
static const Demo3DFiles sDemo3D10_Files2_0[] = {
    { sDemo3D10_FileIds2_0_0, NELEMS(sDemo3D10_FileIds2_0_0) },
    { sDemo3D10_FileIds2_0_1, NELEMS(sDemo3D10_FileIds2_0_1) },
    { sDemo3D10_FileIds2_0_2, NELEMS(sDemo3D10_FileIds2_0_2) },
    { sDemo3D10_FileIds2_0_3, NELEMS(sDemo3D10_FileIds2_0_3) },
    { sDemo3D10_FileIds2_0_4, NELEMS(sDemo3D10_FileIds2_0_4) },
};
static const Demo3DVariant sDemo3D26_Variants1[] = { { sDemo3D26_Files1_0, NELEMS(sDemo3D26_Files1_0) } };
static const Demo3DVariant sDemo3D18_Variants4[] = { { sDemo3D18_Files4_0, NELEMS(sDemo3D18_Files4_0) } };
static const Demo3DFiles sDemo3D03_Files1_6[] = { { sDemo3D03_FileIds1_6_0, NELEMS(sDemo3D03_FileIds1_6_0) } };
static const Demo3DSetup sDemo3D10_Setup = {
    468, 0xffff, ZONE_CASTELIA_CITY_8,
    1, 0, 1, 1, 0, 0, 0, 1, 0, 16,
    FX32_CONST(35), FX32_CONST(35), 0x1555, 0x19a, FX32_CONST(2048), FX32_CONST(1),
    0, 0, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    FALSE, 30, FALSE, 0,
};
static const Demo3DVariant sDemo3D10_Variants2[] = { { sDemo3D10_Files2_0, NELEMS(sDemo3D10_Files2_0) } };
static const Demo3DFiles sDemo3D03_Files1_14[] = { { sDemo3D03_FileIds1_14_0, NELEMS(sDemo3D03_FileIds1_14_0) } };
static const Demo3DFiles sDemo3D03_Files2_0[] = { { sDemo3D03_FileIds2_0_0, NELEMS(sDemo3D03_FileIds2_0_0) } };
static const Demo3DSetup sDemo3D11_Setup = {
    453, 0xffff, ZONE_NUVEMA_TOWN,
    0, 0, 1, 1, 1, 0, 0, 1, 0, 16,
    FX32_CONST(60), FX32_CONST(60), 0x1555, 0x19a, FX32_CONST(2048), 0x19a,
    0, 0, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { GX_RGB(0, 0, 8), GX_RGB(0, 0, 8), GX_RGB(0, 0, 8), GX_RGB(0, 0, 8), GX_RGB(0, 0, 8), GX_RGB(0, 0, 8), GX_RGB(0, 0, 8), GX_RGB(0, 0, 8) },
    TRUE, 10, FALSE, 0,
};
static const u16 sDemo3D05_FileIds3_0_0[] = { 280, 283 };
static const Demo3DVariant sDemo3D11_Variants0[] = { { sDemo3D11_Files0_0, NELEMS(sDemo3D11_Files0_0) } };
static const Demo3DVariant sDemo3D11_Variants2[] = { { sDemo3D11_Files2_0, NELEMS(sDemo3D11_Files2_0) } };
static const Demo3DSetup sDemo3D12_Setup = {
    461, 0xffff, ZONE_NUVEMA_TOWN,
    0, 0, 1, 1, 0, 0, 0, 1, 0, 16,
    FX32_CONST(60), FX32_CONST(60), 0x1555, 0x19a, FX32_CONST(2048), 0x19a,
    0, 0, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    TRUE, 10, FALSE, 0,
};
static const Demo3DUnit sDemo3D10_Units[] = {
    { sDemo3D10_Variants0, NELEMS(sDemo3D10_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D10_Variants1, NELEMS(sDemo3D10_Variants1), DEMO3D_UNIT_SINGLE },
    { sDemo3D10_Variants2, NELEMS(sDemo3D10_Variants2), DEMO3D_UNIT_SINGLE },
};
static const Demo3DUnit sDemo3D11_Units[] = {
    { sDemo3D11_Variants0, NELEMS(sDemo3D11_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D11_Variants1, NELEMS(sDemo3D11_Variants1), DEMO3D_UNIT_SINGLE },
    { sDemo3D11_Variants2, NELEMS(sDemo3D11_Variants2), DEMO3D_UNIT_SINGLE },
};
static const Demo3DCmd sDemo3D15_Cmds[] = {
    { DEMO3D_CMD_BGM_PLAY, DEMO3D_CMD_FRAME_START, { SEQ_BGM_SW_DEMO_AJITO_SHIP } },
    { DEMO3D_CMD_SE_PLAY, 1, { SEQ_SE_SW_SOURYUCITY_01, 105, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 1, { SEQ_SE_SW_SOURYUCITY_02, 90, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 1, { SEQ_SE_SW_SOURYUCITY_03, 110, -1, -1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 121, { SEQ_SE_SW_SOURYUCITY_01, 110, 120, 110 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 121, { SEQ_SE_SW_SOURYUCITY_02, 95, 105, 110, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 121, { SEQ_SE_SW_SOURYUCITY_03, 115, 125, 110, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 251, { SEQ_SE_SW_SOURYUCITY_01, 115, 105, 80 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 251, { SEQ_SE_SW_SOURYUCITY_02, 100, 90, 80, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 251, { SEQ_SE_SW_SOURYUCITY_03, 120, 110, 80, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 300, { SEQ_SE_SW_SOURYUCITY_01, 100, 80, 40 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 300, { SEQ_SE_SW_SOURYUCITY_02, 85, 65, 40, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 300, { SEQ_SE_SW_SOURYUCITY_03, 105, 85, 40, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 360, { SEQ_SE_SW_SOURYUCITY_01, 80, 0, 20 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 360, { SEQ_SE_SW_SOURYUCITY_02, 65, 0, 20, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 360, { SEQ_SE_SW_SOURYUCITY_03, 85, 75, 20, 2 } },
    { DEMO3D_CMD_SE_STOP, 380, { SEQ_SE_SW_SOURYUCITY_01 } },
    { DEMO3D_CMD_SE_STOP, 380, { SEQ_SE_SW_SOURYUCITY_02, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 380, { SEQ_SE_SW_SOURYUCITY_03, 75, 0, 20, 2 } },
    { DEMO3D_CMD_SE_STOP, 400, { SEQ_SE_SW_SOURYUCITY_03, 2 } },
    { DEMO3D_CMD_SE_PLAY, 401, { SEQ_SE_SW_SOURYUCITY_05, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_STOP, 497, { SEQ_SE_SW_SOURYUCITY_05, 1 } },
    { DEMO3D_CMD_SE_PLAY, 497, { SEQ_SE_SW_SOURYUCITY_06, 127, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 541, { SEQ_SE_SW_SOURYUCITY_07, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 601, { SEQ_SE_SW_SOURYUCITY_08, 127, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 650, { SEQ_SE_SW_SOURYUCITY_09, 127, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 690, { SEQ_SE_SW_SOURYUCITY_10, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 701, { SEQ_SE_SW_SOURYUCITY_11, 127, -1, -1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 701, { SEQ_SE_SW_SOURYUCITY_12, 1, 74, 180 } },
    { DEMO3D_CMD_SE_PLAY, 701, { SEQ_SE_SW_SOURYUCITY_12, 74, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 875, { SEQ_SE_SW_SOURYUCITY_13, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_STOP, 890, { SEQ_SE_SW_SOURYUCITY_11, 2 } },
    { DEMO3D_CMD_SE_STOP, 890, { SEQ_SE_SW_SOURYUCITY_12 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 890, { 1, 1, 0, 13 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 900, { 1, 10, 13 } },
    { DEMO3D_CMD_SE_PLAY, 900, { SEQ_SE_SW_SOURYUCITY_14, 127, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 920, { SEQ_SE_SW_SOURYUCITY_15, 127, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 920, { SEQ_SE_SW_SOURYUCITY_17, 127, -1, -1, 1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 925, { 1, 10, 0, 16 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 945, { 1, 10, 16, 5 } },
    { DEMO3D_CMD_SE_PLAY, 955, { SEQ_SE_SW_SOURYUCITY_16, 127, -1, -1, 2 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 1035, { 1, 1, 5 } },
    { DEMO3D_CMD_SE_PLAY, 1036, { SEQ_SE_SW_SOURYUCITY_13, 127, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 1051, { SEQ_SE_SW_SOURYUCITY_14, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 1063, { SEQ_SE_SW_SOURYUCITY_13, 127, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 1075, { SEQ_SE_SW_SOURYUCITY_14, 127, -1, -1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 1093, { 1, 1, 0, 5 } },
    { DEMO3D_CMD_SE_PLAY, 1125, { SEQ_SE_SW_SOURYUCITY_17, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 1125, { SEQ_SE_SW_SOURYUCITY_15, 127, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 1132, { SEQ_SE_SW_SOURYUCITY_16, 127, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 1150, { SEQ_SE_SW_SOURYUCITY_15, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 1163, { SEQ_SE_SW_SOURYUCITY_16, 127, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 1216, { SEQ_SE_SW_SOURYUCITY_18, 120, -1, -1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 1365, { SEQ_SE_SW_SOURYUCITY_18, 105, 80, 130 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 1455, { 1, 60, 5, 16 } },
    { DEMO3D_CMD_SE_STOP, 1500, { SEQ_SE_SW_SOURYUCITY_18 } },
    { DEMO3D_CMD_END },
};
static const Demo3DVariant sDemo3D24_Variants2[] = { { sDemo3D24_Files2_0, NELEMS(sDemo3D24_Files2_0) } };
static const Demo3DFiles sDemo3D01_Files1_0[] = { { sDemo3D01_FileIds1_0_0, NELEMS(sDemo3D01_FileIds1_0_0) } };
static const Demo3DFiles sDemo3D23_Files0_0[] = { { sDemo3D23_FileIds0_0_0, NELEMS(sDemo3D23_FileIds0_0_0) } };
static const Demo3DFiles sDemo3D17_Files4_0[] = { { sDemo3D17_FileIds4_0_0, NELEMS(sDemo3D17_FileIds4_0_0) } };
static const Demo3DFiles sDemo3D24_Files0_1[] = { { sDemo3D24_FileIds0_1_0, NELEMS(sDemo3D24_FileIds0_1_0) } };
static const Demo3DFiles sDemo3D03_Files1_9[] = { { sDemo3D03_FileIds1_9_0, NELEMS(sDemo3D03_FileIds1_9_0) } };
static const Demo3DFiles sDemo3D03_Files1_10[] = { { sDemo3D03_FileIds1_10_0, NELEMS(sDemo3D03_FileIds1_10_0) } };
static const Demo3DFiles sDemo3D03_Files0_0[] = { { sDemo3D03_FileIds0_0_0, NELEMS(sDemo3D03_FileIds0_0_0) } };
static const u16 sDemo3D04_FileIds0_0_0[] = { 256, 257 };
static const Demo3DCmd sDemo3D03_EndCmds[] = {
    { DEMO3D_CMD_END },
};
static const Demo3DVariant sDemo3D01_Variants3[] = { { sDemo3D01_Files3_0, NELEMS(sDemo3D01_Files3_0) } };
static const Demo3DCmd sDemo3D13_EndCmds[] = {
    { DEMO3D_CMD_BGM_STOP, 0, { 0 } },
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D22_FileIds0_0_0[] = { 33 };
static const Demo3DFiles sDemo3D20_Files2_0[] = { { sDemo3D20_FileIds2_0_0, NELEMS(sDemo3D20_FileIds2_0_0) } };
static const Demo3DFiles sDemo3D24_Files0_2[] = { { sDemo3D24_FileIds0_2_0, NELEMS(sDemo3D24_FileIds0_2_0) } };
static const Demo3DFiles sDemo3D04_Files1_0[] = { { sDemo3D04_FileIds1_0_0, NELEMS(sDemo3D04_FileIds1_0_0) } };
static const Demo3DFiles sDemo3D04_Files0_0[] = { { sDemo3D04_FileIds0_0_0, NELEMS(sDemo3D04_FileIds0_0_0) } };
static const Demo3DVariant sDemo3D14_Variants0[] = {
    { sDemo3D14_Files0_0, NELEMS(sDemo3D14_Files0_0) },
    { sDemo3D14_Files0_1, NELEMS(sDemo3D14_Files0_1) },
    { sDemo3D14_Files0_2, NELEMS(sDemo3D14_Files0_2) },
    { sDemo3D14_Files0_3, NELEMS(sDemo3D14_Files0_3) },
};
static const Demo3DVariant sDemo3D23_Variants0[] = { { sDemo3D23_Files0_0, NELEMS(sDemo3D23_Files0_0) } };
static const Demo3DVariant sDemo3D23_Variants1[] = { { sDemo3D23_Files1_0, NELEMS(sDemo3D23_Files1_0) } };
static const Demo3DFiles sDemo3D18_Files1_0[] = { { sDemo3D18_FileIds1_0_0, NELEMS(sDemo3D18_FileIds1_0_0) } };
static const Demo3DCmd sDemo3D26_Cmds[] = {
    { DEMO3D_CMD_BGM_PLAY, DEMO3D_CMD_FRAME_START, { SEQ_BGM_SW_DEMO_SHIP } },
    { DEMO3D_CMD_SE_PLAY, 1, { SEQ_SE_SW_TACHIWAKI_01, 127, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 1, { SEQ_SE_SW_TACHIWAKI_02, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 1, { SEQ_SE_SW_TACHIWAKI_03, 127, -1, -1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 210, { SEQ_SE_SW_TACHIWAKI_01, 127, 0, 30 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 210, { SEQ_SE_SW_TACHIWAKI_02, 127, 0, 30, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 210, { SEQ_SE_SW_TACHIWAKI_03, 127, 0, 30, 2 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 241, { 1, 15, 0, -16 } },
    { DEMO3D_CMD_END },
};
static const u16 sDemo3D05_FileIds5_2_0[] = { 272, 271 };
static const Demo3DFiles sDemo3D05_Files3_3[] = { { sDemo3D05_FileIds3_3_0, NELEMS(sDemo3D05_FileIds3_3_0) } };
static const u16 sDemo3D15_FileIds1_0_4[] = { 429, 430, 428, 431 };
static const Demo3DVariant sDemo3D01_Variants1[] = { { sDemo3D01_Files1_0, NELEMS(sDemo3D01_Files1_0) } };
static const Demo3DFiles sDemo3D15_Files1_0[] = {
    { sDemo3D15_FileIds1_0_0, NELEMS(sDemo3D15_FileIds1_0_0) },
    { sDemo3D15_FileIds1_0_1, NELEMS(sDemo3D15_FileIds1_0_1) },
    { sDemo3D15_FileIds1_0_2, NELEMS(sDemo3D15_FileIds1_0_2) },
    { sDemo3D15_FileIds1_0_3, NELEMS(sDemo3D15_FileIds1_0_3) },
    { sDemo3D15_FileIds1_0_4, NELEMS(sDemo3D15_FileIds1_0_4) },
};
static const Demo3DFiles sDemo3D15_Files0_1[] = { { sDemo3D15_FileIds0_1_0, NELEMS(sDemo3D15_FileIds0_1_0) } };
static const Demo3DVariant sDemo3D15_Variants1[] = { { sDemo3D15_Files1_0, NELEMS(sDemo3D15_Files1_0) } };
static const Demo3DFiles sDemo3D03_Files1_4[] = { { sDemo3D03_FileIds1_4_0, NELEMS(sDemo3D03_FileIds1_4_0) } };
static const Demo3DVariant sDemo3D15_Variants0[] = {
    { sDemo3D15_Files0_0, NELEMS(sDemo3D15_Files0_0) },
    { sDemo3D15_Files0_1, NELEMS(sDemo3D15_Files0_1) },
    { sDemo3D15_Files0_2, NELEMS(sDemo3D15_Files0_2) },
    { sDemo3D15_Files0_3, NELEMS(sDemo3D15_Files0_3) },
};
static const Demo3DCmd sDemo3D05_Cmds[] = {
    { DEMO3D_CMD_BGM_PLAY, 0, { SEQ_BGM_SW_DEMO_OPENING } },
    { DEMO3D_CMD_TELOP, 40, { 0xff, 0, 4, 1, 50, 8, 8 } },
    { DEMO3D_CMD_TELOP, 300, { VERSION_BLACK2, 0, 4, 2, 90, 8, 8 } },
    { DEMO3D_CMD_TELOP, 300, { VERSION_WHITE2, 0, 4, 3, 90, 8, 8 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 940, { 3, 60, 0, -16 } },
    { DEMO3D_CMD_BGM_FADE, 970, { 1, 30 } },
    { DEMO3D_CMD_END },
};
static const Demo3DFiles sDemo3D05_Files0_0[] = { { sDemo3D05_FileIds0_0_0, NELEMS(sDemo3D05_FileIds0_0_0) } };
static const Demo3DFiles sDemo3D16_Files1_0[] = { { sDemo3D16_FileIds1_0_0, NELEMS(sDemo3D16_FileIds1_0_0) } };
static const Demo3DCmd sDemo3D16_Cmds[] = {
    { DEMO3D_CMD_BGM_PLAY, DEMO3D_CMD_FRAME_START, { SEQ_BGM_SW_DEMO_UNION_00 } },
    { DEMO3D_CMD_SE_PLAY, 166, { SEQ_SE_SW_KYUREON_00_01, 127, -1, -1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 235, { 3, 3, 0, 16 } },
    { DEMO3D_CMD_SE_PLAY, 240, { SEQ_SE_SW_KYUREON_00_02a, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 240, { SEQ_SE_SW_KYUREON_00_02b, 112, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 240, { SEQ_SE_SW_KYUREON_00_02d, 127, -1, -1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 240, { 3, 5, 16 } },
    { DEMO3D_CMD_SE_PLAY, 330, { SEQ_SE_SW_KYUREON_00_03a, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 330, { SEQ_SE_SW_KYUREON_00_03b, 127, -1, -1, 2 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 420, { 3, 15, 0, -16 } },
    { DEMO3D_CMD_END },
};
static const Demo3DVariant sDemo3D16_Variants1[] = { { sDemo3D16_Files1_0, NELEMS(sDemo3D16_Files1_0) } };
static const Demo3DFiles sDemo3D05_Files4_3[] = { { sDemo3D05_FileIds4_3_0, NELEMS(sDemo3D05_FileIds4_3_0) } };
static const Demo3DVariant sDemo3D16_Variants3[] = { { sDemo3D16_Files3_0, NELEMS(sDemo3D16_Files3_0) } };
static const Demo3DFiles sDemo3D05_Files5_2[] = { { sDemo3D05_FileIds5_2_0, NELEMS(sDemo3D05_FileIds5_2_0) } };
static const Demo3DFiles sDemo3D05_Files5_3[] = { { sDemo3D05_FileIds5_3_0, NELEMS(sDemo3D05_FileIds5_3_0) } };
static const Demo3DVariant sDemo3D05_Variants0[] = { { sDemo3D05_Files0_0, NELEMS(sDemo3D05_Files0_0) } };
static const Demo3DCmd sDemo3D19_EndCmds[] = {
    { DEMO3D_CMD_BGM_STOP, 0, { 0 } },
    { DEMO3D_CMD_END },
};
static const Demo3DVariant sDemo3D19_Variants1[] = { { sDemo3D19_Files1_0, NELEMS(sDemo3D19_Files1_0) } };
static const Demo3DFiles sDemo3D03_Files1_1[] = { { sDemo3D03_FileIds1_1_0, NELEMS(sDemo3D03_FileIds1_1_0) } };
static const Demo3DFiles sDemo3D03_Files1_0[] = { { sDemo3D03_FileIds1_0_0, NELEMS(sDemo3D03_FileIds1_0_0) } };
static const Demo3DSetup sDemo3D05_Setup = {
    288, 0xffff, ZONE_ASPERTIA_CITY,
    1, 0, 1, 1, 0, 1, 1, 1, 0, 16,
    FX32_CONST(40), FX32_CONST(40), 0x1555, FX32_CONST(1), FX32_CONST(2048), 0x800,
    0, 0, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, GX_RGB(4, 4, 4), GX_RGB(8, 8, 8), GX_RGB(12, 12, 12), GX_RGB(16, 16, 16), GX_RGB(20, 20, 20), GX_RGB(24, 24, 24), GX_RGB(31, 31, 31) },
    TRUE, 30, FALSE, 0,
};
static const Demo3DVariant sDemo3D05_Variants5[] = {
    { sDemo3D05_Files5_0, NELEMS(sDemo3D05_Files5_0) },
    { sDemo3D05_Files5_1, NELEMS(sDemo3D05_Files5_1) },
    { sDemo3D05_Files5_2, NELEMS(sDemo3D05_Files5_2) },
    { sDemo3D05_Files5_3, NELEMS(sDemo3D05_Files5_3) },
};
static const Demo3DFiles sDemo3D07_Files0_0[] = { { sDemo3D07_FileIds0_0_0, NELEMS(sDemo3D07_FileIds0_0_0) } };
static const Demo3DFiles sDemo3D24_Files0_3[] = { { sDemo3D24_FileIds0_3_0, NELEMS(sDemo3D24_FileIds0_3_0) } };
static const Demo3DCmd sDemo3D07_EndCmds[] = {
    { DEMO3D_CMD_END },
};
static const Demo3DSetup sDemo3D07_Setup = {
    202, 0xffff, ZONE_NUVEMA_TOWN,
    1, 1, 1, 1, 0, 0, 0, 1, 0, 16,
    FX32_CONST(13), FX32_CONST(13), 0x1555, 0x19a, FX32_CONST(2048), 0x19a,
    58, -57, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    TRUE, 30, TRUE, 0,
};
static const Demo3DFiles sDemo3D22_Files0_0[] = { { sDemo3D22_FileIds0_0_0, NELEMS(sDemo3D22_FileIds0_0_0) } };
static const Demo3DSetup sDemo3D22_Setup = {
    474, 0xffff, ZONE_NUVEMA_TOWN,
    1, 1, 1, 1, 0, 0, 0, 1, 0, 16,
    FX32_CONST(12), FX32_CONST(12), 0x1555, 0x19a, FX32_CONST(2048), FX32_CONST(1),
    64, -65, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    TRUE, 5, FALSE, 0,
};
static const Demo3DCmd sDemo3D24_Cmds[] = {
    { DEMO3D_CMD_SE_PLAY, 30, { SEQ_SE_SW_AIRPLANE_02_L, 127, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 82, { SEQ_SE_SW_AIRPLANE_01, 102, -1, -1, 1 } },
    { DEMO3D_CMD_BGM_FADE, 100, { 1, 160 } },
    { DEMO3D_CMD_SE_PLAY, 281, { SEQ_SE_SW_AIRPLANE_03, 127, -1, -1, 2 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 330, { 1, 5, 0, -16 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 350, { 1, 5, -16 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 470, { 1, 30, 0, -16 } },
    { DEMO3D_CMD_END },
};
static const Demo3DVariant sDemo3D24_Variants0[] = {
    { sDemo3D24_Files0_0, NELEMS(sDemo3D24_Files0_0) },
    { sDemo3D24_Files0_1, NELEMS(sDemo3D24_Files0_1) },
    { sDemo3D24_Files0_2, NELEMS(sDemo3D24_Files0_2) },
    { sDemo3D24_Files0_3, NELEMS(sDemo3D24_Files0_3) },
};
static const Demo3DVariant sDemo3D18_Variants0[] = { { sDemo3D18_Files0_0, NELEMS(sDemo3D18_Files0_0) } };
static const Demo3DVariant sDemo3D18_Variants1[] = { { sDemo3D18_Files1_0, NELEMS(sDemo3D18_Files1_0) } };
static const Demo3DUnit sDemo3D08_Units[] = {
    { sDemo3D08_Variants0, NELEMS(sDemo3D08_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D08_Variants1, NELEMS(sDemo3D08_Variants1), DEMO3D_UNIT_SINGLE },
    { sDemo3D08_Variants2, NELEMS(sDemo3D08_Variants2), DEMO3D_UNIT_SINGLE },
};
static const Demo3DVariant sDemo3D22_Variants0[] = { { sDemo3D22_Files0_0, NELEMS(sDemo3D22_Files0_0) } };
static const Demo3DVariant sDemo3D22_Variants1[] = { { sDemo3D22_Files1_0, NELEMS(sDemo3D22_Files1_0) } };
static const Demo3DCmd sDemo3D14_Cmds[] = {
    { DEMO3D_CMD_BGM_PLAY, DEMO3D_CMD_FRAME_START, { SEQ_BGM_SW_DEMO_AJITO_SHIP } },
    { DEMO3D_CMD_SE_PLAY, 1, { SEQ_SE_SW_SOURYUCITY_01, 105, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 1, { SEQ_SE_SW_SOURYUCITY_02, 90, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 1, { SEQ_SE_SW_SOURYUCITY_03, 110, -1, -1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 121, { SEQ_SE_SW_SOURYUCITY_01, 110, 120, 110 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 121, { SEQ_SE_SW_SOURYUCITY_02, 95, 105, 110, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 121, { SEQ_SE_SW_SOURYUCITY_03, 115, 125, 110, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 251, { SEQ_SE_SW_SOURYUCITY_01, 115, 105, 80 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 251, { SEQ_SE_SW_SOURYUCITY_02, 100, 90, 80, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 251, { SEQ_SE_SW_SOURYUCITY_03, 120, 110, 80, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 300, { SEQ_SE_SW_SOURYUCITY_01, 100, 80, 40 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 300, { SEQ_SE_SW_SOURYUCITY_02, 85, 65, 40, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 300, { SEQ_SE_SW_SOURYUCITY_03, 105, 85, 40, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 360, { SEQ_SE_SW_SOURYUCITY_01, 80, 0, 20 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 360, { SEQ_SE_SW_SOURYUCITY_02, 65, 0, 20, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 360, { SEQ_SE_SW_SOURYUCITY_03, 85, 75, 20, 2 } },
    { DEMO3D_CMD_SE_STOP, 380, { SEQ_SE_SW_SOURYUCITY_01 } },
    { DEMO3D_CMD_SE_STOP, 380, { SEQ_SE_SW_SOURYUCITY_02, 1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 380, { SEQ_SE_SW_SOURYUCITY_03, 75, 0, 20, 2 } },
    { DEMO3D_CMD_SE_STOP, 400, { SEQ_SE_SW_SOURYUCITY_03, 2 } },
    { DEMO3D_CMD_SE_PLAY, 401, { SEQ_SE_SW_SOURYUCITY_05, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_STOP, 497, { SEQ_SE_SW_SOURYUCITY_05, 1 } },
    { DEMO3D_CMD_SE_PLAY, 497, { SEQ_SE_SW_SOURYUCITY_06, 127, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 541, { SEQ_SE_SW_SOURYUCITY_07, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 601, { SEQ_SE_SW_SOURYUCITY_08, 127, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 650, { SEQ_SE_SW_SOURYUCITY_09, 127, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 690, { SEQ_SE_SW_SOURYUCITY_10, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 701, { SEQ_SE_SW_SOURYUCITY_11, 127, -1, -1, 2 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 701, { SEQ_SE_SW_SOURYUCITY_12, 1, 74, 180 } },
    { DEMO3D_CMD_SE_PLAY, 701, { SEQ_SE_SW_SOURYUCITY_12, 74, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 875, { SEQ_SE_SW_SOURYUCITY_13, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_STOP, 890, { SEQ_SE_SW_SOURYUCITY_11, 2 } },
    { DEMO3D_CMD_SE_STOP, 890, { SEQ_SE_SW_SOURYUCITY_12 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 890, { 1, 1, 0, 13 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 900, { 1, 10, 13 } },
    { DEMO3D_CMD_SE_PLAY, 900, { SEQ_SE_SW_SOURYUCITY_14, 127, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 920, { SEQ_SE_SW_SOURYUCITY_15, 127, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 920, { SEQ_SE_SW_SOURYUCITY_17, 127, -1, -1, 1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 925, { 1, 10, 0, 16 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 945, { 1, 10, 16, 5 } },
    { DEMO3D_CMD_SE_PLAY, 955, { SEQ_SE_SW_SOURYUCITY_16, 127, -1, -1, 2 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 1035, { 1, 1, 5 } },
    { DEMO3D_CMD_SE_PLAY, 1036, { SEQ_SE_SW_SOURYUCITY_13, 127, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 1051, { SEQ_SE_SW_SOURYUCITY_14, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 1063, { SEQ_SE_SW_SOURYUCITY_13, 127, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 1075, { SEQ_SE_SW_SOURYUCITY_14, 127, -1, -1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 1093, { 1, 1, 0, 5 } },
    { DEMO3D_CMD_SE_PLAY, 1125, { SEQ_SE_SW_SOURYUCITY_17, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 1125, { SEQ_SE_SW_SOURYUCITY_15, 127, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 1132, { SEQ_SE_SW_SOURYUCITY_16, 127, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 1150, { SEQ_SE_SW_SOURYUCITY_15, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 1163, { SEQ_SE_SW_SOURYUCITY_16, 127, -1, -1, 2 } },
    { DEMO3D_CMD_SE_PLAY, 1216, { SEQ_SE_SW_SOURYUCITY_18, 120, -1, -1 } },
    { DEMO3D_CMD_SE_VOLUME_FADE, 1365, { SEQ_SE_SW_SOURYUCITY_18, 105, 80, 130 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 1455, { 1, 60, 5, 16 } },
    { DEMO3D_CMD_SE_STOP, 1500, { SEQ_SE_SW_SOURYUCITY_18 } },
    { DEMO3D_CMD_END },
};
static const Demo3DCmd sDemo3D23_Cmds[] = {
    { DEMO3D_CMD_BGM_PLAY, DEMO3D_CMD_FRAME_START, { SEQ_BGM_SW_DEMO_SHIP } },
    { DEMO3D_CMD_SE_PLAY, 1, { SEQ_SE_SW_TACHIWAKI_01, 127, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 1, { SEQ_SE_SW_TACHIWAKI_02, 127, -1, -1, 1 } },
    { DEMO3D_CMD_SE_PLAY, 1, { SEQ_SE_SW_TACHIWAKI_03, 127, -1, -1, 2 } },
    { DEMO3D_CMD_BGM_FADE, 220, { 1, 230 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 246, { 1, 10, 0, 16 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 266, { 1, 15, 16 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 570, { 1, 30, 0, -16 } },
    { DEMO3D_CMD_END },
};
static const Demo3DVariant sDemo3D07_Variants0[] = { { sDemo3D07_Files0_0, NELEMS(sDemo3D07_Files0_0) } };
static const Demo3DVariant sDemo3D17_Variants4[] = { { sDemo3D17_Files4_0, NELEMS(sDemo3D17_Files4_0) } };
static const Demo3DSetup sDemo3D26_Setup = {
    445, 0xffff, ZONE_NUVEMA_TOWN,
    1, 0, 1, 1, 0, 0, 1, 1, 0, 16,
    FX32_CONST(40), FX32_CONST(40), 0x1555, 0x19a, FX32_CONST(2048), FX32_CONST(1),
    0, 0, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    FALSE, 30, FALSE, 0,
};
static const Demo3DVariant sDemo3D19_Variants2[] = { { sDemo3D19_Files2_0, NELEMS(sDemo3D19_Files2_0) } };
static const Demo3DUnit sDemo3D19_Units[] = {
    { sDemo3D19_Variants0, NELEMS(sDemo3D19_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D19_Variants1, NELEMS(sDemo3D19_Variants1), DEMO3D_UNIT_SINGLE },
    { sDemo3D19_Variants2, NELEMS(sDemo3D19_Variants2), DEMO3D_UNIT_SINGLE },
    { sDemo3D19_Variants3, NELEMS(sDemo3D19_Variants3), DEMO3D_UNIT_SINGLE },
    { sDemo3D19_Variants4, NELEMS(sDemo3D19_Variants4), DEMO3D_UNIT_SINGLE },
};
static const Demo3DVariant sDemo3D01_Variants0[] = {
    { sDemo3D01_Files0_0, NELEMS(sDemo3D01_Files0_0) },
    { sDemo3D01_Files0_1, NELEMS(sDemo3D01_Files0_1) },
};
static const Demo3DUnit sDemo3D12_Units[] = {
    { sDemo3D12_Variants0, NELEMS(sDemo3D12_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D12_Variants1, NELEMS(sDemo3D12_Variants1), DEMO3D_UNIT_SINGLE },
    { sDemo3D12_Variants2, NELEMS(sDemo3D12_Variants2), DEMO3D_UNIT_SINGLE },
};
static const Demo3DVariant sDemo3D13_Variants0[] = { { sDemo3D13_Files0_0, NELEMS(sDemo3D13_Files0_0) } };
static const u16 sDemo3D14_FileIds1_0_1[] = { 411, 412, 410, 413 };
static const u16 sDemo3D14_FileIds1_0_2[] = { 420, 421, 419, 422 };
static const Demo3DUnit sDemo3D23_Units[] = {
    { sDemo3D23_Variants0, NELEMS(sDemo3D23_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D23_Variants1, NELEMS(sDemo3D23_Variants1), DEMO3D_UNIT_SINGLE },
};
static const Demo3DFiles sDemo3D14_Files1_0[] = {
    { sDemo3D14_FileIds1_0_0, NELEMS(sDemo3D14_FileIds1_0_0) },
    { sDemo3D14_FileIds1_0_1, NELEMS(sDemo3D14_FileIds1_0_1) },
    { sDemo3D14_FileIds1_0_2, NELEMS(sDemo3D14_FileIds1_0_2) },
    { sDemo3D14_FileIds1_0_3, NELEMS(sDemo3D14_FileIds1_0_3) },
    { sDemo3D14_FileIds1_0_4, NELEMS(sDemo3D14_FileIds1_0_4) },
};
static const Demo3DVariant sDemo3D14_Variants1[] = { { sDemo3D14_Files1_0, NELEMS(sDemo3D14_Files1_0) } };
static const Demo3DVariant sDemo3D20_Variants0[] = { { sDemo3D20_Files0_0, NELEMS(sDemo3D20_Files0_0) } };
static const Demo3DUnit sDemo3D24_Units[] = {
    { sDemo3D24_Variants0, NELEMS(sDemo3D24_Variants0), DEMO3D_UNIT_BY_SEASON },
    { sDemo3D24_Variants1, NELEMS(sDemo3D24_Variants1), DEMO3D_UNIT_SINGLE },
    { sDemo3D24_Variants2, NELEMS(sDemo3D24_Variants2), DEMO3D_UNIT_SINGLE },
};
static const Demo3DVariant sDemo3D20_Variants2[] = { { sDemo3D20_Files2_0, NELEMS(sDemo3D20_Files2_0) } };
static const Demo3DUnit sDemo3D20_Units[] = {
    { sDemo3D20_Variants0, NELEMS(sDemo3D20_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D20_Variants1, NELEMS(sDemo3D20_Variants1), DEMO3D_UNIT_SINGLE },
    { sDemo3D20_Variants2, NELEMS(sDemo3D20_Variants2), DEMO3D_UNIT_SINGLE },
    { sDemo3D20_Variants3, NELEMS(sDemo3D20_Variants3), DEMO3D_UNIT_SINGLE },
    { sDemo3D20_Variants4, NELEMS(sDemo3D20_Variants4), DEMO3D_UNIT_SINGLE },
    { sDemo3D20_Variants5, NELEMS(sDemo3D20_Variants5), DEMO3D_UNIT_SINGLE },
};
static const Demo3DFiles sDemo3D03_Files1_13[] = { { sDemo3D03_FileIds1_13_0, NELEMS(sDemo3D03_FileIds1_13_0) } };
static const Demo3DVariant sDemo3D04_Variants1[] = { { sDemo3D04_Files1_0, NELEMS(sDemo3D04_Files1_0) } };
static const Demo3DVariant sDemo3D04_Variants0[] = { { sDemo3D04_Files0_0, NELEMS(sDemo3D04_Files0_0) } };
static const Demo3DVariant sDemo3D03_Variants4[] = { { sDemo3D03_Files4_0, NELEMS(sDemo3D03_Files4_0) } };
static const Demo3DVariant sDemo3D26_Variants0[] = { { sDemo3D26_Files0_0, NELEMS(sDemo3D26_Files0_0) } };
static const Demo3DUnit sDemo3D26_Units[] = {
    { sDemo3D26_Variants0, NELEMS(sDemo3D26_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D26_Variants1, NELEMS(sDemo3D26_Variants1), DEMO3D_UNIT_SINGLE },
};
static const Demo3DVariant sDemo3D05_Variants1[] = { { sDemo3D05_Files1_0, NELEMS(sDemo3D05_Files1_0) } };
static const Demo3DFiles sDemo3D03_Files2_3[] = { { sDemo3D03_FileIds2_3_0, NELEMS(sDemo3D03_FileIds2_3_0) } };
static const Demo3DSetup sDemo3D17_Setup = {
    14, 0xffff, ZONE_NUVEMA_TOWN,
    1, 1, 1, 1, 0, 0, 0, 1, 0, 16,
    FX32_CONST(27), FX32_CONST(27), 0x1555, 0x19a, FX32_CONST(2048), FX32_CONST(1),
    149, -142, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    FALSE, 15, FALSE, 0,
};
static const Demo3DFiles sDemo3D17_Files5_0[] = { { sDemo3D17_FileIds5_0_0, NELEMS(sDemo3D17_FileIds5_0_0) } };
static const Demo3DVariant sDemo3D17_Variants5[] = { { sDemo3D17_Files5_0, NELEMS(sDemo3D17_Files5_0) } };
static const Demo3DFiles sDemo3D05_Files3_0[] = { { sDemo3D05_FileIds3_0_0, NELEMS(sDemo3D05_FileIds3_0_0) } };
static const Demo3DVariant sDemo3D25_Variants0[] = { { sDemo3D25_Files0_0, NELEMS(sDemo3D25_Files0_0) } };
static const Demo3DCmd sDemo3D12_Cmds[] = {
    { DEMO3D_CMD_BRIGHTNESS_FADE, 701, { 1, 20, 16 } },
    { DEMO3D_CMD_END },
};
static const Demo3DUnit sDemo3D07_Units[] = {
    { sDemo3D07_Variants0, NELEMS(sDemo3D07_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D07_Variants1, NELEMS(sDemo3D07_Variants1), DEMO3D_UNIT_SINGLE },
    { sDemo3D07_Variants2, NELEMS(sDemo3D07_Variants2), DEMO3D_UNIT_SINGLE },
};
static const Demo3DUnit sDemo3D25_Units[] = {
    { sDemo3D25_Variants0, NELEMS(sDemo3D25_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D25_Variants1, NELEMS(sDemo3D25_Variants1), DEMO3D_UNIT_SINGLE },
};
static const Demo3DUnit sDemo3D09_Units[] = {
    { sDemo3D09_Variants0, NELEMS(sDemo3D09_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D09_Variants1, NELEMS(sDemo3D09_Variants1), DEMO3D_UNIT_SINGLE },
    { sDemo3D09_Variants2, NELEMS(sDemo3D09_Variants2), DEMO3D_UNIT_SINGLE },
};
static const Demo3DVariant sDemo3D01_Variants2[] = { { sDemo3D01_Files2_0, NELEMS(sDemo3D01_Files2_0) } };
static const Demo3DSetup sDemo3D01_Setup = {
    81, 0xffff, ZONE_NUVEMA_TOWN,
    1, 0, 1, 1, 0, 0, 1, 1, 0, 16,
    FX32_CONST(40), FX32_CONST(40), 0x1555, 0x19a, FX32_CONST(2048), FX32_CONST(1),
    0, 0, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    FALSE, 30, FALSE, 0,
};
static const Demo3DFiles sDemo3D13_Files1_0[] = {
    { sDemo3D13_FileIds1_0_0, NELEMS(sDemo3D13_FileIds1_0_0) },
    { sDemo3D13_FileIds1_0_1, NELEMS(sDemo3D13_FileIds1_0_1) },
    { sDemo3D13_FileIds1_0_2, NELEMS(sDemo3D13_FileIds1_0_2) },
    { sDemo3D13_FileIds1_0_3, NELEMS(sDemo3D13_FileIds1_0_3) },
    { sDemo3D13_FileIds1_0_4, NELEMS(sDemo3D13_FileIds1_0_4) },
};
static const Demo3DVariant sDemo3D13_Variants1[] = { { sDemo3D13_Files1_0, NELEMS(sDemo3D13_Files1_0) } };
static const Demo3DCmd sDemo3D20_EndCmds[] = {
    { DEMO3D_CMD_BGM_STOP, 0, { 0 } },
    { DEMO3D_CMD_END },
};
static const Demo3DVariant sDemo3D03_Variants2[] = {
    { sDemo3D03_Files2_0, NELEMS(sDemo3D03_Files2_0) },
    { sDemo3D03_Files2_1, NELEMS(sDemo3D03_Files2_1) },
    { sDemo3D03_Files2_2, NELEMS(sDemo3D03_Files2_2) },
    { sDemo3D03_Files2_3, NELEMS(sDemo3D03_Files2_3) },
};
static const Demo3DCmd sDemo3D25_EndCmds[] = {
    { DEMO3D_CMD_END },
};
static const Demo3DVariant sDemo3D03_Variants3[] = { { sDemo3D03_Files3_0, NELEMS(sDemo3D03_Files3_0) } };
static const Demo3DVariant sDemo3D03_Variants0[] = {
    { sDemo3D03_Files0_0, NELEMS(sDemo3D03_Files0_0) },
    { sDemo3D03_Files0_1, NELEMS(sDemo3D03_Files0_1) },
};
static const Demo3DUnit sDemo3D04_Units[] = {
    { sDemo3D04_Variants0, NELEMS(sDemo3D04_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D04_Variants1, NELEMS(sDemo3D04_Variants1), DEMO3D_UNIT_SINGLE },
    { sDemo3D04_Variants2, NELEMS(sDemo3D04_Variants2), DEMO3D_UNIT_SINGLE },
    { sDemo3D04_Variants3, NELEMS(sDemo3D04_Variants3), DEMO3D_UNIT_SINGLE },
};
static const Demo3DFiles sDemo3D05_Files4_0[] = { { sDemo3D05_FileIds4_0_0, NELEMS(sDemo3D05_FileIds4_0_0) } };
static const Demo3DSetup sDemo3D04_Setup = {
    263, 0xffff, ZONE_CASTELIA_CITY_9,
    1, 0, 1, 0, 0, 0, 1, 1, 0, 16,
    FX32_CONST(40), FX32_CONST(40), 0x1555, 0x19a, FX32_CONST(2048), FX32_CONST(1),
    0, 0, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 31, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    FALSE, 20, FALSE, 0,
};
static const Demo3DVariant sDemo3D05_Variants6[] = { { sDemo3D05_Files6_0, NELEMS(sDemo3D05_Files6_0) } };
static const Demo3DVariant sDemo3D17_Variants0[] = { { sDemo3D17_Files0_0, NELEMS(sDemo3D17_Files0_0) } };
static const Demo3DUnit sDemo3D18_Units[] = {
    { sDemo3D18_Variants0, NELEMS(sDemo3D18_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D18_Variants1, NELEMS(sDemo3D18_Variants1), DEMO3D_UNIT_SINGLE },
    { sDemo3D18_Variants2, NELEMS(sDemo3D18_Variants2), DEMO3D_UNIT_SINGLE },
    { sDemo3D18_Variants3, NELEMS(sDemo3D18_Variants3), DEMO3D_UNIT_SINGLE },
    { sDemo3D18_Variants4, NELEMS(sDemo3D18_Variants4), DEMO3D_UNIT_SINGLE },
};
static const Demo3DVariant sDemo3D03_Variants1[] = {
    { sDemo3D03_Files1_0, NELEMS(sDemo3D03_Files1_0) },
    { sDemo3D03_Files1_1, NELEMS(sDemo3D03_Files1_1) },
    { sDemo3D03_Files1_2, NELEMS(sDemo3D03_Files1_2) },
    { sDemo3D03_Files1_3, NELEMS(sDemo3D03_Files1_3) },
    { sDemo3D03_Files1_4, NELEMS(sDemo3D03_Files1_4) },
    { sDemo3D03_Files1_5, NELEMS(sDemo3D03_Files1_5) },
    { sDemo3D03_Files1_6, NELEMS(sDemo3D03_Files1_6) },
    { sDemo3D03_Files1_7, NELEMS(sDemo3D03_Files1_7) },
    { sDemo3D03_Files1_8, NELEMS(sDemo3D03_Files1_8) },
    { sDemo3D03_Files1_9, NELEMS(sDemo3D03_Files1_9) },
    { sDemo3D03_Files1_10, NELEMS(sDemo3D03_Files1_10) },
    { sDemo3D03_Files1_0, NELEMS(sDemo3D03_Files1_0) },
    { sDemo3D03_Files1_12, NELEMS(sDemo3D03_Files1_12) },
    { sDemo3D03_Files1_13, NELEMS(sDemo3D03_Files1_13) },
    { sDemo3D03_Files1_14, NELEMS(sDemo3D03_Files1_14) },
    { sDemo3D03_Files1_13, NELEMS(sDemo3D03_Files1_13) },
    { sDemo3D03_Files1_14, NELEMS(sDemo3D03_Files1_14) },
};
static const Demo3DVariant sDemo3D03_Variants5[] = { { sDemo3D03_Files5_0, NELEMS(sDemo3D03_Files5_0) } };
static const Demo3DVariant sDemo3D05_Variants2[] = { { sDemo3D05_Files2_0, NELEMS(sDemo3D05_Files2_0) } };
static const Demo3DUnit sDemo3D13_Units[] = {
    { sDemo3D13_Variants0, NELEMS(sDemo3D13_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D13_Variants1, NELEMS(sDemo3D13_Variants1), DEMO3D_UNIT_SINGLE },
};
static const Demo3DUnit sDemo3D01_Units[] = {
    { sDemo3D01_Variants0, NELEMS(sDemo3D01_Variants0), DEMO3D_UNIT_BY_SEX },
    { sDemo3D01_Variants1, NELEMS(sDemo3D01_Variants1), DEMO3D_UNIT_SINGLE },
    { sDemo3D01_Variants2, NELEMS(sDemo3D01_Variants2), DEMO3D_UNIT_SINGLE },
    { sDemo3D01_Variants3, NELEMS(sDemo3D01_Variants3), DEMO3D_UNIT_SINGLE },
    { sDemo3D01_Variants4, NELEMS(sDemo3D01_Variants4), DEMO3D_UNIT_SINGLE },
    { sDemo3D01_Variants5, NELEMS(sDemo3D01_Variants5), DEMO3D_UNIT_SINGLE },
    { sDemo3D01_Variants6, NELEMS(sDemo3D01_Variants6), DEMO3D_UNIT_SINGLE },
};
static const Demo3DSetup sDemo3D02_Setup = {
    49, 0xffff, ZONE_CASTELIA_CITY_12,
    1, 0, 1, 0, 0, 0, 0, 1, 0, 16,
    FX32_CONST(40), FX32_CONST(40), 0x1555, FX32_CONST(100), FX32_CONST(2048), FX32_CONST(1),
    0, 0, 0,
    GX_RGB(0, 0, 0), 31, 0, 0x7fff,
    GX_RGB(0, 0, 0), 15, 0, 0, 0,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    FALSE, 30, FALSE, 0,
};
static const Demo3DUnit sDemo3D16_Units[] = {
    { sDemo3D16_Variants0, NELEMS(sDemo3D16_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D16_Variants1, NELEMS(sDemo3D16_Variants1), DEMO3D_UNIT_SINGLE },
    { sDemo3D16_Variants2, NELEMS(sDemo3D16_Variants2), DEMO3D_UNIT_SINGLE },
    { sDemo3D16_Variants3, NELEMS(sDemo3D16_Variants3), DEMO3D_UNIT_SINGLE },
};
static const Demo3DVariant sDemo3D05_Variants3[] = {
    { sDemo3D05_Files3_0, NELEMS(sDemo3D05_Files3_0) },
    { sDemo3D05_Files3_1, NELEMS(sDemo3D05_Files3_1) },
    { sDemo3D05_Files3_2, NELEMS(sDemo3D05_Files3_2) },
    { sDemo3D05_Files3_3, NELEMS(sDemo3D05_Files3_3) },
};
static const Demo3DVariant sDemo3D05_Variants4[] = {
    { sDemo3D05_Files4_0, NELEMS(sDemo3D05_Files4_0) },
    { sDemo3D05_Files4_1, NELEMS(sDemo3D05_Files4_1) },
    { sDemo3D05_Files4_2, NELEMS(sDemo3D05_Files4_2) },
    { sDemo3D05_Files4_3, NELEMS(sDemo3D05_Files4_3) },
};
static const Demo3DUnit sDemo3D06_Units[] = {
    { sDemo3D06_Variants0, NELEMS(sDemo3D06_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D06_Variants1, NELEMS(sDemo3D06_Variants1), DEMO3D_UNIT_SINGLE },
    { sDemo3D06_Variants2, NELEMS(sDemo3D06_Variants2), DEMO3D_UNIT_SINGLE },
};
static const Demo3DUnit sDemo3D22_Units[] = {
    { sDemo3D22_Variants0, NELEMS(sDemo3D22_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D22_Variants1, NELEMS(sDemo3D22_Variants1), DEMO3D_UNIT_SINGLE },
    { sDemo3D22_Variants2, NELEMS(sDemo3D22_Variants2), DEMO3D_UNIT_SINGLE },
    { sDemo3D22_Variants3, NELEMS(sDemo3D22_Variants3), DEMO3D_UNIT_SINGLE },
};
static const Demo3DUnit sDemo3D14_Units[] = {
    { sDemo3D14_Variants0, NELEMS(sDemo3D14_Variants0), DEMO3D_UNIT_BY_SEASON },
    { sDemo3D14_Variants1, NELEMS(sDemo3D14_Variants1), DEMO3D_UNIT_SINGLE },
};
static const Demo3DUnit sDemo3D15_Units[] = {
    { sDemo3D15_Variants0, NELEMS(sDemo3D15_Variants0), DEMO3D_UNIT_BY_SEASON },
    { sDemo3D15_Variants1, NELEMS(sDemo3D15_Variants1), DEMO3D_UNIT_SINGLE },
};
static const Demo3DUnit sDemo3D05_Units[] = {
    { sDemo3D05_Variants0, NELEMS(sDemo3D05_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D05_Variants1, NELEMS(sDemo3D05_Variants1), DEMO3D_UNIT_SINGLE },
    { sDemo3D05_Variants2, NELEMS(sDemo3D05_Variants2), DEMO3D_UNIT_SINGLE },
    { sDemo3D05_Variants3, NELEMS(sDemo3D05_Variants3), DEMO3D_UNIT_BY_SEASON },
    { sDemo3D05_Variants4, NELEMS(sDemo3D05_Variants4), DEMO3D_UNIT_BY_SEASON },
    { sDemo3D05_Variants5, NELEMS(sDemo3D05_Variants5), DEMO3D_UNIT_BY_SEASON },
    { sDemo3D05_Variants6, NELEMS(sDemo3D05_Variants6), DEMO3D_UNIT_SINGLE },
};
static const Demo3DCmd sDemo3D01_Cmds[] = {
    { DEMO3D_CMD_SE_PLAY, DEMO3D_CMD_FRAME_START, { SEQ_SE_FLD_79, -1, -1, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 1350, { SEQ_SE_FLD_78, -1, -1, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 2700, { SEQ_SE_FLD_78, -1, -1, -1, -1 } },
    { DEMO3D_CMD_SE_PLAY, 4050, { SEQ_SE_FLD_78, -1, -1, -1, -1 } },
    { DEMO3D_CMD_BRIGHTNESS_FADE, 5340, { 1, 60, 0, -16 } },
    { DEMO3D_CMD_END },
};
static const Demo3DUnit sDemo3D03_Units[] = {
    { sDemo3D03_Variants0, NELEMS(sDemo3D03_Variants0), DEMO3D_UNIT_BY_SEX },
    { sDemo3D03_Variants1, NELEMS(sDemo3D03_Variants1), DEMO3D_UNIT_BY_PARAM },
    { sDemo3D03_Variants2, NELEMS(sDemo3D03_Variants2), DEMO3D_UNIT_BY_SEASON },
    { sDemo3D03_Variants3, NELEMS(sDemo3D03_Variants3), DEMO3D_UNIT_SINGLE },
    { sDemo3D03_Variants4, NELEMS(sDemo3D03_Variants4), DEMO3D_UNIT_SINGLE },
    { sDemo3D03_Variants5, NELEMS(sDemo3D03_Variants5), DEMO3D_UNIT_SINGLE },
};
static const Demo3DUnit sDemo3D17_Units[] = {
    { sDemo3D17_Variants0, NELEMS(sDemo3D17_Variants0), DEMO3D_UNIT_SINGLE },
    { sDemo3D17_Variants1, NELEMS(sDemo3D17_Variants1), DEMO3D_UNIT_SINGLE },
    { sDemo3D17_Variants2, NELEMS(sDemo3D17_Variants2), DEMO3D_UNIT_SINGLE },
    { sDemo3D17_Variants3, NELEMS(sDemo3D17_Variants3), DEMO3D_UNIT_SINGLE },
    { sDemo3D17_Variants4, NELEMS(sDemo3D17_Variants4), DEMO3D_UNIT_SINGLE },
    { sDemo3D17_Variants5, NELEMS(sDemo3D17_Variants5), DEMO3D_UNIT_SINGLE },
};
static const Demo3DScene sDemo3DScenes[DEMO3D_ID_COUNT] = {
    { NULL, NULL, NULL, NULL, 0 },
    { &sDemo3D01_Setup, sDemo3D01_Cmds, sDemo3D01_EndCmds, sDemo3D01_Units, NELEMS(sDemo3D01_Units) },
    { &sDemo3D02_Setup, sDemo3D02_Cmds, sDemo3D02_EndCmds, sDemo3D02_Units, NELEMS(sDemo3D02_Units) },
    { &sDemo3D03_Setup, sDemo3D03_Cmds, sDemo3D03_EndCmds, sDemo3D03_Units, NELEMS(sDemo3D03_Units) },
    { &sDemo3D04_Setup, sDemo3D04_Cmds, sDemo3D04_EndCmds, sDemo3D04_Units, NELEMS(sDemo3D04_Units) },
    { &sDemo3D05_Setup, sDemo3D05_Cmds, sDemo3D05_EndCmds, sDemo3D05_Units, NELEMS(sDemo3D05_Units) },
    { &sDemo3D06_Setup, sDemo3D06_Cmds, sDemo3D06_EndCmds, sDemo3D06_Units, NELEMS(sDemo3D06_Units) },
    { &sDemo3D07_Setup, sDemo3D07_Cmds, sDemo3D07_EndCmds, sDemo3D07_Units, NELEMS(sDemo3D07_Units) },
    { &sDemo3D08_Setup, sDemo3D08_Cmds, sDemo3D08_EndCmds, sDemo3D08_Units, NELEMS(sDemo3D08_Units) },
    { &sDemo3D09_Setup, sDemo3D09_Cmds, sDemo3D09_EndCmds, sDemo3D09_Units, NELEMS(sDemo3D09_Units) },
    { &sDemo3D10_Setup, sDemo3D10_Cmds, sDemo3D10_EndCmds, sDemo3D10_Units, NELEMS(sDemo3D10_Units) },
    { &sDemo3D11_Setup, sDemo3D11_Cmds, sDemo3D11_EndCmds, sDemo3D11_Units, NELEMS(sDemo3D11_Units) },
    { &sDemo3D12_Setup, sDemo3D12_Cmds, sDemo3D12_EndCmds, sDemo3D12_Units, NELEMS(sDemo3D12_Units) },
    { &sDemo3D13_Setup, sDemo3D13_Cmds, sDemo3D13_EndCmds, sDemo3D13_Units, NELEMS(sDemo3D13_Units) },
    { &sDemo3D14_Setup, sDemo3D14_Cmds, sDemo3D14_EndCmds, sDemo3D14_Units, NELEMS(sDemo3D14_Units) },
    { &sDemo3D15_Setup, sDemo3D15_Cmds, sDemo3D15_EndCmds, sDemo3D15_Units, NELEMS(sDemo3D15_Units) },
    { &sDemo3D16_Setup, sDemo3D16_Cmds, sDemo3D16_EndCmds, sDemo3D16_Units, NELEMS(sDemo3D16_Units) },
    { &sDemo3D17_Setup, sDemo3D17_Cmds, sDemo3D17_EndCmds, sDemo3D17_Units, NELEMS(sDemo3D17_Units) },
    { &sDemo3D18_Setup, sDemo3D18_Cmds, sDemo3D18_EndCmds, sDemo3D18_Units, NELEMS(sDemo3D18_Units) },
    { &sDemo3D19_Setup, sDemo3D19_Cmds, sDemo3D19_EndCmds, sDemo3D19_Units, NELEMS(sDemo3D19_Units) },
    { &sDemo3D20_Setup, sDemo3D20_Cmds, sDemo3D20_EndCmds, sDemo3D20_Units, NELEMS(sDemo3D20_Units) },
    { &sDemo3D21_Setup, sDemo3D21_Cmds, sDemo3D21_EndCmds, sDemo3D21_Units, NELEMS(sDemo3D21_Units) },
    { &sDemo3D22_Setup, sDemo3D22_Cmds, sDemo3D22_EndCmds, sDemo3D22_Units, NELEMS(sDemo3D22_Units) },
    { &sDemo3D23_Setup, sDemo3D23_Cmds, sDemo3D23_EndCmds, sDemo3D23_Units, NELEMS(sDemo3D23_Units) },
    { &sDemo3D24_Setup, sDemo3D24_Cmds, sDemo3D24_EndCmds, sDemo3D24_Units, NELEMS(sDemo3D24_Units) },
    { &sDemo3D25_Setup, sDemo3D25_Cmds, sDemo3D25_EndCmds, sDemo3D25_Units, NELEMS(sDemo3D25_Units) },
    { &sDemo3D26_Setup, sDemo3D26_Cmds, sDemo3D26_EndCmds, sDemo3D26_Units, NELEMS(sDemo3D26_Units) },
};

static u32 Demo3DData_CheckId(u32 demoId) {
    if (demoId == 0 || demoId >= DEMO3D_ID_COUNT) {
        demoId = 0;
    }
    return demoId;
}

const Demo3DSetup *Demo3DData_GetSetup(u32 demoId) {
    return sDemo3DScenes[Demo3DData_CheckId(demoId)].setup;
}

G3DCurve *Demo3DData_LoadCamera(u32 demoId, HeapID heapId, u32 preloadCount) {
    const Demo3DSetup *setup = Demo3DData_GetSetup(demoId);

    return GFL_G3DCurveLoadFileStream(heapId, ARCID_DEMO3D_RESOURCE, setup->cameraFile, preloadCount);
}

u8 Demo3DData_GetUnitCount(u32 demoId) {
    return sDemo3DScenes[Demo3DData_CheckId(demoId)].unitCount;
}

G3DSceneSetup *Demo3DData_CreateSceneSetup(HeapID heapId) {
    G3DSceneSetup *setup = GFL_HeapAllocate(heapId, sizeof(G3DSceneSetup), TRUE, "demo3d_data.c", 171);
    G3DSceneResourceSetup *resources =
        GFL_HeapAllocate(heapId, sizeof(G3DSceneResourceSetup) * RESOURCE_MAX, TRUE, "demo3d_data.c", 172);
    G3DSceneActorSetup *actor = GFL_HeapAllocate(heapId, sizeof(G3DSceneActorSetup), TRUE, "demo3d_data.c", 173);
    int i;

    for (i = 0; i < RESOURCE_MAX; i++) {
        resources[i].arcId = ARCID_DEMO3D_RESOURCE;
        resources[i].type = G3D_SCENE_RES_ARCSYS;
    }
    actor->animations = sDemo3DAnimations;
    setup->resources = resources;
    setup->actors = actor;
    return setup;
}

void Demo3DData_FreeSceneSetup(G3DSceneSetup *setup) {
    GFL_HeapFree((void *)setup->actors);
    GFL_HeapFree((void *)setup->resources);
    GFL_HeapFree(setup);
}

// Fills the scene set-up in for one of a demo's models: the variant that the parameters pick, at their period of the
// day
void Demo3DData_SetSceneSetup(G3DSceneSetup *setup, const Demo3DSceneParam *param, u8 unit) {
    G3DSceneResourceSetup *resources = (G3DSceneResourceSetup *)setup->resources;
    G3DSceneActorSetup *actor = (G3DSceneActorSetup *)setup->actors;
    u32 period = 0;
    const Demo3DVariant *variant = Demo3DData_GetVariant(&sDemo3DScenes[param->demoId].units[unit], param);
    int i;
    const Demo3DFiles *files;

    if (variant->count > 1) {
        period = param->dayPeriod;
    }
    if (period >= variant->count) {
        period = 0;
    }
    files = &variant->files[period];
    setup->resourceCount = files->count;
    for (i = 0; i < RESOURCE_MAX; i++) {
        if (i < files->count) {
            resources[i].fileId = files->files[i];
        } else {
            resources[i].fileId = 0;
        }
    }
    setup->actorCount = 1;
    actor->animationCount = files->count - 1;
}

static const Demo3DVariant *Demo3DData_GetVariant(const Demo3DUnit *unit, const Demo3DSceneParam *param) {
    int index = 0;

    switch (unit->type) {
    case DEMO3D_UNIT_BY_SEX:
        index = param->playerSex;
        break;
    case DEMO3D_UNIT_BY_PARAM:
        index = param->param;
        break;
    case DEMO3D_UNIT_BY_SEASON:
        index = param->season;
        break;
    }
    if (index >= unit->count) {
        index = 0;
    }
    return &unit->variants[index];
}

fx32 Demo3DData_GetFrameStep(u32 demoId) {
    const Demo3DSetup *setup = Demo3DData_GetSetup(demoId);

    return FX32_ONE >> setup->halfRate;
}

BOOL Demo3DData_IsDual3D(u32 demoId) {
    const Demo3DSetup *setup = Demo3DData_GetSetup(demoId);

    return setup->dual3D;
}

void Demo3DData_GetFadeParams(u32 demoId, u8 fadeOut, u8 *white, u8 *frames) {
    const Demo3DSetup *setup = Demo3DData_GetSetup(demoId);

    if (!fadeOut) {
        *white = setup->fadeInWhite;
        *frames = setup->fadeInFrames * (setup->halfRate + 1);
    } else {
        *white = setup->fadeOutWhite;
        *frames = setup->fadeOutFrames * (setup->halfRate + 1);
    }
}

const Demo3DCmd *Demo3DData_GetCmds(u32 demoId) {
    return sDemo3DScenes[Demo3DData_CheckId(demoId)].cmds;
}

const Demo3DCmd *Demo3DData_GetEndCmds(u32 demoId) {
    return sDemo3DScenes[Demo3DData_CheckId(demoId)].endCmds;
}
