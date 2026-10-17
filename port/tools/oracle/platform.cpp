// The host side that melonDS's core asks its frontend for (melonDS::Platform, src/Platform.h of the pinned release),
// for a headless test oracle: files through stdio, threads through the C++ library, and nothing that would let the
// host into the emulation. There is no network, multiplayer, camera, microphone or add-on: each reports nothing.
// Saves and firmware writes stay in memory (oracle.cpp writes the save at the end when asked to). The local files
// a frontend keeps beside its configuration are refused, so that a run depends on its arguments only.
#include "Platform.h"
#include "oracle.h"
#include <chrono>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <thread>

namespace melonDS::Platform {

struct FileHandle {
    FILE *file;
};

struct Thread {
    std::thread thread;
};

struct Semaphore {
    std::mutex mutex;
    std::condition_variable changed;
    int count = 0;
};

struct Mutex {
    std::mutex mutex;
};

void SignalStop(StopReason reason, void *userdata) {
    oracle::OnEmulatorStop(static_cast<int>(reason), userdata);
}

std::string GetLocalFilePath(const std::string &filename) {
    return filename;
}

FileHandle *OpenFile(const std::string &path, FileMode mode) {
    const char *how;

    if ((mode & FileMode::ReadWrite) == FileMode::ReadWrite) {
        how = (mode & FileMode::Preserve) ? "r+b" : "w+b";
    } else if (mode & FileMode::Write) {
        how = (mode & FileMode::Append) ? "ab" : "wb";
    } else if (mode & FileMode::Read) {
        how = "rb";
    } else {
        return nullptr;
    }
    if ((mode & FileMode::NoCreate) && !FileExists(path)) {
        return nullptr;
    }
    FILE *file = std::fopen(path.c_str(), how);
    if (file == nullptr) {
        return nullptr;
    }
    return new FileHandle{ file };
}

FileHandle *OpenLocalFile(const std::string &path, FileMode mode) {
    (void)mode;
    Log(LogLevel::Debug, "oracle: refused the local file %s\n", path.c_str());
    return nullptr;
}

bool FileExists(const std::string &name) {
    FILE *file = std::fopen(name.c_str(), "rb");
    if (file == nullptr) {
        return false;
    }
    std::fclose(file);
    return true;
}

bool LocalFileExists(const std::string &name) {
    (void)name;
    return false;
}

bool CheckFileWritable(const std::string &filepath) {
    (void)filepath;
    return false;
}

bool CheckLocalFileWritable(const std::string &filepath) {
    (void)filepath;
    return false;
}

bool CloseFile(FileHandle *file) {
    bool ok = std::fclose(file->file) == 0;
    delete file;
    return ok;
}

bool IsEndOfFile(FileHandle *file) {
    return std::feof(file->file) != 0;
}

bool FileReadLine(char *str, int count, FileHandle *file) {
    return std::fgets(str, count, file->file) != nullptr;
}

u64 FilePosition(FileHandle *file) {
    long position = std::ftell(file->file);
    return position < 0 ? 0 : static_cast<u64>(position);
}

bool FileSeek(FileHandle *file, s64 offset, FileSeekOrigin origin) {
    int whence = origin == FileSeekOrigin::Start ? SEEK_SET : origin == FileSeekOrigin::Current ? SEEK_CUR : SEEK_END;
    return std::fseek(file->file, static_cast<long>(offset), whence) == 0;
}

void FileRewind(FileHandle *file) {
    std::rewind(file->file);
}

u64 FileRead(void *data, u64 size, u64 count, FileHandle *file) {
    return std::fread(data, size, count, file->file);
}

bool FileFlush(FileHandle *file) {
    return std::fflush(file->file) == 0;
}

u64 FileWrite(const void *data, u64 size, u64 count, FileHandle *file) {
    return std::fwrite(data, size, count, file->file);
}

u64 FileWriteFormatted(FileHandle *file, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int written = std::vfprintf(file->file, fmt, args);
    va_end(args);
    return written < 0 ? 0 : static_cast<u64>(written);
}

u64 FileLength(FileHandle *file) {
    long position = std::ftell(file->file);
    std::fseek(file->file, 0, SEEK_END);
    long length = std::ftell(file->file);
    std::fseek(file->file, position, SEEK_SET);
    return length < 0 ? 0 : static_cast<u64>(length);
}

void Log(LogLevel level, const char *fmt, ...) {
    if (static_cast<int>(level) < oracle::LogThreshold()) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    std::vfprintf(stderr, fmt, args);
    va_end(args);
}

Thread *Thread_Create(std::function<void()> func) {
    return new Thread{ std::thread(std::move(func)) };
}

void Thread_Free(Thread *thread) {
    if (thread->thread.joinable()) {
        thread->thread.detach();
    }
    delete thread;
}

void Thread_Wait(Thread *thread) {
    if (thread->thread.joinable()) {
        thread->thread.join();
    }
}

Semaphore *Semaphore_Create() {
    return new Semaphore();
}

void Semaphore_Free(Semaphore *sema) {
    delete sema;
}

void Semaphore_Reset(Semaphore *sema) {
    std::lock_guard<std::mutex> lock(sema->mutex);
    sema->count = 0;
}

void Semaphore_Wait(Semaphore *sema) {
    std::unique_lock<std::mutex> lock(sema->mutex);
    sema->changed.wait(lock, [sema] { return sema->count > 0; });
    sema->count--;
}

bool Semaphore_TryWait(Semaphore *sema, int timeout_ms) {
    std::unique_lock<std::mutex> lock(sema->mutex);
    if (!sema->changed.wait_for(lock, std::chrono::milliseconds(timeout_ms), [sema] { return sema->count > 0; })) {
        return false;
    }
    sema->count--;
    return true;
}

void Semaphore_Post(Semaphore *sema, int count) {
    {
        std::lock_guard<std::mutex> lock(sema->mutex);
        sema->count += count;
    }
    sema->changed.notify_all();
}

Mutex *Mutex_Create() {
    return new Mutex();
}

void Mutex_Free(Mutex *mutex) {
    delete mutex;
}

void Mutex_Lock(Mutex *mutex) {
    mutex->mutex.lock();
}

void Mutex_Unlock(Mutex *mutex) {
    mutex->mutex.unlock();
}

bool Mutex_TryLock(Mutex *mutex) {
    return mutex->mutex.try_lock();
}

void Sleep(u64 usecs) {
    std::this_thread::sleep_for(std::chrono::microseconds(usecs));
}

// Host time for the frontend's own pacing; the core never lets it into the emulation
u64 GetMSCount() {
    return static_cast<u64>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

u64 GetUSCount() {
    return static_cast<u64>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

void WriteNDSSave(const u8 *savedata, u32 savelen, u32 writeoffset, u32 writelen, void *userdata) {
    (void)savedata;
    (void)savelen;
    (void)writeoffset;
    (void)writelen;
    oracle::OnSaveWritten(userdata);
}

void WriteGBASave(const u8 *savedata, u32 savelen, u32 writeoffset, u32 writelen, void *userdata) {
    (void)savedata;
    (void)savelen;
    (void)writeoffset;
    (void)writelen;
    (void)userdata;
}

void WriteFirmware(const Firmware &firmware, u32 writeoffset, u32 writelen, void *userdata) {
    (void)firmware;
    (void)writeoffset;
    (void)writelen;
    (void)userdata;
}

void WriteDateTime(int year, int month, int day, int hour, int minute, int second, void *userdata) {
    (void)year;
    (void)month;
    (void)day;
    (void)hour;
    (void)minute;
    (void)second;
    (void)userdata;
}

void MP_Begin(void *userdata) {
    (void)userdata;
}

void MP_End(void *userdata) {
    (void)userdata;
}

int MP_SendPacket(u8 *data, int len, u64 timestamp, void *userdata) {
    (void)data;
    (void)timestamp;
    (void)userdata;
    return len;
}

int MP_RecvPacket(u8 *data, u64 *timestamp, void *userdata) {
    (void)data;
    (void)timestamp;
    (void)userdata;
    return 0;
}

int MP_SendCmd(u8 *data, int len, u64 timestamp, void *userdata) {
    (void)data;
    (void)timestamp;
    (void)userdata;
    return len;
}

int MP_SendReply(u8 *data, int len, u64 timestamp, u16 aid, void *userdata) {
    (void)data;
    (void)timestamp;
    (void)aid;
    (void)userdata;
    return len;
}

int MP_SendAck(u8 *data, int len, u64 timestamp, void *userdata) {
    (void)data;
    (void)timestamp;
    (void)userdata;
    return len;
}

int MP_RecvHostPacket(u8 *data, u64 *timestamp, void *userdata) {
    (void)data;
    (void)timestamp;
    (void)userdata;
    return 0;
}

u16 MP_RecvReplies(u8 *data, u64 timestamp, u16 aidmask, void *userdata) {
    (void)data;
    (void)timestamp;
    (void)aidmask;
    (void)userdata;
    return 0;
}

int Net_SendPacket(u8 *data, int len, void *userdata) {
    (void)data;
    (void)userdata;
    return len;
}

int Net_RecvPacket(u8 *data, void *userdata) {
    (void)data;
    (void)userdata;
    return 0;
}

void Camera_Start(int num, void *userdata) {
    (void)num;
    (void)userdata;
}

void Camera_Stop(int num, void *userdata) {
    (void)num;
    (void)userdata;
}

void Camera_CaptureFrame(int num, u32 *frame, int width, int height, bool yuv, void *userdata) {
    (void)num;
    (void)yuv;
    (void)userdata;
    for (int i = 0; i < width * height; i++) {
        frame[i] = 0;
    }
}

void Mic_Start(void *userdata) {
    (void)userdata;
}

void Mic_Stop(void *userdata) {
    (void)userdata;
}

int Mic_ReadInput(s16 *data, int maxlength, void *userdata) {
    (void)data;
    (void)maxlength;
    (void)userdata;
    return 0;
}

AACDecoder *AAC_Init() {
    return nullptr;
}

void AAC_DeInit(AACDecoder *dec) {
    (void)dec;
}

bool AAC_Configure(AACDecoder *dec, int frequency, int channels) {
    (void)dec;
    (void)frequency;
    (void)channels;
    return false;
}

bool AAC_DecodeFrame(AACDecoder *dec, const void *input, int inputlen, void *output, int outputlen) {
    (void)dec;
    (void)input;
    (void)inputlen;
    (void)output;
    (void)outputlen;
    return false;
}

bool Addon_KeyDown(KeyType type, void *userdata) {
    (void)type;
    (void)userdata;
    return false;
}

void Addon_RumbleStart(u32 len, void *userdata) {
    (void)len;
    (void)userdata;
}

void Addon_RumbleStop(void *userdata) {
    (void)userdata;
}

float Addon_MotionQuery(MotionQueryType type, void *userdata) {
    (void)type;
    (void)userdata;
    return 0.0f;
}

DynamicLibrary *DynamicLibrary_Load(const char *lib) {
    (void)lib;
    return nullptr;
}

void DynamicLibrary_Unload(DynamicLibrary *lib) {
    (void)lib;
}

void *DynamicLibrary_LoadFunction(DynamicLibrary *lib, const char *name) {
    (void)lib;
    (void)name;
    return nullptr;
}

} // namespace melonDS::Platform
