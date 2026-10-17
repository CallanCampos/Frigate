// The melonDS test oracle (port/docs/PLAN.md §6, T4): runs the player's ROM on melonDS's emulation core, headless and
// deterministic, plays an input file, and writes the screens and named memory at chosen frames, for compare.py to
// hold the native port against. It is a test tool only: it is never linked into or shipped with the port (PLAN.md §1),
// and it is built only with -DPORT_BUILD_ORACLE=ON (port/cmake/oracle.cmake). README.md in this directory has the
// formats and how frames line up with the port's.
//
//   pbw-oracle --rom PATH --out DIR --frames N [--input FILE] [--screens LIST] [--dump SPEC]... [--config DIR]
//              [--symbols FILE]... [--counter TARGET] [--time "YYYY-MM-DD HH:MM:SS"] [--save FILE]
//              [--save-out FILE] [--verbose]
//
//   --rom PATH       the player's dump (orig/baserom_b2_us.nds)
//   --out DIR        where the run's files go (created; files of an earlier run there are replaced)
//   --frames N       how many frames to run
//   --input FILE     a "pbwinput 1" file; without one nothing is held. A file on game frames (the default) needs
//                    --counter, and a file's "version" must be the ROM's
//   --screens LIST   the frames to write as frame_NNNNNN.png (both screens, top above bottom): N, A-B, A-B/STEP,
//                    "all" or "none", comma-separated; "none" by default
//   --dump SPEC      [LABEL=]TARGET[:SIZE][@LIST]: memory to write to memory.txt at the frames of LIST (the last frame
//                    by default). TARGET is a symbol, MODULE/NAME or FILE.c/NAME, with an optional +OFFSET, or an
//                    address 0x...; SIZE defaults to the symbol's
//   --config DIR     the decomp's symbols: every symbols.txt under DIR (config/b2_us/arm9)
//   --symbols FILE   more symbols in the same format (symbols.py's statics)
//   --counter TARGET a 32-bit counter recorded for every frame in frames.tsv: the game's own frame count
//                    (sVBlankCount) for aligning frames
//   --time TIME      the real-time clock at power-on; else the input file's; else 2012-10-07 12:00:00
//   --save FILE      the save memory at power-on (read only); blank (0xFF) otherwise
//   --save-out FILE  writes the save memory there after the last frame
//   --verbose        melonDS's own informational log
//
// Exit statuses: 0 when every frame ran, 1 for an error writing the results, 2 for bad options (an input file for the
// other version among them), 3 for a ROM melonDS can't load, 4 when the console stopped before the last frame.
#include "oracle.h"
#include "Args.h"
#include "NDS.h"
#include "NDSCart.h"
#include "SPI_Firmware.h"
#include "display3d.h"
#include "pbwinput.h"
#include "pngout.h"
#include "sha1/sha1.hpp"
#include "symtab.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace oracle {

namespace {

// The pinned melonDS release (port/tools/oracle/CMakeLists.txt passes it)
#ifndef ORACLE_MELONDS_VERSION
#define ORACLE_MELONDS_VERSION "unknown"
#endif

constexpr uint32_t SCREEN_WIDTH = 256;
constexpr uint32_t SCREEN_HEIGHT = 192;
constexpr uint32_t SCREEN_PIXELS = SCREEN_WIDTH * SCREEN_HEIGHT;
// A screen is blank when all but this many of its pixels have one colour (a line's worth, so that a stray line doesn't
// count as a picture)
constexpr uint32_t BLANK_SLACK = SCREEN_WIDTH;
// The LCD's timing in melonDS's system clock (33.514 MHz): 2,130 cycles a line, VBlank at line 192
constexpr uint64_t LINE_CYCLES = 355 * 6;
constexpr uint64_t VBLANK_LINE = 192;
// The line where the oracle looks at the capture unit: between line 0, where the unit latches DISPCAPCNT's enable,
// and line 192, where it ends the capture and clears the enable
constexpr uint64_t PROBE_LINE = 96;
// The scheduler slot of the oracle's events, the probe and then the VBlank: a DSi-only device's (the DSP's HLE
// microcode), which a DS-mode console never registers
constexpr uint32_t ORACLE_EVENT = melonDS::Event_DSi_DSPHLE;
constexpr uint32_t ORACLE_EVENT_VBLANK = 0;
constexpr uint32_t ORACLE_EVENT_PROBE = 1;
// NitroSDK's VBlank count, which the ARM7 keeps in the shared memory (HW_VBLANK_COUNT_BUF)
constexpr uint32_t VBLANK_COUNT_ADDRESS = 0x02fffc3c;
// The extended keys' word in melonDS's KeyInput (EXTKEYIN << 16): the debug button is EXTKEYIN's bit 3
constexpr uint32_t KEYINPUT_DEBUG_BIT = 1u << (16 + 3);
// The ROM header's game code (offset 0x0c) of each version an input file can name
constexpr uint32_t HEADER_GAME_CODE = 0x0c;
struct VersionCode {
    const char *version;
    const char *gameCode;
};
constexpr VersionCode VERSION_CODES[] = { { "b2_us", "IREO" }, { "w2_us", "IRDO" } };

const char *const DEFAULT_TIME = PBW_INPUT_DEFAULT_TIME;

int sLogThreshold = melonDS::Platform::LogLevel::Warn;

void Fail(const char *format, ...) {
    va_list args;
    std::fprintf(stderr, "pbw-oracle: ");
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);
    std::fprintf(stderr, "\n");
}

