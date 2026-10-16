#ifndef POKEBW2_NITRO_FS_H
#define POKEBW2_NITRO_FS_H

#include "types.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// NitroSDK's file system, under swan's names: FS_InitFile, FS_OpenFile, FS_GetLength, FS_ReadFile, FS_CloseFile,
// FS_SeekFile, and FS_CreateFileFromMemory, which opens a file over memory

// What a command of the file system returns. A command that an archive runs apart from the caller's thread returns
// FS_RESULT_PROC_ASYNC and ends later, through FS_NotifyArchiveAsyncEnd
typedef enum FSResult {
    FS_RESULT_SUCCESS = 0,
    FS_RESULT_FAILURE = 1,
    FS_RESULT_BUSY = 2,
    FS_RESULT_CANCELED = 3,
    FS_RESULT_UNSUPPORTED = 4,
    FS_RESULT_ERROR = 5,
    FS_RESULT_INVALID_PARAMETER = 6,
    // What the ROM archive answers to opening a range of the part that only the DSi mode reads
    FS_RESULT_PERMISSION_DENIED = 9,
    FS_RESULT_NO_ENTRY = 11,
    FS_RESULT_PROC_ASYNC = 0x100,
    // What an archive's command hook (FSProc) returns to leave the command to the archive's own functions, this time
    // (UNKNOWN) or for good (DEFAULT, which clears the command's bit of the hook's mask)
    FS_RESULT_PROC_UNKNOWN = 0x101,
    FS_RESULT_PROC_DEFAULT = 0x102,
} FSResult;

// The commands that a file sends to its archive, which runs one of the functions of the archive's interface. FSFile's
// stat holds the command in its second byte. The game uses 0, 1, 4, 6, 7, 8 and 13 to 16 (to read and write a file, to
// find a path, to open a file by number, by range and by path, to close it, to seek in it and to ask its length and
// position). 9 to 12, 17, 18 and FS_COMMAND_INVALID are the events, which run at once and return nothing. The
// commands from 19 on act on paths and on the archive; no function of the game sends them. The ROM archive has
// functions for 19, 23 (the information of a path), 28 (the archive's resources) and 32 and 33 (opening and closing a
// directory by path); the others are left as the words an interface function receives
typedef enum FSCommandType {
    FS_COMMAND_READFILE = 0,
    FS_COMMAND_WRITEFILE = 1,
    FS_COMMAND_SEEKDIR = 2,
    FS_COMMAND_READDIR = 3,
    FS_COMMAND_FINDPATH = 4,
    FS_COMMAND_GETPATH = 5,
    FS_COMMAND_OPENFILEFAST = 6,
    FS_COMMAND_OPENFILEDIRECT = 7,
    FS_COMMAND_CLOSEFILE = 8,
    FS_COMMAND_ACTIVATE = 9,
    FS_COMMAND_IDLE = 10,
    FS_COMMAND_SUSPEND = 11,
    FS_COMMAND_RESUME = 12,
    FS_COMMAND_OPENFILE = 13,
    FS_COMMAND_SEEKFILE = 14,
    FS_COMMAND_GETFILELENGTH = 15,
    FS_COMMAND_GETFILEPOSITION = 16,
    FS_COMMAND_MOUNT = 17,
    FS_COMMAND_UNMOUNT = 18,
    FS_COMMAND_19 = 19,
    FS_COMMAND_20 = 20,
    FS_COMMAND_21 = 21,
    FS_COMMAND_22 = 22,
    FS_COMMAND_GETPATHINFO = 23,
    FS_COMMAND_24 = 24,
    FS_COMMAND_25 = 25,
    FS_COMMAND_26 = 26,
    FS_COMMAND_27 = 27,
    FS_COMMAND_GETARCHIVERESOURCE = 28,
    FS_COMMAND_29 = 29,
    FS_COMMAND_30 = 30,
    FS_COMMAND_31 = 31,
    FS_COMMAND_OPENDIRECTORY = 32,
    FS_COMMAND_CLOSEDIRECTORY = 33,
    FS_COMMAND_34 = 34,
    // What a file that has no command holds
    FS_COMMAND_INVALID = 35,
} FSCommandType;

// The room that an archive's interface has for commands. The ROM's has the 35 of FSCommandType, and NULL for the rest
#define FS_COMMAND_MAX 64

