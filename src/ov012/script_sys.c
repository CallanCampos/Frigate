// The field script system: the supervisor of the scripts' VMs, the script call event, the VMs' setup, the global
// scripts and the opcode guard. Function and data names from swan (https://github.com/ds-pokemon-hacking/swan,
// GPL-3.0)

#include "types.h"
#include "constants/arc.h"
#include "constants/field_scripts.h"
#include "constants/script_text_banks.h"
#include "field/event_data.h"
#include "field/field_script.h"
#include "field/field_script_event.h"
#include "field/field_script_plugin.h"
#include "field/field_script_supervisor.h"
#include "field/player_state.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/std.h"
#include "save/event_work.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/main.h"
#include "system/vm.h"

// A range of global script IDs, with its script file and text bank
struct GlobalScriptEntry {
    u16 start;
    u16 end;
    u16 fileId;
    u16 msgArcId;
    u16 msgFileNo;
};

// The feature levels at which a command may run
struct OpcodePermissions {
    u8 level0 : 1;
    u8 level1 : 1;
    u8 level2 : 1;
};

// The global scripts: each range of script IDs with its script file and text bank. FieldScript_ResolveSCRID takes
// the first entry that starts at or below the ID, so the ranges come in descending order. Ranges that share a script
// file or text bank share its constant, which the generated lists name after one of those ranges
static const GlobalScriptEntry GLOBAL_SCRIPT_TABLE[60] = {
    { 10885, 10914, SCRIPTS_GLOBAL_10885, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10885 },
    { 10865, 10884, SCRIPTS_GLOBAL_10865, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10865 },
    { 10860, 10864, SCRIPTS_GLOBAL_10860, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10860 },
    { 10850, 10859, SCRIPTS_GLOBAL_10850, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10850 },
    { 10815, 10834, SCRIPTS_GLOBAL_10815, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10815 },
    { 10795, 10814, SCRIPTS_GLOBAL_10795, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10795 },
    { 10785, 10794, SCRIPTS_GLOBAL_10785, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10785 },
    { 10735, 10784, SCRIPTS_GLOBAL_10735, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10735 },
    { 10685, 10734, SCRIPTS_GLOBAL_10685, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10685 },
    { 10675, 10684, SCRIPTS_GLOBAL_10675, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10675 },
    { 10665, 10674, SCRIPTS_GLOBAL_10665, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10665 },
    { 10655, 10664, SCRIPTS_GLOBAL_10655, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10655 },
    { 10635, 10654, SCRIPTS_GLOBAL_10635, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10635 },
    { 10535, 10634, SCRIPTS_GLOBAL_10535, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10535 },
    { 10530, 10534, SCRIPTS_GLOBAL_10530, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10530 },
    { 10520, 10529, SCRIPTS_GLOBAL_10520, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10520 },
    { 10510, 10519, SCRIPTS_GLOBAL_10510, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10510 },
    { 10490, 10509, SCRIPTS_GLOBAL_10490, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10490 },
    { 10480, 10489, SCRIPTS_GLOBAL_10480, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10480 },
    { 10470, 10479, SCRIPTS_GLOBAL_10470, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10470 },
    { 10450, 10469, SCRIPTS_GLOBAL_10450, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10450 },
    { 10445, 10449, SCRIPTS_GLOBAL_10445, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10445 },
    { 10440, 10444, SCRIPTS_GLOBAL_10440, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10440 },
    { 10435, 10439, SCRIPTS_GLOBAL_10435, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10435 },
    { 10405, 10434, SCRIPTS_GLOBAL_10405, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10405 },
    { 10400, 10404, SCRIPTS_GLOBAL_10400, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10400 },
    { 10395, 10399, SCRIPTS_GLOBAL_10395, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10395 },
    { 10390, 10394, SCRIPTS_GLOBAL_10390, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10390 },
    { 10380, 10389, SCRIPTS_GLOBAL_10380, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10380 },
    { 10350, 10379, SCRIPTS_GLOBAL_10350, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10350 },
    { 10330, 10349, SCRIPTS_GLOBAL_10330, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10330 },
    { 10325, 10329, SCRIPTS_GLOBAL_10325, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10325 },
    { 10315, 10324, SCRIPTS_GLOBAL_10315, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10315 },
    { 10310, 10314, SCRIPTS_GLOBAL_10310, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10310 },
    { 10290, 10309, SCRIPTS_GLOBAL_10290, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10290 },
    { 10280, 10289, SCRIPTS_GLOBAL_10280, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10280 },
    { 10270, 10279, SCRIPTS_GLOBAL_10270, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10270 },
    { 10260, 10269, SCRIPTS_GLOBAL_10260, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10260 },
    { 10250, 10259, SCRIPTS_GLOBAL_10250, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10520 },
    { 10200, 10249, SCRIPTS_GLOBAL_10200, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10200 },
    { 10130, 10149, SCRIPTS_GLOBAL_10130, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10130 },
    { 10110, 10119, SCRIPTS_GLOBAL_10110, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10110 },
    { 10105, 10109, SCRIPTS_GLOBAL_10105, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10105 },
    { 10100, 10104, SCRIPTS_GLOBAL_10100, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10100 },
    { 10090, 10099, SCRIPTS_GLOBAL_10090, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10090 },
    { 10000, 10089, SCRIPTS_GLOBAL_10000, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10000 },
    { 9600, 9699, SCRIPTS_GLOBAL_9600, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10520 },
    { 8000, 8799, SCRIPTS_GLOBAL_8000, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10520 },
    { 7000, 7799, SCRIPTS_GLOBAL_7000, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_7000 },
    { 5000, 6999, SCRIPTS_GLOBAL_3000, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10520 },
    { 3000, 4999, SCRIPTS_GLOBAL_3000, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10520 },
    { 2800, 2819, SCRIPTS_GLOBAL_2800, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_7000 },
    { 2500, 2799, SCRIPTS_GLOBAL_2500, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_2500 },
    { 2280, 2299, SCRIPTS_GLOBAL_2280, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_2280 },
    { 2260, 2279, SCRIPTS_GLOBAL_2260, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_2260 },
    { 2250, 2259, SCRIPTS_GLOBAL_2250, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_2250 },
    { 2225, 2249, SCRIPTS_GLOBAL_2225, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_2225 },
    { 2200, 2224, SCRIPTS_GLOBAL_2200, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_2200 },
    { 2100, 2199, SCRIPTS_GLOBAL_2100, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_2100 },
    { 2000, 2099, SCRIPTS_GLOBAL_2000, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10520 },
};