// A set of frames from a list "1,30,60-90,100-200/10,all,none"
class FrameSet {
  public:
    bool Parse(const std::string &text, std::string &error) {
        size_t at = 0;

        while (at <= text.size()) {
            size_t comma = text.find(',', at);
            std::string item = text.substr(at, comma == std::string::npos ? std::string::npos : comma - at);
            at = comma == std::string::npos ? text.size() + 1 : comma + 1;
            if (item == "all") {
                all_ = true;
                continue;
            }
            if (item == "none") {
                continue;
            }
            Range range;
            if (!ParseRange(item, range)) {
                error = "\"" + item + "\" isn't N, A-B, A-B/STEP, all or none";
                return false;
            }
            ranges_.push_back(range);
        }
        return true;
    }

    void AddLast() {
        last_ = true;
    }

    bool Contains(uint32_t frame, uint32_t lastFrame) const {
        if (all_ || (last_ && frame == lastFrame)) {
            return true;
        }
        for (const Range &range : ranges_) {
            if (frame >= range.first && frame <= range.last && (frame - range.first) % range.step == 0) {
                return true;
            }
        }
        return false;
    }

    bool Empty() const {
        return !all_ && !last_ && ranges_.empty();
    }

  private:
    struct Range {
        uint32_t first = 0;
        uint32_t last = 0;
        uint32_t step = 1;
    };

    static bool ParseNumber(const std::string &text, uint32_t &out) {
        if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos || text.size() > 9) {
            return false;
        }
        out = static_cast<uint32_t>(std::strtoul(text.c_str(), nullptr, 10));
        return true;
    }

    static bool ParseRange(const std::string &item, Range &range) {
        size_t slash = item.find('/');
        std::string span = item.substr(0, slash);
        size_t dash = span.find('-');

        if (slash != std::string::npos && (!ParseNumber(item.substr(slash + 1), range.step) || range.step == 0)) {
            return false;
        }
        if (dash == std::string::npos) {
            if (!ParseNumber(span, range.first) || slash != std::string::npos) {
                return false;
            }
            range.last = range.first;
        } else if (!ParseNumber(span.substr(0, dash), range.first) || !ParseNumber(span.substr(dash + 1), range.last)) {
            return false;
        }
        return range.first >= 1 && range.first <= range.last;
    }

    std::vector<Range> ranges_;
    bool all_ = false;
    bool last_ = false;
};

struct Dump {
    std::string label;
    std::string target;
    uint32_t address = 0;
    uint32_t size = 0;
    FrameSet frames;
    std::vector<uint8_t> bytes; // the bytes taken at this frame's VBlank
    bool taken = false;
};

struct Options {
    std::string rom;
    std::string out;
    uint32_t frames = 0;
    std::string input;
    std::string screens = "none";
    std::vector<std::string> dumps;
    std::string config;
    std::vector<std::string> symbols;
    std::string counter;
    std::string time;
    std::string save;
    std::string saveOut;
    bool verbose = false;
};

void Usage() {
    std::fprintf(stderr,
                 "usage: pbw-oracle --rom PATH --out DIR --frames N [--input FILE] [--screens LIST] [--dump SPEC]...\n"
                 "                  [--config DIR] [--symbols FILE]... [--counter TARGET]\n"
                 "                  [--time \"YYYY-MM-DD HH:MM:SS\"] [--save FILE] [--save-out FILE] [--verbose]\n");
}

