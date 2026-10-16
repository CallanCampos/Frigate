#include "types.h"
#include "app/local_tvt/local_tvt_common.h"
#include "app/local_tvt_local.h"
#include "gfl/heap.h"
#include "gfl/std.h"

// The call with a person on the Xtransceiver, named after the ROM's "local_tvt_person.c": the person greets, then
// asks what to talk about and talks about the topic the player picks, until a topic ends the call. What a person
// says is in its own file

struct LocalTvtPerson {
    u16 heapId;
    LocalTvtPersonFuncs funcs;
    LocalTvtPersonSetup setup;
    LocalTvtSeq *seq;
    LocalTvtMenu *menu;
    u32 selected;
    u32 timer;
};

static void LocalTvtPerson_SeqGreet(LocalTvtSeq *seq, int *step, void *work);
static void LocalTvtPerson_SeqMenu(LocalTvtSeq *seq, int *step, void *work);
static void LocalTvtPerson_SeqTopic(LocalTvtSeq *seq, int *step, void *work);

LocalTvtPerson *LocalTvtPerson_Create(const LocalTvtPersonFuncs *funcs, const LocalTvtPersonSetup *setup,
                                      HeapID heapId) {
    LocalTvtPerson *person = GFL_HeapAllocate(heapId, sizeof(LocalTvtPerson), TRUE, "local_tvt_person.c", 79);

    person->heapId = heapId;
    person->funcs = *funcs;
    person->setup = *setup;
    person->seq = LocalTvtSeq_Create(person, LocalTvtPerson_SeqGreet, heapId);
    return person;
}

void LocalTvtPerson_Free(LocalTvtPerson *person) {
    LocalTvtSeq_Free(person->seq);
    if (person->funcs.free != NULL) {
        person->funcs.free(person, &person->setup, person->funcs.work);
    }
    GFL_HeapFree(person);
}

void LocalTvtPerson_Main(LocalTvtPerson *person) {
    LocalTvtSeq_Main(person->seq);
}

BOOL LocalTvtPerson_IsEnd(LocalTvtPerson *person) {
    return LocalTvtSeq_IsEnd(person->seq);
}

void LocalTvtPerson_End(LocalTvtPerson *person) {
    LocalTvtSeq_End(person->seq);
}

static void LocalTvtPerson_SeqGreet(LocalTvtSeq *seq, int *step, void *work) {
    LocalTvtPerson *person = work;

    switch (*step) {
    case 0:
        if (person->funcs.greet(person, &person->setup, person->funcs.work)) {
            (*step)++;
        }
        break;
    case 1:
        LocalTvtSeq_SetNext(seq, LocalTvtPerson_SeqMenu);
        break;
    }
}

static void LocalTvtPerson_SeqMenu(LocalTvtSeq *seq, int *step, void *work) {
    LocalTvtPerson *person = work;
    u32 i = 0;
    LocalTvtFace *face = LocalTvtChara_GetFace(person->setup.chara, 0);

    switch (*step) {
    case 0:
        LocalTvtFace_Talk(face, person->funcs.msgData, person->funcs.menuMsg);
        (*step)++;
        break;
    case 1:
        if (LocalTvtFace_UpdateTalk(face, person->setup.chara)) {
            person->timer = 0;
            (*step)++;
        }
        break;
    case 2:
        if (person->timer++ > 15) {
            (*step)++;
        }
        break;
    case 3: {
        LocalTvtMenuSetup setup;

        sys_memset(&setup, 0, sizeof(setup));
        setup.message = person->setup.message;
        setup.unit = person->setup.unit;
        setup.msgData = person->funcs.msgData;
        setup.count = person->funcs.topicCount;
        setup.cursor = person->selected;
        for (; i < person->funcs.topicCount; i++) {
            setup.items[i] = person->funcs.topicNames[i];
        }
        person->menu = LocalTvtMenu_Create(&setup, person->heapId);
        (*step)++;
        break;
    }
    case 4:
        LocalTvtMenu_Main(person->menu);
        if (LocalTvtMenu_IsEnd(person->menu)) {
            person->selected = LocalTvtMenu_GetSelected(person->menu);
            (*step)++;
        }
        break;
    case 5:
        LocalTvtMenu_Free(person->menu);
        LocalTvtSeq_SetNext(seq, LocalTvtPerson_SeqTopic);
        break;
    }
}

static void LocalTvtPerson_SeqTopic(LocalTvtSeq *seq, int *step, void *work) {
    LocalTvtPerson *person = work;

    switch (*step) {
    case 0:
        if (person->funcs.topics[person->selected](person, &person->setup, person->funcs.work)) {
            (*step)++;
        }
        break;
    case 1:
        LocalTvtSeq_SetNext(seq, LocalTvtPerson_SeqMenu);
        break;
    }
}
