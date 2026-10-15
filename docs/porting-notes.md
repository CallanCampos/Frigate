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
scratch into three `n`-digit regions. For positive `n` and `n`-digit inputs, its numerator window reads
`workBuf[3*n]` exactly when both inputs have `n` significant digits. A dividend shorter than the divisor takes the
bypass branch; a divisor shorter than `n` digits keeps that read within the three regions. Division does not
initialize the extra digit.
Both originals contain the same access; an extra readable digit alone does not prove arithmetic correctness.

`BigInt_ModExp` has exactly three regions after its scratch pointer. When its SSL caller sets `n` to a positive
modulus byte count of at least two, the modulus needs at most `ceil(n/2) < n` digits, which excludes that extra
read. Fast exponentiation's direct divisions have five regions physically available. Its inverse has three
regions and depends on the valid radix/Euclidean length invariants. The radix marker at `work[modLen]` must also
fit before the next logical region at `work+n`; these storage observations do not prove every arithmetic input.

The leaf client-key path has modulus/exponent limits of 256/8 bytes and a minimum modulus count of 51, giving
extra digit capacity for positive parsed lengths. The initial ASN.1 length check precedes leading-zero removal,
whose loop does not check the remaining length; it does not establish a nonnegative final count. Signature
verification also accepts issuer keys by reference and does not locally bound signature/exponent counts against
the selected modulus capacity. The private-key path derives `n` from modulus bytes plus one, without local
prime/exponent/coefficient bounds; its fixed 48-byte output needs 24 readable digits in the intended result
region. The unrecovered private-key writer and external issuer-key constraints remain relevant to these proofs.
No reachable game capacity violation or general corrective guard has been established. The byte/digit units and
conditional division scratch contract are documented at the private declarations.
