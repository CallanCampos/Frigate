# Code standards

The rules that every C file in `src/` and `lib/` follows, with the wrong and the right form of each, the cases where a
rule doesn't apply, and what checks it. They come from audits of merged work: each wrong form below was in the tree.

The aim behind all of them: **the code reads as what Game Freak, or the SDK's authors, wrote.** A function that only
matches through a workaround keeps the natural C and gets a row in [Nonmatching functions](nonmatching-functions.md),
even when that sends a complete file back to partial. A match is never worth a workaround.

`tools/decomp/check_style.py` finds the rules a script can see. It runs in CI on the lines a pull request adds, after
each edit in Claude Code and Codex sessions, and by hand:

    .venv/bin/python tools/decomp/check_style.py src/ov033/x.c          # every line of a file
    .venv/bin/python tools/decomp/check_style.py --diff origin/main      # the lines a branch adds

The others (2, 3) need a reader: the `decomp-reviewer` agent and pull request review check them.

## 1. Control flow as it reads

**Do:** return where the function returns, and break out of a loop where the loop ends.

**Don't:** jump to a label that only returns, or to a copy of the compiler's shared tail, and don't move the loop into
a `static inline` whose `return` from inside it does the same jump: for the loop below, a helper that returns the
index it finds and gives -1 after the loop is that `goto` under another name.

```c
/* Wrong: the goto only stands in for a return */
    for (i = 0; i < count; i++) {
        if (list[i] == id) {
            index = i;
            goto end;
        }
    }
    index = -1;
end:
    return index;
```

```c
/* Right */
    for (i = 0; i < count; i++) {
        if (list[i] == id) {
            return i;
        }
    }
    return -1;
```

**Does not apply when:** the label is a shared cleanup or error exit that frees or resets something (`fail:` in
`nhttp_request.c`), a retry (`goto retry`), or leaves a search loop to skip its fallback (`goto inserted` in
`vct_buffer.c`), or jumps forward to a labelled tail block that does real work for several paths and then returns, in
place of a flag tested at the end (`goto TRANS_INTERP_2` and `goto TRANS_NONINTERP` in NitroSystem's `nsbca.c`, where
one block averages a pair of stored values and the other reads one). A label whose code only repeats a `return` or an
epilogue is still the compiler's shared tail, unless another decompilation of the same library writes that `goto` too,
which is evidence its authors did: DS Protect's emulator checks (`dsprot_detect.c`) leave the loop over the MAC address
with `goto end` to `return code * 0xbf`, as pret's pokeheartgold has an older DS Protect's MAC check
(`lib/dsprot/src/mac_owner.c`) jump to `EXIT`. Mark such a `goto` with `// style-ok: goto-return` and the evidence. The
user chose this on 2026-10-19. A `do { ... } while (0)` is fine as a macro's body, or as a breakable block with code and
locals between its `break`s (`btl_server_flow.c`), never as a wrapper around one statement.

**Checked:** a `goto` whose label is followed by nothing but a `return` or the function's end.

## 2. No meaningless temporaries

**Don't:** add a local that names nothing, set a variable twice to pick a branch, or split one expression into
statements, only to move the compiler's registers or stop it folding a constant.

```c
/* Wrong: `zero` is there for the register allocator */
    f32 zero = 0.0f;
    scale = (fx32)((1.0f / zoom > zero) ? (1.0f / zoom) * FX32_ONE + 0.5f : (1.0f / zoom) * FX32_ONE - 0.5f);

/* Right */
    scale = FX32_CONST(1.0f / zoom);
```

```c
/* Wrong */
    side = 0;
    if (!(wk->attacker & 1)) {
        side = 1;
    }
    positions[0] = side;

/* Right */
    positions[0] = (wk->attacker & 1) ? 0 : 1;
```

**Does not apply when:** the local names a value the code uses more than once or that has a meaning of its own, or
the original's own spelling is evidently the split one (two statements that each do something, a declaration order
that a comment explains). A local the code needs is fine even if the order of its declaration matters to the match.

**Checked:** by review, not by script.

## 3. Macros and helpers as written

**Don't:** expand a macro or an inline of the project or the SDK (`FX32_CONST`, `NELEMS`, `GX_RGB`, `FX_Mul`) by
hand, or hand-copy a header's inline into a file, to change what the compiler does.

**Checked:** by review.

## 4. Indexing and pointers

**Do:** index an array, or write a pointer sum of the element's own type when it gives the original's address
computation: `*(p + i + 1)` and `(p + n - 1)->next` compile differently from `p[i + 1]` under MWCC, and some twenty
functions match only that way.

**Don't:** reach a field through a cast to another pointer type and a byte offset.

```c
/* Wrong */
    rate = *(u16 *)((u8 *)work + 0x1310);
    flags = ((u16 *)ind)[5];

/* Right: the struct, with the field */
    rate = work->connectBss.rateSetSupport;
    flags = ind->reason;
```

**Does not apply when:** the data is untyped bytes (a packet, a file, a compressed stream) read through a `u8 *`.

