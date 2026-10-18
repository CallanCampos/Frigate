// The program's globals by name (port/app/symbols.h), so that --run-dump can read any of the game's globals by its
// decomp name: through dlsym on macOS, through the program's own ELF symbol table on Linux, and not yet on Windows.
#define _GNU_SOURCE
#include "app/symbols.h"
#include <stddef.h>

#if defined(__APPLE__)
#include <dlfcn.h>

// A Mach-O program's globals are in its export trie without any link option, and the two-level namespace keeps them
// from standing in for a library's own symbols
void *port_symbol_address(const char *name) {
    static void *program;

    if (program == NULL) {
        program = dlopen(NULL, RTLD_NOW);
    }
    return program != NULL ? dlsym(program, name) : NULL;
}

#elif defined(__linux__)
#include <elf.h>
#include <fcntl.h>
#include <link.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// An ELF program exports nothing unless it links with --export-dynamic, and what it exports would stand in for any
// library's symbol of the same name (the game has MD5Init, as libbsd does). So the program's own symbol table, which
// the linker keeps unless the program is stripped, is read from the file, and a symbol's address is its value plus
// where the program was loaded

// The main program's load address: the first object dl_iterate_phdr reports
static int TakeFirst(struct dl_phdr_info *info, size_t size, void *data) {
    (void)size;
    *(ElfW(Addr) *)data = info->dlpi_addr;
    return 1;
}

static const ElfW(Ehdr) * sImage;
static size_t sImageSize;

// Maps the program's own file (/proc/self/exe) read-only, once, to read its ELF symbol table; 0 when it can't, or
// when the file has no section headers (a stripped program)
static int MapProgram(void) {
    struct stat st;
    int fd;
    void *image;

    if (sImage != NULL) {
        return 1;
    }
    fd = open("/proc/self/exe", O_RDONLY);
    if (fd < 0) {
        return 0;
    }
    if (fstat(fd, &st) != 0 || (size_t)st.st_size < sizeof(ElfW(Ehdr))) {
        close(fd);
        return 0;
    }
    image = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (image == MAP_FAILED) {
        return 0;
    }
    sImage = image;
    sImageSize = (size_t)st.st_size;
    if (memcmp(sImage->e_ident, ELFMAG, SELFMAG) != 0 || sImage->e_shoff == 0 ||
        sImage->e_shoff + (size_t)sImage->e_shnum * sizeof(ElfW(Shdr)) > sImageSize) {
        munmap(image, sImageSize);
        sImage = NULL;
        return 0;
    }
    return 1;
}

void *port_symbol_address(const char *name) {
    const ElfW(Shdr) * sections;
    ElfW(Addr) base = 0;

    if (!MapProgram()) {
        return NULL;
    }
    sections = (const ElfW(Shdr) *)((const char *)sImage + sImage->e_shoff);
    for (unsigned i = 0; i < sImage->e_shnum; i++) {
        const ElfW(Shdr) *symtab = &sections[i];
        const ElfW(Shdr) * strtab;
        const ElfW(Sym) * symbols;
        const char *names;

        if (symtab->sh_type != SHT_SYMTAB || symtab->sh_link >= sImage->e_shnum ||
            symtab->sh_offset + symtab->sh_size > sImageSize) {
            continue;
        }
        strtab = &sections[symtab->sh_link];
        if (strtab->sh_offset + strtab->sh_size > sImageSize) {
            continue;
        }
        symbols = (const ElfW(Sym) *)((const char *)sImage + symtab->sh_offset);
        names = (const char *)sImage + strtab->sh_offset;
        for (size_t j = 0; j < symtab->sh_size / sizeof(ElfW(Sym)); j++) {
            const ElfW(Sym) *symbol = &symbols[j];

            if (symbol->st_shndx == SHN_UNDEF || symbol->st_name >= strtab->sh_size ||
                ELF64_ST_BIND(symbol->st_info) == STB_LOCAL || strcmp(names + symbol->st_name, name) != 0) {
                continue;
            }
            dl_iterate_phdr(TakeFirst, &base);
            return (void *)(base + symbol->st_value);
        }
    }
    return NULL;
}

#else

void *port_symbol_address(const char *name) {
    (void)name;
    return NULL;
}

#endif