bool ParseOptions(int argc, char **argv, Options &options) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        std::string value;
        size_t equals = arg.find('=');

        if (arg.rfind("--", 0) != 0) {
            Fail("unexpected argument %s", arg.c_str());
            return false;
        }
        if (arg == "--verbose") {
            options.verbose = true;
            continue;
        }
        if (arg == "--help") {
            Usage();
            std::exit(0);
        }
        if (equals != std::string::npos) {
            value = arg.substr(equals + 1);
            arg = arg.substr(0, equals);
        } else if (i + 1 < argc) {
            value = argv[++i];
        } else {
            Fail("%s needs a value", arg.c_str());
            return false;
        }
        if (arg == "--rom") {
            options.rom = value;
        } else if (arg == "--out") {
            options.out = value;
        } else if (arg == "--frames") {
            char *end = nullptr;
            unsigned long frames = std::strtoul(value.c_str(), &end, 10);
            if (value.empty() || *end != '\0' || frames == 0 || frames > PBW_INPUT_MAX_FRAME) {
                Fail("--frames needs a number from 1 to %u, not \"%s\"", PBW_INPUT_MAX_FRAME, value.c_str());
                return false;
            }
            options.frames = static_cast<uint32_t>(frames);
        } else if (arg == "--input") {
            options.input = value;
        } else if (arg == "--screens") {
            options.screens = value;
        } else if (arg == "--dump") {
            options.dumps.push_back(value);
        } else if (arg == "--config") {
            options.config = value;
        } else if (arg == "--symbols") {
            options.symbols.push_back(value);
        } else if (arg == "--counter") {
            options.counter = value;
        } else if (arg == "--time") {
            options.time = value;
        } else if (arg == "--save") {
            options.save = value;
        } else if (arg == "--save-out") {
            options.saveOut = value;
        } else {
            Fail("unknown option %s", arg.c_str());
            return false;
        }
    }
    if (options.rom.empty() || options.out.empty() || options.frames == 0) {
        Fail("--rom, --out and --frames are needed");
        return false;
    }
    return true;
}

bool ParseNumber(const std::string &text, uint32_t &out) {
    char *end = nullptr;
    unsigned long long value;

    if (text.empty()) {
        return false;
    }
    value = std::strtoull(text.c_str(), &end, text.rfind("0x", 0) == 0 || text.rfind("0X", 0) == 0 ? 16 : 10);
    if (*end != '\0' || value > 0xffffffffull) {
        return false;
    }
    out = static_cast<uint32_t>(value);
    return true;
}

// Where a dump may read: the TCMs, main RAM, the shared WRAM, palettes, VRAM and OAM; never I/O, whose reads act
bool Readable(uint32_t address, uint32_t size) {
    uint32_t end = address + size - 1;

    if (size == 0 || end < address || (address >> 24) != (end >> 24)) {
        return false;
    }
    switch (address >> 24) {
    case 0x01: // the ITCM's mirrors
    case 0x02:
    case 0x03:
    case 0x05:
    case 0x06:
    case 0x07:
        return true;
    default:
        return false;
    }
}

bool ResolveTarget(const std::string &target, const SymbolTable &symbols, uint32_t &address, uint32_t &size,
                   std::string &error) {
    std::string name = target;
    uint32_t offset = 0;
    size_t plus = target.find('+');

    if (target.rfind("0x", 0) == 0) {
        size = 4;
        if (!ParseNumber(target, address)) {
            error = "\"" + target + "\" isn't an address";
            return false;
        }
        return true;
    }
    if (plus != std::string::npos) {
        name = target.substr(0, plus);
        if (!ParseNumber(target.substr(plus + 1), offset)) {
            error = "\"" + target.substr(plus + 1) + "\" isn't an offset";
            return false;
        }
    }
    const Symbol *symbol = symbols.Find(name, error);
    if (symbol == nullptr) {
        return false;
    }
    address = symbol->address + offset;
    size = offset < symbol->size ? symbol->size - offset : 4;
    return true;
}

bool ParseDump(const std::string &spec, const SymbolTable &symbols, Dump &dump, std::string &error) {
    std::string rest = spec;
    std::string frames;
    std::string size;
    size_t at = rest.rfind('@');

    if (at != std::string::npos) {
        frames = rest.substr(at + 1);
        rest = rest.substr(0, at);
    }
    at = rest.find('=');
    if (at != std::string::npos) {
        dump.label = rest.substr(0, at);
        rest = rest.substr(at + 1);
    }
    at = rest.rfind(':');
    if (at != std::string::npos) {
        size = rest.substr(at + 1);
        rest = rest.substr(0, at);
    }
    dump.target = rest;
    if (dump.label.empty()) {
        dump.label = rest;
    }
    if (dump.label.find_first_of(" \t\n") != std::string::npos) {
        error = "a dump's label can't have spaces";
        return false;
    }
    if (!ResolveTarget(rest, symbols, dump.address, dump.size, error)) {
        return false;
    }
    if (!size.empty() && (!ParseNumber(size, dump.size) || dump.size == 0)) {
        error = "\"" + size + "\" isn't a size";
        return false;
    }
    if (!Readable(dump.address, dump.size)) {
        char text[96];
        std::snprintf(text, sizeof(text), "0x%08x+0x%x is outside the memory a dump can read", dump.address, dump.size);
        error = text;
        return false;
    }
    if (frames.empty()) {
        dump.frames.AddLast();
    } else if (!dump.frames.Parse(frames, error)) {
        return false;
    }
    return true;
}

bool ReadFile(const std::string &path, std::vector<uint8_t> &out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return !in.bad();
}

