// The virtual memory calls of the aliasing backends (port/spikes/vram-alias), which let one VRAM bank, and so every
// graphic the game has loaded into it, show at each address the DS maps it to. macOS has three ways to share pages
// between two addresses: a POSIX shared memory object, a Mach named memory entry and mach_vm_remap. Linux has the
// first, with memfd_create instead of shm_open. Windows would use a pagefile-backed section and placeholders (the
// spike's README); it isn't written, so there every aliasing backend reports itself unavailable and copying runs
#include "hal/gx/vram_os.h"
#include <errno.h>
#include <stdio.h>

#if defined(__APPLE__) || defined(__linux__)
#define PORT_VRAM_OS_POSIX 1
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_vm.h>
#endif

size_t port_vram_os_page_size(void) {
#if PORT_VRAM_OS_POSIX
    return (size_t)sysconf(_SC_PAGESIZE);
#else
    return 4096;
#endif
}

int port_vram_os_backend_available(PortVramBackend backend) {
    size_t page = port_vram_os_page_size();

    if (backend == PORT_VRAM_BACKEND_COPY) {
        return 1;
    }
    // A page larger than a slot (64 KiB on some arm64 Linux kernels) can't place F, G, H and I on their own
    if (page > PORT_VRAM_SLOT_SIZE || PORT_VRAM_SLOT_SIZE % page != 0) {
        return 0;
    }
#if defined(__APPLE__)
    return backend == PORT_VRAM_BACKEND_SHM || backend == PORT_VRAM_BACKEND_MACH_ENTRY ||
           backend == PORT_VRAM_BACKEND_MACH_REMAP;
#elif defined(__linux__)
    return backend == PORT_VRAM_BACKEND_SHM;
#else
    return 0;
#endif
}

#if PORT_VRAM_OS_POSIX

int port_vram_os_allocate(size_t size, uint8_t **out) {
    void *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);

    if (p == MAP_FAILED) {
        return errno;
    }
    *out = p;
    return 0;
}

void port_vram_os_release(uint8_t *addr, size_t size) {
    munmap(addr, size);
}

int port_vram_os_map_zero(PortVramOs *os, uint8_t *addr, size_t len) {
    os->calls++;
    if (mmap(addr, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0) == MAP_FAILED) {
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

#else

int port_vram_os_allocate(size_t size, uint8_t **out) {
    (void)size;
    (void)out;
    return ENOTSUP;
}

void port_vram_os_release(uint8_t *addr, size_t size) {
    (void)addr;
    (void)size;
}

int port_vram_os_map_zero(PortVramOs *os, uint8_t *addr, size_t len) {
    (void)os;
    (void)addr;
    (void)len;
    return ENOTSUP;
}

#endif // PORT_VRAM_OS_POSIX

#if defined(__APPLE__)

static int kern_errno(kern_return_t kr) {
    return kr == KERN_SUCCESS ? 0 : kr == KERN_NO_SPACE || kr == KERN_RESOURCE_SHORTAGE ? ENOMEM : EINVAL;
}

#endif

int port_vram_os_banks_create(PortVramOs *os) {
    os->fd = -1;
    os->entry = 0;
    os->window = NULL;
#if PORT_VRAM_OS_POSIX
    if (os->backend == PORT_VRAM_BACKEND_SHM) {
        void *p;

        os->fd = shm_create();
        if (os->fd < 0) {
            return errno;
        }
        if (ftruncate(os->fd, PORT_VRAM_BANKS_SIZE) != 0) {
            return errno;
        }
        p = mmap(NULL, PORT_VRAM_BANKS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, os->fd, 0);
        if (p == MAP_FAILED) {
            return errno;
        }
        os->window = p;
        return 0;
    }
#endif
#if defined(__APPLE__)
    if (os->backend == PORT_VRAM_BACKEND_MACH_ENTRY) {
        // A named entry is a handle on a VM object, like a memfd: mach_vm_map can map any part of it anywhere
        memory_object_size_t size = PORT_VRAM_BANKS_SIZE;
        mach_port_t entry = MACH_PORT_NULL;
        mach_vm_address_t addr = 0;
        kern_return_t kr;

        kr = mach_make_memory_entry_64(mach_task_self(), &size, 0, MAP_MEM_NAMED_CREATE | VM_PROT_READ | VM_PROT_WRITE,
                                       &entry, MACH_PORT_NULL);
        if (kr != KERN_SUCCESS) {
            return kern_errno(kr);
        }
        os->entry = entry;
        kr = mach_vm_map(mach_task_self(), &addr, PORT_VRAM_BANKS_SIZE, 0, VM_FLAGS_ANYWHERE, entry, 0, FALSE,
                         VM_PROT_READ | VM_PROT_WRITE, VM_PROT_READ | VM_PROT_WRITE, VM_INHERIT_SHARE);
        if (kr != KERN_SUCCESS) {
            return kern_errno(kr);
        }
        os->window = (uint8_t *)(uintptr_t)addr;
        return 0;
    }
    if (os->backend == PORT_VRAM_BACKEND_MACH_REMAP) {
        // Plain anonymous memory: mach_vm_remap shares its pages without any named object
        mach_vm_address_t addr = 0;
        kern_return_t kr = mach_vm_allocate(mach_task_self(), &addr, PORT_VRAM_BANKS_SIZE, VM_FLAGS_ANYWHERE);

        if (kr != KERN_SUCCESS) {
            return kern_errno(kr);
        }
        os->window = (uint8_t *)(uintptr_t)addr;
        return 0;
    }
#endif
    return ENOTSUP;
}

void port_vram_os_banks_destroy(PortVramOs *os) {
#if PORT_VRAM_OS_POSIX
    if (os->window != NULL) {
        munmap(os->window, PORT_VRAM_BANKS_SIZE);
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

int port_vram_os_map_banks(PortVramOs *os, uint8_t *addr, size_t len, size_t offset) {
    os->calls++;
#if PORT_VRAM_OS_POSIX
    if (os->backend == PORT_VRAM_BACKEND_SHM) {
        // MAP_FIXED replaces the pages there in one step: no moment where the range is unmapped
        if (mmap(addr, len, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, os->fd, (off_t)offset) == MAP_FAILED) {
            return errno;
        }
        return 0;
    }
#endif
#if defined(__APPLE__)
    if (os->backend == PORT_VRAM_BACKEND_MACH_ENTRY) {
        mach_vm_address_t a = (mach_vm_address_t)(uintptr_t)addr;
        kern_return_t kr =
            mach_vm_map(mach_task_self(), &a, len, 0, VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE, os->entry, offset, FALSE,
                        VM_PROT_READ | VM_PROT_WRITE, VM_PROT_READ | VM_PROT_WRITE, VM_INHERIT_SHARE);
        return kern_errno(kr);
    }
    if (os->backend == PORT_VRAM_BACKEND_MACH_REMAP) {
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