typedef struct FSArchive FSArchive;
typedef struct FSFile FSFile;

// A file by archive and number: NitroSDK's FS_ConvertPathToFileID and FS_OpenFileFast, and FS_OpenFileEx (which opens
// with a mode), under swan's names
typedef struct FSFileID {
    FSArchive *arc;
    u32 file_id;
} FSFileID;

// Where a directory is read: its number, the number that the next file in it has, and where its next entry is in the
// archive's name table
typedef struct FSDirPos {
    FSArchive *arc;
    u16 dirId;
    u16 fileId;
    u32 position;
} FSDirPos;

// What a directory's entry is: a file, by archive and number, or a directory, by position
typedef union FSEntryId {
    FSFileID file;
    FSDirPos dir;
} FSEntryId;

// The longest name that a directory entry of the ROM's name table holds
#define FS_ENTRY_NAME_LEN_MAX 127

// A directory entry as the archive's name table has it
typedef struct FSDirEntry {
    FSEntryId id;
    u32 isDirectory;
    u32 nameLength;
    char name[FS_ENTRY_NAME_LEN_MAX + 1];
} FSDirEntry;

// What an archive of the TWL file system tells about a path (FS_COMMAND_GETPATHINFO) and about a directory entry
// (FS_COMMAND_READDIR through the interface), and about itself (FS_COMMAND_GETARCHIVERESOURCE): NitroSDK's (TwlSDK's)
// FSPathInfo, FSDirectoryEntryInfo and FSArchiveResource
#define FS_ATTRIBUTE_IS_DIRECTORY 0x100
#define FS_ATTRIBUTE_IS_ARCHIVE_ENTRY 0x200 // set on every path that the ROM archive reports
#define FS_ATTRIBUTE_IS_PROTECTED 0x400     // a file in the part of the ROM that only the DSi mode reads

#define FS_ENTRY_SHORTNAME_MAX 16
#define FS_ENTRY_LONGNAME_MAX 260

typedef struct FSDateTime {
    u32 year;
    u32 month;
    u32 day;
    u32 hour;
    u32 minute;
    u32 second;
} FSDateTime;

typedef struct FSPathInfo {
    u32 attributes;
    FSDateTime atime;
    FSDateTime mtime;
    FSDateTime ctime;
    u32 filesize;
    u32 id;
} FSPathInfo;

typedef struct FSDirectoryEntryInfo {
    char shortname[FS_ENTRY_SHORTNAME_MAX];
    u32 shortnameLength;
    char longname[FS_ENTRY_LONGNAME_MAX];
    u32 longnameLength;
    u32 attributes;
    FSDateTime atime;
    FSDateTime mtime;
    FSDateTime ctime;
    u32 filesize;
    u32 id;
} FSDirectoryEntryInfo;

typedef struct FSArchiveResource {
    u64 totalSize;
    u64 availableSize;
    u64 maxFileSize;
    u64 maxDirectorySize;
    u32 unk_20[4];
} FSArchiveResource;

