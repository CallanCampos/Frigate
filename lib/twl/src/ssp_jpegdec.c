#include "types.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "ssp_private.h"
#include "twl/ssp.h"

// TwlSDK's JPEG decoder (the SSP library's), which decodes a baseline JPEG of three components, sampled 1x1, 2x1, 1x2
// or 2x2, into an RGB555 bitmap, after checking the DSi's signature in its maker note. A small one-pass decoder after
// the IJG library: Huffman tables with 8-bit lookahead, the AAN fast integer IDCT and IJG's color tables. The file's
// name is a guess after TwlSDK's ssp/jpegdec.h; the names of its functions and data are guesses too.

// The Huffman tables' work area: the BITS and HUFFVAL of the four tables, then each one's derived table
#define HUFF_WORK_SIZE 0x1800
#define HUFF_WORK_CLEAR_SIZE 0x1000

// The tables, as DHT gives them: luminance DC, luminance AC, chrominance DC, chrominance AC
enum {
    HUFF_TBL_DC_Y,
    HUFF_TBL_AC_Y,
    HUFF_TBL_DC_C,
    HUFF_TBL_AC_C,
};

// Offsets in the work area: the BITS of each table, whose HUFFVAL follow 0x14 bytes after, and its derived table
#define HUFF_BITS_C 0x138
#define HUFF_BITS_AC 0x24
#define HUFF_VALS 0x14
#define HUFF_DERIVED 0x270
#define HUFF_DERIVED_C 0x560
#define HUFF_DERIVED_AC 0x2b0

// A derived table, in units of its arrays' elements: the first code of each length (s32), then the last (s32), the
// index of the first value of each length (s16), and the lookahead tables of the codes of up to 8 bits (u8)
#define HUFF_MAXCODE 17
#define HUFF_VALPTR 0x46
#define HUFF_LOOK_NBITS 0xae
#define HUFF_LOOK_SYM 0x1ae

// The bitstream
static s32 sStuffed;
static s16 *sDerivedTbl16;
static s32 sVSamp;
static s32 sMcuHeight;
static s32 sHSamp;
static s32 sError;
static s32 sBitsLeft;
static u32 sDataSize;
static s32 sBitBuffer;
static s32 *sDerivedTbl32;
static s32 sMcuWidth;

// The offset of each table's derived table in the work area
static s32 sDerivedTblOffsets[4] = { 0x270, 0x520, 0x7d0, 0xa80 };

static u8 sZigzag[64] = {
    0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,  12, 19, 26, 33, 40, 48,
    41, 34, 27, 20, 13, 6,  7,  14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23,
    30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63,
};

// The AAN IDCT's scale factors, in zigzag order, with 14 fraction bits
static s16 sAanScales[64] = {
    16384, 22725, 22725, 21407, 31521, 21407, 19266, 29692, 29692, 19266, 16384, 26722, 27969, 26722, 16384, 12873,
    22725, 25172, 25172, 22725, 12873, 8867,  17855, 21407, 22654, 21407, 17855, 8867,  4520,  12299, 16819, 19266,
    19266, 16819, 12299, 4520,  6270,  11585, 15137, 16384, 15137, 11585, 6270,  5906,  10426, 12873, 12873, 10426,
    5906,  5315,  8867,  10114, 8867,  5315,  4520,  6967,  6967,  4520,  3552,  4799,  3552,  2446,  2446,  1247,
};

// IJG's tables for the conversion from YCbCr (jdcolor.c's build_ycc_rgb_table): Cb's part of blue and Cr's of red,
// and their parts of green with 16 fraction bits
static s16 sCbBTab[256] = {
    -227, -225, -223, -222, -220, -218, -216, -214, -213, -211, -209, -207, -206, -204, -202, -200, -198, -197, -195,
    -193, -191, -190, -188, -186, -184, -183, -181, -179, -177, -175, -174, -172, -170, -168, -167, -165, -163, -161,
    -159, -158, -156, -154, -152, -151, -149, -147, -145, -144, -142, -140, -138, -136, -135, -133, -131, -129, -128,
    -126, -124, -122, -120, -119, -117, -115, -113, -112, -110, -108, -106, -105, -103, -101, -99,  -97,  -96,  -94,
    -92,  -90,  -89,  -87,  -85,  -83,  -82,  -80,  -78,  -76,  -74,  -73,  -71,  -69,  -67,  -66,  -64,  -62,  -60,
    -58,  -57,  -55,  -53,  -51,  -50,  -48,  -46,  -44,  -43,  -41,  -39,  -37,  -35,  -34,  -32,  -30,  -28,  -27,
    -25,  -23,  -21,  -19,  -18,  -16,  -14,  -12,  -11,  -9,   -7,   -5,   -4,   -2,   0,    2,    4,    5,    7,
    9,    11,   12,   14,   16,   18,   19,   21,   23,   25,   27,   28,   30,   32,   34,   35,   37,   39,   41,
    43,   44,   46,   48,   50,   51,   53,   55,   57,   58,   60,   62,   64,   66,   67,   69,   71,   73,   74,
    76,   78,   80,   82,   83,   85,   87,   89,   90,   92,   94,   96,   97,   99,   101,  103,  105,  106,  108,
    110,  112,  113,  115,  117,  119,  120,  122,  124,  126,  128,  129,  131,  133,  135,  136,  138,  140,  142,
    144,  145,  147,  149,  151,  152,  154,  156,  158,  159,  161,  163,  165,  167,  168,  170,  172,  174,  175,
    177,  179,  181,  183,  184,  186,  188,  190,  191,  193,  195,  197,  198,  200,  202,  204,  206,  207,  209,
    211,  213,  214,  216,  218,  220,  222,  223,  225,
};

