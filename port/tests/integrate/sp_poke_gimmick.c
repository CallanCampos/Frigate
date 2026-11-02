// The special Pokémon's field gimmicks (src/ov129/sp_poke_gimmick.c, overlay 129), whose zones are the Castelia City
// café where a Pokémon dances along a curve (ZONE_CASTELIA_CITY_22, gimmick B) and zone 614, with typed spies in this
// TU only. Each of the four gimmicks keeps its state in the field's gimmick work block 1, a SpPokeGimmickWork, which
// its pointers make 64 bytes on a 64-bit host against the DS's 0x30. The fixture gives each initialization a block of
// exactly the size it asks for, under ASan and UBSan, and checks that the size holds the whole struct and that the
// game's writes stay in it. With the DS's 0x30, gimmick B's curve was written past its block into the head of the field
// heap's next free block, and the field's next allocation found the heap empty (the map sweep's silent exit at
// CONTINUE into ZONE_CASTELIA_CITY_22, port/docs/STATUS.md); gimmick C's NPC, whose block comes from the heap's tail,
// went past its block into the head of the block after it. With that size the fixture fails its size check at gimmick
// A, before its own reads of the work's data. No ROM: the scenes' 3D resources are spies.
#define Field_AllocGimmickWorkBlock SpyAllocWork
#define Field_GetGimmickWorkBlock SpyGetWork
#define Field_DeleteGimmickWorkBlock SpyDeleteWork
#define Field_GetExpObjSystem SpyExpObjSystem
#define Field_GetHeapID SpyHeapID
#define GFL_HeapAllocate SpyHeapAllocate
#define GFL_HeapFree SpyHeapFree
#define LoadFieldExpandObjData SpyLoadScene
#define FieldExpObj_AddScene SpyAddScene
#define FieldExpObj_FreeScene SpyFreeScene
#define FieldExpObj_SetActorHidden SpyHidden
#define FieldExpObj_SetActorCulling SpyCulling
#define FieldExpObj_GetActorMatrixPtr SpyMatrix
#define FieldExpObj_GetAnmInfo SpyAnmInfo
#define FieldExpObj_SetAnm SpySetAnm
#define FieldExpObjAnm_SetLooped SpyLooped
#define FieldExpObjAnm_SetPaused SpyPaused
#include "src/ov129/sp_poke_gimmick.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FIELD_HEAP 0x15
#define SCENE_D_ACTORS 6

static u64 sFieldStorage, sSystemStorage, sAnmStorage;
static SRTMatrix sMatrices[SCENE_D_ACTORS];
// The work block as the gimmick asked for it, and the blocks it allocated from the field's heap
static void *sWork;
static u32 sWorkSize;
static unsigned sAllocated, sFreed, sScenesLoaded, sScenesFreed;