// The functions with which an archive runs its commands, indexed by command number (FSi_InvokeCommand tests the entry
// of the command before it calls it). An entry that is NULL makes its command FS_RESULT_UNSUPPORTED. The functions
// that return void are the events
typedef union FSArchiveInterface {
    struct {
        FSResult (*readFile)(FSArchive *arc, FSFile *file, void *buffer, u32 *length);
        FSResult (*writeFile)(FSArchive *arc, FSFile *file, const void *buffer, u32 *length);
        FSResult (*seekDirectory)(FSArchive *arc, FSFile *file, u32 id, u32 position);
        FSResult (*readDirectory)(FSArchive *arc, FSFile *file, FSDirectoryEntryInfo *entry);
        FSResult (*findPath)(FSArchive *arc, u32 baseDirId, const char *path, u32 *targetId, BOOL isDirectory);
        FSResult (*getPath)(FSArchive *arc, FSFile *file, u32 arg0, char *buffer, u32 *length);
        FSResult (*openFileFast)(FSArchive *arc, FSFile *file, u32 fileId, u32 arg1);
        FSResult (*openFileDirect)(FSArchive *arc, FSFile *file, u32 top, u32 bottom, u32 *fileId);
        FSResult (*closeFile)(FSArchive *arc, FSFile *file);
        void (*activate)(FSArchive *arc);
        void (*idle)(FSArchive *arc);
        void (*suspend)(FSArchive *arc);
        void (*resume)(FSArchive *arc);
        FSResult (*openFile)(FSArchive *arc, FSFile *file, u32 baseDirId, const char *path, u32 mode);
        FSResult (*seekFile)(FSArchive *arc, FSFile *file, s32 *offset, int whence);
        FSResult (*getFileLength)(FSArchive *arc, FSFile *file, u32 *length);
        FSResult (*getFilePosition)(FSArchive *arc, FSFile *file, u32 *position);
        void (*mount)(FSArchive *arc);
        void (*unmount)(FSArchive *arc);
        FSResult (*command19)(FSArchive *arc, u32 *arg0);
        FSResult (*command20)(FSArchive *arc, u32 arg0, u32 arg1, u32 arg2);
        FSResult (*command21)(FSArchive *arc, u32 arg0, u32 arg1);
        FSResult (*command22)(FSArchive *arc, u32 arg0, u32 arg1, u32 arg2, u32 arg3);
        FSResult (*getPathInfo)(FSArchive *arc, u32 baseDirId, const char *path, FSPathInfo *info);
        FSResult (*command24)(FSArchive *arc, u32 arg0, u32 arg1, u32 arg2);
        FSResult (*command25)(FSArchive *arc, u32 arg0, u32 arg1, u32 arg2);
        FSResult (*command26)(FSArchive *arc, u32 arg0, u32 arg1);
        FSResult (*command27)(FSArchive *arc, u32 arg0, u32 arg1, u32 arg2, u32 arg3);
        FSResult (*getArchiveResource)(FSArchive *arc, FSArchiveResource *resource);
        void *command29;
        FSResult (*command30)(FSArchive *arc, FSFile *file);
        FSResult (*command31)(FSArchive *arc, FSFile *file, u32 arg0);
        FSResult (*openDirectory)(FSArchive *arc, FSFile *file, u32 baseDirId, const char *path, u32 arg2);
        FSResult (*closeDirectory)(FSArchive *arc, FSFile *file);
        FSResult (*command34)(FSArchive *arc, FSFile *file, u32 arg0, u32 arg1);
    };
    void *command[FS_COMMAND_MAX];
} FSArchiveInterface;

// The words that a command's FSFile.argument holds, for the commands the game sends
typedef struct FSReadFileArgs {
    void *buffer;
    u32 length;
} FSReadFileArgs;

typedef struct FSWriteFileArgs {
    const void *buffer;
    u32 length;
} FSWriteFileArgs;

typedef struct FSFindPathArgs {
    u32 baseDirId;
    const char *path;
    u32 targetId; // where the archive writes the number of the directory or file found
    BOOL isDirectory;
} FSFindPathArgs;

typedef struct FSOpenFileFastArgs {
    u32 fileId;
    u32 unk_04;
} FSOpenFileFastArgs;

typedef struct FSOpenFileDirectArgs {
    u32 fileId;
    u32 top;
    u32 bottom;
    u32 unk_0c;
} FSOpenFileDirectArgs;

typedef struct FSOpenFileArgs {
    u32 baseDirId;
    const char *path;
    u32 mode;
} FSOpenFileArgs;

typedef struct FSSeekFileArgs {
    s32 offset;
    int whence;
} FSSeekFileArgs;

// Both FS_COMMAND_GETFILELENGTH and FS_COMMAND_GETFILEPOSITION
typedef struct FSGetFileValueArgs {
    u32 value;
} FSGetFileValueArgs;

// And the commands that no function of the game sends, as an interface function takes them
typedef struct FSGetPathArgs {
    u32 unk_00;
    char *buffer;
    u32 length; // the size of the buffer, and the length of the path
} FSGetPathArgs;

typedef struct FSGetPathInfoArgs {
    u32 baseDirId;
    const char *path;
    FSPathInfo *info;
} FSGetPathInfoArgs;

typedef struct FSGetArchiveResourceArgs {
    FSArchiveResource *resource;
} FSGetArchiveResourceArgs;

typedef struct FSOpenDirectoryArgs {
    u32 baseDirId;
    const char *path;
    u32 unk_08; // passed on, and not used by the ROM archive's function
} FSOpenDirectoryArgs;