static s16 sCrRTab[256] = {
    -179, -178, -177, -175, -174, -172, -171, -170, -168, -167, -165, -164, -163, -161, -160, -158, -157, -156, -154,
    -153, -151, -150, -149, -147, -146, -144, -143, -142, -140, -139, -137, -136, -135, -133, -132, -130, -129, -128,
    -126, -125, -123, -122, -121, -119, -118, -116, -115, -114, -112, -111, -109, -108, -107, -105, -104, -102, -101,
    -100, -98,  -97,  -95,  -94,  -93,  -91,  -90,  -88,  -87,  -86,  -84,  -83,  -81,  -80,  -79,  -77,  -76,  -74,
    -73,  -72,  -70,  -69,  -67,  -66,  -64,  -63,  -62,  -60,  -59,  -57,  -56,  -55,  -53,  -52,  -50,  -49,  -48,
    -46,  -45,  -43,  -42,  -41,  -39,  -38,  -36,  -35,  -34,  -32,  -31,  -29,  -28,  -27,  -25,  -24,  -22,  -21,
    -20,  -18,  -17,  -15,  -14,  -13,  -11,  -10,  -8,   -7,   -6,   -4,   -3,   -1,   0,    1,    3,    4,    6,
    7,    8,    10,   11,   13,   14,   15,   17,   18,   20,   21,   22,   24,   25,   27,   28,   29,   31,   32,
    34,   35,   36,   38,   39,   41,   42,   43,   45,   46,   48,   49,   50,   52,   53,   55,   56,   57,   59,
    60,   62,   63,   64,   66,   67,   69,   70,   72,   73,   74,   76,   77,   79,   80,   81,   83,   84,   86,
    87,   88,   90,   91,   93,   94,   95,   97,   98,   100,  101,  102,  104,  105,  107,  108,  109,  111,  112,
    114,  115,  116,  118,  119,  121,  122,  123,  125,  126,  128,  129,  130,  132,  133,  135,  136,  137,  139,
    140,  142,  143,  144,  146,  147,  149,  150,  151,  153,  154,  156,  157,  158,  160,  161,  163,  164,  165,
    167,  168,  170,  171,  172,  174,  175,  177,  178,
};

static s32 sCbGTab[256] = {
    2919680,  2897126,  2874572,  2852018,  2829464,  2806910,  2784356,  2761802,  2739248,  2716694,  2694140,
    2671586,  2649032,  2626478,  2603924,  2581370,  2558816,  2536262,  2513708,  2491154,  2468600,  2446046,
    2423492,  2400938,  2378384,  2355830,  2333276,  2310722,  2288168,  2265614,  2243060,  2220506,  2197952,
    2175398,  2152844,  2130290,  2107736,  2085182,  2062628,  2040074,  2017520,  1994966,  1972412,  1949858,
    1927304,  1904750,  1882196,  1859642,  1837088,  1814534,  1791980,  1769426,  1746872,  1724318,  1701764,
    1679210,  1656656,  1634102,  1611548,  1588994,  1566440,  1543886,  1521332,  1498778,  1476224,  1453670,
    1431116,  1408562,  1386008,  1363454,  1340900,  1318346,  1295792,  1273238,  1250684,  1228130,  1205576,
    1183022,  1160468,  1137914,  1115360,  1092806,  1070252,  1047698,  1025144,  1002590,  980036,   957482,
    934928,   912374,   889820,   867266,   844712,   822158,   799604,   777050,   754496,   731942,   709388,
    686834,   664280,   641726,   619172,   596618,   574064,   551510,   528956,   506402,   483848,   461294,
    438740,   416186,   393632,   371078,   348524,   325970,   303416,   280862,   258308,   235754,   213200,
    190646,   168092,   145538,   122984,   100430,   77876,    55322,    32768,    10214,    -12340,   -34894,
    -57448,   -80002,   -102556,  -125110,  -147664,  -170218,  -192772,  -215326,  -237880,  -260434,  -282988,
    -305542,  -328096,  -350650,  -373204,  -395758,  -418312,  -440866,  -463420,  -485974,  -508528,  -531082,
    -553636,  -576190,  -598744,  -621298,  -643852,  -666406,  -688960,  -711514,  -734068,  -756622,  -779176,
    -801730,  -824284,  -846838,  -869392,  -891946,  -914500,  -937054,  -959608,  -982162,  -1004716, -1027270,
    -1049824, -1072378, -1094932, -1117486, -1140040, -1162594, -1185148, -1207702, -1230256, -1252810, -1275364,
    -1297918, -1320472, -1343026, -1365580, -1388134, -1410688, -1433242, -1455796, -1478350, -1500904, -1523458,
    -1546012, -1568566, -1591120, -1613674, -1636228, -1658782, -1681336, -1703890, -1726444, -1748998, -1771552,
    -1794106, -1816660, -1839214, -1861768, -1884322, -1906876, -1929430, -1951984, -1974538, -1997092, -2019646,
    -2042200, -2064754, -2087308, -2109862, -2132416, -2154970, -2177524, -2200078, -2222632, -2245186, -2267740,
    -2290294, -2312848, -2335402, -2357956, -2380510, -2403064, -2425618, -2448172, -2470726, -2493280, -2515834,
    -2538388, -2560942, -2583496, -2606050, -2628604, -2651158, -2673712, -2696266, -2718820, -2741374, -2763928,
    -2786482, -2809036, -2831590,
};

