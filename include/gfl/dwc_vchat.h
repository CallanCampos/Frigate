#ifndef POKEBW2_GFL_DWC_VCHAT_H
#define POKEBW2_GFL_DWC_VCHAT_H

#include "types.h"
#include "gfl/heap.h"

// Overlay 29: the Wi-Fi voice chat, whose file the ROM doesn't name

void func_ov029_021925b4(BOOL a0);
void func_ov029_0219270c(HeapID heapId, int mode, int a2);
int func_ov029_021926e4(u8 netId, u8 *data, int size);
void func_ov029_021928e4(void);
void func_ov029_02192934(void (*callback)(void));
void func_ov029_02192948(void);
void func_ov029_02192a54(int netId);
void func_ov029_02192ab4(BOOL a0);
BOOL func_ov029_021929b8(void);
BOOL func_ov029_021929e0(u32 netIds, int netId);

#endif // POKEBW2_GFL_DWC_VCHAT_H