// The bits of FSArchive.flag
#define FS_ARCHIVE_FLAG_REGISTERED 0x01   // its name is in the list of archives
#define FS_ARCHIVE_FLAG_MOUNTED 0x02      // its interface is set; FS_FindArchive only finds a mounted one
#define FS_ARCHIVE_FLAG_TABLE_LOADED 0x04 // its tables were loaded into memory (func_02071edc)
#define FS_ARCHIVE_FLAG_SUSPENDED 0x08    // it does not start commands
#define FS_ARCHIVE_FLAG_ACTIVE 0x10       // it has run FS_COMMAND_ACTIVATE and not yet FS_COMMAND_IDLE
#define FS_ARCHIVE_FLAG_CANCELING 0x20    // a file asked to cancel its command
#define FS_ARCHIVE_FLAG_SUSPENDING 0x40   // FS_SuspendArchive waits for the running command to end
#define FS_ARCHIVE_FLAG_UNMOUNTING 0x80   // it is being unmounted and takes no more commands

// An archive's functions to read and write its storage: the position counts from the archive's base. The default ones
// copy from and to memory at the base
typedef FSResult (*FSReadFunc)(FSArchive *arc, void *buffer, u32 position, u32 length);
typedef FSResult (*FSWriteFunc)(FSArchive *arc, const void *buffer, u32 position, u32 length);

// A hook that an archive runs a command through before the default one: it returns FS_RESULT_PROC_UNKNOWN to leave the
// command to the default
typedef FSResult (*FSProc)(FSFile *file, FSCommandType command);

// What an archive that has a file allocation table and a name table keeps after the common part of FSArchive; its
// userdata points here. The tables are read from the storage, at positions counted from the base, or from memory once
// func_02071edc loaded them: fat and fnt are then addresses, and the first positions are kept in the backups
typedef struct FSArchiveTables {
    u32 base;
    u32 fat;
    u32 fatSize;
    u32 fnt;
    u32 fntSize;
    u32 fatBackup;
    u32 fntBackup;
    void *loadedMemory; // where func_02071edc put the tables, NULL if it did not
    FSReadFunc readFunc;
    FSWriteFunc writeFunc;
    u32 unk_28;
    FSProc proc;
    u32 procMask; // the commands (1 << FSCommandType) that proc is run for
} FSArchiveTables;

// A file system's archive, such as "rom": a name, and the queue of the commands that its files send, run one at a time
// through its interface
struct FSArchive {
    union {
        char name[4];   // a name of up to 3 characters, in place
        char *longName; // a longer one, in the name pool; name[3] is then the pointer's high byte
    };
    FSArchive *next;       // the next archive of the list
    FSFile *list;          // the files with a command, the one at the head running it
    OSThreadQueue queue;   // where FS_SuspendArchive waits
    u32 flag;              // FS_ARCHIVE_FLAG_*
    FSCommandType command; // the last command that ended
    FSResult result;       // and what it returned
    void *userdata;        // what the archive's functions work on
    const FSArchiveInterface *vtbl;
    FSArchiveTables tables;
};

// The bits of FSFile.stat, under the command in bits 8 to 15
#define FS_FILE_STATUS_BUSY 0x01     // it has a command that did not end
#define FS_FILE_STATUS_CANCEL 0x02   // it asked to cancel it
#define FS_FILE_STATUS_BLOCKING 0x04 // a thread waits for it
#define FS_FILE_STATUS_ENDED 0x08    // the archive ended the command that a thread waits for
#define FS_FILE_STATUS_IS_FILE 0x10  // it is an open file
#define FS_FILE_STATUS_IS_DIR 0x20   // it is an open directory
#define FS_FILE_STATUS_RUNNING 0x40  // its command was started
#define FS_FILE_STATUS_COMMAND_SHIFT 8
// What a command sets and its end clears
#define FS_FILE_STATUS_PROGRESS_MASK 0xcf

// What a file is, once it is open: a range of its archive's data and the position in it
typedef struct FSFileRange {
    u32 fileId;
    s32 top;
    s32 bottom;
    s32 position;
} FSFileRange;

// And a directory: where it is read, and the number of its parent (of the root: the number of directories)
typedef struct FSDirectoryState {
    FSDirPos pos;
    u32 parentId;
} FSDirectoryState;

typedef union FSFileProperty {
    FSFileRange file;
    FSDirectoryState dir;
} FSFileProperty;