static s32 sCrGTab[256] = {
    5990656,  5943854,  5897052,  5850250,  5803448,  5756646,  5709844,  5663042,  5616240,  5569438,  5522636,
    5475834,  5429032,  5382230,  5335428,  5288626,  5241824,  5195022,  5148220,  5101418,  5054616,  5007814,
    4961012,  4914210,  4867408,  4820606,  4773804,  4727002,  4680200,  4633398,  4586596,  4539794,  4492992,
    4446190,  4399388,  4352586,  4305784,  4258982,  4212180,  4165378,  4118576,  4071774,  4024972,  3978170,
    3931368,  3884566,  3837764,  3790962,  3744160,  3697358,  3650556,  3603754,  3556952,  3510150,  3463348,
    3416546,  3369744,  3322942,  3276140,  3229338,  3182536,  3135734,  3088932,  3042130,  2995328,  2948526,
    2901724,  2854922,  2808120,  2761318,  2714516,  2667714,  2620912,  2574110,  2527308,  2480506,  2433704,
    2386902,  2340100,  2293298,  2246496,  2199694,  2152892,  2106090,  2059288,  2012486,  1965684,  1918882,
    1872080,  1825278,  1778476,  1731674,  1684872,  1638070,  1591268,  1544466,  1497664,  1450862,  1404060,
    1357258,  1310456,  1263654,  1216852,  1170050,  1123248,  1076446,  1029644,  982842,   936040,   889238,
    842436,   795634,   748832,   702030,   655228,   608426,   561624,   514822,   468020,   421218,   374416,
    327614,   280812,   234010,   187208,   140406,   93604,    46802,    0,        -46802,   -93604,   -140406,
    -187208,  -234010,  -280812,  -327614,  -374416,  -421218,  -468020,  -514822,  -561624,  -608426,  -655228,
    -702030,  -748832,  -795634,  -842436,  -889238,  -936040,  -982842,  -1029644, -1076446, -1123248, -1170050,
    -1216852, -1263654, -1310456, -1357258, -1404060, -1450862, -1497664, -1544466, -1591268, -1638070, -1684872,
    -1731674, -1778476, -1825278, -1872080, -1918882, -1965684, -2012486, -2059288, -2106090, -2152892, -2199694,
    -2246496, -2293298, -2340100, -2386902, -2433704, -2480506, -2527308, -2574110, -2620912, -2667714, -2714516,
    -2761318, -2808120, -2854922, -2901724, -2948526, -2995328, -3042130, -3088932, -3135734, -3182536, -3229338,
    -3276140, -3322942, -3369744, -3416546, -3463348, -3510150, -3556952, -3603754, -3650556, -3697358, -3744160,
    -3790962, -3837764, -3884566, -3931368, -3978170, -4024972, -4071774, -4118576, -4165378, -4212180, -4258982,
    -4305784, -4352586, -4399388, -4446190, -4492992, -4539794, -4586596, -4633398, -4680200, -4727002, -4773804,
    -4820606, -4867408, -4914210, -4961012, -5007814, -5054616, -5101418, -5148220, -5195022, -5241824, -5288626,
    -5335428, -5382230, -5429032, -5475834, -5522636, -5569438, -5616240, -5663042, -5709844, -5756646, -5803448,
    -5850250, -5897052, -5943854,
};

// IJG's range limit table (jdmaster.c's prepare_range_limit_table): 256 zeros, the samples 0 to 255, then 384
// bytes of 255, 384 zeros and the samples 0 to 127, for the IDCT's output with its wraparound
static u8 sRangeLimit[0x580] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
    20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,
    43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,
    66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,
    89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
    135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157,
    158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
    181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203,
    204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226,
    227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249,
    250, 251, 252, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   2,   3,   4,   5,   6,   7,
    8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,
    31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,
    54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,
    77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
    123, 124, 125, 126, 127,
};

static s16 sCb[64];
static s16 sCr[64];
// The quantization tables, by table number
static s16 sQuant[2][64];
// The coefficients of a Y, Cb and Cr block, whose first is the DC prediction, and the IDCT's input and work area
static s16 sBlocks[5][64];
static s16 sY[16][64];
static u8 sHuffWork[HUFF_WORK_SIZE];

enum {
    BLOCK_Y,
    BLOCK_IDCT_IN,
    BLOCK_IDCT_WORK,
    BLOCK_CB,
    BLOCK_CR,
};

static void func_ov257_021ab764(u8 *data, s16 *coef, s32 *pos, int tbl);
static s32 func_ov257_021ab808(u8 *data, s32 *pos, s32 value, int nbits);
static void func_ov257_021ab884(s16 *dest, int block);
static void func_ov257_021ab8dc(const s16 *in, s16 *out, int quant);
static int func_ov257_021abd3c(u8 *data, s32 *pos);
static int func_ov257_021abd84(u8 *data, s32 *pos, int tbl);
static void func_ov257_021ac018(u8 *dest, int x, int y, int width, int height);

// Checks the markers of a JPEG without decoding it
static s32 func_ov257_021aafdc(u8 *data, u32 size, u32 option) {
    s32 pos;

    sStuffed = 0;
    sBitsLeft = 0;
    sBlocks[BLOCK_Y][0] = sBlocks[BLOCK_CB][0] = sBlocks[BLOCK_CR][0] = 0;
    sys_memset32_fast(0, sHuffWork, HUFF_WORK_CLEAR_SIZE);
    sDataSize = size;
    if (size <= 2) {
        return 0;
    }
    if (data[0] != 0xff || data[1] != 0xd8) {
        return 0;
    }

    pos = 2;
    while (pos + 1 < size) {
        if (data[pos++] == 0xff) {
            u8 marker = data[pos++];
            int len;

            if (marker == 0xd9) {
                break;
            }
            if (pos + 1 >= size) {
                return 0;
            }
            len = (data[pos] << 8) | data[pos + 1];
            if (marker == 0xe1) {
                int result = func_ov257_021acc20(data, size, &pos, option);
                if (result == 0 || result == 1) {
                    break;
                }
                return 0;
            }
            pos += len;
        }
    }
    return -(pos <= sDataSize);
}