**Checked:** a `(u8 *)`, `(char *)` or `(s8 *)` cast followed by `+`, then a cast and a dereference.

## 5. Declarations in their owner's header

**Do:** declare a function or global once, in the header of the file that defines it, and include that header.
Something whose owner isn't written yet goes in its module's shared header, with a comment saying so.

**Don't:** write `extern` or another file's prototype in a `.c` file.

```c
/* Wrong, in fieldmap.c */
extern const FieldMapConfig MAP_CONFIGS[];

/* Right: MAP_CONFIGS is fieldmap.c's own .rodata, so it is defined there from the ROM's bytes */
static const FieldMapConfig MAP_CONFIGS[] = { ... };
```

**Does not apply when:** a `static` function's forward declaration, or a plain declaration of the file's own data
ahead of its definition, which can keep its data order (the two shop scripts' window setups). Also the C library's
string functions that a library declares in its own header (`dwc/ext.h`, `ssl_internal.h`) where its ROM calls them:
the same declarations from a header named `string.h` change the code (`docs/matching.md`, "a header named
`string.h`"); MSL's `exit`, which `nitro/os.h` declares for the same reason; and the functions a file lists in
`config/deadstrip.toml`, which it declares itself rather than in its header, so that no other file can call code the
build removes, unless another file's removed code calls them (`docs/configs.md`, "Dead-stripped code").

**Checked:** `extern` in a `.c` file, and a non-static prototype in a `.c` file that the file doesn't define.

## 6. `volatile` only where the hardware or another thread needs it

**Do:** keep a `volatile` only when the original's code needs it (removing it changes a function that matches), and
say in one line what changes the value behind the compiler's back: an interrupt, another thread, the other CPU.

**Checked:** a `volatile` with no comment on its line or the line before (register macros are exempt).

## 7. Assembly

**Don't:** write `asm` in `src/`. In `lib/`, `asm` is only for code the SDK, or a library it bundles, wrote in
assembly (an MWCC `asm` function), or assembly a library inlined at several sites (a `static inline` with an `asm {}`
block, such as `nitro/math.h`'s `MATH_CountLeadingZeros` in ARM code), with a comment saying which. A library that
is hand-written assembly as a whole, whose routines have several entries or names, is an assembly file in its `src/`
(MSL's runtime, `lib/msl/src/fp/fp_fastI.s`), assembled by the mwasmarm of its compiler's version with
library.toml's `asm_flags`.

**Do:** wrap every `asm` function in `// clang-format off` and `// clang-format on`, with a blank line before `on`:
clang-format joins assembly into one line.

**Checked:** `asm` in `src/`, and an `asm` function in `lib/` with no comment above it.

## 8. Comments explain the code

**Do:** say what the code does, or, for a necessary oddity, why, in one plain sentence.

**Don't:** leave `TODO`, `FIXME` or `XXX`, or narrate how a function was matched: "added to match", "the permuter
found", "this pass", "down from 81 bytes".

```c
/* Wrong */
    // c is declared first to match

/* Right */
    // The order of these declarations sets the locals' registers
```

**Checked:** `TODO`/`FIXME`/`XXX` and narrating phrases in comments.

## 9. Names

Swan's names first, marked as swan's in the file's header; the SDK's own names in `lib/`; ours through
`rename_symbol.py`. A static's symbol is renamed to its C name. A `func_`/`data_` identifier that `symbols.txt`
already names is stale: `tools/decomp/apply_names.py` rewrites it.

**Checked:** in CI, `apply_names.py --dry-run` must list nothing.

## 10. Bugs

A bug of the original game is `// BUG:` with an explanation, and an `#ifdef BUGFIX` that fixes it, with the original
in the `#else`. A mistake of an earlier decompilation is just fixed: it isn't the game's bug.

**Checked:** a `// BUG:` with no `#ifdef BUGFIX` in the next lines.

## 11. Formatting

`clang-format` 23.1.1 (`.clang-format`), on every file changed. Guard with `// clang-format off`/`on`: `asm` functions,
tables laid out by hand, and a `GFL_ASSERT` whose expression has operators, since the macro turns the expression into
a string in the ROM and clang-format's spacing would change it (`GFL_ASSERT((u16)size < (0xffff/2))`).

**Don't:** put a comment between a declaration's specifiers and the rest (`static // Connects ...` above
`int AOSSi_WLANConnect(...)`): clang-format then indents the declaration as a continuation. The comment goes above.

**Checked:** in CI, `clang-format --dry-run --Werror` on the changed files, with the pinned version, and a comment
right after `static`, `inline`, `extern` or `const` that starts a line.

## 12. Rows of the nonmatching functions

A row of [Nonmatching functions](nonmatching-functions.md) says, in the present tense, how the function differs now
(size or bytes, and what the original does differently) and what was tried. No history ("down from 81 lines", "this
pass", "the previous theory"), no counts of permuter iterations.

**Checked:** narrating phrases on the lines a pull request adds to the file.

## 13. The agents' copies

The skills in `.claude/skills/` have Codex copies in `.agents/skills/`; a change to one goes into the other in the
same commit.

**Checked:** in CI, the two trees must be identical.

## 14. The SDK's types, as the SDK has them

`u32` and `s32` are NitroSDK's `unsigned long` and `long`, and its fixed-point constants and conversions are the SDK's
(`FX32_ONE` is `((fx32)0x00001000L)`, `FX32_CONST` is `FX_F32_TO_FX32`), as ntrtwl's NitroSDK and pret's pokediamond
have them. Game Freak's code mixes them with plain C types, so a declaration takes the type the original evidently
had: `u32`/`s32` where it compiles as the SDK's, `int`/`unsigned int` where it compiles as those.
GameSpy's code, which is cross-platform, uses GameSpy's own types from its `nonport.h` (`lib/dwc/include/dwc/nonport.h`:
`gsi_u32` is `unsigned int`, `gsi_i32` is `int`, `gsi_bool` is `int`), and DWC's own code the SDK's.

**Don't:** change a typedef or an SDK constant because more functions match with it, or cast to emulate another type.
A function that matches only under a type the original didn't have isn't faithful.

**Do:** give the declaration that decides it (a local, a parameter, a struct field, a prototype) its original type.
An enum isn't a stand-in for `unsigned int`: under `-enum int` it is a signed `int`.

**Checked:** by review. `docs/matching.md` ("Game Freak's code mixes the SDK's types") has the lever.

## 15. Constants, parameters and tables with their structure

Game Freak's code names its value sets: a task's sequence numbers, a function's results, the kinds, modes, commands
and indices that a field or parameter takes are an `enum`, often closed by a count (`..._MAX`), as in pret's Gen 4
decomps; fixed values, sizes and magic numbers are `#define`s. The SDK's own enums (`GXBGMode`, `GXVRamBG`,
`NNS_GFD_DST_TYPE`) are the constants of its parameters.

**Do:**
- write a value set as an `enum` (anonymous, or a typedef where a declaration names it), keeping our names and, where
  swan has the set, swan's type name;
- pass the constant, not its number, where a parameter takes an enum's or a set's value;
- type a parameter as the struct it always points to, not `void *`, unless callers pass several types (a callback's
  `void *work`, an SDK callback's argument);
- define a type once, in its owner's header, and use it everywhere, rather than a copy with `unk` fields under another
  name;
- write a table whose elements are records as an array of a struct.

**Don't:** change the type a field, parameter or local stores in to the enum unless the original evidently had it:
under `-enum int` an enum is a signed `int` (section 14), so a `u8` field keeps `u8` and takes the enum's constants.

```c
/* Wrong, in field_player_grid.c and gfl/g3d.h */
#define GRID_COMMAND_NONE 0
#define GRID_COMMAND_IDLE 1
void GFL_G3DResFree(void *resource); /* whose first line is G3DResource *res = resource; */

/* Right */
enum {
    GRID_COMMAND_NONE,
    GRID_COMMAND_IDLE,
};
void GFL_G3DResFree(G3DResource *resource);
```

**Does not apply when:** a group of `#define`s are bits that combine (a flag set may be an enum with explicit values
when a field holds the combination), or are generated constant lists (`generated/`).

**Checked:** by review.

## 16. Pointers and storage of one of several types

A pointer keeps the type it evidently has: a pointer that only ever holds one type is that type, and a type is never
stripped back to `void *`. Where it holds one of several types, how the code stores it decides, as in Game Freak's
HGSS code and pret's decomps:

- storage that holds one of several structs inline, in the struct that owns it, is a `union` of them, and pointers to
  it take the union's type;
- a pointer to one of several separately allocated structs, or a parameter whose type a kind picks, is `void *` with a
  comment that lists the types it can hold, and each use takes a typed local from it, with no cast;
- a union of pointers only where the same struct holds the mode field that picks the member.

**Don't:** make a union of the structs that a `void *` points to, and cast each allocation to it.

```c
/* Wrong, in field_controller.h and fieldmap_ctrl_grid.c */
typedef union {
    FieldmapCtrlGrid grid;
    FieldmapCtrlNoGrid noGrid;
    FieldmapCtrlHybrid hybrid;
} FieldmapCtrl;
FieldmapCtrl *Field_GetController(Field *field);

    FieldmapCtrlGrid *controller = &Field_GetController(field)->grid;
    setMapDisplayInfoPtr(env, (ScriptMsgWin *)window);
```

**Do:** keep the `void *` with its types listed, take a typed local at each use, and keep a union for the storage a
struct holds inline.

```c
/* Right, in field_controller.h, fieldmap_ctrl_grid.c and festival.h */
// The field's controller: a FieldmapCtrlGrid, FieldmapCtrlNoGrid or FieldmapCtrlHybrid, by the kind of map
void *Field_GetController(Field *field);

    FieldmapCtrlGrid *controller = Field_GetController(field);

union FestMissionWork {
    RivalEntry hollows[224];
    FesMissionWork people;
    FestSparkleWork sparkle;
};
struct LinkFestival {
    /* ... */
    FestMissionWork missionWork;
};
```

**Checked:** by review.