// Where FS_COMMAND_GETPATH writes the path, the size of the buffer, and what it tells of the path: its length with the
// terminator, and the number of its directory
typedef struct FSPathBuffer {
    char *buffer;
    u32 length;
    u16 pathLength;
    u16 dirId;
} FSPathBuffer;

// Where FS_COMMAND_READDIR puts the entry it reads, and whether it leaves out the entry's name
typedef struct FSDirReadArgs {
    FSDirEntry *entry;
    BOOL skipName;
} FSDirReadArgs;

// The range of the archive that FS_COMMAND_OPENFILEDIRECT opens, and the number it is given
typedef struct FSFileRangeArgs {
    u32 top;
    u32 bottom;
    u32 fileId;
} FSFileRangeArgs;

// The words that the default commands of an archive with tables work on, which the interface functions set in the
// file before they run the command (fs_call_filecmd)
typedef union FSFileCommandArgs {
    struct {
        void *buffer;
        u32 requested; // the length that was asked for
        u32 length;    // and what the file has left of it
    } readFile;
    struct {
        const void *buffer;
        u32 requested;
        u32 length;
    } writeFile;
    FSDirPos seekDirectory;
    FSDirReadArgs readDirectory;
    struct {
        FSDirPos start;
        const char *path;
        BOOL isDirectory;
        FSEntryId *result;
    } findPath;
    FSPathBuffer getPath;
    FSFileID openFileFast;
    FSFileRangeArgs openFileDirect;
} FSFileCommandArgs;

struct FSFile {
    FSFile *next;         // the next file of its archive's queue
    FSFileProperty *prop; // the property below, once the file or directory is open
    FSArchive *arc;
    u32 stat;
    void *argument;      // the words of the command, such as an FSReadFileArgs
    FSResult error;      // the result of the command, FS_RESULT_BUSY while it runs
    OSThreadQueue queue; // where a thread waits for the command
    FSFileProperty body;
    FSFileCommandArgs arg;
};

// NitroSDK's FS_GetResultCode: what the file's last command returned
static inline FSResult FS_GetResultCode(const FSFile *file) {
    return file->error;
}

// NitroSDK's FS_IsBusy: whether the file has a command that did not end. It returns u32 and not BOOL, with which MWCC
// keeps the 0 or 1 as a value to compare, as the ROM's code does, and not folded into the branch
static inline u32 FS_IsBusy(const FSFile *file) {
    return (file->stat & FS_FILE_STATUS_BUSY) != 0;
}

// NitroSDK's FS_IsDir: whether the file is an open directory. Like FS_IsBusy, it returns u32
static inline u32 FS_IsDir(const FSFile *file) {
    return (file->stat & FS_FILE_STATUS_IS_DIR) != 0;
}

// NitroSDK's FS_IsFile: whether the file is open, as FS_IsDir says it of a directory
static inline BOOL FS_IsFile(const FSFile *file) {
    return (file->stat & FS_FILE_STATUS_IS_FILE) != 0;
}

// NitroSDK's FS_IsArchiveLoaded: whether the archive is mounted
static inline u32 FS_IsArchiveMounted(const FSArchive *arc) {
    return (arc->flag & FS_ARCHIVE_FLAG_MOUNTED) != 0;
}

void finit(FSFile *file);
BOOL romfs_fopen(FSFile *file, const char *path);
u32 romfs_fgetsize(FSFile *file);
u32 GetFileSize(FSFile *file);
s32 romfs_fread(FSFile *file, void *dest, s32 size);
s32 func_02070e70(FSFile *file, const void *src, s32 size);
BOOL romfs_fclose(FSFile *file);
BOOL romfs_fseek(FSFile *file, s32 offset, int whence);
BOOL extfs_fopen(FSFile *file, const void *data, u32 size);
// The position in the file
u32 romfs_ftell(FSFile *file);
// The ROM archive's shortcuts for the length and the position of a file: they set *value and return TRUE when the
// file's archive has the ROM interface, and return FALSE (and leave the command to be sent) otherwise
BOOL func_02071e74(FSFile *file, u32 *length);
BOOL func_02071e9c(FSFile *file, u32 *position);
// Opens a file that is a range of an archive's data, from start to end
BOOL fopen_raw(FSFile *file, FSArchive *archive, u32 start, u32 end, int fileId);
// The archive of a name of the given length, or NULL. NitroSDK's FS_FindArchive
FSArchive *func_0207062c(const char *name, int nameLength);
// A field of the file: where its data starts in its archive. NitroSDK's FS_GetFileImageTop
u32 func_02072010(const FSFile *file);