// Decodes a JPEG into dest, and returns its height and width, or 0
static u32 func_ov257_021ab0a4(u8 *dest, u8 *data, u32 size, s32 maxWidth, s32 maxHeight, u32 option) {
    s32 width = 0;
    s32 height = 0;
    s32 restartInterval;
    s32 restartCount;
    s32 p;
    u8 marker;
    int len;
    int result;
    u8 sampling;
    u8 info;
    s32 bitsOffset;
    s32 tblOffset;
    u8 *bits;
    int count;
    int i;
    int k;
    s32 code;
    s32 si;
    s32 lookbits;
    int n;
    int base;
    u8 z;
    s32 end;
    int l;
    s32 x;
    s32 y;
    u16 huffcode[257];
    s8 huffsize[257];
    s32 pos;

    sStuffed = 0;
    sBitsLeft = 0;
    sBlocks[BLOCK_Y][0] = sBlocks[BLOCK_CB][0] = sBlocks[BLOCK_CR][0] = 0;
    restartCount = 0;
    restartInterval = 0;
    sys_memset32_fast(0, sHuffWork, HUFF_WORK_CLEAR_SIZE);
    sDataSize = size;
    if (size <= 2) {
        return 0;
    }
    if (data[0] != 0xff || data[1] != 0xd8) {
        return 0;
    }

    pos = 2;
    while (pos + 1 < sDataSize) {
        if (data[pos++] == 0xff) {
            marker = data[pos++];
            if (marker == 0xd9) {
                break;
            }
            if (pos + 1 >= sDataSize) {
                return 0;
            }
            len = (data[pos] << 8) | data[pos + 1];
            if (marker == 0xe1) {
                result = func_ov257_021acc20(data, sDataSize, &pos, option);
                if (result != 0 && result != 1) {
                    return 0;
                }
            } else if (marker == 0xc0 || marker == 0xc1) {
                if (pos + 9 >= sDataSize) {
                    return 0;
                }
                height = (data[pos + 3] << 8) | data[pos + 4];
                width = (data[pos + 5] << 8) | data[pos + 6];
                sampling = data[pos + 9];
                pos += len;
                if (sampling != 0x11 && sampling != 0x21 && sampling != 0x12 && sampling != 0x22) {
                    return 0;
                }
                sHSamp = sampling >> 4;
                sVSamp = sampling & 0xf;
                sMcuWidth = sHSamp * 8;
                sMcuHeight = (sampling & 0xf) * 8;
                if (width > maxWidth || height > maxHeight) {
                    return 0;
                }
            } else if (marker == 0xc4) {
                p = pos + 2;
                if (p >= sDataSize) {
                    return 0;
                }
                pos += len;
                while (p < pos) {
                    if (p + 17 >= sDataSize) {
                        return 0;
                    }
                    info = data[p++];
                    bitsOffset = 0;
                    tblOffset = 0;
                    if (info & 1) {
                        tblOffset += HUFF_DERIVED_C;
                        bitsOffset += HUFF_BITS_C;
                    }
                    if (info & 0x10) {
                        bitsOffset += HUFF_BITS_AC;
                        tblOffset += HUFF_DERIVED_AC;
                    }
                    sDerivedTbl16 = (s16 *)&sHuffWork[tblOffset + HUFF_DERIVED];
                    sDerivedTbl32 = (s32 *)&sHuffWork[tblOffset + HUFF_DERIVED];
                    count = 0;
                    bits = &sHuffWork[bitsOffset];
                    sHuffWork[bitsOffset] = 0;
                    for (i = 1; i <= 16; i++) {
                        bits[i] = data[p];
                        count += bits[i];
                        p++;
                    }
                    if (p + count >= sDataSize) {
                        return 0;
                    }
                    for (i = 0; i < count; i++) {
                        bits[HUFF_VALS + i] = data[p];
                        p++;
                    }

                    k = 0;
                    for (l = 1; l <= 16; l++) {
                        for (i = 1; i <= bits[l] && k <= 256; i++, k++) {
                            huffsize[k] = l;
                        }
                    }
                    if (k >= 257) {
                        sError = 1;
                        return 0;
                    }
                    huffsize[k] = 0;

                    code = 0;
                    k = 0;
                    si = huffsize[0];
                    while (huffsize[k] != 0) {
                        while (huffsize[k] == si) {
                            huffcode[k++] = code;
                            code++;
                            if (k >= 257) {
                                sError = 1;
                                return 0;
                            }
                        }
                        code <<= 1;
                        si++;
                    }

                    k = 0;
                    for (l = 1; l <= 16; l++) {
                        if (bits[l] != 0) {
                            sDerivedTbl16[HUFF_VALPTR + l] = k;
                            sDerivedTbl32[l] = huffcode[k];
                            k += bits[l];
                            if (k >= 257) {
                                sError = 1;
                                return 0;
                            }
                            sDerivedTbl32[HUFF_MAXCODE + l] = huffcode[k - 1];
                        } else {
                            sDerivedTbl32[HUFF_MAXCODE + l] = -1;
                        }
                    }
                    sDerivedTbl32[HUFF_MAXCODE + 17] = 0xfffff;

                    k = 0;
                    for (l = 1; l <= 8; l++) {
                        for (i = 1; i <= bits[l]; i++, k++) {
                            if (k >= 257) {
                                sError = 1;
                                return 0;
                            }
                            lookbits = huffcode[k] << (8 - l);
                            if (lookbits < -0xc2e || lookbits + (1 << (8 - l)) >= 0xbd3) {
                                sError = 1;
                                return 0;
                            }
                            for (n = 1 << (8 - l); n > 0; n--) {
                                if (tblOffset + HUFF_DERIVED + HUFF_LOOK_NBITS + lookbits < 0 ||
                                    tblOffset + HUFF_DERIVED + HUFF_LOOK_SYM + lookbits >= HUFF_WORK_SIZE) {
                                    sError = 1;
                                    return 0;
                                }
                                sHuffWork[tblOffset + HUFF_DERIVED + HUFF_LOOK_NBITS + lookbits] = l;
                                sHuffWork[tblOffset + HUFF_DERIVED + HUFF_LOOK_SYM + lookbits] = bits[HUFF_VALS + k];
                                lookbits++;
                            }
                        }
                    }
                }
            } else if (marker == 0xd8) {
            } else if (marker == 0xda) {
                if (pos + 12 >= sDataSize) {
                    return 0;
                }
                pos += 12;
                for (y = 0; y < height; y += sMcuHeight) {
                    for (x = 0; x < width; x += sMcuWidth) {
                        if (restartInterval != 0 && --restartCount == 0) {
                            restartCount = restartInterval;
                            if (sBitsLeft > 7) {
                                pos = pos - sStuffed - 1;
                            }
                            pos += 2;
                            sBitsLeft = 0;
                            sBlocks[BLOCK_Y][0] = sBlocks[BLOCK_CB][0] = sBlocks[BLOCK_CR][0] = 0;
                        }
                        for (i = 0; i < sHSamp * sVSamp; i++) {
                            func_ov257_021ab764(data, sBlocks[BLOCK_Y], &pos, HUFF_TBL_DC_Y);
                            func_ov257_021ab884(sY[i], BLOCK_Y);
                        }
                        func_ov257_021ab764(data, sBlocks[BLOCK_CB], &pos, HUFF_TBL_DC_C);
                        func_ov257_021ab884(sCb, BLOCK_CB);
                        func_ov257_021ab764(data, sBlocks[BLOCK_CR], &pos, HUFF_TBL_DC_C);
                        func_ov257_021ab884(sCr, BLOCK_CR);
                        func_ov257_021ac018(dest, x, y, width, height);
                    }
                }
                break;
            } else if (marker == 0xdb) {
                end = pos + len;
                p = pos + 2;

                pos = end;
                while (p < end) {
                    info = data[p];
                    if (info & 0xf0) {
                        if (p + 0x82 >= sDataSize) {
                            return 0;
                        }
                        base = (info & 0xf) * 64;
                        p++;
                        for (i = 0; i < 64; i++) {
                            z = sZigzag[i];
                            if (base + z > 127) {
                                return 0;
                            }
                            sQuant[0][base + z] = ((data[p + 1] + (data[p] << 8)) * sAanScales[i] + 0x800) >> 12;
                            p += 2;
                        }
                    } else {
                        if (p + 0x41 >= sDataSize) {
                            return 0;
                        }
                        base = info * 64;
                        p++;
                        for (i = 0; i < 64; i++) {
                            z = sZigzag[i];
                            if (base + z > 127) {
                                return 0;
                            }
                            sQuant[0][base + z] = (data[p] * sAanScales[i] + 0x800) >> 12;
                            p++;
                        }
                    }
                }
            } else if (marker == 0xdd) {
                if (pos + 3 >= sDataSize) {
                    return 0;
                }
                restartInterval = (data[pos + 2] << 8) | data[pos + 3];
                pos += len;
                restartCount = restartInterval + 1;
            } else {
                pos += len;
            }
        }
    }
    if (pos > sDataSize) {
        return 0;
    }
    return width | (height << 16);
}

