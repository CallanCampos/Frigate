// Frozen phase-0 spike (port/spikes/README.md), outside Frigate's build: the live code is port/hal/gx/vram_os.c; change
// that, not this copy.
// The virtual memory calls of the aliasing backends. macOS has all three ways to share pages between two addresses:
// a POSIX shared memory object, a Mach named memory entry and mach_vm_remap. Linux has the first, with memfd_create
// instead of shm_open. Windows would use a pagefile-backed section and placeholders (README.md); this spike doesn't
// build it, so there every aliasing backend reports ENOTSUP and the copy backend runs
#include "vram_os.h"
#include <errno.h>
#include <stdio.h>

#if defined(__APPLE__) || defined(__linux__)
#define VRAM_OS_POSIX 1
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_vm.h>
#endif

size_t vram_os_page_size(void) {
#if VRAM_OS_POSIX
    return (size_t)sysconf(_SC_PAGESIZE);
#else
    return 4096;
#endif
}

bool vram_os_backend_available(VramBackend backend) {
    size_t page = vram_os_page_size();

    if (backend == VRAM_BACKEND_COPY) {
        return true;
    }
    // A page larger than a slot (64 KiB on some arm64 Linux kernels) can't place F, G, H and I on their own
    if (page > VRAM_SLOT_SIZE || VRAM_SLOT_SIZE % page != 0) {
        return false;
    }
#if defined(__APPLE__)
    return backend == VRAM_BACKEND_SHM || backend == VRAM_BACKEND_MACH_ENTRY || backend == VRAM_BACKEND_MACH_REMAP;
#elif defined(__linux__)
    return backend == VRAM_BACKEND_SHM;
#else
    return false;
#endif
}

#if VRAM_OS_POSIX

int vram_os_reserve(size_t size, uint8_t **out) {
    void *p = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);

    if (p == MAP_FAILED) {
        return errno;
    }
    *out = p;
    return 0;
}

void vram_os_release(uint8_t *addr, size_t size) {
    munmap(addr, size);
}

int vram_os_map_anon(VramOs *os, uint8_t *addr, size_t len, bool accessible) {
    int prot = accessible ? PROT_READ | PROT_WRITE : PROT_NONE;

    os->calls++;
    if (mmap(addr, len, prot, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0) == MAP_FAILED) {
        return errno;
    }
    return 0;
}

// The shared memory object: memfd on Linux; on macOS a POSIX object whose name is removed at once, so nothing is left
// behind if the process dies. macOS lets an object be sized only once and keeps names to 31 bytes
static int shm_create(void) {
#if defined(__linux__)
    return memfd_create("pbw2-vram", MFD_CLOEXEC);
#else
    static unsigned serial;
    char name[32];
    int fd;

    snprintf(name, sizeof(name), "/pbw2vram.%d.%u", (int)getpid(), serial++);
    fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd >= 0) {
        shm_unlink(name);
    }
    return fd;
#endif
}

#endif // VRAM_OS_POSIX

#if defined(__APPLE__)

static int kern_errno(kern_return_t kr) {
    return kr == KERN_SUCCESS ? 0 : kr == KERN_NO_SPACE || kr == KERN_RESOURCE_SHORTAGE ? ENOMEM : EINVAL;
}

#endif