#define FS_SEEK_SET 0

#define FS_FILEMODE_R 1

BOOL fs_resolve_file(FSFileID *fileId, const char *path);
BOOL romfs_fopen_id(FSFile *file, FSFileID fileId);
BOOL romfs_fopen_core(FSFile *file, const char *path, u32 mode);
// NitroSDK's FS_CancelFile: asks the file's archive to stop the command running on it
void FS_CancelFile(FSFile *file);

// Overlays: NitroSDK's FS_LoadOverlayInfo, FS_LoadOverlay, FS_UnloadOverlay and FS_SetDefaultDMA, under swan's names.
// The target is the processor, MI_PROCESSOR_ARM9 (nitro/mi.h)
#define FS_DMA_NOT_USE 0xffffffff

// The bits of FSOverlayInfo.flag. An image that is compressed expands in place (backward LZ). One with a digest in the
// table of the build has it checked, unless the game booted from the ROM
#define FS_OVERLAY_FLAG_COMPRESS 0x01
#define FS_OVERLAY_FLAG_AUTH 0x02

// The size of an overlay's header in a table of them, in the ROM or in memory, which FSOverlayInfo starts with
#define FS_OVERLAY_HEADER_SIZE 0x20

// An overlay's header, as the ROM's table has it, and what is learned about it when it is read: the processor, and
// where its file starts in the ROM and how long it is
typedef struct FSOverlayInfo {
    u32 id;
    u8 *ramAddress;            // where the image is loaded
    u32 ramSize;               // the size of the image in memory, uncompressed
    u32 bssSize;               // and of the zeroed data that follows it
    void (**sinitStart)(void); // the static initializers, to be called in order
    void (**sinitEnd)(void);
    u32 fileId;
    u32 compressedSize : 24; // the size of the stored image, when it is compressed
    u32 flag : 8;            // FS_OVERLAY_FLAG_*
    MIProcessor target;
    u32 filePos;
    u32 fileSize;
} FSOverlayInfo;

BOOL sys_read_overlay_header(FSOverlayInfo *info, MIProcessor target, u32 id);
BOOL sys_load_overlay(MIProcessor target, u32 id);
BOOL sys_unload_overlay(MIProcessor target, u32 id);
u32 fs_set_dma_id(u32 dma);

// The digests of the overlays that the build makes, one of 20 bytes for each overlay in the order of their numbers, and
// the end of the table
typedef struct FSOverlayDigest {
    u8 bytes[20];
} FSOverlayDigest;

extern FSOverlayDigest SDK_OVERLAY_DIGEST[];
extern FSOverlayDigest SDK_OVERLAY_DIGEST_END[];

// The number of overlays that the build has, which the linker script defines as __OVERLAY_COUNT (NitroSDK's
// SDK_OVERLAY_NUMBER). The overlays from this number on exist only in the DSi mode
extern u32 __OVERLAY_COUNT[];
#define FS_OVERLAY_COUNT ((u32)__OVERLAY_COUNT)

// The state of the overlays: NitroSDK's FSi_InitOverlay, which sets it up once the archive "rom" exists
void func_02070ebc(void);
// NitroSDK's FSi_GetOverlayBinarySize: the size of an overlay's image in the ROM, without its zeroed data
u32 sys_get_overlay_size(const FSOverlayInfo *info);
// NitroSDK's FS_ClearOverlayImage: invalidates the caches over the overlay's memory and clears its zeroed data
void sys_prepare_overlay_area(const FSOverlayInfo *info);
// NitroSDK's FS_GetOverlayFileID: the archive and the number of the overlay's file
FSFileID sys_get_overlay_file_id(const FSOverlayInfo *info);
// NitroSDK's FS_LoadOverlayImage: reads the overlay's image into its memory
BOOL sys_mount_overlay(const FSOverlayInfo *info);
// NitroSDK's FS_StartOverlay: checks the image's digest, expands it and calls its static initializers
void sys_start_overlay(const FSOverlayInfo *info);
// NitroSDK's FS_EndOverlay: calls the destructors of the objects that the overlay made, and forgets them
void sys_call_ovl_dtors(const FSOverlayInfo *info);
// NitroSDK's FS_UnloadOverlayImage
BOOL sys_unmount_overlay(const FSOverlayInfo *info);