// Decodes the coefficients of a block, in zigzag order, adding its DC to the prediction in coef[0]
static void func_ov257_021ab764(u8 *data, s16 *coef, s32 *pos, int tbl) {
    int size;
    int left;
    int k;

    size = func_ov257_021abd84(data, pos, tbl);
    coef[0] += (s16)func_ov257_021ab808(data, pos, 0, size);
    left = 63;
    k = 1;
    do {
        int rs = func_ov257_021abd84(data, pos, tbl + 1);
        int run;
        int i;

        if (rs == 0) {
            for (; k < 64; k++) {
                coef[k] = 0;
            }
            return;
        }
        run = rs >> 4;
        left -= run;
        if (k + run > 63) {
            return;
        }
        i = 0;
        if (run > 0) {
            for (; i < run; i++) {
                coef[k] = 0;
                k++;
            }
        }
        coef[k] = func_ov257_021ab808(data, pos, 0, rs & 0xf);
        k++;
    } while (--left > 0);
}

// Reads nbits bits after value, as a signed value whose sign is its first bit
static s32 func_ov257_021ab808(u8 *data, s32 *pos, s32 value, int nbits) {
    s32 negative;
    int i;

    if (nbits != 0) {
        negative = 0;
        if (*pos + 1 >= sDataSize) {
            return negative;
        }
        if (func_ov257_021abd3c(data, pos) == 0) {
            negative = (1 << nbits) - 1;
            nbits--;
        } else {
            value = (value << 1) | 1;
            nbits--;
        }
        for (i = 0; i < nbits; i++) {
            if (*pos + 1 >= sDataSize) {
                return 0;
            }
            value = (value << 1) | func_ov257_021abd3c(data, pos);
        }
        value -= negative;
    }
    return value;
}

// Puts a block's coefficients in natural order and transforms them into dest
static void func_ov257_021ab884(s16 *dest, int block) {
    int i, j, k;

    k = 0;
    j = 0;
    for (i = 0; i < 64; i++) {
        sBlocks[BLOCK_IDCT_IN][sZigzag[k]] = sBlocks[block][j];
        j++;
        k++;
    }
    if (block == BLOCK_Y) {
        func_ov257_021ab8dc(sBlocks[BLOCK_IDCT_IN], dest, 0);
    } else {
        func_ov257_021ab8dc(sBlocks[BLOCK_IDCT_IN], dest, 64);
    }
}

#define FIX_1_082392200 277
#define FIX_1_414213562 362
#define FIX_1_847759065 473
#define FIX_2_613125930 669
#define MULTIPLY(var, c) (((var) * (c)) >> 8)
#define RANGE_MASK 0x3ff
#define IDCT_RANGE_LIMIT (&sRangeLimit[0x180])
// The IDCT's work area, between its passes
#define IDCT_WS sBlocks[BLOCK_IDCT_WORK]