#define CHECK(x)                                                                                                       \
    do {                                                                                                               \
        if (!(x)) {                                                                                                    \
            fprintf(stderr, "sp poke gimmick line %d: %s\n", __LINE__, #x);                                            \
            exit(1);                                                                                                   \
        }                                                                                                              \
    } while (0)

static Field *TheField(void) {
    return (Field *)&sFieldStorage;
}

static FieldExpObjSystem *TheSystem(void) {
    return (FieldExpObjSystem *)&sSystemStorage;
}

// Field_AllocGimmickWorkBlock clears a block of the size asked for (src/ov036/fieldmap.c); this one is a host block of
// exactly that size, so that ASan reports any write past it
void *SpyAllocWork(Field *field, u32 id, HeapID heapId, u32 size) {
    CHECK(field == TheField() && id == WORK_ID && sWork == NULL);
    CHECK((heapId & (HEAPID_TAIL_BIT - 1)) == FIELD_HEAP);
    sWork = calloc(1, size);
    CHECK(sWork != NULL);
    sWorkSize = size;
    return sWork;
}

void *SpyGetWork(Field *field, u32 id) {
    CHECK(field == TheField() && id == WORK_ID);
    return sWork;
}

void SpyDeleteWork(Field *field, u32 id) {
    CHECK(field == TheField() && id == WORK_ID && sWork != NULL);
    free(sWork);
    sWork = NULL;
}

FieldExpObjSystem *SpyExpObjSystem(Field *field) {
    CHECK(field == TheField());
    return TheSystem();
}

u16 SpyHeapID(Field *field) {
    CHECK(field == TheField());
    return FIELD_HEAP;
}

void *SpyHeapAllocate(HeapID heapId, u32 size, BOOL clear, const char *file, u16 line) {
    void *block = calloc(1, size);

    CHECK((heapId & (HEAPID_TAIL_BIT - 1)) == FIELD_HEAP && clear && block != NULL);
    (void)file;
    (void)line;
    sAllocated++;
    return block;
}

void SpyHeapFree(void *ptr) {
    CHECK(ptr != NULL);
    free(ptr);
    sFreed++;
}

void SpyLoadScene(FieldExpObjSystem *system, const G3DSceneSetup *setup, u16 scene) {
    CHECK(system == TheSystem() && setup != NULL && scene == 0);
    sScenesLoaded++;
}

void SpyAddScene(FieldExpObjSystem *system, const G3DSceneSetup *setup, u16 scene) {
    CHECK(system == TheSystem() && setup != NULL && scene == 0);
    sScenesLoaded++;
}

void SpyFreeScene(FieldExpObjSystem *system, u16 scene) {
    CHECK(system == TheSystem() && scene == 0);
    sScenesFreed++;
}

void SpyHidden(FieldExpObjSystem *system, u16 scene, u16 actor, BOOL hidden) {
    CHECK(system == TheSystem() && scene == 0 && actor < SCENE_D_ACTORS);
    (void)hidden;
}

void SpyCulling(FieldExpObjSystem *system, u16 scene, u16 actor, BOOL cullByBBox) {
    CHECK(system == TheSystem() && scene == 0 && actor < SCENE_D_ACTORS);
    (void)cullByBBox;
}

SRTMatrix *SpyMatrix(FieldExpObjSystem *system, u16 scene, u16 actor) {
    CHECK(system == TheSystem() && scene == 0 && actor < SCENE_D_ACTORS);
    return &sMatrices[actor];
}

FieldExpObjAnm *SpyAnmInfo(FieldExpObjSystem *system, u16 scene, u16 actor, u16 anm) {
    CHECK(system == TheSystem() && scene == 0 && actor < SCENE_D_ACTORS);
    (void)anm;
    return (FieldExpObjAnm *)&sAnmStorage;
}

void SpySetAnm(FieldExpObjSystem *system, u16 scene, u16 actor, u16 anm, BOOL bind) {
    CHECK(system == TheSystem() && scene == 0 && actor < SCENE_D_ACTORS);
    (void)anm;
    (void)bind;
}

void SpyLooped(FieldExpObjAnm *anm, u8 looped) {
    CHECK(anm == (FieldExpObjAnm *)&sAnmStorage);
    (void)looped;
}

void SpyPaused(FieldExpObjAnm *anm, u8 paused) {
    CHECK(anm == (FieldExpObjAnm *)&sAnmStorage);
    (void)paused;
}

typedef struct {
    const char *name;
    void (*init)(Field *field);
    void (*end)(Field *field);
    // Which of the work's pointers to the field's heap the gimmick keeps: none, gimmick B's curve, gimmick C's NPC
    int data;
} Gimmick;

enum {
    DATA_NONE,
    DATA_CURVE,
    DATA_NPC,
};

int main(void) {
    static const Gimmick gimmicks[] = {
        { "A", SpPokeGimmickA_Init, SpPokeGimmickA_End, DATA_NONE },
        { "B", SpPokeGimmickB_Init, SpPokeGimmickB_End, DATA_CURVE },
        { "C", SpPokeGimmickC_Init, SpPokeGimmickC_End, DATA_NPC },
        { "D", SpPokeGimmickD_Init, SpPokeGimmickD_End, DATA_NONE },
    };

    for (size_t i = 0; i < sizeof(gimmicks) / sizeof(gimmicks[0]); i++) {
        const Gimmick *gimmick = &gimmicks[i];
        SpPokeGimmickWork *work;

        sWorkSize = 0;
        sAllocated = sFreed = sScenesLoaded = sScenesFreed = 0;
        gimmick->init(TheField());
        work = sWork;
        printf("gimmick %s: work block of %u bytes, SpPokeGimmickWork %zu\n", gimmick->name, (unsigned)sWorkSize,
               sizeof(SpPokeGimmickWork));
        CHECK(work != NULL && sWorkSize >= sizeof(SpPokeGimmickWork));
        CHECK(work->field == TheField() && work->system == TheSystem() && sScenesLoaded == 1);
        if (gimmick->data == DATA_CURVE) {
            CHECK(work->data.curve != NULL && sAllocated == 1);
        } else if (gimmick->data == DATA_NPC) {
            CHECK(work->data.npc != NULL && sAllocated == 1);
        } else {
            CHECK(work->data.curve == NULL && sAllocated == 0);
        }
        gimmick->end(TheField());
        CHECK(sWork == NULL && sFreed == sAllocated && sScenesFreed == 1);
    }
    puts("sp poke gimmick: the four gimmicks' work blocks hold SpPokeGimmickWork PASS");
    return 0;
}
