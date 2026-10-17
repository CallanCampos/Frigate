#!/bin/sh
# Counts MWCC/dialect constructs in src/, lib/, include/. Usage: dialect_scan.sh REPO   (prints "pattern files sites")
cd "$1" || exit 1
scan() { # label, ERE
  files=$(grep -rlE --include='*.c' --include='*.h' "$2" src lib include | wc -l | tr -d ' ')
  sites=$(grep -rhE --include='*.c' --include='*.h' "$2" src lib include | wc -l | tr -d ' ')
  printf '%-34s files=%-5s sites=%s\n' "$1" "$files" "$sites"
}
scan "asm function/block"            '^\s*(static\s+)?(inline\s+)?asm\b|\basm\s*\{|\basm\s+[a-zA-Z_].*\('
scan "#pragma (any)"                 '^\s*#\s*pragma'
scan "#pragma pack"                  '^\s*#\s*pragma\s+pack'
scan "#pragma thumb/arm/inline etc"  '^\s*#\s*pragma\s+(thumb|arm|dont_inline|inline_depth|optimization_level|opt_|scheduling|peephole|section|force_active|unused|global_optimizer|interrupt|warn)'
scan "__declspec"                    '__declspec'
scan "__attribute__"                 '__attribute__'
scan "ATTRIBUTE_ALIGN"               'ATTRIBUTE_ALIGN'
scan "static inline"                 'static\s+inline'
scan "inline (non-static)"           '^\s*inline\s'
scan "_Static_assert/STATIC_ASSERT"  '_Static_assert|STATIC_ASSERT|SDK_COMPILER_ASSERT|COMPILE_ASSERT'
scan "bitfield members"              '^\s*(unsigned|signed|u8|u16|u32|s8|s16|s32|int|uint|BOOL)\s+\w+\s*:\s*[0-9]+\s*;'
scan "anonymous struct/union"        '^\s*(struct|union)\s*\{'
scan "__MWERKS__/__CWCC__"           '__MWERKS__|__CWCC__|__CWARM__'
scan "volatile"                      '\bvolatile\b'
scan "register keyword"              '\bregister\s+\w'
scan "__builtin_"                    '__builtin_'
scan "_Bool/stdbool"                 '_Bool|stdbool'
scan "wchar_t / L\"\" literals"      'wchar_t|L"'
scan "long (bare type)"              '(^|[^a-z_])(unsigned\s+long|signed\s+long|long)\s+(int\s+)?[a-zA-Z_*(]' 
scan "size_t"                        '\bsize_t\b'
scan "va_list/va_arg"                '\bva_(list|arg|start)\b'
scan "setjmp/longjmp"                'setjmp|longjmp'
scan "intptr/uintptr"                'intptr_t|uintptr_t'