// IJG's fast integer IDCT (jidctfst.c), with the dequantization, into samples from 0 to 255
static void func_ov257_021ab8dc(const s16 *in, s16 *out, int quant) {
    int i;
    const s16 *quantPtr;
    const u8 *rangeLimit = IDCT_RANGE_LIMIT;
    s16 *outPtr;
    s32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    s32 tmp10, tmp11, tmp12, tmp13;
    s32 z5, z10, z11, z12, z13;

    i = 0;
    quantPtr = &sQuant[0][quant];
    for (; i < 8; i++) {
        if ((in[i + 8] | in[i + 16] | in[i + 24] | in[i + 32] | in[i + 40] | in[i + 48] | in[i + 56]) == 0) {
            s32 dc = in[i] * quantPtr[i];
            IDCT_WS[i] = IDCT_WS[i + 8] = IDCT_WS[i + 16] = IDCT_WS[i + 24] = IDCT_WS[i + 32] = IDCT_WS[i + 40] =
                IDCT_WS[i + 48] = IDCT_WS[i + 56] = dc;
            continue;
        }

        tmp0 = in[i] * quantPtr[i];
        tmp1 = in[i + 16] * quantPtr[i + 16];
        tmp2 = in[i + 32] * quantPtr[i + 32];
        tmp3 = in[i + 48] * quantPtr[i + 48];

        tmp10 = tmp0 + tmp2;
        tmp11 = tmp0 - tmp2;
        tmp13 = tmp1 + tmp3;
        tmp12 = MULTIPLY(tmp1 - tmp3, FIX_1_414213562) - tmp13;

        tmp0 = tmp10 + tmp13;
        tmp3 = tmp10 - tmp13;
        tmp1 = tmp11 + tmp12;
        tmp2 = tmp11 - tmp12;

        tmp4 = in[i + 8] * quantPtr[i + 8];
        tmp5 = in[i + 24] * quantPtr[i + 24];
        tmp6 = in[i + 40] * quantPtr[i + 40];
        tmp7 = in[i + 56] * quantPtr[i + 56];

        z13 = tmp6 + tmp5;
        z10 = tmp6 - tmp5;
        z11 = tmp4 + tmp7;
        z12 = tmp4 - tmp7;

        tmp7 = z11 + z13;
        tmp11 = MULTIPLY(z11 - z13, FIX_1_414213562);

        z5 = MULTIPLY(z10 + z12, FIX_1_847759065);
        tmp10 = MULTIPLY(z12, FIX_1_082392200) - z5;
        tmp12 = MULTIPLY(z10, -FIX_2_613125930) + z5;

        tmp6 = tmp12 - tmp7;
        tmp5 = tmp11 - tmp6;
        tmp4 = tmp5 + tmp10;

        IDCT_WS[i] = tmp0 + tmp7;
        IDCT_WS[i + 56] = tmp0 - tmp7;
        IDCT_WS[i + 8] = tmp1 + tmp6;
        IDCT_WS[i + 48] = tmp1 - tmp6;
        IDCT_WS[i + 16] = tmp2 + tmp5;
        IDCT_WS[i + 40] = tmp2 - tmp5;
        IDCT_WS[i + 32] = tmp3 + tmp4;
        IDCT_WS[i + 24] = tmp3 - tmp4;
    }

    for (i = 0; i < 8; i++) {
        if ((IDCT_WS[i * 8 + 1] | IDCT_WS[i * 8 + 2] | IDCT_WS[i * 8 + 3] | IDCT_WS[i * 8 + 4] | IDCT_WS[i * 8 + 5] |
             IDCT_WS[i * 8 + 6] | IDCT_WS[i * 8 + 7]) == 0) {
            outPtr = &out[i * 8];
            outPtr[0] = outPtr[1] = outPtr[2] = outPtr[3] = outPtr[4] = outPtr[5] = outPtr[6] = outPtr[7] =
                rangeLimit[(IDCT_WS[i * 8] >> 5) & RANGE_MASK];
            continue;
        }

        tmp10 = IDCT_WS[i * 8] + IDCT_WS[i * 8 + 4];
        tmp11 = IDCT_WS[i * 8] - IDCT_WS[i * 8 + 4];

        tmp13 = IDCT_WS[i * 8 + 2] + IDCT_WS[i * 8 + 6];
        tmp12 = MULTIPLY(IDCT_WS[i * 8 + 2] - IDCT_WS[i * 8 + 6], FIX_1_414213562) - tmp13;

        tmp0 = tmp10 + tmp13;
        tmp3 = tmp10 - tmp13;
        tmp1 = tmp11 + tmp12;
        tmp2 = tmp11 - tmp12;

        z13 = IDCT_WS[i * 8 + 5] + IDCT_WS[i * 8 + 3];
        z10 = IDCT_WS[i * 8 + 5] - IDCT_WS[i * 8 + 3];
        z11 = IDCT_WS[i * 8 + 1] + IDCT_WS[i * 8 + 7];
        z12 = IDCT_WS[i * 8 + 1] - IDCT_WS[i * 8 + 7];

        tmp7 = z11 + z13;
        tmp11 = MULTIPLY(z11 - z13, FIX_1_414213562);

        z5 = MULTIPLY(z10 + z12, FIX_1_847759065);
        tmp10 = MULTIPLY(z12, FIX_1_082392200) - z5;
        tmp12 = MULTIPLY(z10, -FIX_2_613125930) + z5;

        tmp6 = tmp12 - tmp7;
        tmp5 = tmp11 - tmp6;
        tmp4 = tmp5 + tmp10;

        outPtr = &out[i * 8];
        outPtr[0] = rangeLimit[((tmp0 + tmp7) >> 5) & RANGE_MASK];
        outPtr[7] = rangeLimit[((tmp0 - tmp7) >> 5) & RANGE_MASK];
        outPtr[1] = rangeLimit[((tmp1 + tmp6) >> 5) & RANGE_MASK];
        outPtr[6] = rangeLimit[((tmp1 - tmp6) >> 5) & RANGE_MASK];
        outPtr[2] = rangeLimit[((tmp2 + tmp5) >> 5) & RANGE_MASK];
        outPtr[5] = rangeLimit[((tmp2 - tmp5) >> 5) & RANGE_MASK];
        outPtr[4] = rangeLimit[((tmp3 + tmp4) >> 5) & RANGE_MASK];
        outPtr[3] = rangeLimit[((tmp3 - tmp4) >> 5) & RANGE_MASK];
    }
}

// Reads the next bit of the entropy-coded data, skipping the zero byte stuffed after each 0xff
static int func_ov257_021abd3c(u8 *data, s32 *pos) {
    if (sBitsLeft == 0) {
        sBitBuffer = data[(*pos)++];
        sStuffed = 0;
        if (sBitBuffer == 0xff) {
            (*pos)++;
            sBitBuffer = 0xff;
            sStuffed = 1;
        }
        sBitsLeft = 8;
    }
    sBitsLeft--;
    return (sBitBuffer >> sBitsLeft) & 1;
}