std::string Sha1Hex(const uint8_t *data, size_t length) {
    SHA1_CTX context;
    unsigned char digest[20];
    char hex[41];

    SHA1Init(&context);
    SHA1Update(&context, data, static_cast<uint32_t>(length));
    SHA1Final(digest, &context);
    for (int i = 0; i < 20; i++) {
        std::snprintf(hex + i * 2, 3, "%02x", digest[i]);
    }
    return hex;
}

// The firmware's user settings as the port's (port/hal/nvram/nvram7.c) has them, so that both run with the same owner,
// language and MAC address: melonDS's generated firmware with those fields changed. The touch calibration stays
// melonDS's (it sets its own at each reset)
melonDS::Firmware MakeFirmware() {
    static const char16_t nickname[] = u"Player";
    static const melonDS::MacAddress mac = { 0x02, 0x42, 0x57, 0x32, 0x00, 0x01 };
    melonDS::Firmware firmware(0);

    for (auto &user : firmware.GetUserData()) {
        user.Version = 5;
        user.FavoriteColor = 11;
        user.BirthdayMonth = 6;
        user.BirthdayDay = 23;
        std::memset(user.Nickname, 0, sizeof(user.Nickname));
        std::memcpy(user.Nickname, nickname, (sizeof(nickname) / sizeof(nickname[0]) - 1) * sizeof(char16_t));
        user.NameLength = sizeof(nickname) / sizeof(nickname[0]) - 1;
        std::memset(user.Message, 0, sizeof(user.Message));
        user.MessageLength = 0;
        // English, and the flags of the settings the owner completed (nvram7.c's USER_SETTINGS_FLAGS_DONE)
        user.Settings = static_cast<melonDS::u16>(0xec00 | melonDS::Firmware::Language::English);
        user.Year = 12;
        user.RTCOffset = 0;
    }
    firmware.GetHeader().MacAddr = mac;
    firmware.UpdateChecksums();
    return firmware;
}

} // namespace

// One run: the console, the input and the outputs
class Oracle {
  public:
    explicit Oracle(Options options) : options_(std::move(options)) {
    }

    int Run();

    void OnVBlank(uint32_t frame);

    void OnStop(int reason) {
        stopped_ = true;
        stopReason_ = reason;
    }

    void OnSave() {
        saveWritten_ = true;
    }

  private:
    bool Prepare();
    bool Boot();
    void ApplyInput(uint32_t frame);
    void OnProbe(uint32_t frame);
    void SampleDisplay();
    uint8_t Peek(uint32_t address);
    uint32_t Peek32(uint32_t address);
    bool WriteFrame(uint32_t frame);

    Options options_;
    SymbolTable symbols_;
    PbwInput input_{};
    bool haveInput_ = false;
    PbwGameClock clock_{};
    FrameSet screens_;
    std::vector<Dump> dumps_;
    bool haveCounter_ = false;
    uint32_t counterAddress_ = 0;
    std::string time_;
    std::vector<uint8_t> rom_;
    std::vector<uint8_t> save_;
    std::unique_ptr<melonDS::NDS> nds_;

    // What the VBlank event took for the current frame
    bool vblankSeen_ = false;
    uint32_t vblankCount_ = 0;
    uint32_t counter_ = 0;
    uint32_t gameFrame_ = 1; // the game frame whose input the game reads next (game_frame in frames.tsv)
    unsigned screens3d_ = 0; // PBW_DISPLAY_TOP and PBW_DISPLAY_BOTTOM: the screens that showed 3D
    bool warnedLine_ = false;

    // The 3D in the VRAM banks (display3d.h): as the frame started, and the frame's capture as the probe saw it
    PbwBanks3d banks_{};
    uint8_t holds3dAtStart_[PBW_DISPLAY_BANKS] = {};
    PbwCapture capture_{};

    bool stopped_ = false;
    int stopReason_ = 0;
    bool saveWritten_ = false;

    FILE *framesTsv_ = nullptr;
    FILE *memoryTxt_ = nullptr;
    std::vector<uint8_t> rgb_;
};

int LogThreshold() {
    return sLogThreshold;
}

void OnEmulatorStop(int reason, void *userdata) {
    static_cast<Oracle *>(userdata)->OnStop(reason);
}

void OnSaveWritten(void *userdata) {
    static_cast<Oracle *>(userdata)->OnSave();
}

