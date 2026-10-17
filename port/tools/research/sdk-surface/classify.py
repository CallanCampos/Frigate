"""Classify every SDK function (NitroSDK, TwlSDK, boot) by its file into A/B/C, and count the used-from-outside surface."""
import json
import pickle
from collections import Counter, defaultdict
from pathlib import Path

DATA = Path(__file__).resolve().parents[4] / "build/port-research/sdk-surface"  # ignored by git; db.py fills it
db = pickle.load(open(DATA / "binscan.pkl", "rb"))
F = db["funcs"]

A = {  # portable logic: compile natively (asm functions rewritten as C where noted)
    "os_alloc", "os_mutex", "os_message", "os_event", "os_printf", "os_address_check", "os_vramExclusive",
    "fs_api", "fs_archive", "fs_command_default", "fs_file", "fs_memfile",
    "fx", "fx_atanidx", "fx_trig", "fx_sincos", "fx_mtx22", "fx_mtx33", "fx_mtx43",
    "math", "math_crc", "math_md5", "math_pop", "math_sha1", "sha1_block", "std_sprintf", "std_string", "rc4",
    "mi_memory", "mi_swap", "mi_uncompress",
    "nitro_snd_main", "snd_alarm", "snd_bank", "snd_interface", "snd_util", "snd_work",
    "rtc_convert", "wm_common",
}
B_LITE = {  # touch hardware only incidentally: hardware divider/sqrt, arena bounds, owner info, VRAM/IO addresses
    "fx_cp", "fx_vec", "fx_mtx44", "os_arena", "os_ownerInfo", "os_ownerInfoEx", "os_entropy", "cp_context",
}
C = {  # irrelevant on PC: stub
    "os_protectionRegion", "os_protectionUnit", "os_tcm", "os_exception", "os_emulator", "os_codecmode",
    "os_application_jump", "os_deliverArg", "os_reset", "os_reset_pxi", "os_terminate_proc", "os_spinLock",
    "mi_sharedWram", "mi_wram", "mi_init", "mi_dma_card",
    "ctrdg", "ctrdg_proc", "card_pull_out", "fs_overlay",
}


def cls(f):
    path = f["path"] or ""
    lib = f["lib"]
    base = Path(path).stem if path else ""
    if f["module"] == "itcm" and not path:
        return "B" if f["name"] in ("OS_IrqHandler", "func_01ff8ce0") else ("game" if f["addr"] < 0x01FF8BF0 else "C")
    if f["module"] == "main" and f["cat"] == "edge":
        return "C"  # crt0, autoload, SVC wrappers
    if lib.startswith("nitro/mb") or lib.startswith("nitro/ctrdg"):
        return "C"
    if lib.startswith("twl") or f["module"] == "ltd_autoload_0":
        if path.endswith(("ssp_jpeg_decoder.c", "ssp_jpeg_encoder.c", "ssp_exif.c")) or "ssp" in path:
            return "A"
        return "C(twl)"
    if base in A:
        return "A"
    if base in B_LITE:
        return "B-lite"
    if base in C:
        return "C"
    return "B"


SDK_LAYERS = ("nitro", "twl")
for f in F:
    top = f["lib"].split("/")[0].split("..")[0]
    f["is_sdk"] = (f["cat"] != "game" and (top in ("nitro", "twl") or f["module"] in ("itcm", "ltd_autoload_0")
                   or (f["module"] == "main" and f["cat"] == "edge" and f["addr"] < 0x02005124)))
    if f["is_sdk"]:
        f["cls"] = cls(f)

tot = Counter()
byfile = defaultdict(Counter)
for f in F:
    if f.get("is_sdk"):
        tot[f["cls"]] += 1
        byfile[f["cls"]][(f["lib"], Path(f["path"] or "?").stem)] += 1
print("SDK functions by class:", dict(tot))

used = defaultdict(lambda: Counter())
net = {"dwc", "nitrowifi", "dpw", "poke_net", "dwcutil", "dwcutilui", "irc"}
for caller, callee, kind, mod, frm in db["edges"]:
    if caller is None:
        continue
    cf, tf = F[caller], F[callee]
    if tf.get("is_sdk") and not cf.get("is_sdk"):
        top = cf["lib"].split("/")[0].split("..")[0].split("|")[0]
        if cf["module"] == "ov011" and cf["cat"] == "edge":
            top = "dwc"
        if cf["module"] == "ov182" and cf["cat"] == "edge":
            top = "dwcutil"
        used[callee][("game" if cf["cat"] == "game" else top)] += 1
uc = Counter()
uc_offline = Counter()
for c, u in used.items():
    uc[F[c]["cls"]] += 1
    if any(k not in net for k in u):
        uc_offline[F[c]["cls"]] += 1
print("used from outside SDK, by class:", dict(uc), "total", sum(uc.values()))
print("  of which used by non-network code:", dict(uc_offline), "total", sum(uc_offline.values()))
# list B used funcs grouped by file
bf = defaultdict(list)
for c in used:
    if F[c]["cls"] in ("B", "B-lite"):
        bf[(F[c]["lib"], Path(F[c]["path"] or "?").stem)].append(F[c]["name"])
out = {f"{k[0]}:{k[1]}": sorted(v) for k, v in sorted(bf.items())}
json.dump(out, open(DATA / "b_used.json", "w"), indent=1)
for k, v in out.items():
    print(f"  {k} ({len(v)}): {', '.join(v[:14])}{' ...' if len(v) > 14 else ''}")

# closure: B/B-lite functions needed when class-A SDK modules are kept native (called from outside OR from A modules)
need = set(c for c in used if F[c]["cls"] in ("B", "B-lite"))
for caller, callee, kind, mod, frm in db["edges"]:
    if caller is None:
        continue
    cf, tf = F[caller], F[callee]
    if cf.get("is_sdk") and cf["cls"] == "A" and tf.get("is_sdk") and tf["cls"] in ("B", "B-lite"):
        need.add(callee)
c2 = Counter(F[c]["cls"] for c in need)
print("B/B-lite HAL entry points if A-class SDK kept:", len(need), dict(c2))
fam = Counter()
for c in need:
    fam[F[c]["lib"].split("..")[0]] += 1
print("  by module:", dict(fam.most_common()))
# A modules' functions kept natively
print("A-class SDK funcs kept:", sum(1 for f in F if f.get("is_sdk") and f["cls"] == "A"))