// The feature levels at which each command of EVCMD_TABLE may run, which FieldScript_OpcodeGuard checks: level 0 is
// the full feature set, 1 and 2 the reduced ones. It is placed with its only reader: as script_work.c's first object
// it would give the same bytes
static const struct OpcodePermissions EVCMD_PERM_TABLE[] = {
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x000
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x008
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x010
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x018
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x020
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x028
    { 1, 1, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x030
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x038
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x040
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x048
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x050
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x058
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, // 0x060
    { 1, 1, 0 }, { 1, 1, 0 }, { 1, 1, 0 }, { 1, 1, 0 }, { 1, 1, 0 }, { 1, 1, 0 }, { 1, 1, 0 }, { 1, 1, 0 }, // 0x068
    { 1, 1, 0 }, { 1, 1, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x070
    { 1, 1, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x078
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x080
    { 1, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x088
    { 1, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x090
    { 1, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x098
    { 1, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x0a0
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x0a8
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x0b0
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 0, 0 }, { 1, 1, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x0b8
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x0c0
    { 1, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x0c8
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x0d0
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x0d8
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x0e0
    { 1, 1, 1 }, { 0, 0, 0 }, { 1, 1, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x0e8
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x0f0
    { 1, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x0f8
    { 1, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x100
    { 1, 1, 1 }, { 1, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x108
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 0, 0 }, // 0x110
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x118
    { 1, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, // 0x120
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, // 0x128
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 0, 1, 0 }, { 0, 0, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, // 0x130
    { 1, 1, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x138
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x140
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x148
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x150
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, // 0x158
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x160
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, // 0x168
    { 1, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x170
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, // 0x178
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x180
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x188
    { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, // 0x190
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, // 0x198
    { 0, 0, 0 }, { 1, 0, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x1a0
    { 1, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x1a8
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 1, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, // 0x1b0
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, // 0x1b8
    { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x1c0
    { 1, 1, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 1, 0, 1 }, // 0x1c8
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 1, 1 }, // 0x1d0
    { 1, 1, 1 }, { 0, 0, 1 }, { 1, 0, 0 }, { 1, 1, 0 }, { 1, 1, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x1d8
    { 0, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 0, 1, 1 }, { 0, 1, 1 }, { 1, 1, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x1e0
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x1e8
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x1f0
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x1f8
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x200
    { 1, 0, 0 }, { 1, 1, 1 }, { 1, 0, 0 }, { 1, 1, 1 }, { 1, 0, 0 }, { 1, 0, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x208
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x210
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 1, 1 }, // 0x218
    { 1, 1, 1 }, { 1, 1, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 1, 1 }, // 0x220
    { 1, 1, 1 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, // 0x228
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x230
    { 1, 0, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x238
    { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x240
    { 1, 0, 0 }, { 0, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 0, 1, 0 }, { 1, 1, 1 }, { 1, 0, 0 }, // 0x248
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, // 0x250
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 1, 1 }, { 0, 1, 0 }, { 1, 1, 0 }, { 1, 1, 0 }, { 0, 0, 0 }, { 1, 1, 0 }, // 0x258
    { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x260
    { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 0, 0 }, // 0x268
    { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x270
    { 1, 1, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x278
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, // 0x280
    { 0, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x288
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x290
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x298
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 1, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x2a0
    { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x2a8
    { 1, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x2b0
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 1, 1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, // 0x2b8
    { 1, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x2c0
    { 0, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 1 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, // 0x2c8
    { 1, 0, 0 }, { 1, 1, 0 }, { 1, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 0 }, { 0, 0, 0 }, { 1, 1, 0 }, // 0x2d0
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, // 0x2d8
    { 0, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 }, { 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 0 }, // 0x2e0
    { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, // 0x2e8
    { 0, 0, 0 }, { 1, 1, 1 }, { 1, 1, 1 },                                                                  // 0x2f0
};

struct EventScriptCallData {
    ScriptWork *work;
    FieldScriptSupervisor *supervisor;
};

GameEvent *EventScriptCall_Create(GameSystem *gsys, u16 scriptId, FieldActor *actor, HeapID heapId) {
    return EventScriptCall_CreateCore(gsys, heapId, scriptId, actor, 0);
}

ScriptWork *EventScriptCall_Start(GameEvent *event, u16 scriptId, FieldActor *actor, u32 param, HeapID heapId) {
    GameEvent *next = EventScriptCall_CreateCore(GameEvent_GetGameSystem(event), heapId, scriptId, actor, param);
    EventScriptCallData *data = GameEvent_GetData(next);

    GameEvent_ChainNext(event, next);
    return data->work;
}

ScriptWork *EventScriptCall_Replace(GameEvent *event, u16 scriptId, FieldActor *actor, HeapID heapId) {
    GameEvent *next = EventScriptCall_CreateCore(GameEvent_GetGameSystem(event), heapId, scriptId, actor, 0);
    EventScriptCallData *data = GameEvent_GetData(next);

    GameEvent_Replace(event, next);
    return data->work;
}

BOOL FieldScript_VMExists(ScriptWork *work, u8 index) {
    FieldScriptSupervisor *supervisor = ScriptWork_GetSupervisor(work);

    if (supervisor == NULL) {
        return FALSE;
    }
    if (FieldScriptSupervisor_GetVM(supervisor, index) != NULL) {
        return TRUE;
    }
    return FALSE;
}

FieldScriptSupervisor *FieldScriptSupervisor_Create(HeapID heapId) {
    FieldScriptSupervisor *supervisor;

    supervisor = GFL_HeapAllocate(heapId, sizeof(FieldScriptSupervisor), TRUE, "script_sys.c", 0xf1);
    supervisor->heapId = heapId;
    return supervisor;
}

void FieldScriptSupervisor_Free(FieldScriptSupervisor *supervisor) {
    GFL_HeapFree(supervisor);
}

BOOL FieldScriptSupervisor_Update(FieldScriptSupervisor *supervisor) {
    int i;
    VM *vm;

    for (i = 0; i < 3; i++) {
        vm = supervisor->vms[i];
        if (vm != NULL && !VM_Run(vm)) {
            supervisor->vms[i] = NULL;
            supervisor->vmCount--;
            FieldScript_FreeVM(vm);
        }
    }
    return supervisor->vmCount != 0;
}

int FieldScriptSupervisor_AddVM(FieldScriptSupervisor *supervisor, VM *vm) {
    int i;

    for (i = 0; i < 3; i++) {
        if (supervisor->vms[i] == NULL) {
            supervisor->vms[i] = vm;
            supervisor->vmCount++;
            return i;
        }
    }
    return 3;
}

VM *FieldScriptSupervisor_GetVM(FieldScriptSupervisor *supervisor, int index) {
    return supervisor->vms[index];
}

void FieldScriptTerminator_ReplaceEvent(GameEvent *event, void *arg) {
    GameEvent_Replace(event, arg);
}

void FieldScriptSupervisor_SetPostEvent(FieldScriptSupervisor *supervisor, GameEvent *event) {
    supervisor->postFunc = FieldScriptTerminator_ReplaceEvent;
    supervisor->postArg = event;
}

void FieldScriptSupervisor_CallPostFunc(FieldScriptSupervisor *supervisor, GameEvent *event) {
    supervisor->postFunc(event, supervisor->postArg);
}

BOOL FieldScriptSupervisor_HasPostFunc(FieldScriptSupervisor *supervisor) {
    return supervisor->postFunc != NULL;
}

u16 FieldScript_GetZoneIDFromGSys(GameSystem *gsys) {
    return PlayerState_GetZoneID(GSYS_GetPlayerState(gsys));
}

GameEventReturnCode EventScriptCall_Callback(GameEvent *event, u32 *state, void *arg) {
    EventScriptCallData *data = arg;
    ScriptWork *work;
    FieldScriptSupervisor *supervisor;

    supervisor = data->supervisor;
    work = data->work;
    if (FieldScriptSupervisor_Update(supervisor) == 1) {
        return FALSE;
    }
    ScriptWork_Free(work);
    if (FieldScriptSupervisor_HasPostFunc(supervisor) == 1) {
        FieldScriptSupervisor_CallPostFunc(supervisor, event);
        FieldScriptSupervisor_Free(supervisor);
        return FALSE;
    }
    FieldScriptSupervisor_Free(supervisor);
    return TRUE;
}

GameEvent *EventScriptCall_CreateCore(GameSystem *gsys, HeapID heapId, u16 scriptId, FieldActor *actor, u32 param) {
    GameEvent *event;
    EventScriptCallData *data;
    u16 zoneId;

    event = GameEvent_Create(gsys, NULL, EventScriptCall_Callback, sizeof(EventScriptCallData));
    data = GameEvent_GetData(event);
    data->supervisor = FieldScriptSupervisor_Create(4);
    data->work = ScriptWork_Create(4, gsys, event, scriptId, param, 0);
    ScriptWork_SetParentActor(data->work, actor);
    zoneId = FieldScript_GetZoneIDFromGSys(gsys);
    ScriptWork_AddVM(data->work, zoneId, scriptId);
    return event;
}

void ScriptWork_CallEvent(ScriptWork *work, GameEvent *event) {
    GameEvent_ChainNext(ScriptWork_GetEvent(work), event);
}

void ScriptWork_SetPostEvent(ScriptWork *work, GameEvent *event) {
    FieldScriptSupervisor *supervisor = ScriptWork_GetSupervisor(work);

    if (supervisor != NULL) {
        FieldScriptSupervisor_SetPostEvent(supervisor, event);
    }
}

u32 ScriptWork_AddVM(ScriptWork *work, u16 zoneId, u16 scriptId) {
    FieldScriptSupervisor *supervisor;
    VM *vm;

    supervisor = ScriptWork_GetSupervisor(work);
    if (supervisor != NULL) {
        vm = FieldScript_CreateVM(supervisor->heapId, work, zoneId, scriptId, 0);
        return FieldScriptSupervisor_AddVM(supervisor, vm);
    }
    return 3;
}

ScriptWork *EventScriptCall_GetWork(GameEvent *event) {
    EventScriptCallData *data = GameEvent_GetData(event);

    return data->work;
}

FieldScriptSupervisor *ScriptWork_GetSupervisor(ScriptWork *work) {
    GameEvent *event = ScriptWork_GetEvent(work);

    if (event == NULL) {
        return NULL;
    }
    return ((EventScriptCallData *)GameEvent_GetData(event))->supervisor;
}

void FieldScript_ResetMapLocalEvents(EventWork *eventWork) {
    EventWork_FlagResetRange(eventWork, 0, 0x63);
    EventWork_WorkResetRange(eventWork, 0x4000, 0x401f);
}

void FieldScript_CallPlayerInitSetup(GameSystem *gsys, u32 a1) {
    FieldScript_Run(gsys, NULL, 0x2580, 2);
}

void FieldScript_CallPlayerPostHOFSetup(GameSystem *gsys, u32 unused) {
    FieldScript_Run(gsys, NULL, 0x2581, 2);
}

void FieldScript_Run(GameSystem *gsys, ScriptWork *work, u16 scriptId, u32 featureLevel) {
    ScriptWork *owned;
    u16 zoneId;
    VM *vm;

    owned = NULL;
    if (work == NULL) {
        owned = ScriptWork_Create(4, gsys, NULL, scriptId, 0, featureLevel);
        work = owned;
    }
    zoneId = FieldScript_GetZoneIDFromGSys(gsys);
    vm = FieldScript_CreateVM(4, work, zoneId, scriptId, featureLevel);
    while (VM_Run(vm) == 1) {
    }
    FieldScript_FreeVM(vm);
    if (owned != NULL) {
        ScriptWork_Free(owned);
    }
}

u32 FieldScript_CallZoneInitCore(GameSystem *gsys, u32 arg1, u32 mode, u32 featureLevel) {
    GameData *gameData;
    EventData *eventData;
    const u8 *script;
    u16 scriptId;
    u32 result;

    result = 0;
    gameData = GSYS_GetGameData(gsys);
    eventData = GameData_GetEventData(gameData);
    script = GetZoneInitScrPointer(eventData);
    do {
        script = FieldScript_GetInitSCRID(script, mode, &scriptId);
        if (scriptId != 0xffff) {
            FieldScript_Run(gsys, NULL, scriptId, featureLevel);
            result = 1;
        }
    } while (scriptId != 0xffff);
    return result;
}

void FieldScript_CallOnZoneReload(GameSystem *gsys, u32 arg1) {
    FieldScript_CallZoneInitCore(gsys, arg1, 4, 1);
}

void FieldScript_CallOnZoneNewLoad(GameSystem *gsys, u32 arg1) {
    FieldScript_CallZoneInitCore(gsys, arg1, 3, 1);
}

void FieldScript_CallOnZoneInit(GameSystem *gsys, u32 arg1) {
    GameData *gameData;
    EventWork *eventWork;

    gameData = GSYS_GetGameData(gsys);
    eventWork = GameData_GetEventWork(gameData);
    FieldScript_ResetMapLocalEvents(eventWork);
    FieldScript_CallZoneInitCore(gsys, arg1, 2, 2);
}

GameEvent *FieldScript_CheckSceneChangeEvent(GameSystem *gsys, HeapID heapId) {
    GameData *gameData;
    EventData *eventData;
    u16 scriptId;

    gameData = GSYS_GetGameData(gsys);
    eventData = GameData_GetEventData(gameData);
    scriptId = FieldScript_GetSceneChangeSCRID(gameData, GetZoneInitScrPointer(eventData), 1);
    if (scriptId == 0xffff) {
        return NULL;
    }
    return EventScriptCall_Create(gsys, scriptId, NULL, heapId);
}

const u8 *FieldScript_GetInitSCRID(const u8 *script, u32 mode, u16 *scriptId) {
    u16 type;
    u16 value;

    while (TRUE) {
        type = script[0] + (script[1] << 8);
        if (type == 0) {
            *scriptId = 0xffff;
            return script;
        }
        if (mode == type) {
            value = script[2] + (script[3] << 8);
            *scriptId = value;
            return script + 6;
        }
        script += 6;
    }
}

u16 FieldScript_GetSceneChangeSCRID(GameData *gameData, const u8 *script, u32 mode) {
    EventWork *eventWork;
    u16 type;
    u16 workId;
    u16 value;
    u32 offset;

    while (TRUE) {
        type = script[0] + (script[1] << 8);
        if (type == 0) {
            return 0xffff;
        }
        if (mode == type) {
            offset = script[2] + (script[3] << 8) + (script[4] << 16) + (script[5] << 24);
            script += 6;
            break;
        }
        script += 6;
    }
    if (offset == 0) {
        return 0xffff;
    }
    script += offset;
    eventWork = GameData_GetEventWork(gameData);
    while (TRUE) {
        workId = script[0] + (script[1] << 8);
        if (workId == 0) {
            return 0xffff;
        }
        // The original parser checks the terminator again before reading the entry.
        if (workId == 0) {
            return 0xffff;
        }
        value = script[2] + (script[3] << 8);
        if (*EventWork_GetWkPtr(eventWork, workId) == value) {
            return script[4] + (script[5] << 8);
        }
        script += 6;
    }
}

VM *FieldScript_CreateVM(HeapID heapId, ScriptWork *work, u16 zoneId, u16 scriptId, u32 featureLevel) {
    FieldScriptEnvArgs args;
    VMInitParam param;
    FieldScriptEnv *env;
    GameSystem *gsys;
    GameData *gameData;
    VM *vm;
    u16 fileId;
    u16 msgArcId;
    u16 msgFileNo;
    u32 scriptOffset;
    u32 reduced;
    u32 workSize;

    reduced = FieldScript_IsVMFeatureSetReduced(featureLevel);
    args.zoneId = zoneId;
    args.unk02 = scriptId;
    args.featureLevel = featureLevel;
    args.reducedFeatureLevel = reduced;
    args.work = work;
    env = CreateFieldScriptEnv(&args, heapId);
    gsys = ScriptWork_GetGameSystem(work);
    gameData = GSYS_GetGameData(gsys);
    param.stackSize = 0x100;
    workSize = 0x40;
    param.workSize = workSize;
    param.commands = (const VMCommand *)EVCMD_TABLE;
    param.commandCount = EVCMD_MAX;
    param.extraCommands = (const VMCommand *)GetCurrentScrPluginTable(gameData);
    param.extraCommandCount = GetCurrentScrPluginCmdCount(gameData);
    param.extraCommandStart = 1000;
    vm = VM_Create(heapId, &param);
    VM_ChangeEnv(vm, env);
    scriptOffset = FieldScript_ResolveSCRID(zoneId, scriptId, &fileId, &msgArcId, &msgFileNo);
    vm->unk38 = (u32)FieldScript_LoadData(fileId, heapId);
    workSize -= 0x41;
    if (!reduced && msgFileNo != workSize) {
        SetFieldScriptEnvMsgData(env, msgArcId, msgFileNo);
    }
    VM_LoadScript(vm, (void *)vm->unk38);
    vm->pc += scriptOffset * 4;
    vm->pc += VM_Read32(vm);
    FieldScript_AttachOpcodeGuard(vm);
    return vm;
}

void FieldScript_FreeVM(VM *vm) {
    FieldScriptEnv *env;

    env = VM_GetEnv(vm);
    FreeFieldScriptEnv(env);
    GFL_HeapFree((void *)vm->unk38);
    VM_Free(vm);
}

u32 FieldScript_ResolveSCRID(u32 zoneId, u16 scriptId, u16 *fileId, u16 *msgArcId, u16 *msgFileNo) {
    u32 i;
    u16 zoneScriptFile;
    u16 zoneTextFile;
    const GlobalScriptEntry *table;

    table = GLOBAL_SCRIPT_TABLE;
    for (i = 0; i < NELEMS(GLOBAL_SCRIPT_TABLE); i++) {
        if (scriptId >= table[i].start) {
            GFL_ASSERT_MSG(scriptId <= table[i].end, "id=%d end=%d", scriptId, table[i].end);
            *fileId = table[i].fileId;
            *msgArcId = table[i].msgArcId;
            *msgFileNo = table[i].msgFileNo;
            return (u16)(scriptId - table[i].start);
        }
    }
    if (scriptId >= 1) {
        zoneScriptFile = ZoneData_GetScriptDatID(zoneId);
        zoneTextFile = ZoneData_GetTextDatID(zoneId);
        *fileId = zoneScriptFile;
        *msgArcId = ARCID_SCRIPT_MESSAGE;
        *msgFileNo = zoneTextFile;
        return (u16)(scriptId - 1);
    }
    *fileId = SCRIPTS_GLOBAL_2000;
    *msgArcId = ARCID_SCRIPT_MESSAGE;
    *msgFileNo = SCRIPT_TEXT_GLOBAL_10520;
    return 0;
}

// Where a script file is read from. Only the archive is left in the retail game, but the test of the source stays in
// the code, which MWCC only does for a variable of an enum type
typedef enum {
    SCRIPT_SOURCE_ARCHIVE,
    SCRIPT_SOURCE_OTHER,
} ScriptSource;

void *FieldScript_LoadData(u16 fileId, HeapID heapId) {
    ScriptSource source = SCRIPT_SOURCE_ARCHIVE;
    void *data = NULL;

    if (source == SCRIPT_SOURCE_ARCHIVE) {
        data = GFL_ArcSysReadHeapNewLZ(ARCID_EVENT_SCRIPT, fileId, FALSE, heapId);
    }
    return data;
}

BOOL FieldScript_CheckSCRID(u16 scriptId) {
    if (scriptId == 0x7d0) {
        return FALSE;
    }
    if (scriptId < 0x2aa2) {
        return TRUE;
    }
    return FALSE;
}

u32 FieldScript_IsVMFeatureSetReduced(u32 featureLevel) {
    if (featureLevel != 0) {
        return 1;
    }
    return 0;
}

void FieldScript_ThrowOpcodeAccessError(void) {
    if (GFL_NetIsInitialized()) {
        GFL_NetCancelEnd();
        GFL_NetExit(0);
        GFL_NetIwFree();
        do {
            GFL_NetMainWhileWaiting();
        } while (!GFL_NetIsEnded());
    }
    while (TRUE) {
        Machine_ProcessExitRequest();
    }
}

BOOL FieldScript_OpcodeGuard(VM *vm, void *env, void *gsys, u16 cmd) {
    u32 featureLevel;
    u32 allowed;

    GSYS_GetField((GameSystem *)gsys);
    FieldScriptEnv_IsReducedFeatureLevel((FieldScriptEnv *)env);
    featureLevel = FieldScriptEnv_GetFeatureLevel((FieldScriptEnv *)env);
    allowed = 0;
    switch (featureLevel) {
    case 0:
        allowed = EVCMD_PERM_TABLE[cmd].level0;
        break;
    case 1:
        allowed = EVCMD_PERM_TABLE[cmd].level1;
        break;
    case 2:
        allowed = EVCMD_PERM_TABLE[cmd].level2;
        break;
    }
    if (!allowed) {
        FieldScript_ThrowOpcodeAccessError();
    }
    return allowed;
}

void FieldScript_AttachOpcodeGuard(VM *vm) {
    FieldScriptEnv *env;
    GameSystem *gsys;

    env = VM_GetEnv(vm);
    gsys = FieldScriptEnv_GetGameSystem(env);
    VM_SetCallbackVerifier(vm, FieldScript_OpcodeGuard, gsys);
}
