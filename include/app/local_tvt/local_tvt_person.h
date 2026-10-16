#ifndef POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_PERSON_H
#define POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_PERSON_H

#include "types.h"
#include "app/local_tvt/local_tvt_common.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// local_tvt_person.c: the call with a person, who greets and then talks about the topic the player picks

#define LOCAL_TVT_TOPIC_MAX 4

struct LocalTvtPersonSetup {
    LocalTvtMessage *message;
    LocalTvtChara *chara;
    ClActUnit *unit;
    LocalTvtCallData *callData;
};

// A step of a call, which returns TRUE once it is over. The work starts with the step's own counter
typedef BOOL (*LocalTvtPersonFunc)(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);

// What each local_tvt_person_*.c gives of its person
struct LocalTvtPersonFuncs {
    void *work;
    LocalTvtPersonFunc greet;
    LocalTvtPersonFunc topics[LOCAL_TVT_TOPIC_MAX];
    void (*free)(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
    // The messages of the topics in the menu
    u32 topicNames[LOCAL_TVT_TOPIC_MAX];
    u32 topicCount;
    // The person's question before the menu
    u32 menuMsg;
    MsgData *msgData;
};

LocalTvtPerson *LocalTvtPerson_Create(const LocalTvtPersonFuncs *funcs, const LocalTvtPersonSetup *setup,
                                      HeapID heapId);
void LocalTvtPerson_Free(LocalTvtPerson *person);
void LocalTvtPerson_Main(LocalTvtPerson *person);
BOOL LocalTvtPerson_IsEnd(LocalTvtPerson *person);
// Ends the call; the topic that says goodbye calls it
void LocalTvtPerson_End(LocalTvtPerson *person);

#endif // POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_PERSON_H