bool Oracle::Prepare() {
    std::string error;

    if (!options_.config.empty() && !symbols_.LoadConfig(options_.config, error)) {
        Fail("%s", error.c_str());
        return false;
    }
    for (const std::string &file : options_.symbols) {
        if (!symbols_.LoadFile(file, "main", error)) {
            Fail("%s", error.c_str());
            return false;
        }
    }
    symbols_.Finish();

    if (!screens_.Parse(options_.screens, error)) {
        Fail("--screens: %s", error.c_str());
        return false;
    }
    for (const std::string &spec : options_.dumps) {
        Dump dump;
        if (!ParseDump(spec, symbols_, dump, error)) {
            Fail("--dump %s: %s", spec.c_str(), error.c_str());
            return false;
        }
        dumps_.push_back(std::move(dump));
    }
    if (!options_.counter.empty()) {
        uint32_t size;
        if (!ResolveTarget(options_.counter, symbols_, counterAddress_, size, error) || !Readable(counterAddress_, 4)) {
            Fail("--counter %s: %s", options_.counter.c_str(), error.empty() ? "unreadable" : error.c_str());
            return false;
        }
        haveCounter_ = true;
    }

    if (!options_.input.empty()) {
        char message[256];
        if (!pbw_input_load(&input_, options_.input.c_str(), message, sizeof(message))) {
            Fail("--input %s: %s", options_.input.c_str(), message);
            return false;
        }
        haveInput_ = true;
        if (input_.frames == PBW_INPUT_GAME_FRAMES && !haveCounter_) {
            Fail("--input %s counts game frames, which need --counter (GFL's frame count, sVBlankCount); or give it a "
                 "\"frames video\" line",
                 options_.input.c_str());
            return false;
        }
    }
    time_ = !options_.time.empty() ? options_.time : haveInput_ && input_.time[0] != '\0' ? input_.time : DEFAULT_TIME;
    if (!pbw_input_valid_time(time_.c_str())) {
        Fail("the time must be \"YYYY-MM-DD HH:MM:SS\" from 2000 to 2099, not \"%s\"", time_.c_str());
        return false;
    }

    if (!ReadFile(options_.rom, rom_) || rom_.size() < 0x200) {
        Fail("can't read the ROM %s", options_.rom.c_str());
        return false;
    }
    if (haveInput_ && input_.version[0] != '\0') {
        const char *gameCode = "";
        for (const VersionCode &code : VERSION_CODES) {
            if (std::strcmp(code.version, input_.version) == 0) {
                gameCode = code.gameCode;
            }
        }
        if (std::memcmp(rom_.data() + HEADER_GAME_CODE, gameCode, 4) != 0) {
            Fail("--input %s is for %s (game code %s), and the ROM's game code is %.4s", options_.input.c_str(),
                 input_.version, gameCode, reinterpret_cast<const char *>(rom_.data() + HEADER_GAME_CODE));
            return false;
        }
    }
    if (!options_.save.empty() && (!ReadFile(options_.save, save_) || save_.empty())) {
        Fail("can't read the save %s", options_.save.c_str());
        return false;
    }

    std::error_code code;
    std::filesystem::create_directories(options_.out, code);
    framesTsv_ = std::fopen((options_.out + "/frames.tsv").c_str(), "w");
    memoryTxt_ = std::fopen((options_.out + "/memory.txt").c_str(), "w");
    if (framesTsv_ == nullptr || memoryTxt_ == nullptr) {
        Fail("can't write in %s", options_.out.c_str());
        return false;
    }
    std::fprintf(framesTsv_, "#pbwframes 2\n");
    std::fprintf(framesTsv_, "frame\tvblank\tcounter\tgame_frame\tkeyread\ttop_3d\tbottom_3d\ttop\tbottom\ttop_blank\t"
                             "bottom_blank\n");
    std::fprintf(memoryTxt_, "#pbwmem 1\n");
    rgb_.resize(SCREEN_PIXELS * 2 * 3);
    return true;
}