// An archive with a file allocation table and a name table (fs_command_default.c): NitroSDK's FS_LoadArchive, which
// gives the archive its tables (counted from the base) and the functions that read and write the storage, NULL for the
// ones that copy from and to memory at the base, and mounts it
BOOL fs_bind(FSArchive *arc, u32 base, u32 fat, u32 fatSize, u32 fnt, u32 fntSize, FSReadFunc readFunc,
             FSWriteFunc writeFunc);
// NitroSDK's FS_UnloadArchive: unmounts the archive
BOOL func_02071e6c(FSArchive *arc);
// NitroSDK's FS_SetArchiveProc: sets the hook that is run for the commands of the mask, 1 << FSCommandType, before the
// archive's own. Neither goes without the other
void func_02071ec4(FSArchive *arc, FSProc proc, u32 procMask);
// NitroSDK's FS_LoadArchiveTables: copies the archive's tables into the memory given, which keeps them for reads from
// then on. Returns the size of memory it needs, and loads them if it was given that much
u32 func_02071edc(FSArchive *arc, void *memory, u32 memorySize);
// NitroSDK's FS_UnloadArchiveTables: back to reading the tables from the storage. Returns the memory they were in
void *func_02071fa0(FSArchive *arc);
// NitroSDK's FS_GetArchiveBase
u32 func_02071ff0(const FSArchive *arc);
// Whether the ROM archive holds a file that starts at the position in the part of the ROM that only the DSi mode reads
// (fs_rom.c)
BOOL func_02072130(const FSArchive *arc, u32 position);

// The archives and the command queue, in swan's names. NitroSDK's FS_RegisterArchiveName (the longest name is 15
// characters), FS_ReleaseArchiveName, FS_InitArchive, FS_SuspendArchive, FS_ResumeArchive, FSi_SendCommand (with
// blocking set, it returns after the command ended), FS_NotifyArchiveAsyncEnd, FS_ChangeDir, FSi_NormalizePath
// (which returns the archive a path is in, with the number of the directory it starts at and the path inside the
// archive), FS_GetArchiveName and FSi_IncrementSjisPositionToSlash
#define FS_ARCHIVE_SHORT_NAME_LEN_MAX 3 // the longest name that the archive holds in place
#define FS_ARCHIVE_NAME_LEN_MAX 15
#define FS_PATH_BUFFER_SIZE 0x104

void fs_create(FSArchive *arc);
BOOL fs_open(FSArchive *arc, const char *name, u32 nameLength);
void func_020709b8(FSArchive *arc);
// The name of the archive, in place or in the pool
const char *func_02070a24(const FSArchive *arc);
// Gives an archive its interface and the data its functions use, and mounts it: runs FS_COMMAND_MOUNT
BOOL func_02070a30(FSArchive *arc, void *userdata, const FSArchiveInterface *vtbl);
// Unmounts it: cancels its commands, runs FS_COMMAND_UNMOUNT and takes it out of FS_FindArchive's reach
BOOL func_02070a5c(FSArchive *arc);
BOOL func_02070ad4(FSArchive *arc);
BOOL func_02070b44(FSArchive *arc);
void func_02070b88(FSArchive *arc, FSResult result);
FSResult func_02070130(FSFile *file, FSResult result);
BOOL fs_call_syscmd(FSFile *file, FSCommandType command, BOOL blocking);
FSArchive *fs_normalize_path(const char *path, u32 *baseDirId, char *normalized);
BOOL func_02070680(const char *path);
int func_02070bfc(const char *path, int position);
BOOL func_02070eb4(const char *path);
// Opens a file that is a range of the ROM, from start, for size bytes
BOOL romfs_fopen_raw(FSFile *file, u32 start, u32 size);

// The ROM archive (fs_rom.c) and the file system as a whole (fs_api.c): NitroSDK's FSi_InitRomArchive, which makes the
// archive "rom" with the default DMA channel (FS_DMA_NOT_USE for none); FS_Init, which runs it and the overlays' setup
// once; FS_IsAvailable; and FS_LoadTable, which loads the ROM archive's tables into the memory given, as func_02071edc
// does for any archive
void fs_init_core(u32 dma);
void fs_init(u32 dma);
BOOL fs_init_check(void);
u32 func_02072358(void *memory, u32 size);

#endif // POKEBW2_NITRO_FS_H