// Decodes a Huffman code with table tbl: through the lookahead tables for a code of up to 8 bits, otherwise bit by bit
static int func_ov257_021abd84(u8 *data, s32 *pos, int tbl) {
    s32 offset = sDerivedTblOffsets[tbl];
    s32 idx;
    s32 code;
    int nbits;
    int l;

    sDerivedTbl16 = (s16 *)&sHuffWork[offset];
    sDerivedTbl32 = (s32 *)&sHuffWork[offset];
    if (sBitsLeft < 8) {
        int c;

        sBitBuffer <<= 8;
        if (*pos + 1 >= sDataSize) {
            return 0;
        }
        c = data[(*pos)++];
        sStuffed = 0;
        if (c == 0xff) {
            (*pos)++;
            c = 0xff;
            sStuffed = 1;
        }
        sBitsLeft += 8;
        sBitBuffer |= c;
    }

    code = (u8)(sBitBuffer >> (sBitsLeft - 8));
    if (offset + HUFF_LOOK_NBITS + code < 0 || offset + HUFF_LOOK_NBITS + code >= HUFF_WORK_SIZE) {
        sError = 1;
        return 0;
    }
    nbits = sHuffWork[offset + HUFF_LOOK_NBITS + code];
    if (nbits != 0) {
        sBitsLeft -= nbits;
        if (offset + HUFF_LOOK_SYM + code < 0 || offset + HUFF_LOOK_SYM + code >= HUFF_WORK_SIZE) {
            sError = 1;
            return 0;
        }
        return sHuffWork[offset + HUFF_LOOK_SYM + code];
    }

    while (sBitsLeft < 9) {
        int c;

        sBitBuffer <<= 8;
        if (*pos + 1 >= sDataSize) {
            return 0;
        }
        c = data[(*pos)++];
        sStuffed = 0;
        if (c == 0xff) {
            (*pos)++;
            c = 0xff;
            sStuffed = 1;
        }
        sBitsLeft += 8;
        sBitBuffer |= c;
    }
    code = (sBitBuffer >> (sBitsLeft - 9)) & 0x1ff;
    sBitsLeft -= 9;
    l = 9;
    if (offset + HUFF_MAXCODE + 9 >= HUFF_WORK_SIZE) {
        sError = 1;
        return 0;
    }
    while (code > sDerivedTbl32[HUFF_MAXCODE + l]) {
        if (sBitsLeft == 0) {
            if (*pos + 1 >= sDataSize) {
                return 0;
            }
            sBitBuffer = data[(*pos)++];
            sStuffed = 0;
            if (sBitBuffer == 0xff) {
                (*pos)++;
                sBitBuffer = 0xff;
                sStuffed = 1;
            }
            sBitsLeft = 8;
        }
        code = (code << 1) | ((sBitBuffer >> (sBitsLeft - 1)) & 1);
        sBitsLeft--;
        l++;
        if (offset + HUFF_MAXCODE + l >= HUFF_WORK_SIZE) {
            sError = 1;
            return 0;
        }
    }

    if (l > 16 || offset + l < 0 || offset + l + HUFF_VALPTR >= HUFF_WORK_SIZE) {
        sError = 1;
        return 0;
    }
    // The values of each table follow its BITS in the work area
    switch (tbl) {
    case HUFF_TBL_DC_Y:
        idx = code + sDerivedTbl16[HUFF_VALPTR + l] - sDerivedTbl32[l] + HUFF_VALS;
        if (idx < 0 || idx >= HUFF_WORK_SIZE) {
            sError = 1;
            return 0;
        }
        code = sHuffWork[idx];
        break;
    case HUFF_TBL_AC_Y:
        idx = code + sDerivedTbl16[HUFF_VALPTR + l] - sDerivedTbl32[l] + HUFF_BITS_AC + HUFF_VALS;
        if (idx < 0 || idx >= HUFF_WORK_SIZE) {
            sError = 1;
            return 0;
        }
        code = sHuffWork[idx];
        break;
    case HUFF_TBL_DC_C:
        idx = code + sDerivedTbl16[HUFF_VALPTR + l] - sDerivedTbl32[l] + HUFF_BITS_C + HUFF_VALS;
        if (idx < 0 || idx >= HUFF_WORK_SIZE) {
            sError = 1;
            return 0;
        }
        code = sHuffWork[idx];
        break;
    case HUFF_TBL_AC_C:
        idx = code + sDerivedTbl16[HUFF_VALPTR + l] - sDerivedTbl32[l] + HUFF_BITS_C + HUFF_BITS_AC + HUFF_VALS;
        if (idx < 0 || idx >= HUFF_WORK_SIZE) {
            sError = 1;
            return 0;
        }
        code = sHuffWork[idx];
        break;
    }
    return code;
}
// IJG's sample_range_limit: the samples 0 to 255 clamped, reached with offsets from -256 to 639
#define SAMPLE_RANGE_LIMIT (&sRangeLimit[0x100])

// The RGB555 color of a pixel from its Y and the Cb and Cr at index c of the MCU, as IJG's ycc_rgb_convert, opaque
static inline int YccToRgb555(int y, int c) {
    const u8 *rangeLimit = SAMPLE_RANGE_LIMIT;
    int cr = sCr[c];
    int r = rangeLimit[y + sCrRTab[cr]] & 0xf8;
    int cb = sCb[c];
    int b = rangeLimit[y + sCbBTab[cb]] & 0xf8;
    int g = rangeLimit[y + ((sCbGTab[cb] + sCrGTab[cr]) >> 16)] & 0xf8;
    return (b << 7) | (g << 2) | (r >> 3) | 0x8000;
}