int vram_os_banks_create(VramOs *os) {
    os->fd = -1;
    os->entry = 0;
    os->window = NULL;
#if VRAM_OS_POSIX
    if (os->backend == VRAM_BACKEND_SHM) {
        void *p;

        os->fd = shm_create();
        if (os->fd < 0) {
            return errno;
        }
        if (ftruncate(os->fd, VRAM_BANKS_SIZE) != 0) {
            return errno;
        }
        p = mmap(NULL, VRAM_BANKS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, os->fd, 0);
        if (p == MAP_FAILED) {
            return errno;
        }
        os->window = p;
        return 0;
    }
#endif
#if defined(__APPLE__)
    if (os->backend == VRAM_BACKEND_MACH_ENTRY) {
        // A named entry is a handle on a VM object, like a memfd: mach_vm_map can map any part of it anywhere
        memory_object_size_t size = VRAM_BANKS_SIZE;
        mach_port_t entry = MACH_PORT_NULL;
        mach_vm_address_t addr = 0;
        kern_return_t kr;

        kr = mach_make_memory_entry_64(mach_task_self(), &size, 0, MAP_MEM_NAMED_CREATE | VM_PROT_READ | VM_PROT_WRITE,
                                       &entry, MACH_PORT_NULL);
        if (kr != KERN_SUCCESS) {
            return kern_errno(kr);
        }
        os->entry = entry;
        kr = mach_vm_map(mach_task_self(), &addr, VRAM_BANKS_SIZE, 0, VM_FLAGS_ANYWHERE, entry, 0, FALSE,
                         VM_PROT_READ | VM_PROT_WRITE, VM_PROT_READ | VM_PROT_WRITE, VM_INHERIT_SHARE);
        if (kr != KERN_SUCCESS) {
            return kern_errno(kr);
        }
        os->window = (uint8_t *)(uintptr_t)addr;
        return 0;
    }
    if (os->backend == VRAM_BACKEND_MACH_REMAP) {
        // Plain anonymous memory: mach_vm_remap shares its pages without any named object
        mach_vm_address_t addr = 0;
        kern_return_t kr = mach_vm_allocate(mach_task_self(), &addr, VRAM_BANKS_SIZE, VM_FLAGS_ANYWHERE);

        if (kr != KERN_SUCCESS) {
            return kern_errno(kr);
        }
        os->window = (uint8_t *)(uintptr_t)addr;
        return 0;
    }
#endif
    return ENOTSUP;
}

void vram_os_banks_destroy(VramOs *os) {
#if VRAM_OS_POSIX
    if (os->window != NULL) {
        munmap(os->window, VRAM_BANKS_SIZE);
    }
    if (os->fd >= 0) {
        close(os->fd);
    }
#endif
#if defined(__APPLE__)
    if (os->entry != 0) {
        mach_port_deallocate(mach_task_self(), os->entry);
    }
#endif
    os->window = NULL;
    os->fd = -1;
    os->entry = 0;
}

int vram_os_map_banks(VramOs *os, uint8_t *addr, size_t len, size_t offset) {
    os->calls++;
#if VRAM_OS_POSIX
    if (os->backend == VRAM_BACKEND_SHM) {
        // MAP_FIXED replaces the pages there in one step: no window where the range is unmapped
        if (mmap(addr, len, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, os->fd, (off_t)offset) == MAP_FAILED) {
            return errno;
        }
        return 0;
    }
#endif
#if defined(__APPLE__)
    if (os->backend == VRAM_BACKEND_MACH_ENTRY) {
        mach_vm_address_t a = (mach_vm_address_t)(uintptr_t)addr;
        kern_return_t kr =
            mach_vm_map(mach_task_self(), &a, len, 0, VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE, os->entry, offset, FALSE,
                        VM_PROT_READ | VM_PROT_WRITE, VM_PROT_READ | VM_PROT_WRITE, VM_INHERIT_SHARE);
        return kern_errno(kr);
    }
    if (os->backend == VRAM_BACKEND_MACH_REMAP) {
        mach_vm_address_t a = (mach_vm_address_t)(uintptr_t)addr;
        vm_prot_t cur, max;
        kern_return_t kr =
            mach_vm_remap(mach_task_self(), &a, len, 0, VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE, mach_task_self(),
                          (mach_vm_address_t)(uintptr_t)(os->window + offset), FALSE, &cur, &max, VM_INHERIT_SHARE);
        return kern_errno(kr);
    }
#endif
    (void)addr;
    (void)len;
    (void)offset;
    return ENOTSUP;
}

#if !VRAM_OS_POSIX

int vram_os_reserve(size_t size, uint8_t **out) {
    (void)size;
    (void)out;
    return ENOTSUP;
}

void vram_os_release(uint8_t *addr, size_t size) {
    (void)addr;
    (void)size;
}

int vram_os_map_anon(VramOs *os, uint8_t *addr, size_t len, bool accessible) {
    (void)os;
    (void)addr;
    (void)len;
    (void)accessible;
    return ENOTSUP;
}

#endif
