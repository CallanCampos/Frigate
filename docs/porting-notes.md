# Native port notes

These notes collect target assumptions and open questions found in recovered code. The observations below cover
NitroWiFi's SSL and WCM layers. Their remaining reconstruction differences are tracked in
[Nonmatching functions](nonmatching-functions.md); source ownership and naming are described in
[Code organization](code-organization.md).

## Pointers and layouts

[`ssl.c`](../lib/nitrowifi/src/ssl/ssl.c)'s `SSLContext` mixes pointers with fields recovered at original ARM32
offsets. Its `key_storage` union overlays generated key bytes with pointer slots assigned after generation.
[`wcm.c`](../lib/nitrowifi/src/wcm/wcm.c)'s work, global and AP-list structures also contain pointers, thread queues,
alarms and mutexes. Their recovered DS sizes and offsets do not establish a native 64-bit layout. Host object
representations and any original binary layouts remain separate porting questions.

`WCM_NotifyResult` forwards mixed pointer/integer payloads through 32-bit words. `WCM_Notify` assembles its notice
and extra payload fields on the stack, then calls the registered callback synchronously. A wider pointer
representation alone would not extend that temporary notice's lifetime. The declarations and lifetime contract
are in [`wcm_internal.h`](../lib/nitrowifi/include/nitrowifi/wcm_internal.h).

## Hardware and synchronization

`WCM_SetState` masks IRQs around state/alarm transitions and rearms the alarm for requested state 9.
`WCM_ReleaseIrqOwnedMutex` recognizes the address of `CPU_IRQInvoke` as an owner marker, decrements the recursion
count, and wakes the queue when ownership clears. That marker cannot be treated as an ordinary thread object.
The mapping of these operations to host synchronization and timers is unresolved.

The DS channel-span helper queries WM hardware. Mode 2 dispatches to a separate DSi backend in LTD; its extra
work-storage tail remains unmapped. These recovered calls do not identify a host wireless implementation.

## Storage and lifetimes

`WCM_SetConfig` borrows and aligns the caller's AP-list allocation. `WCM_FillApEntry` copies a descriptor prefix
and selected information elements into a record. The connected SSID and AP-MAC getters return interior pointers
into connection storage; the SSID has a separate length and need not be NUL-terminated. AP-list clearing
invalidates borrowed list records. These contracts are described in
[`wcm.h`](../lib/nitrowifi/include/nitrowifi/wcm.h).

`WCM_SendDCFDataEx` copies the caller's header/data fragments into WCM work before backend submission. The
original fragment pointers and the assembled backend buffer have different lifetimes; backend completion and
buffer-reuse requirements need to be preserved when replacing that path.

SSL's retained receive pointer, length and position are distinct from its handshake scratch. A buffer can be
owned before the application-record-ready flag is set. Certificate parsing also keeps borrowed pointers into
certificate bytes alongside copied fields. Storage changes need a lifetime audit of the parser, record
dispatcher, `CPS_SslRead` and release paths together.

## Word and byte order

[`md5.c`](../lib/nitrowifi/src/ssl/md5.c)'s `MD5Encode` and `MD5Decode` use native copies, relying on little-endian
words. [`sha1.c`](../lib/nitrowifi/src/ssl/sha1.c) converts words explicitly in its big-endian helpers. Fixed-width
`u32` state words alone do not remove MD5's endian dependency.

Hash input lengths count bytes; stored counters count bits. Ordinary finalization mutates the context and uses
the output buffer as temporary length storage. The caller contracts, including the different no-length SHA1
operation, are in [`ssl_internal.h`](../lib/nitrowifi/src/ssl/ssl_internal.h).

## Big-integer boundaries and capacity

[`biginteger.c`](../lib/nitrowifi/src/ssl/biginteger.c)'s byte conversions traverse big-endian byte streams in
reverse. With an even positive byte count, the final decrement can form a pointer before the supplied array;
zero bytes also form that pointer at initialization. Whether the supplied pointer is interior to a larger actual
array matters. The originals perform corresponding register arithmetic, and the completed path does not
dereference that terminal pointer. Portable C still restricts pointer arithmetic to the actual array and its
one-past position ([C11 draft N1570, section 6.5.6 paragraph 8](https://open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf)).
Avoiding before-array pointer formation remains native implementation work; no reachable game failure has been
established by this observation.

Capacity also needs proof at callers. Multiplication retains only the low `n` 16-bit digits. Division partitions
scratch into three `n`-digit regions, but its loop reads `top[0]` with `top = workBuf + 2*n + lenB`: a full-width
divisor can place that access beyond those regions. `BigInt_ModExpFast` writes a radix marker at `work[modLen]`,
with its next logical region starting at `work+n`. These accesses make high-digit headroom relevant to the
executing path; describing every three-region scratch buffer as sufficient would omit that condition.

The inspected SSL callers allocate extra digit capacity, but upstream signature/exponent lengths and private-key
output extents have not all been proven against parser or key-format constraints. A reachable capacity violation
and a general corrective guard remain unestablished. These questions are independent of the byte/digit units
already documented at the private declarations.