bool Oracle::Boot() {
    melonDS::NDSArgs args;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;

    // FreeBIOS (the defaults) and direct boot: no Nintendo BIOS or firmware file; no JIT, so that every run executes
    // the same instructions in the same order
    args.Firmware = MakeFirmware();
    args.JIT = std::nullopt;
    nds_ = std::make_unique<melonDS::NDS>(std::move(args), this);
    if (nds_->ConsoleType != 0) {
        Fail("melonDS made a DSi; the oracle needs a DS");
        return false;
    }

    melonDS::NDSCart::NDSCartArgs cartArgs{};
    if (!save_.empty()) {
        cartArgs.SRAM = std::make_unique<melonDS::u8[]>(save_.size());
        std::memcpy(cartArgs.SRAM.get(), save_.data(), save_.size());
        cartArgs.SRAMLength = static_cast<melonDS::u32>(save_.size());
    }
    auto cart =
        melonDS::NDSCart::ParseROM(rom_.data(), static_cast<melonDS::u32>(rom_.size()), this, std::move(cartArgs));
    if (cart == nullptr) {
        Fail("melonDS can't load %s as a DS ROM", options_.rom.c_str());
        return false;
    }
    nds_->SetNDSCart(std::move(cart));
    nds_->Reset();
    if (!nds_->NeedsDirectBoot()) {
        Fail("melonDS would boot through the firmware; the oracle expects FreeBIOS and direct boot");
        return false;
    }
    nds_->SetupDirectBoot("rom.nds");

    std::sscanf(time_.c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
    nds_->RTC.SetDateTime(year, month, day, hour, minute, second);

    // In the order of ORACLE_EVENT_VBLANK and ORACLE_EVENT_PROBE
    nds_->RegisterEventFuncs(ORACLE_EVENT, this,
                             { [](void *that, melonDS::u32 frame) { static_cast<Oracle *>(that)->OnVBlank(frame); },
                               [](void *that, melonDS::u32 frame) { static_cast<Oracle *>(that)->OnProbe(frame); } });
    pbw_game_clock_init(&clock_);
    pbw_display3d_init(&banks_);
    nds_->Start();
    return true;
}

// Holds the state of the input file's frame, on the file's count, until the next call
void Oracle::ApplyInput(uint32_t frame) {
    PbwInputState state = haveInput_ ? pbw_input_state(&input_, frame) : PbwInputState{};
    // melonDS's mask has a bit set for each button released: KEYINPUT's ten, then X and Y
    melonDS::u32 released = ~state.buttons & 0xfffu;

    nds_->SetKeyMask(released);
    if (state.buttons & PBW_INPUT_DEBUG) {
        nds_->KeyInput &= ~KEYINPUT_DEBUG_BIT;
    } else {
        nds_->KeyInput |= KEYINPUT_DEBUG_BIT;
    }
    if (state.touching) {
        nds_->TouchScreen(state.touchX, state.touchY);
    } else {
        nds_->ReleaseScreen();
    }
    if (nds_->IsLidClosed() != (state.lidClosed != 0)) {
        nds_->SetLidClosed(state.lidClosed != 0);
    }
}

// The ARM9's view of memory, without the bus's side effects: the ITCM and DTCM where CP15 maps them, else main RAM,
// and the shared WRAM, palettes, VRAM and OAM through the ARM9's reads (Readable() keeps I/O out)
uint8_t Oracle::Peek(uint32_t address) {
    melonDS::ARMv5 &arm9 = nds_->ARM9;

    if (address < arm9.ITCMSize) {
        return arm9.ITCM[address & (melonDS::ITCMPhysicalSize - 1)];
    }
    if ((address & arm9.DTCMMask) == arm9.DTCMBase) {
        return arm9.DTCM[address & (melonDS::DTCMPhysicalSize - 1)];
    }
    if ((address >> 24) == 0x02) {
        return nds_->MainRAM[address & nds_->MainRAMMask];
    }
    return nds_->ARM9Read8(address);
}

uint32_t Oracle::Peek32(uint32_t address) {
    return static_cast<uint32_t>(Peek(address)) | static_cast<uint32_t>(Peek(address + 1)) << 8 |
           static_cast<uint32_t>(Peek(address + 2)) << 16 | static_cast<uint32_t>(Peek(address + 3)) << 24;
}

// Mid-frame, while the capture unit has DISPCAPCNT's enable latched if it captures this frame: what it captures and
// where. Schedules the VBlank event
void Oracle::OnProbe(uint32_t frame) {
    const melonDS::GPU2D::Unit &engineA = nds_->GPU.GPU2D_A;

    capture_.ran = engineA.CaptureLatch ? 1 : 0;
    capture_.dispcapcnt = engineA.CaptureCnt;
    capture_.dispcntA = engineA.DispCnt;
    capture_.destInLcdc = (nds_->GPU.VRAMMap_LCDC >> ((engineA.CaptureCnt >> 16) & 3)) & 1;
    nds_->ScheduleEvent(ORACLE_EVENT, true, static_cast<melonDS::s32>((VBLANK_LINE - PROBE_LINE) * LINE_CYCLES),
                        ORACLE_EVENT_VBLANK, frame);
}

// The screens that showed 3D in the frame just drawn, from the registers it was drawn with, and the frame's capture
void Oracle::SampleDisplay() {
    const melonDS::GPU &gpu = nds_->GPU;
    PbwDisplayRegs regs{};

    regs.dispcnt[0] = gpu.GPU2D_A.DispCnt;
    regs.dispcnt[1] = gpu.GPU2D_B.DispCnt;
    for (int bg = 0; bg < 4; bg++) {
        regs.bgcnt[0][bg] = gpu.GPU2D_A.BGCnt[bg];
        regs.bgcnt[1][bg] = gpu.GPU2D_B.BGCnt[bg];
    }
    regs.powcnt1 = nds_->PowerControl9;
    for (int bank = 0; bank < PBW_DISPLAY_BANKS; bank++) {
        regs.vramcnt[bank] = gpu.VRAMCNT[bank];
    }
    screens3d_ = pbw_display3d_screens(&regs, holds3dAtStart_);
    pbw_display3d_capture(&banks_, &capture_, holds3dAtStart_, gpu.VRAM);
}

// At the start of line 192 of each frame, after the LCD's own event (which raises the VBlank interrupt) and before
// either CPU takes it: the frame's 192 lines are drawn and the game waits for the VBlank, as at the port's frame hook
void Oracle::OnVBlank(uint32_t frame) {
    uint32_t inputFrame;

    if (nds_->GPU.VCount != VBLANK_LINE && !warnedLine_) {
        Fail("warning: frame %u's VBlank event came on line %u", frame, nds_->GPU.VCount);
        warnedLine_ = true;
    }
    vblankSeen_ = true;
    vblankCount_ = Peek32(VBLANK_COUNT_ADDRESS);
    counter_ = haveCounter_ ? Peek32(counterAddress_) : 0;
    SampleDisplay();
    for (Dump &dump : dumps_) {
        dump.taken = dump.frames.Contains(frame, options_.frames);
        if (dump.taken) {
            dump.bytes.resize(dump.size);
            for (uint32_t i = 0; i < dump.size; i++) {
                dump.bytes[i] = Peek(dump.address + i);
            }
        }
    }
    // The state the game reads after this VBlank: the file's frame on its count, game frames from GFL's frame count
    inputFrame = pbw_input_frame_at_vblank(&input_, &clock_, frame, counter_);
    gameFrame_ = clock_.frames + 1;
    ApplyInput(inputFrame);
}

bool Oracle::WriteFrame(uint32_t frame) {
    const int front = nds_->GPU.FrontBuffer;
    const melonDS::u32 *screens[2] = { nds_->GPU.Framebuffer[front][0].get(), nds_->GPU.Framebuffer[front][1].get() };
    std::string hashes[2];
    bool blank[2];

    // 0xAARRGGBB, each channel the DS's 6 bits widened to 8 ((c << 2) | (c >> 4)), as the port's renderer gives them
    for (int screen = 0; screen < 2; screen++) {
        const melonDS::u32 *pixels = screens[screen];
        uint8_t *out = rgb_.data() + static_cast<size_t>(screen) * SCREEN_PIXELS * 3;
        melonDS::u32 candidate = 0;
        uint32_t votes = 0;
        uint32_t same = 0;

        for (uint32_t i = 0; i < SCREEN_PIXELS; i++) {
            melonDS::u32 pixel = pixels[i] & 0xffffffu;
            out[i * 3] = static_cast<uint8_t>(pixel >> 16);
            out[i * 3 + 1] = static_cast<uint8_t>(pixel >> 8);
            out[i * 3 + 2] = static_cast<uint8_t>(pixel);
            // The colour most of the screen has, if one has more than half of it (Boyer and Moore's majority vote)
            if (votes == 0) {
                candidate = pixel;
                votes = 1;
            } else if (pixel == candidate) {
                votes++;
            } else {
                votes--;
            }
        }
        for (uint32_t i = 0; i < SCREEN_PIXELS; i++) {
            same += (pixels[i] & 0xffffffu) == candidate;
        }
        blank[screen] = same + BLANK_SLACK >= SCREEN_PIXELS;
        hashes[screen] = Sha1Hex(out, SCREEN_PIXELS * 3);
    }
    std::fprintf(framesTsv_, "%u\t", frame);
    if (vblankSeen_) {
        std::fprintf(framesTsv_, "%u\t", vblankCount_);
    } else {
        // A frame without a VBlank (the console asleep)
        std::fprintf(framesTsv_, "-\t");
    }
    if (haveCounter_ && vblankSeen_) {
        std::fprintf(framesTsv_, "%u\t", counter_);
    } else {
        std::fprintf(framesTsv_, "-\t");
    }
    if (haveCounter_) {
        std::fprintf(framesTsv_, "%u\t", gameFrame_);
    } else {
        std::fprintf(framesTsv_, "-\t");
    }
    std::fprintf(framesTsv_, "%d\t", nds_->LagFrameFlag ? 0 : 1);
    if (vblankSeen_) {
        std::fprintf(framesTsv_, "%d\t%d\t", (screens3d_ & PBW_DISPLAY_TOP) ? 1 : 0,
                     (screens3d_ & PBW_DISPLAY_BOTTOM) ? 1 : 0);
    } else {
        std::fprintf(framesTsv_, "-\t-\t");
    }
    std::fprintf(framesTsv_, "%s\t%s\t%d\t%d\n", hashes[0].c_str(), hashes[1].c_str(), blank[0] ? 1 : 0,
                 blank[1] ? 1 : 0);

    if (screens_.Contains(frame, options_.frames)) {
        char name[32];
        std::string error;
        std::snprintf(name, sizeof(name), "/frame_%06u.png", frame);
        if (!WritePng(options_.out + name, rgb_.data(), SCREEN_WIDTH, SCREEN_HEIGHT * 2, error)) {
            Fail("%s", error.c_str());
            return false;
        }
    }
    for (Dump &dump : dumps_) {
        if (!dump.taken || !vblankSeen_) {
            continue;
        }
        std::fprintf(memoryTxt_, "%u %s 0x%08x %u ", frame, dump.label.c_str(), dump.address, dump.size);
        for (uint8_t byte : dump.bytes) {
            std::fprintf(memoryTxt_, "%02x", byte);
        }
        std::fprintf(memoryTxt_, "\n");
        dump.taken = false;
    }
    return !std::ferror(framesTsv_) && !std::ferror(memoryTxt_);
}

int Oracle::Run() {
    uint32_t frame;
    int status = 0;

    sLogThreshold = options_.verbose ? melonDS::Platform::LogLevel::Info : melonDS::Platform::LogLevel::Warn;
    if (!Prepare()) {
        return 2;
    }
    if (!Boot()) {
        return 3;
    }

    // The state of frame 1 holds from power-on
    ApplyInput(1);
    for (frame = 1; frame <= options_.frames; frame++) {
        // The LCD's last event was the end of the previous frame, which is where this one's line 0 starts
        melonDS::u64 lineZero = nds_->SchedList[melonDS::Event_LCD].Timestamp;

        // What the banks hold as the frame starts: a captured region that the CPU or a DMA wrote since holds no 3D
        pbw_display3d_check(&banks_, nds_->GPU.VRAM);
        std::memcpy(holds3dAtStart_, banks_.holds3d, sizeof(holds3dAtStart_));
        capture_ = PbwCapture{};
        vblankSeen_ = false;
        nds_->SchedList[ORACLE_EVENT].Timestamp = lineZero + PROBE_LINE * LINE_CYCLES;
        nds_->ScheduleEvent(ORACLE_EVENT, true, 0, ORACLE_EVENT_PROBE, frame);
        nds_->RunFrame();
        if (!vblankSeen_) {
            // Asleep: the frame counts as a game frame, so that a later line can open the lid
            nds_->CancelEvent(ORACLE_EVENT);
            ApplyInput(pbw_input_frame_asleep(&input_, &clock_, frame));
            gameFrame_ = clock_.frames + 1;
        }
        if (!WriteFrame(frame)) {
            status = 1;
            break;
        }
        if (stopped_) {
            Fail("the console stopped (melonDS reason %d) at frame %u", stopReason_, frame);
            status = 4;
            break;
        }
    }

    std::FILE *run = std::fopen((options_.out + "/run.txt").c_str(), "w");
    if (run != nullptr) {
        const melonDS::NDSHeader &header = nds_->NDSCartSlot.GetCart()->GetHeader();
        std::fprintf(run, "#pbwrun 1\n");
        std::fprintf(run, "program: pbw-oracle\n");
        std::fprintf(run, "melonds: %s\n", ORACLE_MELONDS_VERSION);
        std::fprintf(run, "rom: %s\n", options_.rom.c_str());
        std::fprintf(run, "gamecode: %.4s\n", header.GameCode);
        std::fprintf(run, "frames: %u\n", options_.frames);
        std::fprintf(run, "completed: %u\n", frame > options_.frames ? options_.frames : frame);
        std::fprintf(run, "time: %s\n", time_.c_str());
        std::fprintf(run, "input: %s\n", options_.input.empty() ? "-" : options_.input.c_str());
        std::fprintf(run, "input_frames: %s\n",
                     !haveInput_                               ? "-"
                     : input_.frames == PBW_INPUT_VIDEO_FRAMES ? "video"
                                                               : "game");
        std::fprintf(run, "counter: %s\n", options_.counter.empty() ? "-" : options_.counter.c_str());
        std::fprintf(run, "save: %s\n", options_.save.empty() ? "-" : options_.save.c_str());
        std::fprintf(run, "save_written: %d\n", saveWritten_ ? 1 : 0);
        std::fprintf(run, "status: %d\n", status);
        std::fclose(run);
    }
    if (!options_.saveOut.empty()) {
        const melonDS::u8 *save = nds_->GetNDSSave();
        melonDS::u32 length = nds_->GetNDSSaveLength();
        std::FILE *out = std::fopen(options_.saveOut.c_str(), "wb");
        if (save == nullptr || out == nullptr || std::fwrite(save, 1, length, out) != length) {
            Fail("can't write the save to %s", options_.saveOut.c_str());
            status = status != 0 ? status : 1;
        }
        if (out != nullptr) {
            std::fclose(out);
        }
    }
    if (std::fclose(framesTsv_) != 0 || std::fclose(memoryTxt_) != 0) {
        Fail("can't write in %s", options_.out.c_str());
        status = status != 0 ? status : 1;
    }
    pbw_input_free(&input_);
    return status;
}

} // namespace oracle

int main(int argc, char **argv) {
    oracle::Options options;

    if (!oracle::ParseOptions(argc, argv, options)) {
        oracle::Usage();
        return 2;
    }
    oracle::Oracle run(std::move(options));
    return run.Run();
}
