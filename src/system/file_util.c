#include "types.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "nitro/fs.h"
#include "system/file_util.h"

// Reading archive files without their NARC structure

void GFL_ArcSysInitRawHandle(FSFile *file, u32 arcId) {
    const char *path = GFL_ArcSysGetResourcePath(arcId);

    FS_InitFile(file);
    FS_OpenFile(file, path);
}

void *GFL_ArcSysReadRawResource(u32 arcId, HeapID heapId, u32 offset, u32 size) {
    FSFile file;
    void *data;

    GFL_ArcSysInitRawHandle(&file, arcId);
    if (size == 0) {
        size = FS_GetLength(&file);
    }
    data = GFL_HeapAllocate(heapId, size, FALSE, "file_util.c", 178);
    FS_SeekFile(&file, offset, FS_SEEK_SET);
    FS_ReadFile(&file, data, size);
    FS_CloseFile(&file);
    return data;
}