// Converts the decoded MCU at (x, y) into RGB555 pixels of dest, a bitmap width pixels wide, clipped to height
static void func_ov257_021ac018(u8 *dest, int x, int y, int width, int height) {
    int h, v, i, j;
    int k;
    int p;
    int c;
    int color;

    switch (sHSamp + sVSamp * 2) {
    case 3:
        k = 0;
        for (j = 0; j < 8; j++) {
            if (y + j >= height) {
                return;
            }
            p = ((y + j) * width + x) * 2;
            for (i = 0; i < 8; i++) {
                if (x + i >= width) {
                    break;
                }
                color = YccToRgb555(sY[0][k], k);
                dest[p] = color;
                dest[p + 1] = color >> 8;
                p += 2;

                k++;
            }
        }
        break;
    case 4:
        k = 0;
        for (h = 0; h < sHSamp; h++) {
            for (j = 0; j < 8; j++) {
                if (y + j >= height) {
                    break;
                }
                p = ((y + j) * width + x + h * 8) * 2;
                for (i = 0; i < 8; i++) {
                    if (x + h * 8 + i >= width) {
                        break;
                    }
                    c = (i >> 1) + h * 4 + j * 8;
                    color = YccToRgb555(sY[0][k], c);
                    dest[p] = color;
                    dest[p + 1] = color >> 8;
                    p += 2;

                    k++;
                }
            }
        }
        break;
    case 5:
        k = 0;
        for (v = 0; v < sVSamp; v++) {
            for (j = 0; j < 8; j++) {
                if (y + v * 8 + j >= height) {
                    break;
                }
                p = ((y + v * 8 + j) * width + x) * 2;
                for (i = 0; i < 8; i++) {
                    if (x + i >= width) {
                        break;
                    }
                    c = i + (j >> 1) * 8 + v * 32;
                    color = YccToRgb555(sY[0][k], c);
                    dest[p] = color;
                    dest[p + 1] = color >> 8;
                    p += 2;

                    k++;
                }
            }
        }
        break;
    case 6:
        k = 0;
        for (v = 0; v < sVSamp; v++) {
            for (h = 0; h < sHSamp; h++) {
                for (j = 0; j < 8; j++) {
                    if (y + v * 8 + j >= height) {
                        break;
                    }
                    p = ((y + v * 8 + j) * width + x + h * 8) * 2;
                    for (i = 0; i < 8; i++) {
                        if (x + h * 8 + i >= width) {
                            break;
                        }
                        c = (i >> 1) + h * 4 + (j >> 1) * 8 + v * 32;
                        color = YccToRgb555(sY[0][k], c);
                        dest[p] = color;
                        dest[p + 1] = color >> 8;
                        p += 2;

                        k++;
                    }
                }
            }
        }
        break;
    }
}

// The EXIF data the signature check reads is big-endian
#define READ_BE16(p) ((u16)((p)[1] + ((p)[0] << 8)))
#define READ_BE32(p) (((p)[0] << 24) + ((p)[1] << 16) + ((p)[2] << 8) + (p)[3])

BOOL SSP_StartJpegDecoder(u8 *data, u32 size, void *dst, s16 *width, s16 *height, u32 option) {
    u32 decodeOption;
    u32 result;

    if (data_ov257_021b6240 == TRUE && hw_isDSi() == TRUE) {
        u8 sig[SSP_SIGNATURE_SIZE];
        u32 pos, count, i;
        u16 tag;
        u8 *entry;
        u32 sigOffset;
        int signResult;

        if (size < 0x17) {
            return FALSE;
        }
        pos = data[0x13] + 12;
        if (pos + 2 > size) {
            return FALSE;
        }
        count = READ_BE16(&data[pos]);
        pos += 2;
        if (pos + count * 12 > size) {
            return FALSE;
        }
        tag = 0;
        for (i = 0; i < count; i++) {
            entry = &data[pos];
            tag = READ_BE16(entry);
            if (tag == 0x8769) {
                pos = READ_BE32(entry + 8) + 12;
                break;
            }
            pos += 12;
        }
        if (tag != 0x8769) {
            return FALSE;
        }
        if (pos + 2 > size) {
            return FALSE;
        }
        count = READ_BE16(&data[pos]);
        pos += 2;
        if (pos + count * 12 > size) {
            return FALSE;
        }
        tag = 0;
        for (i = 0; i < count; i++) {
            entry = &data[pos];
            tag = READ_BE16(entry);
            if (tag == 0x927c) {
                pos = READ_BE32(entry + 8) + 12;
                break;
            }
            pos += 12;
        }
        if (tag != 0x927c) {
            return FALSE;
        }
        if (pos + 4 > size) {
            return FALSE;
        }
        entry = &data[pos];
        if (entry[2] != 0x10 || entry[3] != 0) {
            return FALSE;
        }
        if (pos + 14 > size) {
            return FALSE;
        }
        sigOffset = READ_BE32(entry + 10) + 12;
        if (sigOffset + SSP_SIGNATURE_SIZE > size) {
            return FALSE;
        }
        sys_memcpy(data + sigOffset, sig, SSP_SIGNATURE_SIZE);
        sys_memset(data + sigOffset, 0, SSP_SIGNATURE_SIZE);
        if (hw_isDSi()) {
            PhotoSign_Init();
        }
        signResult = PhotoSign_Verify(sig, data, size);
        sys_memcpy(sig, data + sigOffset, SSP_SIGNATURE_SIZE);
        if (signResult != 1) {
            return FALSE;
        }
    }

    sError = 0;
    decodeOption = ((option & 1) ? 1 : 0) | ((option & 4) ? 4 : 0) | ((option & 0x10) ? 0x10 : 0);
    if (option & 0x10) {
        result = func_ov257_021aafdc(data, size, decodeOption);
    } else {
        result = func_ov257_021ab0a4(dst, data, size, *width, *height, decodeOption);
    }
    if (sError) {
        return FALSE;
    }
    if ((result & 0xffff0000) && (result & 0xffff)) {
        if (!(option & 0x10)) {
            *width = result;
            *height = (s32)result >> 16;
        }
        return TRUE;
    }
    return FALSE;
}
